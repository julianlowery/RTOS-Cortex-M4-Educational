# Cortex-M4 Real-Time Operating System

## Project Overview
An educational, custom-built preemptive kernel designed for the ARM Cortex-M4 architecture. Implements fixed-priority preemptive task scheduling, semaphores, priority-inheritance mutexes, and message queues. Originally implemented in **2019**, and recently revived and extended on new hardware.

**Hardware Used:** STM32F4xx (STM32 Nucleo-F401RE)

**Toolchain:** STM32CubeIDE (GCC ARM)

---

## Scope
#### Kernel Features
* **Preemptive Scheduling:** Fixed-priority scheduler. Uses Round-Robin for same-priority tasks.
* **Task Management:** Static allocation for up to 11 user-defined tasks (fixed-size TCB and stack arrays). Includes a system Idle Task.
* **Context Switching:** Manual implementation of the context switch using `PendSV`.
* **Stack Initialization:** Software-defined construction of the initial stack frame (xPSR, PC, LR, R0-R3, R12) for new tasks.
* **Mutexes:** Binary mutexes with priority inheritance.
* **Semaphores:** Counting semaphores.
* **Communication:** Message queues for inter-task data transfer.
* **Yield API:** Explicitly relinquishes the CPU to allow another ready task of equal priority to run.

#### Limitations (아직!)
* **Recursive Locking:** Mutexes do not support nested access (recursive take/give).
* **Stack Configuration:** Stack size is fixed globally; per-task stack sizes are not supported.
* **Dynamic Allocation:** No kernel support for dynamic task creation or kernel-managed heap allocation. (`malloc` possible, but not thread safe).
* **FPU Support:** Floating Point Unit context is not saved during context switches. Tasks are limited to integer arithmetic.
* **I/O Safety:** `printf` output (mapped to UART) is strictly for debugging and is not thread-safe.

## Architecture

#### Project Layout
This project separates the **Application Logic**, **RTOS Kernel**, and **System/Hardware**.

* **Application Layer (`/Application`):** Contains user logic and tasks and includes four demonstration applications. It is decoupled from the hardware and only interacts with the RTOS API.
* **RTOS Layer (`/RTOS`):** The kernel. It handles scheduling, TCB management, synchronization primitives, and the specific Cortex-M4 context switching mechanics.
* **System Layer (`/System`):** Bridges the chip and the software. Contains the Startup code, Linker scripts, and HAL configuration.


```text
rtos_project/
├── Application/       # User Code (Hardware Agnostic)
│   ├── Inc/           # Application headers (app_main.h)
│   └── Src/           # Task implementations (app_main.c)
├── RTOS/              # The Kernel (Cortex-M4 Specific)
│   ├── Kernel/Inc     # Public API (rtos.h)
│   └── Kernel/Src     # Implementation (rtos.c, scheduler.c, context_switch.c)
├── System/            # Hardware & Vendor Specifics
│   ├── LowLevel/      # Startup.s, syscalls.c, sysmem.c
│   ├── Interrupts/    # Default vector table handlers (unused)
│   └── Config/        # HAL Configuration
└── Drivers/           # ST-HAL and CMSIS Libraries
```

## Design

#### Interrupt Model
The kernel uses ARM Cortex-M system exceptions for scheduling:
- **SysTick**: Periodic tick interrupt used to drive time slicing and preemption.
- **PendSV**: Lowest-priority exception used exclusively for context switching.
- **SVC**: Used only during RTOS startup; all kernel APIs are invoked from thread mode.

#### Concurrency Model
- Kernel data structures are protected using critical sections implemented via PRIMASK  (global interrupt masking).
- RTOS APIs are intended to be called from thread mode only.

#### RTOS Memory Map (96KiB SRAM)

```
Higher Addresses
   +-------------------------+  <-- 0x2001 8000 (End of SRAM)
   |                         |
   |   MSP (Handler) Stack   |  ⬇️ Stack Grows Down
   |      (Size: 2KB)        |
   |                         |
   +-------------------------+  <-- 0x2001 7800
   |     Idle Task Stack     |
   |      (Size: 1KB)        |
   +-------------------------+  <-- 0x2001 7400
   |      Task 0 Stack       |
   |      (Size: 1KB)        |
   +-------------------------+  <-- 0x2001 7000
   |      Task 1 Stack       |
   |      (Size: 1KB)        |
   +-------------------------+  <-- 0x2001 6C00
   |                         |
   ~   . . . [Tasks 2-9] . . ~
   |                         |
   +-------------------------+  <-- 0x2001 4C00
   |      Task 10 Stack      |
   |      (Size: 1KB)        |
   +=========================+  <-- 0x2001 4800 (Stack Top Limit)
   |                         |
   |    Available Memory     |
   |   (Heap Area if used)   |
   |                         |
   |                         |  ⬆️ Heap Grows Up
   +=========================+  <-- [ _end symbol ]
   |          .bss           |
   |  [ TCB Array (Static) ] |  <-- RTOS State/Metadata
   |  [ Queue Buffers      ] |
   +-------------------------+
   |          .data          |
   | (Initialized variables) |
   +-------------------------+  <-- 0x2000 0000 (Start of SRAM)
        Lower Addresses

* Note: The Main Stack Pointer (MSP) is used for the Handler Stack (Interrupts/Kernel), 
  while the Process Stack Pointer (PSP) is used for all User Tasks.
```

## Demo Output

#### Scheduler Demo
**Path:** Application/Src/scheduler_demo.c
**Setup:** Four tasks of the same priority.
**Sequence:** Round robin scheduling between tasks, each task logging a message each time they are scheduled.
**Output:**
```
Starting...

Running Scheduling Demo...

Task1: 1
Task2: 2
Task3: 3
Task4: 4
Task1: 5
Task2: 6
Task3: 7
Task4: 8
Task1: 9
Task2: 10
Task3: 11
Task4: 12

Complete
```

#### Semaphore Demo
**Path:** Application/Src/semaphore_demo.c
**Setup:** Two high priority tasks, two low priority tasks.
**Sequence:** 
* High priority tasks are running, one at a time they block on a semaphore.
* Low priority tasks run in round robin.
* Low task2 gives the semaphore - unblocks high task1.
* High task1 runs and gives the semaphore again - unblocks high task2.

**Output:**
```
Starting...

Running Semaphore Demo...

High task1 running
High task1 taking high_sem (and blocking)
High task2 running
High task2 taking high_sem (and blocking)
Low task1 running
Low task1 yielding
Low task2 running
Low task2 yielding
Low task1 running
Low task1 yielding
Low task2 running
Low task2 giving sem_high (unblocking high task1)
High task1 running
High task1 giving sem_high (unblocking high task2) and yielding
High task2 running
High task2 yielding
High task1 running

Complete
```

#### Mutex Demo (Priority Inheritance)
**Path:** Application/Src/mutex_demo.c
**Setup:** One high priority task, one medium priority task, one low priority task.
**Sequence:**
* High task and medium task both block on semaphore.
* Low task takes a mutex (now holding mutex).
* Low task releases semaphore - unblocks high task
* High task releases semaphore - unblocks medium task.
* High task tries to take the mutex and blocks.
* Low task is temporarily promoted to high priority, preventing the medium task from preempting it.
* Low task runs and releases mutex.
* High task takes mutex and runs.

**Output:**
```
Starting...

Running Mutex (Priority Inheritance) Demo...

High task running
High task taking semaphore - blocking
Medium task running
Medium task taking semaphore - blocking
Low task running - taking mutex
Low task releasing semaphore - unblocking High task
High task running - releasing semaphore, unblocking medium task
High task taking mutex - blocking
Low task running at HIGH priority (NOTE: medium task is currently READY) (inheritance)
Low task releasing mutex - unblocking High task
High task running
High task releasing mutex

Complete
```

#### Message Queue Demo
**Path:** Application/Src/message_queue_demo.c
**Setup:** Two medium-priority producer tasks, one high priority consumer task
**Sequence:**
* High priority consumer blocks on the empty message queue.
* Producer task1 and producer task2 take turns producing values.
* As soon as a producer produces a message, it is consumed and printed by the high priority consumer.

**Output:**
```
Starting...

Running Message Queue Demo... 

Producer 1 producing value 11
Consumer received 11
Producer 2 producing value 21
Consumer received 21
Producer 1 producing value 12
Consumer received 12
Producer 2 producing value 22
Consumer received 22

Complete
```
