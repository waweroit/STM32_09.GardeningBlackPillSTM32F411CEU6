/*
 * USARTCommunication.h
 *
 *  Created on: Jul 14, 2025
 *      Author: wawer
 */

#ifndef INC_USARTCOMMUNICATION_H_
#define INC_USARTCOMMUNICATION_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include "usart.h"

#define Channel_USART &huart1

#define RX_BUFFER_SIZE 200
#define TX_BUFFER_SIZE 200
#define RX_LINE_QUEUE_DEPTH 4

extern volatile bool isTransmissionComplete;
extern volatile bool isReceivingComplete;
#define isRecivingComplete isReceivingComplete
extern volatile uint8_t rxByte;

extern uint8_t txBuffer[TX_BUFFER_SIZE];

void InitUSART(void);

bool USART_TakeLine(char *out, size_t outSize);
void USART_OnByteReceived(uint8_t b);
void USART_StartReceiveIT(void);
void USART_OnTxComplete(void);

HAL_StatusTypeDef Sendf(const char *fmt, ...);
HAL_StatusTypeDef Send(const char *text);

#endif /* INC_USARTCOMMUNICATION_H_ */
