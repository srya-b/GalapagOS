/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */
#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "rtc.h"
#include "debug.h"
#include "int_handlers.h"
#include "paging.h"
#include "assembly_linkage.h"
#include "terminal.h"
#include "file_system.h"
/* Macros. */

/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

static uint32_t fs_address;
/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;

	/* Clear the screen. */
	clear();

	int valid_fs = 0;
	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;
		while(mod_count < mbi->mods_count) {
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			fs_address = mod->mod_start;
			mod_count++;
			mod++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;

		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
					+ mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}

	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}


	/* Init the variables in IDT */
	{
		idt_desc_t the_idt_desc;
		the_idt_desc.seg_selector = KERNEL_CS;
		the_idt_desc.reserved4 = 0;
		the_idt_desc.reserved3 = 1;
		the_idt_desc.reserved2 = 1;
		the_idt_desc.reserved1 = 1;
		the_idt_desc.size = 1;
		the_idt_desc.reserved0 = 0;
		the_idt_desc.dpl = 0;
		the_idt_desc.present = 1;
		SET_IDT_ENTRY(the_idt_desc, divide_by_zero_linkage);
		idt[0] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, debugger_linkage);
		idt[1] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, NMI_linkage);
		idt[2] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, breakpoint_linkage);
		idt[3] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, overflow_linkage);
		idt[4] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, bounds_linkage);
		idt[5] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, invalid_opcode_linkage);
		idt[6] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, coprocessor_linkage);
		idt[7] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, double_fault_linkage);
		idt[8] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, coprocessor_segment_linkage);
		idt[9] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, invalid_tss_linkage);
		idt[10] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, segment_not_present_linkage);
		idt[11] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, stack_segment_fault_linkage);
		idt[12] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, general_protection_fault_linkage);
		idt[13] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, page_fault_linkage);
		idt[14] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, floating_point_linkage);
		idt[15] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, alignment_check_linkage);
		idt[16] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, machine_check_linkage);
		idt[17] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, SIMD_floating_point_linkage);
		idt[18] = the_idt_desc;

		the_idt_desc.reserved3 = 0;
		SET_IDT_ENTRY(the_idt_desc, keyboard_int_linkage);
		idt[33] = the_idt_desc;
		SET_IDT_ENTRY(the_idt_desc, rtc_int_linkage);
		idt[40] = the_idt_desc;
	}

	/* Init the IDTR */
	lidt(idt_desc_ptr);

	/* Init the PIC */
	i8259_init();

    /* Init the RTC */
	rtc_open();
	
	/* Enable interrupts */
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	printf("Enabling Interrupts\n");
	
	/*Enable the slave irq line on the master*/
    enable_irq(SLAVE_INT);

    /*Enable the keyboard irq line on the master*/
	enable_irq(KEYBOARD_INT);

	/*Enable the rtc irq line on the master*/
	enable_irq(RTC_INT);
	
	/*Initialize paging*/
	paging_init();
	
	terminal_open();
	
	sti();

	update_cursor(0,0);
	fs_init(fs_address);

	/* Print contents of a file by file-name */
	clear_terminal();
	uint8_t bro[5000];
	const char * file_name = "grep";
	int r = fs_read(file_name, 0,0, (uint8_t*)(&bro));
	clear_terminal();
	terminal_write(r, (uint8_t*)(&bro));

	/* Print files in file system */
	clear_terminal();
	uint8_t read_num = 32; // So that we can also read the file size (and write it to the screen in a meaningful way)
	int num_read = dir_read(read_num, (uint8_t*)(&bro));
	terminal_write(num_read, (uint8_t*)(&bro));
	terminal_write(1, "\n");
	while (0 != num_read) {
		num_read = dir_read(read_num, (uint8_t*)(&bro)); 
		terminal_write(num_read, (uint8_t*)(&bro));
		terminal_write(1, "\n");
	}

	/* Execute the first program (`shell') ... */
	uint8_t tru[128];
	while (1) {
		int ret = terminal_read(128,(uint8_t*)(&tru));
		terminal_write(ret, (uint8_t*)(&tru));
	}

	clear_terminal();	
	/* Demonstrate rtc works with rtc_read and rtc_write */

	uint32_t frequency = 0;
	while (1) {
		uint8_t * wait = (uint8_t*) "0, wait for flag to be set\n";
		terminal_write(strlen( (int8_t*)wait ), wait);

		uint8_t * set = (uint8_t*) "1, FLAG SET\n";
		if (rtc_read() == 0) terminal_write(strlen( (int8_t*)set ), set);

		rtc_write(frequency);
		frequency = (frequency + 1) % 1024;
	}

	/* Spin (nicely, so we don't chew up cycles) */
	asm volatile(".1: hlt; jmp .1;");
}

