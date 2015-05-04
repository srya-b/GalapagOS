#ifndef _FILE_ARRAY_H
#define _FILE_ARRAY_H

#include "x86_desc.h"
#include "types.h"
#include "lib.h"
#include "paging.h"
#include "file_system.h"
#include "terminal.h"
#include "rtc.h"
#include "schedule.h"
#include "i8259.h"
#include "pit.h"

//#include "process.h"
#define FILE_ARRAY_SIZE 8
#define MAX_ARGS 100
#define MAX_COMMAND 33

#ifndef ASM

typedef struct op_table {
  int (*open)();
  int (*read)();
  int (*write)();
  int (*close)();
}  __attribute__((packed)) op_table_t;

typedef struct file_descriptor {
  op_table_t* file_op_table_ptr;
  struct inode_t*  inode_ptr;
  uint32_t  file_position;
  uint32_t  flags;  
}  __attribute__((packed)) file_descriptor_t;

typedef struct pcb {
  int32_t pid;
  void * program;
  void * kernel_stack;
  void * user_stack;
  void * instr_ptr;
  struct pcb* parent;
  struct pcb* child;  
  file_descriptor_t files[FILE_ARRAY_SIZE];
  char command[MAX_COMMAND];
  char args[MAX_ARGS];
  uint32_t esp;
  uint32_t ebp;
  uint8_t ret_status;
  int terminal;
} __attribute__((packed)) pcb_t;

int32_t khalt(uint8_t status);
int32_t kexecute(const uint8_t* command);
int32_t kread(int32_t fd, void* buf, int32_t nbytes);
int32_t kwrite(int32_t fd, const void* buf, int32_t nbytes);
int32_t kopen(const uint8_t* fileame);
int32_t kclose(int32_t fd);
int32_t kget_args(uint8_t* buf, int32_t nbytes);
int32_t kvidmap(uint8_t** screen_start);
int32_t kset_handler(int32_t signum, void* handler_address);
int32_t ksigreturn(void);
int32_t init_process(int32_t curr_pid);
void increment_position(struct inode_t* inode, int amt);
int32_t get_curr_process_terminal();
int32_t swap_vid_mem(int old_term, int new_term);
pcb_t* get_pcb_address(int pid);
int32_t set_current_process(pcb_t* pcb);
void reset_curr_process(pcb_t* pcb);
pcb_t* get_curr_process();

#define set_esp(addr)               \
do {                                \
    asm volatile(                   \
        "movl %0, %%esp"            \
        :                           \
        : "r" (addr)                \
         );                         \
} while(0)


#define set_ebp(addr)               \
do {                                \
    asm volatile(                   \
        "movl %0, %%ebp"            \
        :                           \
        : "r" (addr)                \
         );                         \
} while(0)

#define set_selectors(num)          \
do {                                \
      asm volatile(                 \
        "mov  %%ax, %%ds"           \
         : /* no outputs */         \
         : "a"(num)                 \
        );                          \
} while (0)

#define setup_iret_stack(ss, esp, flags, cs, eip)          \
do {                                                        \
    asm volatile(                                           \
       "pushl %0\n                                         \
        pushl %1\n                                          \
        pushl %2\n                                         \
        pushl %3 \n                                         \
        pushl %4 \n                                         \
        iret"                                               \
        :                                                   \
        : "r"(ss), "r"(esp), "r"(flags), "r"(cs), "r"(eip)                    \
         );                                                 \
} while(0)  

#define get_esp_and_ebp(esp, ebp)   \
do {                                \
    asm volatile(                   \
        "movl %%esp, %0\n            \
        movl %%ebp, %1"             \
        : "=a" (esp), "=b" (ebp)    \
        );                          \
} while(0)


#endif
#endif
