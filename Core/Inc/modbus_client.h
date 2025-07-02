/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : modbus_client.h
  * @brief          : modbus_client header body
  ******************************************************************************
  * @attention
  *
  * Program version: V1.0
  * initial release date: 23/05/2025
  * Updated release date: N.A.
  *
  *	Author: Max Besseling
  *	Institute: Hogeschool Utrecht
  *	Company: Yokogawa Process Analyzer Europe B.V.
  *	Project: SENCOM Ethernet-APL
  *
  *	Description:
  *	This is the header file for the modbus_client.c file so it can be included
  *	in the main.c program file
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef INC_MODBUS_CLIENT_H_
#define INC_MODBUS_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "lwip/tcp.h"

// Public API to start the Modbus client TCP connection

#define UART_TIMEOUT		500
#define UART_BUFFER_LEN		256
#define RS485_DE_GPIO_Port	GPIOC
#define RS485_DE_Pin		GPIO_PIN_9
#define RS485_RE_GPIO_Port	GPIOC
#define RS485_RE_Pin		GPIO_PIN_8

extern uint8_t uart_rx_buffer[UART_BUFFER_LEN];
extern uint8_t uart_tx_buffer[UART_BUFFER_LEN];

int Modbus_send(UART_HandleTypeDef *huart2, uint16_t request_len);

#ifdef __cplusplus
}
#endif

#endif /* INC_MODBUS_CLIENT_H_ */
