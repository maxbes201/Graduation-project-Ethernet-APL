/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lwip.h"
#include "modbus_client.h"
#include "retarget.h"
#include <string.h>
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SERVER_PORT 8888
#define ADIN1100_RESET_GPIO_Port GPIOD
#define ADIN1100_RESET_Pin GPIO_PIN_2
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

uint16_t tcp_request_len = 0;

static uint8_t connected = 0;
static uint8_t tcp_callback = 0;
volatile uint8_t processing_tx = 0;

static struct tcp_pcb *client_pcb;
static ip_addr_t server_ip;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  extern struct netif gnetif;
  //extern uint16_t rx_count;
  static uint8_t link_ready = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  ADIN1100_HardwareReset();

  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_LWIP_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Startup message*/
  printf("STM32 Modbus RTU over TCP client starting...\r\n");
  printf("Using MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", gnetif.hwaddr[0], gnetif.hwaddr[1], gnetif.hwaddr[2], gnetif.hwaddr[3], gnetif.hwaddr[4], gnetif.hwaddr[5]);
  printf("STM32 IP: %s\r\n", ipaddr_ntoa(&gnetif.ip_addr));
  //char* m_ptr = "STM32 Modbus RTU over TCP client starting...\r\n";
  //uint16_t len = strlen(m_ptr);
  //HAL_UART_Transmit(&huart3, m_ptr, len, HAL_MAX_DELAY);

  uint32_t last_link_check = 0;
  /* Main Loop ---------------------------------------------------------------*/
  /* USER CODE BEGIN WHILE */
  printf("PCLK2: %lu\n", HAL_RCC_GetPCLK2Freq()); // should be 84000000
  while(1)
  {
	  // Check if the network interface is up and connected once
	  ethernetif_input(&gnetif);

	  // Check link every 1000ms
	  if (HAL_GetTick() - last_link_check > 1000)
	  {
		  ethernet_link_check_state(&gnetif);
		  last_link_check = HAL_GetTick();
	  }

	  if (netif_is_link_up(&gnetif) && !link_ready)
	  {
		  printf(">> Ethernet link is UP, Sending ARP and trying to connect...\r\n");
		  etharp_gratuitous(&gnetif); // Send APR
		  tcp_client_connect(); // Connect only once
		  connected = 1;
		  link_ready = 1;
	  }

	  if (tcp_callback)
	  {
		  printf("Sending function code request to SA11...");

		  int result = Modbus_send(&huart2, tcp_request_len);
		  if (result != 0)
		  {
			  printf("[RS485] Modbus_send failed with code: %d\r\n", result);
			  tcp_callback = 0;
			  processing_tx = 0;
			  continue; // prevent broken tcp_write
		  }

		  // Send RTU response back to TCP server
		  tcp_write(client_pcb, uart_rx_buffer, UART_BUFFER_LEN, TCP_WRITE_FLAG_COPY);
		  tcp_output(client_pcb);

		  printf("[RS485->TCP] Sending response back to host...\r\n");

		  tcp_callback = 0;
		  processing_tx = 0;

	  }

	  /* Handle LwIP timeouts */
	  sys_check_timeouts(); // Must call this function to let lwIP run
    /* USER CODE END WHILE */
  }
    /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 9600;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_EVEN;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK)
	{
	Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
  HAL_GPIO_WritePin(ADIN1100_RESET_GPIO_Port, ADIN1100_RESET_Pin, GPIO_PIN_SET);
  /*Configure GPIO pin : PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PC8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* Configure GPIO pin : PD2 */
  GPIO_InitStruct.Pin = ADIN1100_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ADIN1100_RESET_GPIO_Port, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* ADIN1100 Hardware Reset */
void ADIN1100_HardwareReset(void)
{
	printf("[PHY] Resetting ADIN1100 via PD2...\r\n");
	HAL_GPIO_WritePin(ADIN1100_RESET_GPIO_Port, ADIN1100_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(20); // Hold low for >10µs
	HAL_GPIO_WritePin(ADIN1100_RESET_GPIO_Port, ADIN1100_RESET_Pin, GPIO_PIN_SET);
	HAL_Delay(100); // Wait for PLL and link init
	printf("[PHY] Reset complete.\r\n");
}

/* TCP Data Received Callback */
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	if (processing_tx) return ERR_INPROGRESS;

	if (!p)
	{
		tcp_close(tpcb);
		return ERR_OK;
	}

	uint16_t len = p->len;
	if (len > UART_BUFFER_LEN) len = UART_BUFFER_LEN;

	memcpy(uart_tx_buffer, p->payload, len);
	tcp_request_len = len;

	uint8_t function_code = uart_tx_buffer[1];
	printf("\r\n[TPC->RS485] Received Function code: 0x%02x, Length: %d\r\n", function_code, len);

	tcp_callback = 1;
	processing_tx = 1;

	pbuf_free(p); // free the pbuf after processing

	return ERR_OK;
}


/* TCP Connect Callback */
err_t tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	if (err != ERR_OK)
	{
		printf("Connection failed with error: %d\r\n", err);
		return err;
	}

	printf("Connected to TCP server.\r\n");

	// Set TCP receive callback
	tcp_recv(tpcb, tcp_recv_callback);
	return ERR_OK;
}


/* public API to connect Client to Server */
void tcp_client_connect(void)
{
	client_pcb = tcp_new();

	if (!client_pcb)
	{
		printf("Failed to create TCP PCB\r\n");
		return;
	}

	IP_ADDR4(&server_ip, 196,168,1,2); // replace with local server IP, for testing use loopback IP
	printf("Connecting to %s:%d...\r\n", ipaddr_ntoa(&server_ip), SERVER_PORT);

	err_t err = tcp_connect(client_pcb, &server_ip, SERVER_PORT, tcp_connected_callback);
	if (err != ERR_OK)
	{
		printf("tcp_connect() failed with code: %d\r\n", err);
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
