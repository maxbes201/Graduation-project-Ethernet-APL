/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : retarget.h
  * @brief          : retarget header body
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
  *	This is the header file for the retarget.c file so it can be included
  *	in the main.c program file
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef INC_RETARGET_H_
#define INC_RETARGET_H_

#ifdef _cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "main.h" // required for huart3 and HAL includes

// this enables printf to be redirected to UART
int _write(int file, char *ptr, int len);

#ifdef _cplusplus
}
#endif

#endif /* INC_RETARGET_H_ */
