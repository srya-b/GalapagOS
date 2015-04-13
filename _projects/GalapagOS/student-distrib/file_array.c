#include "file_array.h"
#include "file_system.h"
#include "rtc.h"
#include "terminal.h"
#include "paging.h"
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
#define PID_ARR_SIZE 7
#define DEL 0x7f
#define UCSELECTOR 0x18
#define UDSELECTOR 0x20
#define KCSELECTOR 0x08
#define KDSELECTOR 0x10
#define USER_PRIV 0x3
#define STATUS_MASK 0x000000ff
#define EHALT 239
#define EXCEPTION_RETURN 256

static pcb_t* curr_process;
static file_descriptor_t* desc;
static int pid_arr[PID_ARR_SIZE];
static op_table_t rtc_op = {.open = (void*)rtc_open, .read = (void*)rtc_read, .write = (void*)rtc_write, .close = (void*)rtc_close};
static op_table_t dir_op = {.open = (void*)dir_open, .read = (void*)dir_read, .write = (void*)dir_write, .close = (void*)dir_close};
static op_table_t fs_op = {.open = (void*)fs_open, .read = (void*)fs_read, .write = (void*)fs_write, .close = (void*)fs_close};

static op_table_t std_ops = {.open = (void*)terminal_open, .read = (void*)terminal_read, .write = (void*)terminal_write, .close = (void*)terminal_close};

static int32_t set_current_process(pcb_t* pcb);

uint32_t get_kstack_address (int pid)
{
  return STACK_START - ((pid) * STACK_SIZE) - 4;
}

pcb_t* get_pcb_address(int pid)
{
  return (pcb_t*)( STACK_START - ((pid + 1) * STACK_SIZE));
}

uint32_t get_program_address(int pid)
{
  return PROGRAM_START + ((pid) * PROGRAM_SIZE);
}

uint32_t get_ustack_address(int pid)
{
  return PROGRAM_START + (pid * PROGRAM_SIZE) - 4;
}

void pid_init(){
  memset((void*)(&pid_arr), 0, PID_ARR_SIZE * 4);
}

int free_pid_idx() {
	int idx;
  for(idx = 0; idx < PID_ARR_SIZE; idx++){
    if(pid_arr[idx] == 0) return idx;
  }
  return ERROR;
}

int check_executable(unsigned char * file_name)
{
	char elfbuf[4];
	char ELF[] = {DEL, 'E', 'L', 'F'};
   	int32_t ret = fs_read((uint8_t*)(file_name), 0, 4, (uint8_t*)elfbuf);
   	if ( ret == ERROR || ret < 4 ) return ERROR;

  	if(strncmp(elfbuf, ELF, 4) != 0) return ERROR;

  	return SUCCESS;
}

int clear_fdarray(file_descriptor_t* f)
{
	int i;
	for (i = 0; i < FILE_ARRAY_SIZE; i++)
	{
		f[i].flags = 0;
	}
	return 0;
}

int get_file_info(char* cmd, dentry_t* dentry)
{
  	if ( ERROR == read_dentry_by_name((const uint8_t*)cmd, dentry) ) return ERROR;	
  	return SUCCESS;
}

int32_t switch_process()
{
	return 0;	
}

void pcb_init(pcb_t* pcb, int pid, uint32_t eip, const char* cmd,const char* args) {
    pcb->pid = pid;
    pcb->program = (void*)PROGRAM_START;
    pcb->kernel_stack = (void*)get_kstack_address(pid);
    pcb->user_stack =  (void*)USTACK_START;
    pcb->instr_ptr = (void*)eip;
    pcb->parent = curr_process;
    strcpy(pcb->command, cmd);
    strcpy(pcb->args, args);
}

int32_t open_stdin_stdout()
{
	file_descriptor_t * stdin = &desc[0];
	file_descriptor_t * stdout = &desc[1];

	stdin->file_op_table_ptr = stdout->file_op_table_ptr = &std_ops; 	
	stdin->flags = FD_IN_USE | STDIN;
	stdout->flags = FD_IN_USE | STDOUT;
	stdin->file_position = stdout->file_position = 0;
	stdin->inode_ptr = stdout->inode_ptr = NULL;
	return 0;
}

int32_t kexecute(const uint8_t* command) {
  /*
      1. Parse the command into "command" + "args"
      2. Check file exists and ELF at start of file
      3. Add entry page table
      4. File loader to put contents of executable file into user space (thru virtual address)
         Get EIP from 24-27 bytes within the file
      5.  PCB - [kernel space] Create new file descriptor array, open stdin and stdout, record parent process info
      6.  Context Switching - Switch from kernel space to user prorg
  */
	int i;
	int no_args;
  	/* get next available pid */

	int pid = free_pid_idx();

	if(command == NULL || pid == ERROR) return ERROR;
	//pid_arr[pid] = 1;

	/* parse the command */
	for (i = 0; i < CMD_LENGTH; i++) {    //Find length of the command
		if (command[i] == ' ') break;
		if (command[i] == '\0') break;
		if (command[i] == '\n') {
			no_args = 1;
			break;
		}
	}

	char cmd[i+1];                          //Create buffer with found length
	strncpy((int8_t*)cmd, (int8_t*)command, i);
	cmd[i] = '\0';

	/* Search for the exectuable and return the dentry */
	dentry_t data;
	if (ERROR == get_file_info(cmd, &data))
		return ERROR;

  	/* check 'ELF' condition for exe */
	if (check_executable((unsigned char*)(data.file_name)) == ERROR) 
		return ERROR;  
  	
	 // increment i to first arg character in buffer, create new buffer
	while (command[i] == ' ') i++;
	char args[CMD_LENGTH-i];
  	if (no_args == 0)
  	{
		//copy the args with spaces into its own buffer
		int j;
		for (j = 0; j < CMD_LENGTH - i; j++)
			args[j] = command[i+j];
	}

	switch_paging(pid);

	/* Copy the executable file info into memory */
	inode_t* i_ptr = get_inode_ptr(data.inode_num);
	int ret = read_data(i_ptr, 0, (uint8_t*) (PROGRAM_DIR_START + PROGRAM_OFFSET), i_ptr->length);
	if (ret == ERROR || ret < 1) return ERROR;

	pid_arr[pid] = 1;
	//Extract EIP from the file
	uint32_t eip;
	read_data(i_ptr, 24,(uint8_t*)(&eip), 4);

	pcb_t* ctrl_blk = get_pcb_address(pid);
    pcb_init(ctrl_blk, pid, eip, cmd, args);

    //sti();
    set_current_process(ctrl_blk);
    open_stdin_stdout();

    if (curr_process->parent != 0)
	    get_esp_and_ebp(curr_process->parent->esp, curr_process->parent->ebp);

    setup_iret_stack((uint32_t)USER_DS, curr_process->user_stack, (uint32_t)USER_CS, curr_process->instr_ptr);
   
	return 0;
}


int32_t khalt(uint8_t status)
{
	if (curr_process->pid > 0)
	{

		/* grab parent process descriptor */
		pcb_t* parent_pcb;
		parent_pcb = curr_process->parent;
		pid_arr[curr_process->pid] = 0;

		/* grab parent ebp */
		int parent_ebp = parent_pcb->ebp;

		/* restore parent's paging */	
		int parent_pid = parent_pcb->pid;
		curr_process = parent_pcb;
		desc = (file_descriptor_t*)(&curr_process->files);
		curr_process->ret_status = status;
		switch_paging(parent_pid);

		tss.esp0 = (uint32_t)(curr_process->kernel_stack);
		/* set old stack pointer and base pointer */
		set_esp(parent_ebp);
		set_ebp(parent_ebp);

	} else {
		init_process();
		kexecute((uint8_t *)"shell");
	}

	uint32_t ret_val = 0;
	if (curr_process->ret_status == EHALT)
		ret_val = EXCEPTION_RETURN;
	else
		ret_val = (curr_process->ret_status & STATUS_MASK);


	asm (" jmp halt_return"
		:
		: "a"(ret_val));
	return ret_val;

}

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
		desc[chosen_index].inode_ptr = get_inode_ptr(data.inode_num);
		desc[chosen_index].flags |= REGULAR_FILE;
		desc[chosen_index].file_op_table_ptr = &fs_op;
	}
	return 0;
}

int32_t kopen(const uint8_t* file_name) {
	//sti();
	// are there any available file descriptors?
	uint32_t chosen_index = ERROR;
	int i;
	for (i = FD_START; i < FILE_ARRAY_SIZE; i++) {
		if ((desc[i].flags & FD_IN_USE) == 0) {
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

int32_t kread(int32_t fd, void* buf, int32_t nbytes){
	//sti();
	if(fd < 0 || fd >= FILE_ARRAY_SIZE || buf == NULL || nbytes < 0){
		return ERROR;
	}
	unsigned char * name;
	int ret;
	//sti();
	op_table_t* operations = desc[fd].file_op_table_ptr; 
	switch(desc[fd].flags & FILE_TYPE){
		case RTC:
			return operations->read();
		case DIRECTORY:
			return operations->read(NAME_LENGTH, (uint8_t*)buf);
		case REGULAR_FILE:
			name = return_dentry_by_ptr(desc[fd].inode_ptr);
			ret = operations->read((uint8_t*)name, desc[fd].file_position, nbytes, (uint8_t*)buf);
			desc[fd].file_position += ret;
			return ret;
		case STDIN:
            return operations->read(nbytes, buf);
		default:
			return ERROR;
	}
}

int32_t kwrite (int32_t fd, const void *buf, int32_t nbytes)
{
	//sti();
	if(fd < 0 || fd >= FILE_ARRAY_SIZE || buf == NULL || nbytes < 0
		|| !(desc[fd].flags & FD_IN_USE)) {
		return ERROR;
	}

	op_table_t *operations = desc[fd].file_op_table_ptr;
	switch (desc[fd].flags & FILE_TYPE)
	{
		case RTC:
			return operations->write((uint32_t)buf);
		case DIRECTORY:
			return operations->write();
		case REGULAR_FILE:
			return operations->write();
		case STDOUT:
		    return operations->write(nbytes, buf);
		default:
			return ERROR;
	}
}

int32_t kclose(int32_t fd)
{
	//sti();
	if (fd < 0 || fd >= FILE_ARRAY_SIZE || !(desc[fd].flags & FD_IN_USE))
		return ERROR;

	op_table_t *operations = (op_table_t*)(desc[fd].file_op_table_ptr);
	desc[fd].flags = FD_FREE;	
	return operations->close();
}


int32_t set_current_process(pcb_t* pcb)
{
	curr_process = pcb;
	desc = (file_descriptor_t*)(pcb->files);
	clear_fdarray(desc);
	tss.esp0 = (uint32_t)(curr_process->kernel_stack);
	return 0;
}

int32_t init_process()
{
	int i;
	for (i = 0; i < PID_ARR_SIZE; i++)
	{
		pid_arr[i] = 0;
	}
	curr_process = 0;
    return 0;
}

void setup_test()
{

}

int32_t kget_args(uint8_t* buf, int32_t nbytes) { return ERROR;}
int32_t kvidmap(uint8_t** screen_start) { return ERROR;}
int32_t kset_handler(int32_t signum, void* handler_address) { return ERROR;}
int32_t ksigreturn(void) { return ERROR;}
