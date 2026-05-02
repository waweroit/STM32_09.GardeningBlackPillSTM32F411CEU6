/*
 * USARTCommunication.h
 *
 *  Created on: Jul 14, 2025
 *      Author: wawer
 */

#ifndef INC_USARTCOMMUNICATION_H_
#define INC_USARTCOMMUNICATION_H_

#include <stdbool.h>
#include <string.h> // Potrzebne do strcmp, strcpy
#include "usart.h"
#include <stdarg.h>   // <-- to jest kluczowe

#define Channel_USART &huart1

#define RX_BUFFER_SIZE 200
#define TX_BUFFER_SIZE 200

extern volatile bool isTransmissionComplete;
extern volatile bool isRecivingComplete;
extern volatile uint8_t rxByte;

extern uint8_t txBuffer[TX_BUFFER_SIZE];


void InitUSART();



bool USART_TakeLine(char *out, size_t outSize);   // pobiera gotową linię (jeśli jest)
void USART_OnByteReceived(uint8_t b);             // sklejanie linii (wołane z RX IRQ)
void USART_StartReceiveIT(void);                  // restart pojedynczego RX


//memset(txBuffer, 0, sizeof(txBuffer));
//snprintf(txBuffer, sizeof(txBuffer), "Przyklad uzycia 1: %d %%\r\n", MoistureHumidityPercent_Dev_1);
//Send(txBuffer);
HAL_StatusTypeDef Sendf(const char *fmt, ...);
HAL_StatusTypeDef Send(const char *text);

// do dodania w MAIN !
//#include <stdio.h>
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//    if (huart->Instance == USART1)
//    {
//
//    }
//}
//
//void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
//{
//    if (huart->Instance == USART1) // Sprawdź, czy to Twój UART
//    {
//        isTransmissionComplete = true;
//    }
//}
#endif /* INC_USARTCOMMUNICATION_H_ */
