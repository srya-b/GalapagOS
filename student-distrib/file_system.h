#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#include "types.h"
#include "lib.h"
#include "terminal.h"

typedef struct boot_info {
	uint32_t num_dir_entries;
	uint32_t num_inodes;
	uint32_t num_data_blocks;
	char reserved[52];
} __attribute__((packed)) boot_info_t;

typedef struct dentry {
	unsigned char file_name[32];
	uint32_t file_type;
	uint32_t inode_num;
	char reserved[24];
} __attribute__((packed)) dentry_t;

typedef struct boot_block {
	boot_info_t fs_info;
	dentry_t entries[63];
} __attribute__((packed)) boot_block_t;

typedef struct inode {
	uint32_t length;
	uint32_t data_blocks[1023];
} __attribute__((packed)) inode_t;

typedef struct data_block {
	uint8_t data[4096];
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

#endif
