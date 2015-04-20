#include "sys_calls.h"
#define HALT 1
#define EXECUTE 2
#define READ 3
#define WRITE 4
#define OPEN 5
#define CLOSE 6
#define GET_ARGS 7
#define VIDMAP 8
#define SET_HANDLER 9
#define SIGRETURN 10
#define NO_ARG 0

int32_t do_sys_call(int num, int arg1, int arg2, int arg3)
{
	int ret;
	asm (
		"int $0x80"
		: "=a"(ret),
	 	  "+b"(arg1),
	 	  "+c"(arg2),
	 	  "+d"(arg3)
	 	: "a" (num)
	 	: "memory", "cc");
	if (ret < 0) return -1;
	return ret;
}

int32_t halt(uint8_t status) { return do_sys_call(HALT, (int)status, NO_ARG, NO_ARG); }
int32_t execute(const uint8_t* command) { return do_sys_call(HALT, (int)command, NO_ARG, NO_ARG); }
int32_t read(int32_t fd, void* buf, int32_t nbytes) { return do_sys_call(HALT, (int)fd, (int)buf, (int)nbytes); }
int32_t write(int32_t fd, const void* buf, int32_t nbytes) { return do_sys_call(HALT, (int)fd, (int)buf, (int)nbytes); }
int32_t open(const uint8_t* filename) { return do_sys_call(OPEN, (int)filename, NO_ARG, NO_ARG); }
int32_t close(int32_t fd) { return do_sys_call(CLOSE, (int)fd, NO_ARG, NO_ARG); }
int32_t get_args(uint8_t* buf, int32_t nbytes) { return do_sys_call(GET_ARGS, (int)buf, (int)nbytes, NO_ARG); }
int32_t vidmap(uint8_t** screen_start) { return do_sys_call(VIDMAP, (int)screen_start, NO_ARG, NO_ARG); }
int32_t set_handler(int32_t signum, void* handler_address) { return do_sys_call(SET_HANDLER, (int)signum, (int)handler_address, NO_ARG); }
int32_t sigreturn(void){ return do_sys_call(SIGRETURN, NO_ARG, NO_ARG, NO_ARG); }
