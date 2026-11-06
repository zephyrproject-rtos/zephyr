/*
 * Copyright (c) Qualcomm Technologies, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <zephyr/sys/__assert.h>

/*
 * Offset between the link-time and run-time address, stored by the
 * early self-relocation code.
 */
extern unsigned int g_load_offset;

int main(void);

/* Statically initialized, so it only holds the run-time address if the dynamic relocations ran. */
static int (*volatile main_ptr)(void) = main;

int main(void)
{
	uintptr_t run_addr;
	uintptr_t link_addr;

	/* PC-relative, so this is the address main() is really executing at. */
	__asm__ volatile("lla %0, %1" : "=r"(run_addr) : "i"(main));
	link_addr = run_addr - g_load_offset;

	__ASSERT(g_load_offset == SAMPLE_LOAD_OFFSET,
		 "load offset %#x, expected %#x", g_load_offset, (unsigned int)SAMPLE_LOAD_OFFSET);
	__ASSERT((uintptr_t)main_ptr == run_addr,
		 "%s() relocated to %#lx but runs at %#lx", __func__,
		 (unsigned long)(uintptr_t)main_ptr, (unsigned long)run_addr);

	printf("%s() linked at:  %#lx\n", __func__, (unsigned long)link_addr);
	printf("%s() running at: %#lx\n", __func__, (unsigned long)run_addr);
	printf("Relocation OK: load offset %#x\n", g_load_offset);

	return 0;
}
