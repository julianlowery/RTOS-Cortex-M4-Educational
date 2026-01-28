#ifndef __globals_h
#define __globals_h

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum{
    RUNNING = 0,
    READY = 1,
    BLOCKED = 2,
    TERMINATED = 3,
    INNACTIVE = 4
}task_state_t;

typedef enum{
    HIGHEST,
    HIGH,
    MED_HIGH,
    MEDIUM,
    MED_LOW,
    LOW,
    LOWEST,
    IDLE
}priority_t;

typedef struct{
    uint32_t task_id;
    uint32_t *stack_pointer;
    task_state_t state;
    priority_t priority;
    void *tcb_pointer; // used in linked data structures
	bool mutex_released;
    uint32_t *stack_base_address;
    uint32_t *stack_overflow_address;
    uint32_t stack_size;
}tcb_t;

// Typedef for function that takes a void* and returns void
typedef void (*rtosTaskFunc_t)(void *args);

typedef struct{
    tcb_t* head;
    tcb_t* tail;
    uint32_t size;
}tcb_list_t;

typedef struct{
    tcb_t* running_task;
    tcb_list_t ready_lists[8];
    tcb_list_t terminated_list; // TODO: terminating tasks not yet supported
    priority_t current_priority;
}scheduler_t;

typedef struct{
	uint8_t count;
	tcb_list_t block_list;
}semaphore_t;

typedef struct{
	bool available;
	tcb_t *owner_tcb;
	priority_t owner_true_priority;
	priority_t inherited_priority;
	tcb_list_t block_list;
}mutex_t;

typedef struct{
	uint8_t *buffer;
	size_t cap;
	mutex_t mut;
	semaphore_t sem_avail_data;
	semaphore_t sem_avail_space;
	size_t item_size_bytes;
	size_t head;
	size_t tail;
}queue_t;

#endif // globals_h
