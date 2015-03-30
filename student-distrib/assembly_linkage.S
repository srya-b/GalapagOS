#define ASM 1
#include "assembly_linkage.h"

.text
.globl divide_by_zero_linkage, debugger_linkage, NMI_linkage, breakpoint_linkage, overflow_linkage, bounds_linkage, invalid_opcode_linkage
.globl coprocessor_linkage, double_fault_linkage, coprocessor_segment_linkage, invalid_tss_linkage, segment_not_present_linkage, stack_segment_fault_linkage
.globl general_protection_fault_linkage, page_fault_linkage, floating_point_linkage, alignment_check_linkage, machine_check_linkage, SIMD_floating_point_linkage
.globl keyboard_int_linkage, rtc_int_linkage

 
 # divide_by_zero_linkage
 #   DESCRIPTION: divide by zero exception assembly linkage, pushes flags and registers before
 #					calling divide by zero expection handler, then pops flags and registers after handler
 #   INPUTS: none
 #   OUTPUTS: none
 #   RETURN VALUE: none
 #   SIDE EFFECTS saves/restores all registers & flags, calls exception handler

divide_by_zero_linkage:
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	divide_by_zero
	popal
	popfl
	leave
	iret		

 # debugger_linkage
 #   DESCRIPTION: debugger exception assembly linkage, pushes flags and registers before
 #					calling expection handler, then pops flags and registers after handler
 #   INPUTS: none
 #   OUTPUTS: none
 #   RETURN VALUE: none
 #   SIDE EFFECTS: saves/restores all registers & flags, calls exception handler
debugger_linkage: 
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	debugger
	popal
	popfl
	leave
	iret

NMI_linkage: 
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	NMI
	popal
	popfl
	leave
	iret

breakpoint_linkage:
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	breakpoint
	popal
	popfl
	leave
	iret

overflow_linkage:
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	overflow
	popal
	popfl
	leave
	iret

bounds_linkage: 
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	bounds
	popal
	popfl
	leave
	iret

invalid_opcode_linkage:
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	invalid_opcode
	popal
	popfl
	leave
	iret

coprocessor_linkage:
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	coprocessor
	popal
	popfl
	leave
	iret

double_fault_linkage: 
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	double_fault
	popal
	popfl
	leave
	iret

coprocessor_segment_linkage: 
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call	coprocessor_segment
	popal
	popfl
	leave
	iret

invalid_tss_linkage:
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	invalid_tss
	popal
	popfl
	leave
	iret

segment_not_present_linkage:
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	segment_not_present 
	popal
	popfl
	leave
	iret

stack_segment_fault_linkage:
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	stack_segment_fault
	popal
	popfl
	leave
	iret


general_protection_fault_linkage:
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	general_protection_fault
	popal
	popfl
	leave
	iret

page_fault_linkage:
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	page_fault
	popal
	popfl
	leave
	iret

floating_point_linkage: 
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	floating_point
	popal
	popfl
	leave
	iret

alignment_check_linkage:
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	alignment_check
	popal
	popfl
	leave
	iret

machine_check_linkage: 
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	machine_check
	popal
	popfl
	leave
	iret

SIMD_floating_point_linkage: 
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	SIMD_floating_point
	popal
	popfl
	leave
	iret

keyboard_int_linkage: 
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	keyboard_int
	popal
	popfl
	leave
	iret

rtc_int_linkage:
	pushl 	%ebp
	movl	%esp, %ebp
	pushfl
	pushal
	call 	rtc_int
	popal
	popfl
	leave
	iret
