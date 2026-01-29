/*
 * semaphore_demo.c
 *
 *  Created on: Jan 29, 2026
 *      Author: julianlowery
 */

#include <stdio.h>

#include "logger.h"
#include "rtos.h"

static semaphore_t sem_high;

void sem_demo_high_task1(void* arg) {
    log_msg("High task1 running\n\r");
    log_msg("High task1 taking high_sem (and blocking)\n\r");
    semaphore_take(&sem_high);
    log_msg("High task1 running\n\r");
    log_msg("High task1 giving sem_high (unblocking high task2) and yielding\n\r");
    semaphore_give(&sem_high);
    yield();
    log_msg("High task1 running\n\r");

    log_msg("\nComplete\r\n");
    log_print();
    while (1) {
    }
}

void sem_demo_high_task2(void* arg) {
    log_msg("High task2 running\n\r");
    log_msg("High task2 taking high_sem (and blocking)\n\r");
    semaphore_take(&sem_high);
    log_msg("High task2 running\n\r");
    log_msg("High task2 yielding\n\r");
    yield();
    while (1) {
    }
}

void sem_demo_low_task1(void* arg) {
    log_msg("Low task1 running\n\r");
    log_msg("Low task1 yielding\n\r");
    yield();
    log_msg("Low task1 running\n\r");
    log_msg("Low task1 yielding\n\r");
    yield();
    while (1) {
    }
}

void sem_demo_low_task2(void* arg) {
    log_msg("Low task2 running\n\r");
    log_msg("Low task2 yielding\n\r");
    yield();
    log_msg("Low task2 running\n\r");
    log_msg("Low task2 giving sem_high (unblocking high task1)\n\r");
    semaphore_give(&sem_high);
    while (1) {
    }
}

void semaphore_demo_init() {
    printf("Running Semaphore Demo...\r\n\n");
    semaphore_init(&sem_high, 0);
    task_create(sem_demo_high_task1, (void*)0X0, HIGH);
    task_create(sem_demo_high_task2, (void*)0X0, HIGH);
    task_create(sem_demo_low_task1, (void*)0X0, LOW);
    task_create(sem_demo_low_task2, (void*)0X0, LOW);
}
