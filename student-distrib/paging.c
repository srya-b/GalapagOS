#include "paging.h"
#include "lib.h"

/* Directory ad table in in kernel space */
static uint32_t dir[NUM_ENT] __attribute__((aligned (4096)));
static uint32_t table[NUM_ENT] __attribute__((aligned(4096)));

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

	/* Set all dir and table entries to 2 (r/w) */
	for (i = 0; i < NUM_ENT; i++) {
		dir[i] = 2;
		entry = (page_dir_entry_small_t*)(&table[i]);
		table[i] = 2;
		entry->read_write = 1;
		/* Identity mape table base address */
		entry->table_base = i;
	}


	/* Set protected mode */
	protect_enable(void);

	/* Allow 4kB and 4MB pages */
	pse_enable(void);
	
	/* Set CR3 as address of dir */
	set_cr3(&dir[0]);

	/* set PDE for video memory */
	entry = (page_dir_entry_small_t*)(&dir[0]);
	entry->present = 1;
	entry->read_write = 1;
	entry->table_base = ((uint32_t)(&table[0]) >> 12);

	/* set PTE for video memory */
	entry = (page_dir_entry_small_t*)(&table[(0xb8000 >> 12)]);
	entry->super = 0;
	entry->present = 1;
	entry->read_write = 1;


	/* Kernel at second entry in dir */
	k_entry = (page_dir_entry_big_t*)(&dir[1]);
	k_entry->table_base = 1;
	k_entry->page_size = 1;
	k_entry->super = 0;
	k_entry->read_write = 1;
	k_entry->present = 1;

	page_enable(void);
}
