/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _SHARED_PAGE_H
#define _SHARED_PAGE_H

/* Defines a simple interface for sharing a single page of data across
 * CPU modes and SMP cores where it can be easily found and relied
 * upon.
 */

#include "xuk-config.h"
#include "x86_64-hw.h"

/* The shared block lives in the 5th page of memory, immediately after
 * the 16k null guard region
 */
#define SHARED_ADDR 0x4000

/* Magic cookies passed to stub32 to tell it what's going on */
#define BOOT_MAGIC_MULTIBOOT 0x2badb002 /* initial handoff from bootloader */
#define BOOT_MAGIC_STUB16    0xaaf08df7 /* AP cpu initialization */

struct xuk_shared_mem {
	/* Stack to be used by SMP cpus at startup.  MUST BE FIRST. */
	unsigned int smpinit_stack;

	/* Spinlock used to serialize SMP initialization */
	int smpinit_lock;

	/* Byte address of next page to allocate */
	unsigned int next_page;

	/* Top-level page table address */
	unsigned int base_cr3;

	/* 64 bit GDT */
	struct gdt64 gdt[3 + (2 * CONFIG_MP_NUM_CPUS)];

	/* 64 bit IDT */
	unsigned int idt_addr;

	/* Precomputed GDT for the 16 bit stub */
	unsigned int gdt16_addr;

	/* Each pointer in these arrays is the base of the FS/GS
	 * segment for the indexed CPU.
	 */
	long long fs_ptrs[CONFIG_MP_NUM_CPUS];
	long long gs_ptrs[CONFIG_MP_NUM_CPUS];

	int num_active_cpus;

	/* Current output column in the VGA console */
	int vgacol;
};

#define _shared (*((struct xuk_shared_mem *)(long)SHARED_ADDR))

static inline void shared_init(void)
{
	for (int i = 0; i < sizeof(_shared)/sizeof(int); i++) {
		((int *)&_shared)[i] = 0;
	}

	_shared.next_page = 0x5000;
	_shared.vgacol = 80;
}

static inline void *alloc_page(int clear)
{
	int *p = (int *)(long)_shared.next_page;

	_shared.next_page += 4096;

	for (int i = 0; clear && i < 1024; i++) {
		p[i] = 0;
	}

	return p;
}

#endif /* _SHARED_PAGE_H */
