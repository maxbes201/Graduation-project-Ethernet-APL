/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : retarget.c
  * @brief          : retarget UASRT 3 to FT230X program body
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
  *	This program ensure that the printf() function is redirected to USART 3
  *	This enables the user to monitor and debug the software via a USB to a
  *	local terminal
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes */
#include "main.h"
#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "retarget.h"

//UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart3;
/* main program code */

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart3, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

// Make sure to disable #define USE_HAL_UART_REGISTER_CALLBACKS 0 in stm32f4xx_hal_conf.h
