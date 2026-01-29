/*
 * message_queue_demo.c
 *
 *  Created on: Jan 29, 2026
 *      Author: julianlowery
 */

#include <stdio.h>

#include "logger.h"
#include "rtos.h"

#define QUEUE_CAP (10)
#define QUEUE_ITEM_SIZE (sizeof(uint8_t))
static uint8_t queue_buffer[QUEUE_CAP];
static queue_t message_queue;

void queue_demo_producer1_task(void* arg) {
    log_msg("Producer 1 producing value 11\r\n");
    uint8_t val1 = 11;
    queue_send(&message_queue, &val1);
    yield();

    log_msg("Producer 1 producing value 12\r\n");
    uint8_t val2 = 12;
    queue_send(&message_queue, &val2);

    yield();
    while (1) {
    }
}

void queue_demo_producer2_task(void* arg) {
    log_msg("Producer 2 producing value 21\r\n");
    uint8_t val1 = 21;
    queue_send(&message_queue, &val1);
    yield();

    log_msg("Producer 2 producing value 22\r\n");
    uint8_t val2 = 22;
    queue_send(&message_queue, &val2);

    yield();
    while (1) {
    }
}

void queue_demo_consumer_task(void* arg) {
    for (int i = 0; i < 4; i++) {
        uint8_t recieved_val = 0;
        queue_recieve(&message_queue, &recieved_val);

        char log_msg_buf[25];
        snprintf(log_msg_buf, sizeof(log_msg_buf), "Consumer recieved %i\r\n", recieved_val);
        log_msg(log_msg_buf);
    }
    log_msg("\nComplete\r\n");
    log_print();
    while (1) {
    }
}

void message_queue_demo_init() {
    printf("Running Message Queue Demo... \r\n\n");
    queue_init(&message_queue, &queue_buffer, QUEUE_ITEM_SIZE, QUEUE_CAP);
    task_create(queue_demo_producer1_task, (void*)0X0, MEDIUM);
    task_create(queue_demo_producer2_task, (void*)0X0, MEDIUM);
    task_create(queue_demo_consumer_task, (void*)0X0, HIGH);
}
