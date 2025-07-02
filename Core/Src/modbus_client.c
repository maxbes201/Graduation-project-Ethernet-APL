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
#include "stm32f4xx_hal.h"

/* Configuration */


//UART_HandleTypeDef huart2;
uint8_t uart_rx_buffer[UART_BUFFER_LEN];
uint8_t uart_tx_buffer[UART_BUFFER_LEN];
uint8_t byte = 0;
//uint16_t rx_count = 0;
/* main program code */

/* RS485 direction control */
void RS485_SetTransmit(void)
{
	HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_SET); // DE = 1 (Transmit)
	HAL_GPIO_WritePin(RS485_RE_GPIO_Port, RS485_RE_Pin, GPIO_PIN_SET);
	printf("[RS485] Transmit mode enabled (GPIO HIGH)\r\n");
}

void RS485_SetReceive(void)
{
	HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_RESET); // DE = 0 (Receive)
	HAL_GPIO_WritePin(RS485_RE_GPIO_Port, RS485_RE_Pin, GPIO_PIN_RESET);
	printf("[RS485] Receive mode enabled (GPIO LOW)\r\n");
}



int Modbus_send(UART_HandleTypeDef *huart2, uint16_t request_len)
{
	uint16_t i = 0;

	__HAL_UART_CLEAR_OREFLAG(huart2);  // Clear Overrun

	while (__HAL_UART_GET_FLAG(huart2, UART_FLAG_RXNE))
	{
	    volatile uint8_t dummy = huart2->Instance->DR;
	    (void)dummy;
	}

	//HAL_GPIO_WritePin(RS485_RE_GPIO_Port, RS485_RE_Pin, GPIO_PIN_RESET);

	if (request_len == 0 || request_len > UART_BUFFER_LEN)
	{
		printf("[RS485] Invalid request length: %u\r\n", request_len);
		return -1;
	}

	// Debug: Print request contents in hex
	printf("[RS485] Transmitting %d bytes: ", request_len);
	for (i = 0; i < request_len; i++)
	{
		printf("%02X ", uart_tx_buffer[i]);
	}
	printf("\r\n");

	// Send RTU request over RS485
	RS485_SetTransmit();
	HAL_Delay(3);

	if (HAL_UART_Transmit(huart2, uart_tx_buffer, request_len, UART_TIMEOUT) != HAL_OK)
	{
		printf("[RS485] UART transmit failed.\r\n");
		return -2;
	}

	while (__HAL_UART_GET_FLAG(huart2, UART_FLAG_TC) == RESET); // wait for completion of transmission
	HAL_Delay(2);

	// Switch to receive mode
	RS485_SetReceive();
	HAL_Delay(3);

	memset(uart_rx_buffer, 0, UART_BUFFER_LEN); // Clear buffer and receive response

	// === Manual RX using polling and inter-byte timeout ===
	i = 0;

	#define RESPONSE_TIMEOUT 500  // max total time
	#define INTERBYTE_TIMEOUT 10  // time between bytes

	uint32_t start = HAL_GetTick();
	uint32_t last_byte = start;

	while ((HAL_GetTick() - last_byte < INTERBYTE_TIMEOUT) && (HAL_GetTick() - start < RESPONSE_TIMEOUT))
	{
	    if (__HAL_UART_GET_FLAG(huart2, UART_FLAG_RXNE))
	    {
	        if (i < UART_BUFFER_LEN)
	        {
	            uart_rx_buffer[i] = huart2->Instance->DR;
	            if (uart_rx_buffer[i] == -1)
	            {
	            	continue;
	            }
	            else
	            {
	            	i++;
	            }

	            last_byte = HAL_GetTick();
	        }
	        else
	        {
	            printf("[RS485] RX buffer overflow, stopping.\r\n");
	            break;
	        }
	    }
	}

	if (i == 0)
	{
		printf("[RS485] No response received.\r\n");
		return -3;
	}

	printf("[RS485] Received %d bytes: ", i);
	for (int j = 0; j < i; j++) {
	    printf("%02X ", uart_rx_buffer[j]);
	}
	printf("\r\n");

	if (i >= 3)
	{
		uint8_t func_code = uart_rx_buffer[1];
		uint8_t length = uart_rx_buffer[2];
		printf("Received response code from SA11: 0x%02X, Length: %d\r\n", func_code, length);
	}

	return 0;
}
