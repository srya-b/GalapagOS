#ifndef _ASSEMBLY_LINNKAGE_H
#define _ASSEMBLY_LINNKAGE_H

#ifndef ASM

#include "types.h"
#include "lib.h"
#include "int_handlers.h"
#include "file_array.h"


extern void divide_by_zero_linkage();
extern void debugger_linkage();
extern void NMI_linkage();
extern void breakpoint_linkage();
extern void overflow_linkage();
extern void bounds_linkage();
extern void invalid_opcode_linkage();
extern void coprocessor_linkage();
extern void double_fault_linkage();
extern void coprocessor_segment_linkage();
extern void invalid_tss_linkage();
extern void segment_not_present_linkage();
extern void stack_segment_fault_linkage();
extern void general_protection_fault_linkage();
extern void page_fault_linkage();
extern void floating_point_linkage();
extern void alignment_check_linkage();
extern void machine_check_linkage();
extern void SIMD_floating_point_linkage();
extern void keyboard_int_linkage();
extern void rtc_int_linkage();
extern void system_call_linkage();
extern void* halt_return;


#endif
#endif


