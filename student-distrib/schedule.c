#include "schedule.h"

#define NUM_PROCESSES 6
#define NUM_SHELLS 3
#define STACK_FLAGS 0x00000202
#define NUM_IRET_STACK 5
#define NUM_REG_STACK 8
#define MOVE_NEW_STACK 10

static run_queue_node_t run_queue[NUM_PROCESSES];
static run_queue_node_t sentinel = {.next_task = NULL, .prev_task = NULL,  .process = NULL, .running = TASK_SENTINEL};
static run_queue_node_t* curr_node;
static run_queue_node_t* head;
static run_queue_node_t* tail;
static int first_shell = 1;
static int first_schedule = 1;


/* 
 * pit_handler
 *   DESCRIPTION: changes what task to process
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void pit_handler(iret_t* regs)
{

	/* Get next task to be executed */
	run_queue_node_t* new_task;
	pcb_t* curr_process;

	/*setup next task to process*/
	if (first_shell && curr_node == &sentinel)
	{
		new_task = curr_node->next_task;
		curr_process = (pcb_t*)new_task->process;
		aregs_t new_stack;
		new_stack.iret.eip = (uint32_t)curr_process->instr_ptr;
		new_stack.iret.cs = USER_CS;
		new_stack.iret.flags = STACK_FLAGS;
		new_stack.iret.esp = (uint32_t)curr_process->user_stack;
		new_stack.iret.ss = USER_DS;
		reset_curr_process((pcb_t*)curr_process);
		curr_node = new_task;
		memcpy((void*)regs, (const void*)(&(new_stack.iret)), sizeof(iret_t));
		first_shell = 0;
	} else {
		new_task = curr_node->next_task;
		/*take out task if finished*/	
		if (curr_node->running == TASK_FINISHED)
		{
			run_queue_node_t* prev = curr_node->prev_task;
			prev->next_task = new_task;
			new_task->prev_task = prev;
			curr_node->prev_task = curr_node->next_task = NULL;
			
			if (curr_node == tail)
			{
				tail = prev;
			}
		}

		while (new_task->running != TASK_RUNNING) { new_task = new_task->next_task; }
		curr_process = (pcb_t*)curr_node->process;
		/* Set tss.ep0 and current_process in file_array.c */
		asm(
			"movl %%esp,%0 \n"
		    "movl %%ebp,%1 \n"
		    : "=r"(curr_process->esp), "=r"(curr_process->ebp)
		   	);

		reset_curr_process((pcb_t*)new_task->process);
		curr_node = new_task;
		curr_process = (pcb_t*)curr_node->process;

		asm(
			"movl %0, %%ebp \n"
			"movl %1, %%esp \n"
			:
			: "r"(curr_process->ebp), "r"(curr_process->esp)
			);
	}

	return;
}

/* 
 * schedule
 *   DESCRIPTION: schedules a task onto our linked list
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void schedule(uint32_t pid) 
{	
	/* Create new task node and put onto list*/
	pcb_t* task = get_pcb_address(pid);
	uint32_t* new_stack = task->kernel_stack;

	run_queue_node_t* new_task = &run_queue[pid];  
	new_task->process = (struct pcb_t*)task;
	new_task->prev_task = tail;
	new_task->next_task = tail->next_task;
	new_task->running = TASK_RUNNING;

	tail->next_task = new_task;
	tail = new_task;
	head->prev_task = tail;

	/*set parent's task as inactive*/
	if (((pcb_t*)new_task->process)->parent != NULL)                  
	{
		int parent_pid = ((pcb_t*)new_task->process)->parent->pid;
		run_queue_node_t* parent_task = &run_queue[parent_pid];
		parent_task->running = TASK_INACTIVE;
	}	
	
	/*setup task's stack*/
	if (!(first_schedule && curr_node == &sentinel))
	{
		/*setup iret stack*/
		aregs_t registers;
		registers.iret.ss = (uint32_t)USER_DS;
		registers.iret.esp = (uint32_t)task->user_stack;
		registers.iret.cs = (uint32_t)USER_CS;
		registers.iret.flags = STACK_FLAGS;
		registers.iret.eip = (uint32_t)task->instr_ptr;

		new_stack = (new_stack - NUM_IRET_STACK);
		memcpy((void*)new_stack, (const void*)(&registers.iret), sizeof(iret_t));	
		new_stack--;
		*new_stack = STACK_FLAGS;                    // set flags
		gen_regs_t* all_registers = (gen_regs_t*)(new_stack - NUM_REG_STACK);
		all_registers->iesp = (uint32_t)new_stack;	// stack should be at pushfl
		new_stack = (new_stack - MOVE_NEW_STACK );  // move new_stack up number of registers, argument and return address
		
		uint32_t pit_ret_val;
		asm("	lea pit_ret, %0 "   //get address of return value
			: "=r" (pit_ret_val)
			);

		*new_stack = pit_ret_val;	// return address into asm linkage
		 new_stack--; 	            // callee: pushl ebp	
		*new_stack = (uint32_t)task->kernel_stack;

		task->ebp = (uint32_t)new_stack;   //set up ebp and esp of task
		task->esp = (uint32_t)new_stack;
	}
	first_schedule = 0;

}

/* 
 * unschedule
 *   DESCRIPTION: unschedule a task on the run queue 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void unschedule(uint32_t pid, uint32_t parent_pid)
{

	/*set the running task state to finished*/
	cli(); 
		run_queue_node_t* r = &run_queue[pid];
		r->running = TASK_FINISHED;
		run_queue_node_t* parent = &run_queue[parent_pid];

		/*do not unschedule the shell*/
		if (pid >= NUM_SHELLS) parent->running = TASK_RUNNING;
	sti();

}

/* 
 * sched_init
 *   DESCRIPTION: sets up scheduling linked list
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void sched_init()
{
	head = &sentinel;
	tail = head;
	head->next_task = head;
	head->prev_task = tail;
	curr_node = head;
}


