#include "file_array.h"
//#include "assembly_linkage.h"
//#define ERROR -1
#define FD_IN_USE 0x00000100
#define FD_FREE 0
#define SUCCESS 0
#define FD_START 2
#define RTC 0
#define DIRECTORY 0x00000001
#define REGULAR_FILE 0x00000002
#define STDIN 3
#define STDOUT 4
#define FILE_TYPE 0x0000000f
#define NAME_LENGTH 32
#define CMD_LENGTH 128
#define PROGRAM_DIR_START 0x08000000
#define FIRST_PROGRAM 0x08048000
#define PROGRAM_OFFSET 0x00048000
#define STACK_START 0x800000
#define PROGRAM_START 0x08048000
#define USTACK_START 0x83ffffc
#define PROGRAM_SIZE 0x400000
#define STACK_SIZE 0x2000
#define PID_ARR_SIZE 6
#define DEL 0x7f
#define UCSELECTOR 0x18
#define UDSELECTOR 0x20
#define KCSELECTOR 0x08
#define KDSELECTOR 0x10
#define USER_PRIV 0x3
#define STATUS_MASK 0x000000ff
#define EHALT 239
#define EXCEPTION_RETURN 256
#define VIRT_VID_MEM 0xdeadb000
#define SECOND_TERMINAL 1
#define THIRD_TERMINAL 2
#define NUM_SHELLS	3
#define FIRST_TERMINAL 0
#define TASK_SWITCH_LOCAL_VARS 0x18
#define TASK_SWITCH_ARGS 4
#define TASK_SWITCH_STACK_ADD (TASK_SWITCH_ARGS + 4 + 4 + 4 + TASK_SWITCH_LOCAL_VARS)
#define WORD 4
#define TOP_OF_PROCESSs_KERNEL_STACK 1
#define CLEAR 0
#define ALLOW_INTERRUPT_FLAGS 0x00000202
#define ON 1
#define CHECK_EXEC_OFFSET 24
#define JUMP_BACK_TO_IRET 14

static pcb_t* curr_process;
static file_descriptor_t* desc;
static int pid_arr[PID_ARR_SIZE];
static op_table_t rtc_op = {.open = (void*)rtc_open, .read = (void*)rtc_read, .write = (void*)rtc_write, .close = (void*)rtc_close};
static op_table_t dir_op = {.open = (void*)dir_open, .read = (void*)dir_read, .write = (void*)dir_write, .close = (void*)dir_close};
static op_table_t fs_op = {.open = (void*)fs_open, .read = (void*)fs_read, .write = (void*)fs_write, .close = (void*)fs_close};
static pcb_t* root1;
static pcb_t* root2;
static pcb_t* root3;
static pcb_t** rootptrs[NUM_SHELLS] = {&root1, &root2, &root3};
static op_table_t std_ops = {.open = (void*)terminal_open, .read = (void*)terminal_read, .write = (void*)terminal_write, .close = (void*)terminal_close};


/* get_kstack_address
 * DESCRIPTION: gets address of bottom of kernel stack per pid
 * INPUTS: pid - the process ID
 * OUPUTS: none
 * RETURN VALUE: uint32_t - kernel stack address
 * SIDE EFFECTS: none
*/
uint32_t get_kstack_address (int pid)
{
  return STACK_START - ((pid) * STACK_SIZE) - WORD;
}

/* get_pcb_address 
 * DESCRIPTION: gets address of process control block inside kernel per pid
 * INPUTS: pid - the process ID
 * OUPUTS: none
 * RETURN VALUE: pcb_t* - pointer to the process control block in memory
 * SIDE EFFECTS: none
*/
pcb_t* get_pcb_address(int pid)
{
  return (pcb_t*)( STACK_START - ((pid + TOP_OF_PROCESSs_KERNEL_STACK) * STACK_SIZE));
}

/* get_program_address
 * DESCRIPTION: gets the address of the first user program instruction per pid
 * INPUTS: pid - the process ID
 * OUPUTS: none
 * RETURN VALUE: uint32_t - address of first instruction in user program 
 * SIDE EFFECTS: none
*/
uint32_t get_program_address(int pid)
{
  return PROGRAM_START + ((pid) * PROGRAM_SIZE);
}

/* get_ustack_address 
 * DESCRIPTION: gets address of first byte in user program stack
 * INPUTS: pid - the process ID
 * OUPUTS: none
 * RETURN VALUE: uint32_t - the user stack address
 * SIDE EFFECTS: none
*/
uint32_t get_ustack_address(int pid)
{
  return PROGRAM_START + (pid * PROGRAM_SIZE) - WORD;
}

/* pid_init
 * DESCRIPTION: zeroes the pid array, which is used to keep track of which 
 * 				processes (and respective blocks of memory) are live
 * INPUTS: none
 * OUPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: sets each index of the pid array to 0 (which means all pids are available for future use)
*/
void pid_init(){
  memset((void*)(&pid_arr), CLEAR, PID_ARR_SIZE * WORD);
}

/* free_pid_idx()
 * DESCRIPTION: finds the next available pid for corresponding process memory allocation
 * INPUTS: none
 * OUPUTS: none
 * RETURN VALUE: int - error code representing whether a free pid was found (0 for success, -1 for failure)
 * SIDE EFFECTS: reads pid array for next available pid, does NOT write a 1 to chosen pid index
*/

int free_pid_idx() {
	int idx;
  for(idx = 0; idx < PID_ARR_SIZE; idx++){
    if(pid_arr[idx] == CLEAR) return idx;
  }
  return ERROR;
}

/* check_executable
 * DESCRIPTION: searches file for ELF bytes which indicate it is an executable
 * INPUTS: unsigned char * file_name - the file you want to check to see if it is an executable
 * OUPUTS: none
 * RETURN VALUE: int - returns error code 0 if file is an executable, -1 if not
 * SIDE EFFECTS: performs an fs_read_by_name to determine whether file contans special char & ELF, indicating executable
*/
int check_executable(unsigned char * file_name)
{
	char elfbuf[WORD];
	char ELF[] = {DEL, 'E', 'L', 'F'};
   	int32_t ret = fs_read_by_name((uint8_t*)(file_name), 0, WORD, (uint8_t*)elfbuf);
   	if ( ret == ERROR || ret < WORD ) return ERROR;

  	if(strncmp(elfbuf, ELF, WORD) != SUCCESS) return ERROR;

  	return SUCCESS;
}

/* clear_fdarray
 * DESCRIPTION: zeroes a file descriptor array
 * INPUTS: file_descriptor_t* f - file descriptor array you want to clear
 * OUPUTS: none
 * RETURN VALUE: int - returns 0 upon successful clear of file descriptor array
 * SIDE EFFECTS: file descriptor array is cleared to 0s
*/
int clear_fdarray(file_descriptor_t* f)
{
	int i;
	for (i = 0; i < FILE_ARRAY_SIZE; i++)
	{
		f[i].flags = CLEAR;
	}
	return SUCCESS;
}

/* get_file_info
 * DESCRIPTION: searches through files to find data entry associated with command (a file)
 * INPUTS: char* cmd -- file to search for
 *			dentry_t* dentry -- data entry corresponding to file name
 * OUPUTS: writes to dentry param
 * RETURN VALUE: int -- returns error code -- 0 on successful assignment of dentry to corresponding file's data entry, -1 on failure
 * SIDE EFFECTS: edits dentry to point to valid data entry corresponding to file 
*/
int get_file_info(char* cmd, dentry_t* dentry)
{
  	if ( ERROR == read_dentry_by_name((const uint8_t*)cmd, dentry) ) return ERROR;	
  	return SUCCESS;
}

/* get_curr_process_terminal
 * DESCRIPTION: returns the terminal that's tied to the current process
 * INPUTS: none
 * OUPUTS: none
 * RETURN VALUE: int32_t - terminal number associated with currently running process (returns 0, for terminal 1, if no current process)
 * SIDE EFFECTS: none
*/

int32_t get_curr_process_terminal()
{
	if (curr_process == NULL) return 0;
	return curr_process->terminal;
}

/* pcb_init
 * DESCRIPTION: initializes the process control block for a new process
 * INPUTS: pcb_t* pcb -- pointer to start of process control block in memory per pid
 			 int pid -- process id associated with pcb
 			 uint32_t eip -- process's instruction pointer
 			  const char* cmd -- command which launched the process
 			  const char* args -- arguments entered into shell when process was launched
 * OUPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: sets pcb fields, links new pcb to parent pcb, associates pcb with currently viewed terminal 
*/
void pcb_init(pcb_t* pcb, int pid, uint32_t eip, const char* cmd,const char* args) {
    pcb->pid = pid;
    pcb->program = (void*)PROGRAM_START;
    pcb->kernel_stack = (void*)get_kstack_address(pid);
    pcb->user_stack =  (void*)USTACK_START;
    pcb->instr_ptr = (void*)eip;
    pcb->parent = curr_process;
	pcb->child = NULL;
	if (curr_process != NULL && pid > THIRD_TERMINAL)
		pcb->parent->child = pcb;

	if (pid >= NUM_SHELLS)
		pcb->terminal = get_curr_terminal();
	else
		pcb->terminal = pid;
    strcpy(pcb->command, cmd);
    strcpy(pcb->args, args);
}

/* open_stdin_stdout
 * DESCRIPTION: opens std in and std out in the current process' file descriptor array
 * INPUTS: none
 * OUPUTS: int32_t - error code (1 on success, -1 on failure)
 * RETURN VALUE: int - returns 0 upon successful opening of stdin and stdout for current process
 * SIDE EFFECTS: sets file descriptors 0 and 1 to point to appropriate file operations table (for  open, read, etc. syscalls),
 *				sets flags in descriptor 0 and 1 marking files open, sets positions in each file to 0
 *				stdin and stdout are not associated with any inode!
*/

int32_t open_stdin_stdout()
{
	file_descriptor_t * stdin = &desc[0];
	file_descriptor_t * stdout = &desc[1];

	stdin->file_op_table_ptr = stdout->file_op_table_ptr = &std_ops; 	
	stdin->flags = FD_IN_USE | STDIN;
	stdout->flags = FD_IN_USE | STDOUT;
	stdin->file_position = stdout->file_position = CLEAR;
	stdin->inode_ptr = stdout->inode_ptr = NULL;
	return SUCCESS;
}

/* open_shells
 * DESCRIPTION: opens shells two and three after first shell is opened, sets up iret stacks for each shell, sets up pcbs
 * INPUTS: uint32_t eip - instruction pointer, address of first instruction for each shell user program
 *			inode_t * i_ptr -- inode pointer corresponding to shell executable file
 *			 char* cmd -- cmd used to trigger shells ("shell")
 *			 char* args -- args used when triggers shells (should be empty)
 * OUPUTS: none
 * RETURN VALUE: int - returns 0 upon successful memory set up of second and third shells 
 * SIDE EFFECTS: file loader for shells into physical memory (per pid paging), pcb initialization, root setup for pcb chaining, opening
 				of stdin and stdout, sets up iret context at base of kernel stacks for shells, marks pid 1 and 2 in use, 
 				schedules shells 2 and 3 to run
*/
int open_shells(uint32_t eip, inode_t * i_ptr, char* cmd, char* args) {

	/* Load shell code into memory */
	int i;
	for (i = SECOND_TERMINAL; i < NUM_SHELLS; i++) 
	{
		switch_paging(i);
		int ret = read_data(i_ptr, 0, (uint8_t*) (PROGRAM_DIR_START + PROGRAM_OFFSET), i_ptr->length);
		if (ret == ERROR || ret < i_ptr->length) return ERROR;

		// Extract EIP from the file
		pcb_t* ctrl_blk = get_pcb_address(i);
   		pcb_init(ctrl_blk, i, eip, cmd, args);

		// this clears all files and only works because you set the stdin and stdout later
    	set_current_process(ctrl_blk);

		if (i == SECOND_TERMINAL) root2 = curr_process;
		else root3 = curr_process;
		
		curr_process->parent = NULL;
		ctrl_blk->terminal = i;
    	open_stdin_stdout();

    	//setup iret stack
		iret_t context;
		context.eip = (uint32_t)curr_process->instr_ptr;
		context.cs = USER_CS;
		context.flags = ALLOW_INTERRUPT_FLAGS;
		context.esp = (uint32_t)curr_process->user_stack;
		context.ss = KERNEL_DS;

		memcpy((void*)(curr_process->kernel_stack - sizeof(iret_t)), (const void*)(&(context)), sizeof(iret_t));

		schedule(curr_process->pid);
	}

	/* Set PIDs of terminals in use */
	pid_arr[SECOND_TERMINAL] = pid_arr[THIRD_TERMINAL] = ON;

	/* Switch back to first shell */ 
	switch_paging(FIRST_TERMINAL);
	reset_curr_process(get_pcb_address(FIRST_TERMINAL));
	return SUCCESS;
}

/* kget_args
 * DESCRIPTION: gets the arguments corresponding to the current running process
 * INPUTS: uint8_t* buf -- buffer to transfer arguments to, int32_t nbytes -- number of bytes to transfer (number of bytes in argument)
 * OUPUTS: int32_t - error code (0 on success, -1 on failure)
 * RETURN VALUE: int - returns 0 upon successful opening of stdin and stdout for current process
 * SIDE EFFECTS: transfers args from current process' pcb to buffer passed in
*/

int32_t kget_args(uint8_t* buf, int32_t nbytes){
	if(buf == NULL || nbytes < 0 || curr_process->args == NULL)
		return ERROR; 

	int i;
	for(i = 0; i < strlen(curr_process->args); i++){
		buf[i] = curr_process->args[i];
	}

	buf[strlen(curr_process->args)] = '\0';

	return SUCCESS;
}

/* kset_handler
 * DESCRIPTION: used to pass in a handler to affilated with a specified signal number
 * INPUTS: int32_t signum -- signal number you want to add a handler to, void* handler_address -- start of memory block for new handler
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 on failure)
 * SIDE EFFECTS: none
*/
int32_t kset_handler(int32_t signum, void* handler_address){
	return ERROR;
}


/* ksigreturn
 * DESCRIPTION: used for returning from a signal
 * INPUTS: none
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 on failure)
 * SIDE EFFECTS: none
*/
int32_t ksigreturn(void){
	return ERROR;
}


/* kexecute
 * DESCRIPTION: used to pass in a handler to affilated with a specified signal number
 * INPUTS: const uint8_t* command -- command you want to execute (program you want to start from shell)
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 on failure)
 * SIDE EFFECTS: checks to see if the command is an executable file, assigns new program a pid, creates pcb for process, schedules new program,
 				switches paging to work for new command, opens first three shells upon first call to execute("shell")
*/
int32_t kexecute(const uint8_t* command) {
 	int i;
	int no_args;
  	/* get next available pid */
	int pid = free_pid_idx();

	if(command == NULL || pid == ERROR) { return ERROR; }

	/* parse the command */
	for (i = 0; i < CMD_LENGTH; i++) {    //Find length of the command
		if (command[i] == ' ') break;
		if (command[i] == '\0') break;
		if (command[i] == '\n') {
			no_args = ON;
			break;
		}
	}

	char cmd[i+1];                          //Create buffer with found length
	strncpy((int8_t*)cmd, (int8_t*)command, i);
	cmd[i] = '\0';

	/* Search for the exectuable and return the dentry */
	dentry_t data;
	if (ERROR == get_file_info(cmd, &data)) {
		return ERROR;
	}

  	/* check 'ELF' condition for exe */
	if (check_executable((unsigned char*)(data.file_name)) == ERROR) {
		return ERROR;  
	}
  	
	 // increment i to first arg character in buffer, create new buffer
	while (command[i] == ' ') i++;
	if(command[i] != '\0') no_args = CLEAR;

	char args[CMD_LENGTH-i];
  	if (no_args == CLEAR)
  	{
		//copy the args with spaces into its own buffer
		int j;
		for (j = 0; j < CMD_LENGTH - i; j++) {
			if(command[i+j] == '\0' || command[i+j] == '\n') {
				args[j] = '\0';
				break;
			}
			args[j] = command[i+j];
		}
	}

	if (pid > THIRD_TERMINAL)
		new_paging(pid, curr_process->pid);
	else
		switch_paging(pid);

	/* Copy the executable file info into memory */
	inode_t* i_ptr = get_inode_ptr(data.inode_num);
	int ret = read_data(i_ptr, 0, (uint8_t*) (PROGRAM_DIR_START + PROGRAM_OFFSET), i_ptr->length);
	if (ret == ERROR || ret < i_ptr->length) {
		return ERROR;
	}

	pid_arr[pid] = ON;

	//Extract EIP from the file
	uint32_t eip;
	read_data(i_ptr, CHECK_EXEC_OFFSET,(uint8_t*)(&eip), WORD);

	pcb_t* ctrl_blk = get_pcb_address(pid);
    pcb_init(ctrl_blk, pid, eip, cmd, args);

    set_current_process(ctrl_blk);
    open_stdin_stdout();

    if (curr_process->parent != NULL)
    {
	    get_esp_and_ebp(curr_process->parent->esp, curr_process->parent->ebp);
	    switch_paging(curr_process->parent->pid);
	}

 	cli();
 	schedule(curr_process->pid);
	sti();		

     /* Decide whether we should open 2 other shells */
 	/* fix the root if someone tries to exit a terminal! */
	if (pid == FIRST_TERMINAL) root1 = curr_process;
	if (pid == SECOND_TERMINAL) root2 = curr_process;
	if (pid == THIRD_TERMINAL) root3 = curr_process;

 	if (pid_arr[FIRST_TERMINAL]==ON && pid_arr[SECOND_TERMINAL]==CLEAR && pid_arr[THIRD_TERMINAL]==CLEAR) {
 		ctrl_blk->terminal = FIRST_TERMINAL;
  		root1 = curr_process;
 		if (open_shells(eip, i_ptr, cmd, args) == ERROR) { sti(); return ERROR; }

		pit_init();
 	}	

	asm("int $32");

	return curr_process->ret_status;
}

/* khalt
 * DESCRIPTION: halts the currently running program
 * INPUTS: uint8_t status - passed in to indicate whether termination of program after execute is successful
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 on failure)
 * SIDE EFFECTS: if shell, restores iret stack to start shell over again; if child process of any shell, unmaps child's video memory, takes
 it out of pcb tree, stores return value in parent pcb, unschedules child
*/

int32_t khalt(uint8_t status)
{
	if (curr_process->pid > THIRD_TERMINAL)
	{
		int fd;
		/* grab parent process descriptor */
		pcb_t* parent_pcb;
		parent_pcb = curr_process->parent;
		pid_arr[curr_process->pid] = CLEAR;
		unmap_vid_memory(curr_process->pid);
		for (fd = 0; fd < FILE_ARRAY_SIZE; fd++)
		{
			if (desc[fd].flags & FD_IN_USE)
			{
				kclose(fd);
			}
		}

		/* restore parent's paging */	
		int parent_pid = parent_pcb->pid;

		curr_process->ret_status = status;
		if (curr_process->parent != NULL)
			curr_process->parent->child = NULL;
		curr_process->parent = NULL;

		uint32_t ret_val = 0;
		if (curr_process->ret_status == EHALT)
			ret_val = EXCEPTION_RETURN;
		else
			ret_val = (curr_process->ret_status & STATUS_MASK);

		parent_pcb->ret_status = ret_val;
		unschedule(curr_process->pid, parent_pid);

		asm("int $32");

	} else {
		uint32_t* curr_ebp;
		asm("movl %%ebp, %0"
			: "=r"(curr_ebp));

		iret_t* curr_iret = (iret_t*)(curr_ebp + JUMP_BACK_TO_IRET);
		curr_iret->eip = (uint32_t)curr_process->instr_ptr;
		curr_iret->esp = (uint32_t)curr_process->user_stack;
	}

	return SUCCESS;
}

/* fd_init
 * DESCRIPTION: used to pass in a handler to affilated with a specified signal number
 * INPUTS: dentry_t data -- data entry of file you're adding to current file descriptor array, uint32_t chosen_index -- file descriptor to set
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 on failure)
 * SIDE EFFECTS: opens file in file descriptor array, affiliates fd with data entry for reading, sets flags that corresponding to file type, sets fd to point to proper file operations table
 				for sys calls
*/

int32_t fd_init(dentry_t data, uint32_t chosen_index){
	if (data.file_type == RTC || data.file_type == DIRECTORY || data.file_type == REGULAR_FILE) {
		desc[chosen_index].file_position = 0;
		desc[chosen_index].flags = FD_IN_USE;
	}
	else return ERROR;

	if (data.file_type == RTC) {
		desc[chosen_index].inode_ptr = NULL;
		desc[chosen_index].flags |= RTC;
		desc[chosen_index].file_op_table_ptr = &rtc_op;
	}

	else if (data.file_type == DIRECTORY) {
		desc[chosen_index].inode_ptr = NULL;
		desc[chosen_index].flags |= DIRECTORY;
		desc[chosen_index].file_op_table_ptr = &dir_op;
	} 

	else if (data.file_type == REGULAR_FILE) {
		desc[chosen_index].inode_ptr = (struct inode_t*)(get_inode_ptr(data.inode_num));
		desc[chosen_index].flags |= REGULAR_FILE;
		desc[chosen_index].file_op_table_ptr = &fs_op;
	}
	return SUCCESS;
}

/* k_open
 * DESCRIPTION: opens a file in the current process' file descriptor array
 * INPUTS: const uint8_t* file_name -- name of file to open
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 on failure)
 * SIDE EFFECTS: uses read_dentry_by_name to find data entry for file_name, then associates data entry to spot in file descriptor array with fd_init
*/
int32_t kopen(const uint8_t* file_name) {
 	uint32_t chosen_index = ERROR;
	int i;
	for (i = FD_START; i < FILE_ARRAY_SIZE; i++) {
		if ((desc[i].flags & FD_IN_USE) == CLEAR) {
			chosen_index = i; 
			break;
		}
	} 
	if (chosen_index == ERROR) return ERROR;

	// are we trying to open an actual file?
	dentry_t data;
	if (SUCCESS != read_dentry_by_name(file_name, &data)) {
		return ERROR;
	}


    if(ERROR == fd_init(data, chosen_index)) return ERROR;

	// call open!
	desc[chosen_index].file_op_table_ptr->open();
	return chosen_index;
}

/* kread
 * DESCRIPTION: system call to read a file
 * INPUTS: int32_t fd -- file descriptor you want to read from, void* buf -- buffer you want to write to on a read, int32_t nbytes - number of bytes to read
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 on failure)
 * SIDE EFFECTS: reads from a file if the file descriptor flags are valid and the buffer you want to read to is not NULL
*/
int32_t kread(int32_t fd, void* buf, int32_t nbytes){
	if(fd < 0 || fd >= FILE_ARRAY_SIZE || buf == NULL || nbytes < 0 || !(desc[fd].flags & FD_IN_USE) ||
		((desc[fd].flags & FILE_TYPE) != RTC && (desc[fd].flags & FILE_TYPE) !=DIRECTORY && 
		(desc[fd].flags & FILE_TYPE) != REGULAR_FILE && (desc[fd].flags & FILE_TYPE) != STDIN)){
		return ERROR;
	}

	return desc[fd].file_op_table_ptr->read(desc[fd].inode_ptr, desc[fd].file_position, nbytes, (uint8_t*)buf);
}

/* kwrite
 * DESCRIPTION: system call to write to a file
 * INPUTS: int32_t fd -- file descriptor you want to read from, void* buf -- buffer you want to write to on a read, int32_t nbytes - number of bytes to write
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 on failure)
 * SIDE EFFECTS: write to a file if the file descriptor flags are valid and the buffer you want to write to is not NULL
*/
int32_t kwrite (int32_t fd, const void *buf, int32_t nbytes)
{
	if(fd < 0 || fd >= FILE_ARRAY_SIZE || buf == NULL || nbytes < 0
		|| !(desc[fd].flags & FD_IN_USE) || ((desc[fd].flags & FILE_TYPE) != RTC && 
		(desc[fd].flags & FILE_TYPE) !=DIRECTORY && (desc[fd].flags & FILE_TYPE) != REGULAR_FILE && 
		(desc[fd].flags & FILE_TYPE) != STDOUT)) {
		return ERROR;
	}
	return desc[fd].file_op_table_ptr->write(fd, buf, nbytes);
}

/* kclose
 * DESCRIPTION: system call to read a file
 * INPUTS: int32_t fd -- file descriptor you want to close and free
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 on failure)
 * SIDE EFFECTS: closes a file from the current file descriptor array (desc)
*/
int32_t kclose(int32_t fd)
{
	if (fd < FD_START || fd >= FILE_ARRAY_SIZE || !(desc[fd].flags & FD_IN_USE))
		return ERROR;

	op_table_t *operations = (op_table_t*)(desc[fd].file_op_table_ptr);
	desc[fd].flags = FD_FREE;	
	return operations->close();
}

/* set_current_process
 * DESCRIPTION: sets the process control block for the currently running process
 * INPUTS: pcb_t* pcb -- process control block corresponding to the process you want to set as the currently running process
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 on failure)
 * SIDE EFFECTS: writes to the tss.esp0 so the kernel has a place to put the automatic iret stack on interrupts, assigns a new file array to
 the desc (or current fd array pointer), clears the fd array
*/
int32_t set_current_process(pcb_t* pcb)
{
	curr_process = pcb;
	desc = (file_descriptor_t*)(pcb->files);
	clear_fdarray(desc);
	tss.esp0 = (uint32_t)(curr_process->kernel_stack);
	
	return 0;
}


/* init_process
 * DESCRIPTION: takes the specific pid out of the pid array (making it available), sets curr_process to NULL
 * INPUTS: int32_t curr_pid -- the pid of the process you want to remove from the fd array
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 on failure)
 * SIDE EFFECTS: sets curr process to a NUll pointer, a real initialization, so you have to be careful
*/
int32_t init_process(int32_t curr_pid)
{
	pid_arr[curr_pid] = 0;
	curr_process = NULL;
    return SUCCESS;
}

/* reset_curr_process
 * DESCRIPTION: resets current process to a process which already has a file array, switches paging to match process, also sets esp0 for interrupt use
 * INPUTS: pcb_t* pcb -- process control block of the process we want to switch to
 * OUPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: sets curr process to pcb paramter, set up file descriptor array, set tss to corresponding place on kernel stack, switch paging
 for pcb's process 
*/
void reset_curr_process(pcb_t* pcb)
{
	curr_process = pcb;
	desc = (file_descriptor_t*)(pcb->files);
	tss.esp0 = (uint32_t)(curr_process->kernel_stack);
	switch_paging(pcb->pid);
}

/* kvidmap
 * DESCRIPTION: changes a pointer to directly map to video memory in the first 4 MB of memory
 * INPUTS: uint8_t** screen_start -- address to pointer so you can change the address the pointer represents
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 on failure) OR address in memory which will point directly to vga memory
 * SIDE EFFECTS: maps 0xdeadbe000 straight to physical vga memory, then sets point to contain this address 
*/
int32_t kvidmap(uint8_t** screen_start) 
{ 
	if (ERROR == address_in_user((int32_t)(screen_start), curr_process->pid))
		return ERROR;

	int ret = map_vid_memory(curr_process->pid);

	if (ret == ERROR) return ERROR;

	*screen_start = (uint8_t*)VIRT_VID_MEM;

	return VIRT_VID_MEM;
}

/* get_fd_entry
 * DESCRIPTION: returns file descriptor entry (of current process) corresponding to file descriptor
 * INPUTS: int32_t fd - file descriptor
 * OUPUTS: none
 * RETURN VALUE: file_descriptor_t* - contains file name, inode pointer, location, pointer to file operation table
 * SIDE EFFECTS: none
*/
file_descriptor_t* get_fd_entry(int32_t fd)
{
	return (&(desc[fd]));
}

/* increment_position
 * DESCRIPTION: increments file position inside file descriptor block by specified amount
 * INPUTS: struct inode_t* inode -- inode corresponding to file we're reading, int amt -- amount to increment file position by
 * OUPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: writes incremental amount to file_position in file_descriptor's file position, if it matches with the inode passed
*/
void increment_position(struct inode_t* inode, int amt)
{
	if (inode == NULL) return;
	int i;
	for (i = FD_START; i < FILE_ARRAY_SIZE; i++) {
		if (desc[i].inode_ptr == inode) {
			desc[i].file_position += amt;
		}
	} 
}

/* swap_vid_mem
 * DESCRIPTION: climbs through pcb tree to change old terminal (and its associated processes) to write to buffer which is not vga,
 				 and new terminal (and its processes) to write to VGA!
 * INPUTS: int old_term -- old terminal user was in, int new_term -- new terminal user is switching to
 * OUPUTS: none
 * RETURN VALUE: int32_t -- error code (0 on success, -1 on failure)
 * SIDE EFFECTS: changes paging of processes associated with new terminal and old terminal, clears interrupts for this change up the tree
*/
int32_t swap_vid_mem(int old_term, int new_term)
{
	if (new_term < FIRST_TERMINAL || new_term > THIRD_TERMINAL || old_term < FIRST_TERMINAL || old_term > THIRD_TERMINAL) return ERROR;

	pcb_t* old = *(rootptrs[old_term]);
	pcb_t* new = *(rootptrs[new_term]);

	cli();
	while (old != NULL)
	{
		switch_vid_mem_out(old->pid, old_term, curr_process->pid);
		old = old->child;
	}
	while (new != NULL)
	{
		switch_vid_mem_in(new->pid, curr_process->pid);
		new = new->child;
	}
	sti();

	uint32_t* pd = get_process_directory(curr_process->pid);
	set_cr3(pd);
	
	return SUCCESS;
}


/* get_curr_process()
 * DESCRIPTION: returns current process process control block
 * INPUTS: none
 * OUPUTS: none
 * RETURN VALUE: pcb_t* -- the process descriptor of the currently running process
 * SIDE EFFECTS: none
*/

pcb_t* get_curr_process()
{
	return curr_process;
}

