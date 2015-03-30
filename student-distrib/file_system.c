#include "file_system.h"

static void *file_system;
static boot_block_t *boot_block;
static void *inodes;
static void *data;
static int curr_file_number;

#define ERROR (-1)
#define BLOCK_SIZE 4096
#define BOOT_BLOCK_SIZE() (sizeof(boot_block_t))
#define BOOT_INFO_SIZE() (sizeof(boot_info_t))
#define INODE_SIZE() (sizeof(inode_t))
#define NUM_DIR_ENTRIES() (boot_block->fs_info.num_dir_entries)
#define NUM_INODES() (boot_block->fs_info.num_inodes)
#define NUM_DATA_BLOCKS() (boot_block->fs_info.num_data_blocks)
#define SIZE_LOCATION 45

int32_t fs_init(uint32_t fs_address)
{
	file_system = (void*)fs_address;
	boot_block = (boot_block_t*)(file_system);
	inodes = file_system + BOOT_BLOCK_SIZE();
	curr_file_number = 0;
	int num_inodes = boot_block->fs_info.num_inodes;
	if (num_inodes > 0)
	{
		data = inodes + num_inodes * INODE_SIZE();
	} else return ERROR;
	return 0;
}

int32_t check_file_name(const uint8_t* fname, dentry_t* entry)
{
	int i;
	if (fname == NULL || entry == NULL) return ERROR;
	for (i = 0; i < 32; i++)
	{
		if (fname[i] != entry->file_name[i])
			return ERROR;
		
		if(fname[i] == '\0' && entry->file_name[i] == '\0')
			break;
	}
	return 0;
}

int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry)
{
	int i;
	if (fname == NULL || dentry == NULL) return ERROR;
	for (i = 0; i < 63; i++)
	{
		if (0 == check_file_name(fname, &(boot_block->entries[i])))
		{
			*dentry = (boot_block->entries[i]);
			return 0;
		}
	}

	return ERROR;
}


int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry)
{
	int i; 
	if (dentry == NULL || index > (NUM_DIR_ENTRIES()-1)) return ERROR;
	*dentry = (boot_block->entries[i]);
	return 0;
}

int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
	int block_idx;
	int ret;
	int i;
	int data_idx;
	inode_t* i_ptr;
	data_block_t *d_ptr;

	i_ptr = (inode_t*)(inodes + (inode * INODE_SIZE()));

	if (buf == NULL || inode > (NUM_INODES()-1) || inode < 0
		|| length < 0 || offset < 0 || i_ptr == NULL || offset > i_ptr->length) return ERROR;
  
	
	block_idx = offset / BLOCK_SIZE;
	ret = 0; 

 	for(i = 0; i < length; i++) {		
		if(offset + i > i_ptr->length) break;
		
		data_idx = i_ptr->data_blocks[block_idx];
		if (data_idx > (NUM_DATA_BLOCKS() - 1)) return ERROR;
		
		d_ptr = (data_block_t*)(data + data_idx * BLOCK_SIZE);

		if((offset+i) % BLOCK_SIZE == 0 && i != 0)
			block_idx++;
		
		ret++;
		buf[i] = d_ptr->data[(offset+i) % BLOCK_SIZE];
	}

	return ret;
}

int write_dec_to_char(int num, uint8_t *buf)
{
	uint8_t *start = buf;
	int ret = 0;
	if (num == 0)
	{
		*buf = '0';
		buf++;
		ret++;
	}
	while (num != 0)
	{
		int lsd = num % 10;
		*buf = (char)(lsd + '0');
		buf++;
		ret++;
		num /= 10;
	}
	int i;
	for (i = 0; i < (ret/2); i++)
	{
		char t = start[i];
		start[i] = start[ret - i - 1];
		start[ret - i - 1] = t;
	}
	return ret;
}

int fs_open() { return 0; }
int fs_read(uint8_t *fname, int offset, int count, uint8_t * buf)
{
	dentry_t d;
	int ret = read_dentry_by_name(fname, &d);
	if (ret == ERROR) return ERROR;

	return read_data(d.inode_num, offset, buf, count);
}

int fs_write() { return -1; }
int fs_close() { return 0; }

int dir_open() { return 0; }
int dir_read(uint32_t count, uint8_t *buf)
{
	int j;
	int i_num;
	int file_size;
	int num_written;
	int read;
	int temp = count;
	if (curr_file_number >= NUM_DIR_ENTRIES()) return 0;

	read = 0;
	i_num = boot_block->entries[curr_file_number].inode_num;
	inode_t *ptr = (inode_t*)(inodes + (i_num * INODE_SIZE()) );
	file_size = ptr->length;
	

	for (j = 0; j < 32 && boot_block->entries[curr_file_number].file_name[j] != '\0'; j++)
	{
		*buf = boot_block->entries[curr_file_number].file_name[j];
		read++;
		buf++;
		temp--;
		if (temp == 0) 
		{
			curr_file_number++;
			return read;
		}
	}

	if (count >  32) 
	{
		for (j = read; j < SIZE_LOCATION; j++)
		{
			*buf = '.';
			buf++;
			read++;
		}

		num_written = write_dec_to_char(file_size, buf);
		buf += (num_written);
		read += (num_written);
		*buf = ' ';
		buf++;
		*buf = 'B';
		buf++;
		read += 2;
	}
	// *buf = '\n';
	// read++;
	curr_file_number++;
	return read;
}

int dir_write() { return -1; }
int dir_close() { return 0; }
