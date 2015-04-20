#ifndef _FILE_ARRAY_H
#define _FILE_ARRAY_H

#ifndef ASM
#include "assembly_linkage.h"
#include "types.h"
#include "lib.h"
#include "paging.h"
#include "file_system.h"
#include "terminal.h"
#include "x86_desc.h"
//#include "process.h"
#define FILE_ARRAY_SIZE 8
#define MAX_ARGS 100
#define MAX_COMMAND 33

int32_t halt(uint8_t status);
int32_t execute(const uint8_t* command);
int32_t read(int32_t fd, void* buf, int32_t nbytes);
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
int32_t open(const uint8_t* fileame);
int32_t close(int32_t fd);
int32_t get_args(uint8_t* buf, int32_t nbytes);
int32_t vidmap(uint8_t** screen_start);
int32_t set_handler(int32_t signum, void* handler_address);
int32_t sigreturn(void);
int32_t init_process();

#endif
#endif
