/*
 * semaphore_demo.h
 *
 *  Created on: Jan 29, 2026
 *      Author: julianlowery
 */

#ifndef INC_SEMAPHORE_DEMO_H_
#define INC_SEMAPHORE_DEMO_H_

#include "logger.h"
#include "rtos.h"

void sem_demo_high_task1(void* arg);
void sem_demo_high_task2(void* arg);
void sem_demo_low_task1(void* arg);
void sem_demo_low_task2(void* arg);
void semaphore_demo_init();

#endif /* INC_SEMAPHORE_DEMO_H_ */
