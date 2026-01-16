#include <stm32f4xx.h>
#include <stdio.h>

#include "scheduler.h"

uint32_t msTicks = 0;
uint32_t msp_enable_not = (1<<1);

extern tcb_t tcb_array[6];

tcb_t tcb_array[6];
tcb_t tcb_main;

bool run_scheduler = false;

scheduler_t scheduler = {
	.running_task = &tcb_array[0],
	.current_priority = IDLE
};


bool push_to_stack(tcb_t *tcb, uint32_t value){
	tcb->stack_pointer--;
	
	// Check for stack overflow
	if(tcb->stack_pointer <= tcb->stack_overflow_address){
		printf("STACK OVERFLOW from task id: %li/n", tcb->task_id);
		return false;
	}
	
	*(tcb->stack_pointer) = value;
	return true;
}

uint32_t pop_from_stack(tcb_t *tcb){
	uint32_t returnValue = *(tcb->stack_pointer);
	tcb->stack_pointer++;
	
	return returnValue;
}

__attribute__((naked, noreturn))
void context_switch(uint32_t *new_sp, uint32_t old_stack_ptr_location) {
	
	__asm volatile (
		// move PSP into R2
		"MRS   R2, PSP			\n"

		// "Store Multiple, Decrement Before". Decrement R2 and store all registers to
		// address starting at R2. "!" causes final address to be store back in R2
		"STMDB R2!, {R4-R11}	\n"

		// store updated old task stack pointer into tcb struct
		"STR R2, [R1]			\n"

		// new sp value is currently stored in R0 (first argument register)
		// "Load Multiple, Increment After". Effectively pop R4-R11 from task
		// stack starting at value in R0. "!" causes the final address update be in R0
		"LDMIA R0!, {R4-R11}	\n"

		// move new psp value from R0 back to PSP register (setting PSP)
		"MSR   PSP, R0			\n"

		"BX LR					\n"

		:
		:
		: "r0", "r1", "r2", "memory"
	);
	// Note on clobbers:
	// Our assembly clobbers r0, r1, r2 and reads/writes to/from memory.
	// This tells compiler to make no assumptions about the states of these.
	// "memory" is key - forces compiler to flush stores, reload values, and not reorder memory
	// access across this point (our inline assembly).
}

void SysTick_Handler(void) {
	const uint32_t time_slice_len = 100;
	const uint32_t set_pendsv = (1<<28);
	static uint32_t time_slice_count = time_slice_len;
	
    msTicks++;
	time_slice_count--;
	
	if(run_scheduler){
		run_scheduler = false;
		
		// Set pendSV exception to pending
		SCB->ICSR=(SCB->ICSR|set_pendsv);
	}
	
	else if(time_slice_count == 0){
		time_slice_count = time_slice_len;
		
		// If old task and new task are same priority, rearrange (cycle) the queue
		// for the scheduler
		if(scheduler.running_task->priority == scheduler.current_priority) {
			tcb_t *old_task = scheduler.running_task;

			tcb_list_t *list = &scheduler.ready_lists[scheduler.current_priority];

			dequeue(list);
			enqueue(list, old_task);
		}
		
		// Set pendSV exception to pending
		SCB->ICSR=(SCB->ICSR|set_pendsv);
	}
}

__attribute__((noreturn, naked)) // naked to prevent compiler from using r7 as the frame pointer
void PendSV_Handler(void) {
	// THREE scheduling cases:
	//	1. New higher priority task became ready - need to move up in priority
	//	2. Current priority task was blocked and current priority list is empty - need to move down in priority
	//	3. Current priority task was blocked and there are other tasks at this priority - round robin
	
	tcb_t *old_task;
	tcb_t *new_task;

	// Check if scheduler was called for new higher priority task
	if(scheduler.current_priority < scheduler.running_task->priority) {
		priority_t old_priority = scheduler.running_task->priority;
		
		// If old task just released a mutex (READYing a higher priority) it will be at the tail of its queue
		if(scheduler.running_task->mutex_released) {
			old_task = scheduler.ready_lists[old_priority].tail;
			scheduler.running_task->mutex_released = false;
		} else {
			old_task = scheduler.ready_lists[old_priority].head;
		}
		
		new_task = scheduler.ready_lists[scheduler.current_priority].head;
		
		// Update running task to new task
		scheduler.running_task = new_task;
	}
	
	// Else the (old) running task of the current priority level could have been removed
	else if(scheduler.ready_lists[scheduler.current_priority].head == NULL) {
		old_task = scheduler.running_task;
		
		while(scheduler.ready_lists[scheduler.current_priority].head == NULL) {
			scheduler.current_priority++; // Incrementing is decreasing priority
		}
		
		new_task = scheduler.ready_lists[scheduler.current_priority].head;
		scheduler.running_task = new_task;
	}
	
	// Else the (old) running task of current priority was dequeued.
	// Run round-robin for the current priority level
	else {
		old_task = scheduler.running_task;
		new_task = scheduler.ready_lists[scheduler.current_priority].head;
		
		scheduler.running_task = new_task;
	}

	// Remove pending state from pendSV exception
	const uint32_t clear_pendsv = (1<<27);
	SCB->ICSR=(SCB->ICSR|clear_pendsv);

	// context_switch logic does not handle switching from and to the same task
	if(old_task == new_task) {
		__asm volatile (
			"BX LR		\n"
			:::
		);
	}

//	context_switch(new_task->stack_pointer, (uint32_t)&old_task->stack_pointer);

	uint32_t *new_task_stack_ptr = new_task->stack_pointer;
	uint32_t old_task_stack_ptr_loc = (uint32_t)&old_task->stack_pointer;

// Reason for this assembly is so we can call `b context_switch` (as opposed to compiler's `BL context_switch`)
// This is a pure branch - LR will be preserved with our exception return value (EXC_RETURN)
	__asm volatile (
			"mov r0, %0			\n"
			"mov r1, %1			\n"
			"b context_switch	\n"
			:
			: "r"(new_task_stack_ptr), "r"(old_task_stack_ptr_loc)
			: "r0", "r1", "memory"
		);

	// Tells C compiler this will not be reached, therefore no function epilogue which is good
	// because we blew up the compilers assumptions in context_switch()
	__builtin_unreachable();
}

void enqueue(tcb_list_t *list, tcb_t *tcb){
	if(list->head == NULL){
		list->head = tcb;
		list->tail = tcb;
	}
	else{
		list->tail->tcb_pointer = tcb;
		list->tail = tcb;
	}
	list->size++;
	return;
}

tcb_t* dequeue(tcb_list_t *list){
	if(list->head == NULL)
		return NULL;
	else if(list->head == list->tail){
		tcb_t* temp_ptr = list->head;
		temp_ptr->tcb_pointer = NULL;
		list->head = NULL;
		list->tail = NULL;
		list->size--;
		return temp_ptr;
	}
	else{
		// Set dequeued tcb's tcb_pointer to NULL and set new head
		tcb_t* temp_ptr = list->head;
		list->head = list->head->tcb_pointer;
		temp_ptr->tcb_pointer = NULL;
		list->size--;
		return temp_ptr;
	}
}

tcb_t* remove_from_list(tcb_list_t *list, tcb_t *tcb){
	if(list->head == NULL)
		return NULL;
	else{
		tcb_t *current = list->head;
		tcb_t *previous = NULL;
		
		// If tcb to be returned is first in list
		if(current == tcb){
			if(current->tcb_pointer == NULL){
				list->head = NULL;
			}
			else{
				list->head = current->tcb_pointer;
				current->tcb_pointer = NULL;
			}
			list->size--;
			return current;
		}
		
		// Iterate through list to find tcb
		while(current != tcb && current != NULL){
			previous = current;
			current = current->tcb_pointer;
		}
		if(current == NULL){
			return NULL;
		}
		
		// Two cases, tcb being removed is last in list, or it is not last in list
		if(current->tcb_pointer == NULL){
			previous->tcb_pointer = NULL;
		}
		else{
			previous->tcb_pointer = current->tcb_pointer;
			current->tcb_pointer = NULL;
		}
		list->size--;
		return current;
	}
}

void add_task_to_sched(scheduler_t *scheduler, tcb_t *tcb){
	// Note: this is now called only (multiple times) before scheduler is "started"

	priority_t priority = tcb->priority;
	
	// Get ready list of proper priority
	tcb_list_t *list = &scheduler->ready_lists[priority];
	
	enqueue(list, tcb);
	
	// Check if added task is of higher priority than current running task (lower value = higher priority)
	if(priority < scheduler->current_priority){
		// Update scheduler with new priority and new task to run
		scheduler->current_priority = priority;
	}
}
