#include "paging.h"
#include "lib.h"

#define VID_MEM 0xb8000
#define PAGE_BITS 12
#define KERNEL_DIR_ENTRY 1
#define USR_DIR_ENTRY 32
#define VIDMEM_DIR_ENTRY 0

/* Directory ad table in in kernel space */
static uint32_t dir[NUM_ENT] __attribute__((aligned (KB4)));
static uint32_t table[NUM_ENT] __attribute__((aligned(KB4)));
static uint32_t first_dir[NUM_ENT] __attribute__((aligned(KB4)));
static uint32_t first_table[NUM_ENT] __attribute__((aligned(KB4)));
static page_directory_t process_directories[NUM_TASKS];
static page_table_t page_tables[NUM_TASKS];

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

//	page_enable(void);
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

