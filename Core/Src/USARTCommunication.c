/*
 * USARTCommunication.c
 *
 *  Created on: Jul 14, 2025
 *      Author: wawer
 */

#include "USARTCommunication.h"
#include <stdio.h>
#include <string.h>

volatile bool isTransmissionComplete = false;
volatile bool isReceivingComplete = false;

volatile uint8_t rxByte;
static char rxLine[RX_BUFFER_SIZE];
static size_t rxLen = 0;

static char rxLineQueue[RX_LINE_QUEUE_DEPTH][RX_BUFFER_SIZE];
static volatile size_t rxLineQueueHead = 0u;
static volatile size_t rxLineQueueTail = 0u;
static volatile size_t rxLineQueueCount = 0u;

uint8_t txBuffer[TX_BUFFER_SIZE];

static bool USART_RxQueuePushLine(const char *line);

void InitUSART(void)
{
    HAL_UART_DeInit(Channel_USART);
    HAL_Delay(100);
    MX_USART1_UART_Init();
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
            if (rxLen >= RX_BUFFER_SIZE) rxLen = RX_BUFFER_SIZE - 1u;
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
    isTransmissionComplete = true;
}

static HAL_StatusTypeDef Send_IT_internal(const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    if (data == NULL) return HAL_ERROR;
    if (len == 0u) return HAL_OK;
    if (len > sizeof(txBuffer)) len = sizeof(txBuffer);

    if (data != txBuffer) {
        memcpy(txBuffer, data, len);
    }

    isTransmissionComplete = false;
    HAL_StatusTypeDef st = HAL_UART_Transmit_IT(Channel_USART, txBuffer, (uint16_t)len);
    if (st != HAL_OK) return st;

    uint32_t t0 = HAL_GetTick();
    while (!isTransmissionComplete) {
        if ((uint32_t)(HAL_GetTick() - t0) >= timeout_ms) {
            return HAL_TIMEOUT;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef Sendf(const char *fmt, ...)
{
    if (fmt == NULL) return HAL_ERROR;

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf((char*)txBuffer, sizeof(txBuffer), fmt, ap);
    va_end(ap);

    if (n < 0) return HAL_ERROR;
    size_t len = (n < (int)sizeof(txBuffer)) ? (size_t)n : sizeof(txBuffer);

    return Send_IT_internal(txBuffer, len, 1000u);
}

HAL_StatusTypeDef Send(const char *text)
{
    if (text == NULL) return HAL_ERROR;
    return Sendf("%s", text);
}

static bool USART_RxQueuePushLine(const char *line)
{
    if (line == NULL) return false;

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
