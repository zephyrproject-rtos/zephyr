/*
 * Copyright (c) 2022 Jeppe Odgaard
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>

#define HEAP_SIZE	256

static char heap_mem[HEAP_SIZE];
static struct sys_heap heap;

static void print_sys_memory_stats(void);

int main(void)
{
	void *p;

	printk("System heap sample\n\n");

	sys_heap_init(&heap, heap_mem, HEAP_SIZE);
	print_sys_memory_stats();

	p = sys_heap_alloc(&heap, 150);
	print_sys_memory_stats();

	p = sys_heap_realloc(&heap, p, 100);
	print_sys_memory_stats();

	sys_heap_free(&heap, p);
	print_sys_memory_stats();
	return 0;
}

static void print_sys_memory_stats(void)
{
	struct sys_memory_stats stats;

	sys_heap_runtime_stats_get(&heap, &stats);

	printk("allocated %zu, free %zu, max allocated %zu, heap size %u\n",
		stats.allocated_bytes, stats.free_bytes,
		stats.max_allocated_bytes, HEAP_SIZE);
}
