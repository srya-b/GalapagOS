#include "paging.h"
#include "lib.h"

#define VID_MEM 0xb8000
#define ADDRESS_BITS 32
#define PAGE_BITS 12
#define TABLE_BITS 10
#define DIR_BITS 10
#define KERNEL_DIR_ENTRY 1
#define USR_DIR_ENTRY 32
#define VIDMEM_DIR_ENTRY 0
#define TEN_MASK 0x000003ff
#define TWELVE_MASK 0x00000fff
#define BIG_MASK 0x000fffff
#define VIRT_VID_MEM 0xdeadb000
#define VIRT_VIDMEM_DIR_ENTRY ((VIRT_VID_MEM >> 22) & TEN_MASK)
#define VIRT_VIDMEM_TABLE_ENTRY ((VIRT_VID_MEM >> 12) & TEN_MASK)

/* Directory ad table in in kernel space */
static uint32_t dir[NUM_ENT] __attribute__((aligned (KB4)));
static uint32_t table[NUM_ENT] __attribute__((aligned(KB4)));
static uint32_t first_dir[NUM_ENT] __attribute__((aligned(KB4)));
static uint32_t first_table[NUM_ENT] __attribute__((aligned(KB4)));
static page_directory_t process_directories[NUM_TASKS];
static page_table_t page_tables[NUM_TASKS];
static vid_mem_table_t vid_tables[NUM_TASKS] __attribute__((aligned(KB4)));
// static page_table_t page_tables;

void new_init(void)
{
	int i;
	int j;
	page_table_entry_t * table_entry;

	/* Set all dir and table entries to 2 (r/w) */
	for (j = 0; j < NUM_TASKS; j++)
	{
		for (i = 0; i < NUM_ENT; i++) {
			first_dir[i] = 2;
			first_table[i] = 2;
			process_directories[j].directory[i] = 2;
			table_entry = (page_table_entry_t*)(&(page_tables[j].table[i]));
			page_tables[j].table[i] = 2;
			table_entry->r_w = 1;
			/* Identity mape table base address */
			table_entry->base_31_12 = i;
			//vid_table[i] = 0;
			vid_tables[j].table[i] = 2;
		}
	}

	paging_init();

	int pid;
	for (pid = 0; pid < NUM_TASKS; pid++)
	{
		page_directory_t * pd = &(process_directories[pid]);
        page_table_t* pt = &(page_tables[pid]);

		page_dir_entry_small_t * ps = (page_dir_entry_small_t*)(&(pd->directory[VIDMEM_DIR_ENTRY]));
		ps->present = 1;
		ps->read_write = 1;
		ps->table_base = /*((uint32_t)(&table[0]) >> 12)*/ ((uint32_t)(&(pt->table[0])) >> PAGE_BITS);
		
		/* set PTE for video memory */
		table_entry = (page_table_entry_t*)(&(pt->table[(VID_MEM >> PAGE_BITS)]));
		table_entry->usr_su = 0;
		table_entry->present = 1;
		table_entry->r_w = 1;

		/* Kernel at second entry in dir */
		page_dir_entry_big_t* k_entry = (page_dir_entry_big_t*)(&(pd->directory[KERNEL_DIR_ENTRY]));
		k_entry->table_base = 1;
		k_entry->page_size = 1;
		k_entry->super = 0;
		k_entry->read_write = 1;
		k_entry->present = 1;

		page_dir_entry_big_t* process = (page_dir_entry_big_t*)(&(pd->directory[USR_DIR_ENTRY])); 
	    process->table_base = /*1 + */ 2 + pid;
	    process->page_size = 1;
	    process->super = 1;
	    process->read_write = 1;
	    process->present = 1;

	    /* set vid mem but nor present */
	    ps = (page_dir_entry_small_t*)(&(pd->directory[VIRT_VIDMEM_DIR_ENTRY]));
	    //table_entry = (page_table_entry_t*)(&vid_table[VIRT_VIDMEM_TABLE_ENTRY]);
	    table_entry = (page_table_entry_t*)(&(vid_tables[pid].table[VIRT_VIDMEM_TABLE_ENTRY]));
	    ps->present = 0;
	    ps->read_write = 1;
	    ps->table_base = ((uint32_t)(&(vid_tables[pid].table[0])) >> PAGE_BITS);
	    ps->page_size = 0;
	    ps->super = 1;
	    ps->global_page = 1;
	   	table_entry->usr_su = 1;
	   	table_entry->present = 0;
	   	table_entry->r_w = 1;
	   	table_entry->base_31_12 = (VID_MEM >> PAGE_BITS) & BIG_MASK;
	}
	page_enable(void);
}


/*
 * paging_init
 * DESCRIPTION: fills out first two entries of table directory, and sets up pages for first 4MB of memory 
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: all addresses will go through translation through the page directory and tables
 */
void paging_init(void) {

	int i;
	page_dir_entry_big_t * k_entry;
	page_dir_entry_small_t * entry;
	page_table_entry_t * table_entry;


	/* Set all dir and table entries to 2 (r/w) */
	for (i = 0; i < NUM_ENT; i++) {
		dir[i] = 2;
		table_entry = (page_table_entry_t*)(&table[i]);
		table[i] = 2;
		table_entry->r_w = 1;
		/* Identity mape table base address */
		table_entry->base_31_12 = i;
	}


	/* Set protected mode */
	protect_enable(void);

	/* Allow 4kB and 4MB pages */
	pse_enable(void);
	
	/* Set CR3 as address of dir */
	set_cr3(&dir[0]);

	/* set PDE for video memory */
	entry = (page_dir_entry_small_t*)(&dir[VIDMEM_DIR_ENTRY]);
	entry->present = 1;
	entry->read_write = 1;
	entry->table_base = ((uint32_t)(&table[0]) >> PAGE_BITS);


	/* set PTE for video memory */
	table_entry = (page_table_entry_t*)(&table[(VID_MEM >> PAGE_BITS)]);
	table_entry->usr_su = 0;
	table_entry->present = 1;
	table_entry->r_w = 1;

	/* Kernel at second entry in dir */
	k_entry = (page_dir_entry_big_t*)(&dir[KERNEL_DIR_ENTRY]);
	k_entry->table_base = 1;
	k_entry->page_size = 1;
	k_entry->super = 0;
	k_entry->read_write = 1;
	k_entry->present = 1;

}

int paging_init_per_process(uint32_t pid) {
    page_dir_entry_big_t * process = (page_dir_entry_big_t *)(&(process_directories[pid].directory[USR_DIR_ENTRY]));
    if(process->present == 0){
      process->table_base = 1 + pid;
      process->page_size = 1;
      process->super = 0;
      process->read_write = 1;
      process->present = 1;
      set_cr3(&(process_directories[pid]));
      return 0;
    }
    else return -1;
}

int switch_paging(uint32_t pid) {
	uint32_t* pd = &(process_directories[pid].directory[0]);
	if (pd == NULL) return ERROR;
	set_cr3(pd);
	return 0;
}

int get_dir_entry(int32_t vaddr)
{
	return (vaddr >> (PAGE_BITS + TABLE_BITS)) & TEN_MASK;
}

int get_table_entry(int32_t vaddr)
{
	return (vaddr >> PAGE_BITS) & TEN_MASK;
}

int get_page_offset_small(int32_t vaddr)
{
	return vaddr & TWELVE_MASK;
}

int get_page_offset_big(int32_t vaddr)
{
	return vaddr & BIG_MASK;
}

int address_mapped(int32_t vaddr, int pid)
{
	int dir_entry = get_dir_entry(vaddr);
	page_dir_entry_small_t * curr_entry = (page_dir_entry_small_t*)&(process_directories[pid].directory[dir_entry]);
	
	if (curr_entry->present == 1)
		return ERROR;
	return 0;
}

int address_in_user(int32_t vaddr, int pid)
{
	if (pid < 0) return ERROR;
	
	int dir_entry = get_dir_entry(vaddr);
	
	if (dir_entry != USR_DIR_ENTRY) return ERROR;

	return 0;
}

int map_vid_memory(int pid)
{
	if (pid < 0) return ERROR;

	page_dir_entry_small_t* d = (page_dir_entry_small_t*)(&(process_directories[pid].directory[VIRT_VIDMEM_DIR_ENTRY]));
	page_table_entry_t* pt = (page_table_entry_t*)(&(vid_tables[pid].table[VIRT_VIDMEM_TABLE_ENTRY]));
	pt->present = 1;	
	d->present = 1;
	set_cr3((uint32_t*)(&(process_directories[pid].directory[0])));
	return 0;
}

int unmap_vid_memory(int pid)
{
	if (pid < 0) return ERROR;

	page_dir_entry_small_t* d = (page_dir_entry_small_t*)(&(process_directories[pid].directory[VIRT_VIDMEM_DIR_ENTRY]));
	d->present = 0;
	page_table_entry_t* pt = (page_table_entry_t*)(&(vid_tables[pid].table[VIRT_VIDMEM_TABLE_ENTRY]));
	pt->present = 0;
	return 0;
}
