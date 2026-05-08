/*
 * USARTCommunication.c
 *
 *  Created on: Jul 14, 2025
 *      Author: wawer
 */

#include "USARTCommunication.h"
#include <stdio.h>
#include <string.h>

volatile bool isTransmissionComplete = true;
volatile bool isReceivingComplete = false;

volatile uint8_t rxByte;

static char rxLine[RX_BUFFER_SIZE];
static size_t rxLen = 0u;

static char rxLineQueue[RX_LINE_QUEUE_DEPTH][RX_BUFFER_SIZE];
static volatile size_t rxLineQueueHead = 0u;
static volatile size_t rxLineQueueTail = 0u;
static volatile size_t rxLineQueueCount = 0u;

uint8_t txBuffer[TX_BUFFER_SIZE];

static uint8_t txQueue[TX_QUEUE_SIZE];
static volatile size_t txQueueHead = 0u;
static volatile size_t txQueueTail = 0u;
static volatile size_t txQueueCount = 0u;
static volatile bool txBusy = false;
static uint8_t txByte = 0u;

static bool USART_RxQueuePushLine(const char *line);
static HAL_StatusTypeDef USART_TxQueuePush(const uint8_t *data, size_t len);
static void USART_TxStartNextLocked(void);

void InitUSART(void)
{
    HAL_UART_DeInit(Channel_USART);
    HAL_Delay(100);
    MX_USART1_UART_Init();

    isTransmissionComplete = true;
    isReceivingComplete = false;
    txBusy = false;

    USART_StartReceiveIT();
}

void USART_StartReceiveIT(void)
{
    (void)HAL_UART_Receive_IT(Channel_USART, (uint8_t*)&rxByte, 1);
}

void USART_OnByteReceived(uint8_t b)
{
    if (b == '\n' || b == '\r') {
        if (rxLen > 0u) {
            if (rxLen >= RX_BUFFER_SIZE) {
                rxLen = RX_BUFFER_SIZE - 1u;
            }

            rxLine[rxLen] = '\0';
            (void)USART_RxQueuePushLine(rxLine);
            rxLen = 0u;
        }
    } else {
        if (rxLen < RX_BUFFER_SIZE - 1u) {
            rxLine[rxLen++] = (char)b;
        } else {
            rxLen = 0u;
        }
    }
}

bool USART_TakeLine(char *out, size_t outSize)
{
    if (out == NULL || outSize == 0u) {
        return false;
    }

    __disable_irq();

    bool ready = (rxLineQueueCount > 0u);

    if (ready) {
        strncpy(out, rxLineQueue[rxLineQueueTail], outSize - 1u);
        out[outSize - 1u] = '\0';

        rxLineQueueTail = (rxLineQueueTail + 1u) % RX_LINE_QUEUE_DEPTH;
        rxLineQueueCount--;

        isReceivingComplete = (rxLineQueueCount > 0u);
    } else {
        isReceivingComplete = false;
    }

    __enable_irq();

    return ready;
}

void USART_OnTxComplete(void)
{
    __disable_irq();

    isTransmissionComplete = true;
    txBusy = false;
    USART_TxStartNextLocked();

    __enable_irq();
}

HAL_StatusTypeDef Sendf(const char *fmt, ...)
{
    if (fmt == NULL) {
        return HAL_ERROR;
    }

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf((char*)txBuffer, sizeof(txBuffer), fmt, ap);
    va_end(ap);

    if (n < 0) {
        return HAL_ERROR;
    }

    size_t len = (n < (int)sizeof(txBuffer)) ? (size_t)n : (sizeof(txBuffer) - 1u);

    return USART_TxQueuePush(txBuffer, len);
}

HAL_StatusTypeDef Send(const char *text)
{
    if (text == NULL) {
        return HAL_ERROR;
    }

    return USART_TxQueuePush((const uint8_t*)text, strlen(text));
}

static bool USART_RxQueuePushLine(const char *line)
{
    if (line == NULL) {
        return false;
    }

    if (rxLineQueueCount >= RX_LINE_QUEUE_DEPTH) {
        return false;
    }

    strncpy(rxLineQueue[rxLineQueueHead], line, RX_BUFFER_SIZE - 1u);
    rxLineQueue[rxLineQueueHead][RX_BUFFER_SIZE - 1u] = '\0';

    rxLineQueueHead = (rxLineQueueHead + 1u) % RX_LINE_QUEUE_DEPTH;
    rxLineQueueCount++;

    isReceivingComplete = true;

    return true;
}

static HAL_StatusTypeDef USART_TxQueuePush(const uint8_t *data, size_t len)
{
    if (data == NULL) {
        return HAL_ERROR;
    }

    if (len == 0u) {
        return HAL_OK;
    }

    if (len > TX_QUEUE_SIZE) {
        return HAL_BUSY;
    }

    __disable_irq();

    if ((TX_QUEUE_SIZE - txQueueCount) < len) {
        __enable_irq();
        return HAL_BUSY;
    }

    for (size_t i = 0u; i < len; i++) {
        txQueue[txQueueHead] = data[i];
        txQueueHead = (txQueueHead + 1u) % TX_QUEUE_SIZE;
        txQueueCount++;
    }

    USART_TxStartNextLocked();

    __enable_irq();

    return HAL_OK;
}

static void USART_TxStartNextLocked(void)
{
    if (txBusy || txQueueCount == 0u) {
        return;
    }

    txByte = txQueue[txQueueTail];
    txQueueTail = (txQueueTail + 1u) % TX_QUEUE_SIZE;
    txQueueCount--;

    txBusy = true;
    isTransmissionComplete = false;

    if (HAL_UART_Transmit_IT(Channel_USART, &txByte, 1u) != HAL_OK) {
        txBusy = false;
        isTransmissionComplete = true;
    }
}
