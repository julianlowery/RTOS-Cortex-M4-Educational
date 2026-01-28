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

#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include "rtos.h"
#include "logger.h"

UART_HandleTypeDef huart2;


void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);


// ================= Scheduler Demo Tasks =================

void ScheduleTest_Task1(void *arg) {
	log_msg("Task1: 1\n\r");
	yield();
	log_msg("Task1: 5\n\r");
	yield();
	log_msg("Task1: 9\n\r");
	yield();
	while(1) {}
}

void ScheduleTest_Task2(void *arg) {
	log_msg("Task2: 2\n\r");
	yield();
	log_msg("Task2: 6\n\r");
	yield();
	log_msg("Task2: 10\n\r");
	yield();
	while(1) {}
}

void ScheduleTest_Task3(void *arg) {
	log_msg("Task3: 3\n\r");
	yield();
	log_msg("Task3: 7\n\r");
	yield();
	log_msg("Task3: 11\n\r");
	yield();
	while(1) {}
}

void ScheduleTest_Task4(void *arg) {
	log_msg("Task4: 4\n\r");
	yield();
	log_msg("Task4: 8\n\r");
	yield();
	log_msg("Task4: 12\n\r");

	log_msg("\nComplete\r\n");
	log_print();
	while(1) {}
}

void scheduling_test() {
	printf("Running Scheduling Demo...\r\n\n");

  	rtos_init();

  	task_create(ScheduleTest_Task1, (void*)0X0, HIGH);
  	task_create(ScheduleTest_Task2, (void*)0X0, HIGH);
  	task_create(ScheduleTest_Task3, (void*)0X0, HIGH);
  	task_create(ScheduleTest_Task4, (void*)0X0, HIGH);
}



// ================= Semaphore Demo Tasks =================

semaphore_t sem_high;

void SemTest_HighTask1(void* arg){
	log_msg("HighTask1 Running\n\r");
	log_msg("HighTask1 Taking high_sem (and blocking)\n\r");
	semaphore_take(&sem_high);
	log_msg("HighTask1 Running\n\r");
	log_msg("HighTask1 Giving sem_high (unblocking HighTask2) and yielding\n\r");
	semaphore_give(&sem_high);
	yield();
	log_msg("HighTask1 Running\n\r");

	log_msg("\nComplete\r\n");
	log_print();
	while(1) {}
}

void SemTest_HighTask2(void* arg){
	log_msg("HighTask2 Running\r");
	log_msg("HighTask2 Taking high_sem (and blocking)\n\r");
	semaphore_take(&sem_high);
	log_msg("HighTask2 Running\n\r");
	log_msg("HighTask2 Yielding\n\r");
	yield();
	while (1) {}
}

void SemTest_LowTask1(void* arg){
	log_msg("LowTask1 Running\n\r");
	log_msg("LowTask1 Yielding\n\r");
	yield();
	log_msg("LowTask1 Running\n\r");
	log_msg("LowTask1 Yielding\n\r");
	yield();
	while(1) { }
}

void SemTest_LowTask2(void* arg){
	log_msg("LowTask2 Running\n\r");
	log_msg("LowTask2 Yielding\n\r");
	yield();
	log_msg("LowTask2 Running\n\r");
	log_msg("LowTask2 Giving sem_high (unblocking HighTask1)\n\r");
	semaphore_give(&sem_high);
	while(1) { }
}

void semaphore_test() {
	printf("Running Semaphore Demo...\r\n\n");
  	semaphore_init(&sem_high, 0);

  	rtos_init();

  	task_create(SemTest_HighTask1, (void*)0X0, HIGH);
  	task_create(SemTest_HighTask2, (void*)0X0, HIGH);
  	task_create(SemTest_LowTask1, (void*)0X0, LOW);
  	task_create(SemTest_LowTask2, (void*)0X0, LOW);
}



// ================= Mutex (Priority Inheritance) Demo Tasks =================

semaphore_t sem;
mutex_t mut;

void MutTest_High(void *arg) {
	log_msg("HighTask running\r\n");
	log_msg("HighTask taking semaphore - blocking\r\n");
	semaphore_take(&sem);
	log_msg("HighTask running - releasing semaphore, unblocking MediumTask\r\n");
	log_msg("HighTask taking mutex - blocking\r\n");
	mutex_take(&mut);
	log_msg("HighTask running\r\n");
	log_msg("HighTask releasing mutex\r\n");
	mutex_give(&mut);

	log_msg("\nComplete\r\n");
	log_print();
	while(1) {}
}

void MutTest_Medium(void *arg) {
	log_msg("MediumTask running\r\n");
	log_msg("MediumTask taking semaphore - blocking\r\n");
	semaphore_take(&sem);
	log_msg("MEDIUM SHOULD NOT RUN HERE\r\n");
	while(1) {}
}

void MutTest_Low(void *arg) {
	log_msg("LowTask running - taking mutex\r\n");
	mutex_take(&mut);
	log_msg("LowTask releasing semaphore - unblocking HighTask\r\n");
	semaphore_give(&sem);
	log_msg("LowTask running at HIGH priority (NOTE: MediumTask is currently READY) (inheritance)\r\n");
	log_msg("LowTask releasing mutex - unblocking HighTask\r\n");
	mutex_give(&mut);
	while(1) {}
}


void mutex_test() {
	printf("Running Mutex (Priority Inheritance) Demo...\r\n\n");
	semaphore_init(&sem, 0);
	mutex_init(&mut);

  	rtos_init();

  	task_create(MutTest_High, (void*)0X0, HIGH);
  	task_create(MutTest_Medium, (void*)0X0, MEDIUM);
  	task_create(MutTest_Low, (void*)0X0, LOW);
}


#define QUEUE_CAP (10)
#define QUEUE_ITEM_SIZE (sizeof(uint8_t))
uint8_t queue_buffer[QUEUE_CAP];
queue_t message_queue;

void QueueTest_Prod1(void *arg) {
	log_msg("Prod1 producing value 11\r\n");
	uint8_t val1 = 11;
	queue_send(&message_queue, &val1);
	yield();

	log_msg("Prod1 producing value 12\r\n");
	uint8_t val2 = 12;
	queue_send(&message_queue, &val2);

	yield();
	while(1) {}
}

void QueueTest_Prod2(void *arg) {
	log_msg("Prod2 producing value 21\r\n");
	uint8_t val1 = 21;
	queue_send(&message_queue, &val1);
	yield();

	log_msg("Prod2 producing value 22\r\n");
	uint8_t val2 = 22;
	queue_send(&message_queue, &val2);

	yield();
	while(1) {}
}

void QueueTest_Cons(void *arg) {
	for(int i = 0; i < 4; i++) {
		uint8_t recieved_val = 0;
		queue_recieve(&message_queue, &recieved_val);

		char log_msg_buf[25];
		snprintf(log_msg_buf, sizeof(log_msg_buf), "Cons Recieved %i\r\n", recieved_val);
		log_msg(log_msg_buf);
	}
	log_msg("\nComplete\r\n");
	log_print();
	while(1) {}
}

void message_queue_test() {
	printf("Running Message Queue Demo... \r\n\n");
	queue_init(&message_queue, &queue_buffer, QUEUE_ITEM_SIZE, QUEUE_CAP);

	rtos_init();

  	task_create(QueueTest_Prod1, (void*)0X0, MEDIUM);
  	task_create(QueueTest_Prod2, (void*)0X0, MEDIUM);
  	task_create(QueueTest_Cons, (void*)0X0, HIGH);
}


int main(void)
{
	HAL_Init();

	SystemClock_Config();

	MX_GPIO_Init();
	MX_USART2_UART_Init();


  	printf("\nStarting...\r\n\n");


//  	scheduling_test();
//  	semaphore_test();
//  	mutex_test();
  	message_queue_test();



  	rtos_start();


	while (1) {
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
	HAL_Delay((uint32_t) 500);
	}
}


// Overwrite _write so printf is sent to uart - this is not thread safe - do not use
extern UART_HandleTypeDef huart2;

int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
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
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
#ifdef USE_FULL_ASSERT
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
