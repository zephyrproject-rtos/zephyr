/*
 * Copyright (c) 2022 Jeppe Odgaard
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>

#define HEAP_SIZE	256

K_HEAP_DEFINE(my_kernel_heap, HEAP_SIZE);

static char heap_mem[HEAP_SIZE];
static struct sys_heap heap;

static void print_sys_memory_stats(struct sys_heap *);
static void print_static_heaps(void);
static void print_all_heaps(void);

int main(void)
{
	void *p;

	printk("System heap sample\n\n");

	sys_heap_init(&heap, heap_mem, HEAP_SIZE);
	print_sys_memory_stats(&heap);

	p = sys_heap_alloc(&heap, 150);
	print_sys_memory_stats(&heap);

	p = sys_heap_realloc(&heap, p, 100);
	print_sys_memory_stats(&heap);

	sys_heap_free(&heap, p);
	print_sys_memory_stats(&heap);

	/* Allocate from our custom k_heap to make the output
	 * more interesting and prevent compiler optimisations from
	 * discarding it.
	 */
	p = k_heap_alloc(&my_kernel_heap, 10, K_FOREVER);

	print_static_heaps();
	print_all_heaps();

	/* Cleanup memory */
	k_heap_free(&my_kernel_heap, p);
	return 0;
}

static void print_sys_memory_stats(struct sys_heap *hp)
{
	struct sys_memory_stats stats;

	sys_heap_runtime_stats_get(hp, &stats);

	printk("allocated %zu, free %zu, max allocated %zu, heap size %u\n",
		stats.allocated_bytes, stats.free_bytes,
		stats.max_allocated_bytes, HEAP_SIZE);
}

static void print_static_heaps(void)
{
	struct k_heap *ha;
	int n;

	n = k_heap_array_get(&ha);
	printk("%d static heap(s) allocated:\n", n);

	for (int i = 0; i < n; i++) {
		printk("\t%d - address %p ", i, &ha[i]);
		print_sys_memory_stats(&ha[i].heap);
	}
}

static void print_all_heaps(void)
{
	struct sys_heap **ha;
	int n;

	n = sys_heap_array_get(&ha);
	printk("%d heap(s) allocated (including static):\n", n);

	for (int i = 0; i < n; i++) {
		printk("\t%d - address %p ", i, ha[i]);
		print_sys_memory_stats(ha[i]);
	}
}
