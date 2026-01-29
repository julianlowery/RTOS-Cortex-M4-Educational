/*
 * scheduler_demo.c
 *
 *  Created on: Jan 29, 2026
 *      Author: julianlowery
 */

#include <stdio.h>

#include "rtos.h"
#include "logger.h"


void scheduler_demo_task1(void *arg) {
	log_msg("Task1: 1\n\r");
	yield();
	log_msg("Task1: 5\n\r");
	yield();
	log_msg("Task1: 9\n\r");
	yield();
	while(1) {}
}

void scheduler_demo_task2(void *arg) {
	log_msg("Task2: 2\n\r");
	yield();
	log_msg("Task2: 6\n\r");
	yield();
	log_msg("Task2: 10\n\r");
	yield();
	while(1) {}
}

void scheduler_demo_task3(void *arg) {
	log_msg("Task3: 3\n\r");
	yield();
	log_msg("Task3: 7\n\r");
	yield();
	log_msg("Task3: 11\n\r");
	yield();
	while(1) {}
}

void scheduler_demo_task4(void *arg) {
	log_msg("Task4: 4\n\r");
	yield();
	log_msg("Task4: 8\n\r");
	yield();
	log_msg("Task4: 12\n\r");

	log_msg("\nComplete\r\n");
	log_print();
	while(1) {}
}

void scheduler_demo_init() {
  	printf("Running Scheduling Demo...\r\n\n");
  	task_create(scheduler_demo_task1, (void*)0X0, HIGH);
  	task_create(scheduler_demo_task2, (void*)0X0, HIGH);
  	task_create(scheduler_demo_task3, (void*)0X0, HIGH);
  	task_create(scheduler_demo_task4, (void*)0X0, HIGH);
}
