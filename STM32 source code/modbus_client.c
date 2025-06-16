/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : modbus_client.c
  * @brief          : Modbus TCP <-> RS485 bridge program body
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
  *	This program receives the incoming Ethernet message via tcp_recv callback
  *	function and stores it in the lwIP buffer called pbuf and will be handled in
  *	the lwIP stack.
  *
  * The Ethernet message contains a Modbus PDU which includes the Device address
  * and function code. The PDU message will then be sent over over UART to request
  * data from the SA11. the SA11 will respond with its own PDU. This response message
  * will be handled by the lwIP stack and will be sent back to the host wit the
  * callback function tcp_write and tcp_output.
  *
  * The API used for the lwIP is RAW API
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes */
#include "main.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include "retarget.h"
#include "modbus_client.h"

/* Configuration */


//UART_HandleTypeDef huart1;
uint8_t uart_rx_buffer[UART_BUFFER_LEN];
uint8_t uart_tx_buffer[UART_BUFFER_LEN];

/* main program code */

/* RS485 direction control */
void RS485_SetTransmit(void)
{
	HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_SET); // DE = 1 (Transmit)
}

void RS485_SetReceive(void)
{
	HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_RESET); // DE = 0 (Receive)
}



void Modbus_send(UART_HandleTypeDef *huart1)
{
	// Send RTU request over RS485
	RS485_SetTransmit();
	HAL_UART_Transmit(huart1, uart_tx_buffer, UART_BUFFER_LEN, UART_TIMEOUT);

	RS485_SetReceive();
	// Read response from SA11
	memset(uart_rx_buffer, 0, UART_BUFFER_LEN);
	HAL_UART_Receive(huart1, uart_rx_buffer, UART_BUFFER_LEN, UART_TIMEOUT);

	printf("\r\nReceived response code from SA11: 0x%02x, Length: %d\r\n", uart_rx_buffer[1], uart_rx_buffer[2]);
}




