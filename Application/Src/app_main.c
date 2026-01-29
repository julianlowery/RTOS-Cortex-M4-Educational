/*
 * app_main.c
 *
 *  Created on: Jan 29, 2026
 *      Author: julianlowery
 */

#include <stdio.h>

#include "logger.h"
#include "message_queue_demo.h"
#include "mutex_demo.h"
#include "rtos.h"
#include "scheduler_demo.h"
#include "semaphore_demo.h"

#define SCHEDULER_DEMO (1)
#define SEMAPHORE_DEMO (2)
#define MUTEX_DEMO (3)
#define MESSAGE_QUEUE_DEMO (4)

// Select Demo
#define DEMO_SELECT (MESSAGE_QUEUE_DEMO)

void app_start() {
    printf("\nStarting...\r\n\n");

    rtos_init();

#if DEMO_SELECT == SCHEDULER_DEMO
    scheduler_demo_init();

#elif DEMO_SELECT == SEMAPHORE_DEMO
    semaphore_demo_init();

#elif DEMO_SELECT == MUTEX_DEMO
    mutex_demo_init();

#elif DEMO_SELECT == MESSAGE_QUEUE_DEMO
    message_queue_demo_init();

#endif

    rtos_start();
}
