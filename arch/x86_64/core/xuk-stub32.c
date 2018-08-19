/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "xuk-config.h"
#include "shared-page.h"
#include "x86_64-hw.h"

#ifdef CONFIG_XUK_DEBUG
#include "printf.h"
#include "vgacon.h"
#include "serial.h"
#else
int printf(const char *fmt, ...)
{
	return 0;
}
#endif

/* This i386 code stub is designed to link internally (i.e. it shares
 * nothing with the 64 bit world) and be loaded into RAM in high
 * memory (generally at 0x100000) in a single (R/W/X) block with its
 * .text, .rodata, .data and .bss included.  Its stack lives in the
 * fifth page of memory at 0x04000-0x4fff.  After finishing 64 bit
 * initialization, it will JMP to the 16-byte-aligned address that
 * immediately follows this block in memory (exposed by the linker as
 * _start64), which should then be able to run in an environment where
 * all of physical RAM is mapped, except for the bottom 16kb.
 *
 * Memory layout on exit:
 *
 * + Pages 0-3   are an unmapped NULL guard
 * + Page 4:     contains stack and bss for the setup code, and a GDT.
 *               After 64 bit setup, it's likely this will be reused .
 * + Pages 5-11: are the bootstrap page table
 *
 * Note that the initial page table makes no attempt to identify
 * memory regions.  Everything in the first 4G is mapped as cachable
 * RAM.  MMIO drivers will need to remap their memory based on PCI BAR
 * regions or whatever.
 */

/* Cute trick to turn a preprocessor macro containing a number literal
 * into a string immediate in gcc basic asm context
 */
#define _ASM_IMM(s) #s
#define ASM_IMM(s) "$" _ASM_IMM(s)

/* Entry point, to be linked at the very start of the image.  Set a
 * known-good stack (either the top of the shared page for the boot
 * CPU, or one provided by stub16 on others), push the multiboot
 * arguments in EAX, EBX and call into C code.
 */
__asm__(".pushsection .start32\n"
	"   mov $0x5000, %esp\n"
	"   xor %edx, %edx\n"
	"   cmp " ASM_IMM(BOOT_MAGIC_STUB16) ", %eax\n"
	"   cmove 0x4000(%edx), %esp\n"
	"   pushl %ebx\n"
	"   pushl %eax\n"
	"   call cstart\n"
	".popsection\n");

/* The multiboot header can be anywhere in the first 4k of the file.
 * This stub doesn't get that big, so we don't bother with special
 * linkage.
 */
#define MULTIBOOT_MAGIC 0x1badb002
#define MULTIBOOT_FLAGS (1<<1) /* 2nd bit is "want memory map" */
const int multiboot_header[] = {
	MULTIBOOT_MAGIC,
	MULTIBOOT_FLAGS,
	-(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS), /* csum: -(magic+flags) */
};

/* Creates and returns a generic/sane page table for 64 bit startup
 * (64 bit mode requires paging enabled).  All of the bottom 4G
 * (whether backing memory is present or not) gets a mapping with 2M
 * pages, except that the bottom 2M are mapped with 4k pages and leave
 * the first four pages unmapped as a NULL guard.
 *
 * Makes no attempt to identify non-RAM/MMIO regions, it just maps
 * everything.  We rely on the firmware to have set up MTRRs for us
 * where needed, otherwise that will all be cacheable memory.
 */
void *init_page_tables(void)
{
	/* Top level PML4E points to a single PDPTE in its first entry */
	struct pte64 *pml4e = alloc_page(1);
	struct pte64 *pdpte = alloc_page(1);

	pml4e[0].addr = (unsigned long)pdpte;
	pml4e[0].present = 1;
	pml4e[0].writable = 1;

	/* The PDPTE has four entries covering the first 4G of memory,
	 * each pointing to a PDE
	 */
	for (unsigned int gb = 0; gb < 4; gb++) {
		struct pte64 *pde = alloc_page(0);

		pdpte[gb].addr = (unsigned long)pde;
		pdpte[gb].present = 1;
		pdpte[gb].writable = 1;

		/* Each PDE filled with 2M supervisor pages */
		for (int i = 0; i < 512; i++) {
			if (!(gb == 0 && i == 0)) {
				pde[i].addr = (gb << 30) | (i << 21);
				pde[i].present = 1;
				pde[i].writable = 1;
				pde[i].pagesize_pat = 1;
			} else {
				/* EXCEPT the very first entry of the
				 * first GB, which is a pointer to a
				 * PTE of 4k pages so that we can have
				 * a 16k (4-page) NULL guard unmapped.
				 */
				struct pte64 *pte = alloc_page(0);

				pde[0].addr = (unsigned long)pte;
				pde[0].present = 1;
				pde[0].writable = 1;

				for (int j = 0; j < 512; j++) {
					if (j < 4) {
						pte[j].addr = 0;
					} else {
						pte[j].addr = j << 12;
						pte[j].present = 1;
						pte[j].writable = 1;
					}
				}
			}
		}
	}

	/* Flush caches out of paranoia.  In theory, x86 page walking
	 * happens downstream of the system-coherent dcache and this
	 * isn't needed.
	 */
	__asm__ volatile("wbinvd");
	return pml4e;
}

#ifdef CONFIG_XUK_DEBUG
void putchar(int c)
{
	serial_putc(c);
	vgacon_putc(c);
}
#endif

void cstart(unsigned int magic, unsigned int arg)
{
	if (magic == BOOT_MAGIC_STUB16) {
		printf("SMP CPU up in 32 bit protected mode.  Stack ~%xh\n",
		       &magic);
	}

	if (magic != BOOT_MAGIC_STUB16) {
		shared_init();
#ifdef CONFIG_XUK_DEBUG
		serial_init();
		_putchar = putchar;
#endif

		printf("Entering stub32 on boot cpu, magic %xh stack ~%xh\n",
		       magic, (int)&magic);
	}

	/* The multiboot memory map turns out not to be very useful.
	 * The basic numbers logged here are only a subset of the true
	 * memory map if it has holes or >4G memory, and the full map
	 * passed in the second argument tends to live in low memory
	 * and get easily clobbered by our own muckery.  If we care
	 * about reading memory maps at runtime we probably want to be
	 * using BIOS e820 like Linux does.
	 */
	if (magic == BOOT_MAGIC_MULTIBOOT) {
		printf("Hi there!\n");
		printf("This is a second line!\n");
		printf("And this line was generated from %s\n", "printf!");

		printf("Magic: %p MBI Addr: %p\n", magic, arg);

		int mem_lower = *(int *)(arg + 4);
		int mem_upper = *(int *)(arg + 8);
		int mmap_length = *(int *)(arg + 44);
		int *mmap_addr = *(void **)(arg + 48);

		printf("mem lower %d upper %d mmap_len %d mmap_addr %p\n",
		       mem_lower, mem_upper, mmap_length, mmap_addr);
	}

	/* Choose a stack pointer and CPU ID for the 64 bit code to
	 * use.  Then if we're not the boot CPU, release the spinlock
	 * (taken in stub16) so the other CPUs can continue.
	 */
	int cpu_id = 0;
	unsigned int init_stack = 0x5000;

	if (magic == BOOT_MAGIC_STUB16) {
		cpu_id = _shared.num_active_cpus++;
		init_stack = _shared.smpinit_stack;
		_shared.smpinit_stack = 0;
		__asm__ volatile("movl $0, (%0)" : : "m"(_shared.smpinit_lock));
	}

	/* Page table goes in CR3.  This is a noop until paging is
	 * enabled later
	 */
	if (magic != BOOT_MAGIC_STUB16) {
		_shared.base_cr3 = (unsigned int)init_page_tables();
	}
	SET_CR("cr3", _shared.base_cr3);

	/* Enable PAE bit (5) in CR4, required because in long mode
	 * we'll be using the 64 bit page entry format.  Likewise a
	 * noop until the CPU starts loading pages.
	 */
	SET_CR_BIT("cr4", 5);

	/* Set LME (long mode enable) in IA32_EFER.  Still not a mode
	 * transition, simply tells the CPU that, once paging is
	 * enabled, we should enter long mode.  At that point the LMA
	 * bit (10) will be set to indicate that it's active.
	 */
	const int MSR_IA32_EFER = 0xc0000080;

	set_msr_bit(MSR_IA32_EFER, 8);

	/* NOW we transition by turning paging on.  The CPU will start
	 * page translation (which has been carefully
	 * identity-mapped!) and enter the 32 bit compatibility
	 * submode of long mode.  So we're reading 64 bit page tables
	 * but still executing 32 bit instructions.
	 */
	SET_CR_BIT("cr0", 31);

	printf("Hello memory mapped world!\n");

	/* Now we can enter true 64 bit long mode via a far call to a
	 * code segment with the 64 bit flag set.  Allocate a 2-entry
	 * GDT (entry 0 is always a "null segment" architecturally and
	 * can't be used) here on the stack and throw it away after
	 * the jump.  The 64 bit OS code will need to set the
	 * descriptors up for itself anyway
	 */
	struct gdt64 cs[] = {
		{ },
		{
		 .readable = 1,
		 .codeseg = 1,
		 .notsystem = 1,
		 .present = 1,
		 .long64 = 1,
		 },
	};

	/* The limit comes first, but is 16 bits.  The dummy is there
	 * for alignment, though docs aren't clear on whether it's
	 * required or not
	 */
	struct {
		unsigned short dummy;
		unsigned short limit;
		unsigned int addr;
	} gdtp = { .limit = sizeof(cs), .addr = (int)&cs[0], };

	printf("CS descriptor 0x%x 0x%x\n", cs[1].dwords[1], cs[1].dwords[0]);
	__asm__ volatile("lgdt %0" : : "m"(gdtp.limit) : "memory");

	/* Finally, make a far jump into the 64 bit world.  The entry
	 * point is a 16-byte-aligned address that immediately follows
	 * our stub, and is exposed by our linkage as "_start64".
	 *
	 * Indirect far jumps have a similar crazy setup to descriptor
	 * tables, but here the segment selector comes last so no
	 * alignment worries.
	 *
	 * The 64 bit entry reuses the same stack we're on, and takes
	 * the cpu_id in its first argument.
	 */
	extern int _start64;
	unsigned int jmpaddr = (unsigned int) &_start64;
	struct {
		unsigned int addr;
		unsigned short segment;
	} farjmp  = { .segment = GDT_SELECTOR(1), .addr = jmpaddr };

	printf("Making far jump to 64 bit mode @%xh...\n", &_start64);
	__asm__ volatile("mov %0, %%esp; ljmp *%1" ::
			 "r"(init_stack), "m"(farjmp), "D"(cpu_id)
			 : "memory");
}
