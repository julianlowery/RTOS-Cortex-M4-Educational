#ifndef __scheduler_h
#define __scheduler_h

#include "globals.h"

#define NUM_SUPPORTED_TASKS (12)


void SysTick_Handler(void);
void PendSV_Handler(void);

// Used to manipulate ready queues, block queues
void enqueue(tcb_list_t *list, tcb_t *tcb);
tcb_t* dequeue(tcb_list_t *list);
tcb_t* remove_from_list(tcb_list_t *list, tcb_t *tcb);

void add_task_to_sched(scheduler_t *scheduler, tcb_t *tcb);

#endif // __scheduler_h
