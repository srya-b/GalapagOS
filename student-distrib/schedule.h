#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include "x86_desc.h"
#include "types.h"
#include "lib.h"
#include "file_array.h"
#include "i8259.h"
#include "assembly_linkage.h"

#ifndef ASM

typedef enum task_state {
	TASK_RUNNING = 1,
	TASK_INACTIVE = 0,
	TASK_SENTINEL = -1,
	TASK_FINISHED = 2
} task_state_t;

typedef struct iret_context {
  uint32_t eip;
  uint32_t cs;
  uint32_t flags;
  uint32_t esp;
  uint32_t ss;
} __attribute__((packed)) iret_t;

typedef struct gen_regs
{
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t iesp;
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;  
} __attribute__((packed)) gen_regs_t;

typedef struct aregs_t
{
  gen_regs_t gen_regs;
  uint32_t iflags;
  iret_t iret;
} __attribute__((packed)) aregs_t;

typedef struct run_queue_node {
	struct run_queue_node * next_task;
	struct run_queue_node * prev_task;
	struct pcb_t * process;
	task_state_t running;
	aregs_t registers;
} __attribute__((packed)) run_queue_node_t;

void pit_handler(iret_t* registers);
void schedule(uint32_t pid);
void sched_init();
void unschedule(uint32_t pid, uint32_t parent_pid);

#define setup_switch(ss, esp, flags, cs, eip)          \
do {                                                        \
    asm volatile(                                           \
       "pushl %0\n                                         \
        pushl %1\n                                          \
        pushl %2\n                                         \
        pushl %3 \n                                         \
        pushl %4 \n   									    \
        movl %0, %%ds  \n                 \
        iret"                                               \
        :                                                   \
        : "r"(ss), "r"(esp), "r"(flags), "r"(cs), "r"(eip)                    \
         );                                                 \
} while(0)  

#endif
#endif
