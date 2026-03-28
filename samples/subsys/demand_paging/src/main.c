/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>

#include <mmu.h> /* for k_mem_num_pagefaults_get() */

/*
 * We want to artificially create a huge body of code hence the volatile
 * to prevent the compiler from optimizing everything down.
 */
volatile long foo_val1;
volatile long foo_val2;

/*
 * This function is tagged with __ondemand_func to be placed in a special
 * "ondemand" segment by the linker. This means it is not loaded into memory
 * at boot time but rather page-by-page on demand when actually executed.
 * It is also subject to being evicted when memory reclamation occurs.
 */
static void __ondemand_func huge_evictable_function(void)
{
	foo_val1 = 13131313;
	foo_val2 = 45454545;

#define CODE \
	foo_val1 *= foo_val2; \
	foo_val2 += 1234567; \
	foo_val1 -= 9876543210; \
	__asm__ __volatile__ (".rep 1000; nop; .endr");

#define REPEAT_10(x) x x x x x x x x x x
#define REPEAT_1000(x) REPEAT_10(REPEAT_10(REPEAT_10(x)))

	REPEAT_1000(CODE)

#undef REPEAT_1000
#undef REPEAT_10
#undef CODE
}

int main(void)
{
	size_t free_pages_before, free_pages_after;
	unsigned long faults_before, faults_after;

	printk("Calling huge body of code that doesn't fit in memory\n");
	free_pages_before = k_mem_free_get() / CONFIG_MMU_PAGE_SIZE;
	faults_before = k_mem_num_pagefaults_get();

	huge_evictable_function();

	free_pages_after = k_mem_free_get() / CONFIG_MMU_PAGE_SIZE;
	faults_after = k_mem_num_pagefaults_get();
	printk("free memory pages: from %zd to %zd, %ld page faults\n",
	       free_pages_before, free_pages_after, faults_after - faults_before);

	printk("Calling it a second time\n");
	free_pages_before = k_mem_free_get() / CONFIG_MMU_PAGE_SIZE;
	faults_before = k_mem_num_pagefaults_get();

	huge_evictable_function();

	free_pages_after = k_mem_free_get() / CONFIG_MMU_PAGE_SIZE;
	faults_after = k_mem_num_pagefaults_get();
	printk("free memory pages: from %zd to %zd, %ld page faults\n",
	       free_pages_before, free_pages_after, faults_after - faults_before);

	printk("Done.\n");
	return 0;
}
