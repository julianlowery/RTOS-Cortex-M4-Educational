#ifndef __rtos_h
#define __rtos_h

#include "globals.h"
#include <stm32f4xx.h>

// Interface Functions
void rtos_init(void);
void task_create(rtosTaskFunc_t function_pointer, void* function_arg, priority_t task_priority);
void rtos_start();

// Semaphore interface
void semaphore_init(semaphore_t *sem, uint8_t initial_count);
void semaphore_take(semaphore_t *sem);
void semaphore_give(semaphore_t *sem);

// Mutex interface
void mutex_init(mutex_t *mutex);
void mutex_give(mutex_t *mutex);
void mutex_take(mutex_t *mutex);

// Message Queue interface
bool queue_init(queue_t *queue, void *buffer, size_t item_size, size_t cap);
void queue_send(queue_t *queue, void *src);
void queue_recieve(queue_t *queue, void *dest);

void yield();

#endif //__rtos_h
