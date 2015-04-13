#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#include "types.h"
#include "lib.h"
#include "terminal.h"
#define BINFO_RESERVED 52
#define FILE_NAME 32
#define DENTRY_RESERVED 24
#define BOOT_BLOCK_ENTRIES 63
#define BLOCK_SIZE 4096
#define KB4 4096
#define KB1 1024

typedef struct boot_info {
	uint32_t num_dir_entries;
	uint32_t num_inodes;
	uint32_t num_data_blocks;
	char reserved[BINFO_RESERVED];
} __attribute__((packed)) boot_info_t;

typedef struct dentry {
	unsigned char file_name[FILE_NAME];
	uint32_t file_type;
	uint32_t inode_num;
	char reserved[DENTRY_RESERVED];
} __attribute__((packed)) dentry_t;

typedef struct boot_block {
	boot_info_t fs_info;
	dentry_t entries[BOOT_BLOCK_ENTRIES];
} __attribute__((packed)) boot_block_t;

typedef struct inode {
	uint32_t length;
	uint32_t data_blocks[KB1 - 1];
} __attribute__((packed)) inode_t;

typedef struct data_block {
	uint8_t data[KB4];
} __attribute__((packed)) data_block_t;

int32_t fs_init(uint32_t fs_address);
int fs_open();
int fs_read(uint8_t *fname, int offset, int count, uint8_t * buf);
int fs_write();
int fs_close();

int dir_open();
int dir_read(uint32_t count, uint8_t *buf);
int dir_write();
int dir_close();

/* helpful functions */
int write_dec_to_char(int num, uint8_t *buf);
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(inode_t * inode, uint32_t offset, uint8_t* buf, uint32_t length);
unsigned char * return_dentry_by_ptr(inode_t *inode);
inode_t* get_inode_ptr(int inode_num);
// extern void *file_system;
// extern boot_block_t *boot_block;
// extern void *inodes;
// extern void *data;
// extern int curr_file_number;
#endif
