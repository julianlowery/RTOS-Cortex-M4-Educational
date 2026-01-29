/*
 * mutex_demo.c
 *
 *  Created on: Jan 29, 2026
 *      Author: julianlowery
 */

#include <stdio.h>

#include "rtos.h"
#include "logger.h"
#include "mutex_demo.h"

static semaphore_t sem;
static mutex_t mut;

void mutex_demo_high_task(void *arg) {
	log_msg("High task running\r\n");
	log_msg("High task taking semaphore - blocking\r\n");
	semaphore_take(&sem);
	log_msg("High task running - releasing semaphore, unblocking medium task\r\n");
	log_msg("High task taking mutex - blocking\r\n");
	mutex_take(&mut);
	log_msg("High task running\r\n");
	log_msg("High task releasing mutex\r\n");
	mutex_give(&mut);

	log_msg("\nComplete\r\n");
	log_print();
	while(1) {}
}

void mutex_demo_medium_task(void *arg) {
	log_msg("Medium task running\r\n");
	log_msg("Medium task taking semaphore - blocking\r\n");
	semaphore_take(&sem);
	log_msg("MEDIUM SHOULD NOT RUN HERE\r\n");
	while(1) {}
}

void mutex_demo_low_task(void *arg) {
	log_msg("Low task running - taking mutex\r\n");
	mutex_take(&mut);
	log_msg("Low task releasing semaphore - unblocking High task\r\n");
	semaphore_give(&sem);
	log_msg("Low task running at HIGH priority (NOTE: medium task is currently READY) (inheritance)\r\n");
	log_msg("Low task releasing mutex - unblocking High task\r\n");
	mutex_give(&mut);
	while(1) {}
}


void mutex_demo_init() {
	printf("Running Mutex (Priority Inheritance) Demo...\r\n\n");
	semaphore_init(&sem, 0);
	mutex_init(&mut);
  	task_create(mutex_demo_high_task, (void*)0X0, HIGH);
  	task_create(mutex_demo_medium_task, (void*)0X0, MEDIUM);
  	task_create(mutex_demo_low_task, (void*)0X0, LOW);
}
