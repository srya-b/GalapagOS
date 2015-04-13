#ifndef _INT_HANDLERS_H
#define _INT_HANDLERS_H

#ifndef ASM
#include "types.h"
#include "lib.h"
#include "i8259.h"
#include "terminal.h"

#define KEYBOARD_INT 1
#define SLAVE_INT 2
#define RTC_INT 8
#define KEYBOARD_PORT 0x60
#define REG_C 0x0C
#define SCAN_LIMIT 80

 /* Exceptions */
 void divide_by_zero();

 void debugger();

 void NMI();

 void breakpoint();

 void overflow();

 void bounds();

 void invalid_opcode();

 void coprocessor();

 void double_fault();

 void coprocessor_segment();

 void invalid_tss();

 void segment_not_present();

 void stack_segment_fault();

 void general_protection_fault();

 void page_fault();

 void floating_point();

 void alignment_check();

 void machine_check();

 void SIMD_floating_point();

/* Interrupts */
 void keyboard_int();

 void rtc_int();

#define call_halt(void)             \
do {                                \
    asm volatile(                   \
        "movl $0, %%eax;           \
        int $0x80;"					\
         );                         \
} while(0)


#endif
#endif
