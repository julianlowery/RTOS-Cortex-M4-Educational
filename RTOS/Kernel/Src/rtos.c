#include <stdio.h>
#include <string.h>
#include <stm32f4xx.h>

#include "rtos.h"
#include "scheduler.h"

extern tcb_t tcb_array[6];
extern tcb_t tcb_main;

extern scheduler_t scheduler;
extern bool run_scheduler;

uint32_t msTicks = 0;

static void idle_task() {
	while (1) {
		__WFI();
	}
}

void rtos_init(){
	NVIC_SetPriority(PendSV_IRQn, 0xFF);   // lowest
	NVIC_SetPriority(SysTick_IRQn, 0xFE);  // just above PendSV (we don't want Pend_SV to nest inside sysTick)

	const uint32_t stack_size = 0x100; // Actaully physical stack_size/4 to account for 32 bit pointer incrementation
	const uint32_t num_tcbs = 6;
	const uint8_t main_task_num = 0;
	const uint32_t vector_table_address = 0x0;
	
	tcb_array[main_task_num].task_id = main_task_num;
	tcb_array[main_task_num].stack_pointer = tcb_array[main_task_num].stack_base_address; // This will be set properly as soon as task is switched out of
	tcb_array[main_task_num].state = READY;
	tcb_array[main_task_num].priority = IDLE;
	tcb_array[main_task_num].stack_size = 0; // not used yet
	tcb_array[main_task_num].mutex_released = false;
	
	// Initalize all task stack addresses (including main task stack)
	uint32_t *initial_sp_pointer = (uint32_t*)vector_table_address; // first vector table entry holds initial msp
	tcb_main.stack_base_address = (uint32_t*)(*initial_sp_pointer);
	tcb_main.stack_overflow_address = tcb_main.stack_base_address-(stack_size*2);
	
	for(uint8_t tcb_num = 0; tcb_num < num_tcbs; tcb_num++){
		tcb_array[tcb_num].stack_base_address = (tcb_main.stack_overflow_address)-(stack_size*(tcb_num));
		tcb_array[tcb_num].stack_overflow_address = tcb_main.stack_overflow_address-((stack_size*(tcb_num+1))); // remove -1
	}
	
	// TODO (additional safety) enforce stack 8 byte alignment, should do a check here.

	task_create(idle_task, NULL, IDLE);
}

static bool push_to_stack(tcb_t *tcb, uint32_t value){
	tcb->stack_pointer--;

	if(tcb->stack_pointer <= tcb->stack_overflow_address){
		return false;
	}

	*(tcb->stack_pointer) = value;
	return true;
}

static void task_exit_error() {
	while(1) {}
}

void task_create(rtosTaskFunc_t function_pointer, void* function_arg, priority_t task_priority){
	static uint8_t task_number = 0;
	uint32_t default_psr_val = 0x01000000;

 	tcb_array[task_number].task_id = task_number;
	tcb_array[task_number].stack_pointer = tcb_array[task_number].stack_base_address;
	tcb_array[task_number].state = READY;
	tcb_array[task_number].priority = task_priority;
	tcb_array[task_number].stack_size = 0; // not used yet

	// push all stack values incrementing psp (set up the stack for first context switch)
	push_to_stack(&tcb_array[task_number], default_psr_val);
	push_to_stack(&tcb_array[task_number], (uint32_t)function_pointer);
	push_to_stack(&tcb_array[task_number], (uint32_t) task_exit_error);
	for(uint8_t count = 0; count < 4; count++)
		push_to_stack(&tcb_array[task_number], 0x1);
	push_to_stack(&tcb_array[task_number], (uint32_t)function_arg);
	for(uint8_t count = 0; count < 8; count++)
		push_to_stack(&tcb_array[task_number], 0x1);

	add_task_to_sched(&scheduler, &tcb_array[task_number]);

	task_number++;
}

void rtos_start() {
	// SysTick to 1ms
	// 16,000,000 Hz / 1,000 = 16,000 ticks (systick will trigger every 16,000 ticks)
	// On a 16 MHz clock that's every 1ms
	SysTick_Config(SystemCoreClock/1000);

	// Set MSP register back to base address of main() stack - effectively nukes the main() thread (locals are lost here)
	__set_MSP((uint32_t)tcb_main.stack_base_address);

	// We should find the highest priority and start with that. Just starting with task 0 for now
	__set_PSP((uint32_t)tcb_array[0].stack_pointer);
	const uint32_t psp_enable = (1<<1);

	// Enable PSP, when we exit the Handler we will start executing the first task
	__set_CONTROL((uint32_t)__get_CONTROL()|psp_enable);
	__ISB(); // flush (dump) pipeline, guarantee new instructions are fetched in new state


	__asm volatile ("svc 0");

	while(true){}

}

void SysTick_Handler(void) {
	const uint32_t time_slice_len = 100;
	static uint32_t time_slice_count = time_slice_len;

    msTicks++;

    // time slice math and reset is decoupled from scheduler run logic
    bool time_slice_expired = false;
    if (time_slice_count > 0) {
    	time_slice_count--;
    }

    if (time_slice_count == 0) {
    	time_slice_expired = true;
    	time_slice_count = time_slice_len;
    }

	if(run_scheduler){
		run_scheduler = false;
		SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
	}

	else if(time_slice_expired){
		// If old task and new task are same priority, rearrange (cycle) the queue
		// for the scheduler
		if(scheduler.running_task->priority == scheduler.current_priority) {
			tcb_t *old_task = scheduler.running_task;
			tcb_list_t *list = &scheduler.ready_lists[scheduler.current_priority];
			dequeue(list);
			enqueue(list, old_task);
		}

		SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
	}
}

void semaphore_init(semaphore_t *sem, uint8_t initial_count){
	sem->count = initial_count;
	sem->block_list.head = NULL;
	sem->block_list.tail = NULL;
	sem->block_list.size = 0;
}

void semaphore_take(semaphore_t *sem){
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	if(sem->count > 0)
		sem->count--;
	else{
		enqueue(&sem->block_list, scheduler.running_task);
		dequeue(&scheduler.ready_lists[scheduler.current_priority]);
	}
	__enable_irq();
	__set_PRIMASK(primask);

	// Immediately run scheduler to switch tasks.
	// Barriers are used to guarantee immediate effect.
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    __DSB(); // ensure write to hardware completes
    __ISB(); // flush CPU pipeline, guarantee new instruction fetch accounts for the updated hardware state
}

void semaphore_give(semaphore_t *sem){
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	if(sem->block_list.head == NULL) {
		sem->count++;
	} else {
		tcb_t * freed_task = dequeue(&sem->block_list);
		// Add unblocked task to scheduler ready queue
		enqueue(&scheduler.ready_lists[freed_task->priority], freed_task);
		if(freed_task->priority < scheduler.running_task->priority){ // lower number is higher priority
			// Update priority to scheduler can switch to higher priority
			scheduler.current_priority = freed_task->priority;
		}
	}
	__enable_irq();
	__set_PRIMASK(primask);

	// Immediately run scheduler to switch tasks
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}

void mutex_init(mutex_t *mutex){
	mutex->available = true;
	mutex->owner_tcb = NULL;
	mutex->owner_true_priority = IDLE;
	mutex->inherited_priority = IDLE;
	mutex->block_list.head = NULL;
	mutex->block_list.tail = NULL;
	mutex->block_list.size = 0;
}

void mutex_take(mutex_t *mutex){
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	if(mutex->available == true) {
		mutex->available = false;
		mutex->owner_tcb = scheduler.running_task;
		mutex->owner_true_priority = scheduler.running_task->priority;
		mutex->inherited_priority = scheduler.running_task->priority;
		mutex->block_list.head = NULL;
		mutex->block_list.tail = NULL;
		mutex->block_list.size = 0;
		__enable_irq();
		__set_PRIMASK(primask);

	} else {
		enqueue(&mutex->block_list, scheduler.running_task);
		dequeue(&scheduler.ready_lists[scheduler.current_priority]);
		
		// Handle priority inheritance case
		if(scheduler.current_priority < mutex->inherited_priority){ // low priority number is a higher priority	
			
			// Remove mutex owner task from its ready queue with remove function
			remove_from_list(&scheduler.ready_lists[mutex->inherited_priority], mutex->owner_tcb);
			// Add mutex owner task to proper new priority level in ready queue
			enqueue(&scheduler.ready_lists[scheduler.current_priority], mutex->owner_tcb);
			
			mutex->inherited_priority = scheduler.current_priority;
			mutex->owner_tcb->priority = mutex->inherited_priority;
		}

		__enable_irq();
		__set_PRIMASK(primask);

		// Immediately run scheduler to switch tasks
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	    __DSB();
	    __ISB();
	}
}

void mutex_give(mutex_t *mutex) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	
	bool should_yield = false;
	
	// Only proceed if running task owns the mutex and mutex is locked
	if(scheduler.running_task == mutex->owner_tcb && mutex->available == false) {

		// If no tasks are blocked on the mutex, reset it and exit
		if(mutex->block_list.head == NULL){
			mutex->available = true;
			mutex->owner_tcb = NULL;
			mutex->owner_true_priority = IDLE;
			mutex->inherited_priority = IDLE;
			mutex->block_list.head = NULL;
			mutex->block_list.tail = NULL;
		}
		
		// Else we need to handoff the mutex
		else {
		
			// If we we had inherited a high priority while holding mutex
			if(mutex->inherited_priority < mutex->owner_true_priority) {

				// Set running task back to true priority
				scheduler.running_task->priority = mutex->owner_true_priority;

				// Set running task back to its true priority ready list
				dequeue(&scheduler.ready_lists[scheduler.current_priority]);
				enqueue(&scheduler.ready_lists[mutex->owner_true_priority], mutex->owner_tcb);

				// Set flag to handle special scheduling case
				scheduler.running_task->mutex_released = true;

			}

			// Adjust mutex attributes to new unblocked task
			mutex->owner_tcb = mutex->block_list.head;
			mutex->owner_true_priority = mutex->block_list.head->priority;
			mutex->inherited_priority = mutex->block_list.head->priority;

			// Set unblocked task to scheduler ready queue
			tcb_t * freed_task = dequeue(&mutex->block_list);
			enqueue(&scheduler.ready_lists[freed_task->priority], freed_task);

			should_yield = true;

		}
		
	}

	__enable_irq();
	__set_PRIMASK(primask);

	if (should_yield) {
		// Immediately run scheduler to switch tasks
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		__DSB();
		__ISB();
	}
}


bool queue_init(queue_t *queue, void *buffer, size_t item_size, size_t cap) {
	if (queue == NULL || buffer == NULL) {
		return false;
	}
	queue->buffer = (uint8_t*)buffer;
	queue->cap = cap;
	queue->item_size_bytes = item_size;
	queue->head = 0;
	queue->tail = 0;

	mutex_init(&queue->mut);
	semaphore_init(&queue->sem_avail_data, 0);
	semaphore_init(&queue->sem_avail_space, cap);
	return true;
}

void queue_send(queue_t *queue, void *src) {
	semaphore_take(&queue->sem_avail_space);
	mutex_take(&queue->mut);

	void *dest = &queue->buffer[queue->head * queue->item_size_bytes];
	memcpy(dest, src, queue->item_size_bytes);
	queue->head = (queue->head + 1) % queue->cap;

	mutex_give(&queue->mut);
	semaphore_give(&queue->sem_avail_data);
	return;
}

void queue_recieve(queue_t *queue, void *dest){
	semaphore_take(&queue->sem_avail_data);
	mutex_take(&queue->mut);

	void *src = &queue->buffer[queue->tail * queue->item_size_bytes];
	memcpy(dest, src, queue->item_size_bytes);
	queue->tail = (queue->tail + 1) % queue->cap;

	mutex_give(&queue->mut);
	semaphore_give(&queue->sem_avail_space);
	return;
}

void yield() {
	// Just cycle current priority list
	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	tcb_t *old_task = scheduler.running_task;
	tcb_list_t *list = &scheduler.ready_lists[scheduler.current_priority];

	dequeue(list);
	enqueue(list, old_task);

	__enable_irq();
	__set_PRIMASK(primask);

	// Immediately run scheduler to switch tasks
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}
