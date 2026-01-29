/*
 * context_switch.c
 *
 *  Created on: Jan 15, 2026
 *      Author: julianlowery
 */

__attribute__((noreturn, naked)) void PendSV_Handler(void) {
    __asm volatile(
        // save current task context to stack immediately
        "MRS   R0, PSP				\n"
        "STMDB R0!, {R4-R11}		\n"

        // store updated old task stack pointer into old task tcb struct
        "LDR	R1, =scheduler		\n"          // R1 now holds address of scheduler struct
        "LDR	R1, [R1]			\n"  // R1 now holds address of old tcb
        "STR	R0, [R1, #4]		\n"          // Store PSP into old task tcb (second member)

        "PUSH	{R3, LR}			\n"  // save LR (EXC_RETURN) (R3 as well just to
                                                     // keep stack 8-byte aligned)
        "BL		scheduler_update	\n"  // scheduler now points to new task tcb
        "POP	{R3, LR}			\n"

        // get new task stack pointer from tcb
        "LDR	R1, =scheduler		\n"
        "LDR	R1, [R1]			\n"
        "LDR	R0, [R1, #4]		\n"

        // restore context of new task
        "LDMIA	R0!, {R4-R11}		\n"
        "MSR	PSP, R0				\n"

        "BX		LR					\n"

        ::
            :);
}

__attribute__((naked)) void SVC_Handler(void) {
    // TODO: could use the svc # to determine which task should be kicked off first based on
    // priority. but for now just using task0

    __asm volatile(
        // move PSP into R0
        "MRS   R0, PSP	\n"
        "ADD   R0, R0, #32	\n"  // remove/skip R0–R3,R12,LR,PC,xPSR - these were stacked on PSP
                                     // entering SVC_Handler

        // "Load Multiple, Increment After". Effectively pop R4-R11 from task
        // stack starting at value in R0. "!" causes the final address update be in R0
        "LDMIA R0!, {R4-R11}	\n"

        // move new psp value from R0 back to PSP register (setting PSP)
        "MSR   PSP, R0	\n"

        :
        :
        : "r0", "memory");
}
