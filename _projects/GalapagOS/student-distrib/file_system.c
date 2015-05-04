#include "file_system.h"
#include "file_array.h"

static void *file_system;
static boot_block_t *boot_block;
static void *inodes;
static void *data;
static int curr_file_number;

#define ERROR (-1)
#define BOOT_BLOCK_SIZE() (sizeof(boot_block_t))
#define BOOT_INFO_SIZE() (sizeof(boot_info_t))
#define INODE_SIZE() (sizeof(inode_t))
#define NUM_DIR_ENTRIES() (boot_block->fs_info.num_dir_entries)
#define NUM_INODES() (boot_block->fs_info.num_inodes)
#define NUM_DATA_BLOCKS() (boot_block->fs_info.num_data_blocks)
#define SIZE_LOCATION 45
#define FNAME_SIZE 32
#define SUCCESS 0

/* get_inode_ptr
 * DESCRIPTION: takes an inode number and returns an inode pointer
 * INPUTS: int inode_num -- inode number (not consecutive in boot block)
 * OUPUTS: none
 * RETURN VALUE: inode_t* - address of the inode
 * SIDE EFFECTS: none
*/
inode_t* get_inode_ptr(int inode_num)
{
	if (inode_num < 0) return (inode_t*)ERROR;
	return (inode_t*)(inodes + (inode_num * INODE_SIZE()));
}

/* fs_init
 * DESCRIPTION: initializes the file system variables
 * INPUTS: uint32_t fs_address - the starting address of the file system
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 failure)
 * SIDE EFFECTS: none
*/
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
	return SUCCESS;
}

/* check_file_name
 * DESCRIPTION: checks to see if a file name exists for a given data entry
 * INPUTS: const uint8_t* fname -- file name, dentry_t* entry -- data entry to check for file name
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 failure)
 * SIDE EFFECTS: none
*/
int32_t check_file_name(const uint8_t* fname, dentry_t* entry)
{
	int i;
	if (fname == NULL || entry == NULL) return ERROR;
	for (i = 0; i < FNAME_SIZE; i++)
	{
		if (fname[i] != entry->file_name[i])
			return ERROR;
		
		if(fname[i] == '\0' && entry->file_name[i] == '\0')
			break;
	}
	return SUCCESS;
}

/* read_dentry_by_name
 * DESCRIPTION: assigns a data entry for use if the file name is found in the boot block
 * INPUTS: const uint8_t* fname -- file name, dentry_t* dentry -- data entry that will be assigned with boot block info
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 failure)
 * SIDE EFFECTS: grabs data entry in boot block and dereferences dentry pointer to change the contents of its memory
*/

int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry)
{
	int i;
	if (fname == NULL || dentry == NULL || fname[0] == '\0') return ERROR;
	for (i = 0; i < 63; i++)
	{
		if (SUCCESS == check_file_name(fname, &(boot_block->entries[i])))
		{
			*dentry = (boot_block->entries[i]);
			return SUCCESS;
		}
	}

	return ERROR;
}

/* read_dentry_by_index
 * DESCRIPTION: checks to see if a file name exists for a given data entry
 * INPUTS: uint32_t index -- index into boot block, dentry_t* dentry -- data entry to be assigned by index into boot block
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code (0 on success, -1 failure)
 * SIDE EFFECTS: dentry is dereferenced and assigned book block at file index
*/

int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry)
{
	int i; 
	if (dentry == NULL || index > (NUM_DIR_ENTRIES()-1)) return ERROR;
	*dentry = (boot_block->entries[i]);
	return SUCCESS;
}

/* read_dentry_by_ptr
 * DESCRIPTION: takes inode pointer and returns corresponding data entry's file name
 * INPUTS: inode_t *inode - inode pointer
 * OUPUTS: none
 * RETURN VALUE: unsigned char * - file name (NULL if error)
 * SIDE EFFECTS: boot block is searched through for proper inode pointer, reads but no writes
*/

unsigned char * return_dentry_by_ptr(inode_t *inode)
{
	int i;
	if (inode == NULL) return NULL;

 	int inode_number = ((uint32_t)inode - (uint32_t)inodes) / INODE_SIZE();
	
	int number_dir_entries = NUM_DIR_ENTRIES();

	// search for right dir entry in the boot block based on inode number
	for (i = 0; i < number_dir_entries; i++) {
		if ( boot_block->entries[i].inode_num == inode_number ) 
			return (unsigned char *) boot_block->entries[i].file_name;
	}

	// if you can't find the right dir entry in the boot block, return error
 	return NULL;
}

/* read_data
 * DESCRIPTION: reads data by virtue of an inode pointer into a buffer
 * INPUTS: inode_t* inode -- where you will gain access to data blocks, uint32_t offset, uint8_t* buf, uint32_t length -- length in bytes of desired
 						file read
 * OUPUTS: none
 * RETURN VALUE: int32_t - number of bytes read
 * SIDE EFFECTS: buffer is dereferenced and written to, data blocks (not necessarily consecutive) are jumped between seamlessly for file read
*/

int32_t read_data(inode_t* inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
	int block_idx;
	int ret;
	int i;
	int data_idx;
	inode_t* i_ptr = inode;
	data_block_t *d_ptr;

//	i_ptr = (inode_t*)(inodes + (inode * INODE_SIZE()));

	if (buf == NULL || /*inode > (NUM_INODES()-1) || inode < 0*/ inode == NULL
		|| length < 0 || offset < 0 || i_ptr == NULL)
	if (offset > i_ptr->length) return 0;
  
	
	block_idx = offset / BLOCK_SIZE;
	ret = 0; 

 	for(i = 0; i < length; i++) {		
		if(offset + i >= i_ptr->length) break;
		
		data_idx = i_ptr->data_blocks[block_idx];
		if (data_idx > (NUM_DATA_BLOCKS() - 1)) return ERROR;
		
		d_ptr = (data_block_t*)(data + data_idx * BLOCK_SIZE);

		if((offset+i+1) % BLOCK_SIZE == 0)
		{
			block_idx++;
		}
		ret++;
		buf[i] = d_ptr->data[(offset+i) % BLOCK_SIZE];
	}

	return ret;
}

/* write_dec_to_char
 * DESCRIPTION: take an integer and write is as a character
 * INPUTS: int num -- integer, uint8_t *buf -- buffer where you will write it as a character
 * OUPUTS: none
 * RETURN VALUE: int32_t - ret keeps track of number of characters written to buffer
 * SIDE EFFECTS: ascii value is writen to buffer
*/
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

/* fs_open
 * DESCRIPTION: open file system
 * INPUTS: none
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code
 * SIDE EFFECTS: always returns successfully
*/

int fs_open() { return SUCCESS; }

/* fs_read_by_name
 * DESCRIPTION: perform a read into a buffer through the aid of the file's name
 * INPUTS: uint8_t *fname -- file name, int offset -- offset into file, int count -- number of bytes to read, uint8_t * buf -- buffer to write to
 * OUPUTS: none
 * RETURN VALUE: int - keeps track of number of bytes written to buffer
 * SIDE EFFECTS: calls read_dentry_by_name to get a hold of data entry for file name, then calls read data() after getting inode pointer from data entry
*/

int fs_read_by_name(uint8_t *fname, int offset, int count, uint8_t * buf)
{
	dentry_t d;
	int ret = read_dentry_by_name(fname, &d);
	if (ret == ERROR) return ERROR;

	inode_t* i_ptr = (inode_t*)(inodes + (d.inode_num * INODE_SIZE()));
	return read_data(i_ptr, offset, buf, count);
}


/* fs_read
 * DESCRIPTION: perform a read into a buffer through the aid of the file's inode pointer
 * INPUTS: inode_t* ptr -- file's inode pointer, int offset -- offset into file, int count -- number of bytes to read, uint8_t * buf -- buffer to write to
 * OUPUTS: none
 * RETURN VALUE: int - keeps track of number of bytes written to buffer/read
 * SIDE EFFECTS: calls return_dentry_by_ptr to get name, then read_dentry_by_name to get a hold of data entry for file name, then calls read data() after getting inode pointer from data entry
				also calls increment position to update file descriptor location field of file
*/

//int fs_read(uint8_t *fname, int offset, int count, uint8_t * buf)
int fs_read(inode_t* ptr, int offset, int count, uint8_t * buf)
{
	dentry_t d;
//	file_descriptor_t* fdptr = (file_descriptor_t*)ptr;
	uint8_t* fname = (uint8_t*) return_dentry_by_ptr(ptr);
	int ret = read_dentry_by_name(fname, &d);
	if (ret == ERROR) return ERROR;

	inode_t* i_ptr = (inode_t*)(inodes + (d.inode_num * INODE_SIZE()));

	ret = read_data(i_ptr, offset, buf, count);
//	desc[fd].file_position += ret;
	increment_position((struct inode_t*)ptr, ret);
//	return read_data(i_ptr, offset, buf, count);
	return ret;
}

/* fs_write
 * DESCRIPTION: writes to file system
 * INPUTS: int32_t fd -- file descriptor, void* buf -- buffer, int32_t nbytes -- number of bytes to write to file
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code
 * SIDE EFFECTS: never returns successfully -- WE'VE IMPLEMENTED A READ ONLY FILE SYSTEM!
*/
int fs_write(int32_t fd, void* buf, int32_t nbytes) { return ERROR; }

/* fs_close
 * DESCRIPTION: closes a file_system
 * INPUTS: none
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code
 * SIDE EFFECTS: always returns successfully
*/
int fs_close() { return SUCCESS; }

/* dir_open
 * DESCRIPTION: opens a directory
 * INPUTS: none
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code
 * SIDE EFFECTS: always returns successfully
*/
int dir_open() { 
	return SUCCESS; 
}


/* dir_read
 * DESCRIPTION: reads from the directory at a current inode (into a buffer)
 * INPUTS: inode_t* p - inode, int offset, int count, uint8_t * buf
 * OUPUTS: none
 * RETURN VALUE: int32_t - number of bytes read
 * SIDE EFFECTS: very helpful in ls for a read of all the names of files in a current directory
*/
//int dir_read(uint32_t count, uint8_t *buf)
int dir_read(inode_t* p, int offset, int count, uint8_t * buf)
{
	int j;
	int i_num;
	int file_size;
	int num_written;
	int read;
	int temp = count;
	if (curr_file_number >= NUM_DIR_ENTRIES()){
		curr_file_number = 0;
		return 0;
	}
	count = FNAME_SIZE;
	read = 0;
	i_num = boot_block->entries[curr_file_number].inode_num;
	inode_t *ptr = (inode_t*)(inodes + (i_num * INODE_SIZE()) );
	file_size = ptr->length;
	

	for (j = 0; j < FNAME_SIZE && boot_block->entries[curr_file_number].file_name[j] != '\0'; j++)
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

	if (count >  FNAME_SIZE) 
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
	curr_file_number++;
	return read;
}

/* dir_write
 * DESCRIPTION: writes a new directory
 * INPUTS: int32_t fd -- file descriptor, void* buf, int32_t nbytes
 * OUPUTS: none
 * RETURN VALUE: int32_t - error code
 * SIDE EFFECTS: will always return failure because we CANNOT wrtie new directories (read-only file system)
*/
int dir_write(int32_t fd, void* buf, int32_t nbytes) { return ERROR; }


/* dir_close
 * DESCRIPTION: closes a directory
 * INPUTS: none
 * OUPUTS: none
 * RETURN VALUE: int - error code
 * SIDE EFFECTS: will always return successfully (because new directory was never opened!)
*/
int dir_close() { return SUCCESS; }
