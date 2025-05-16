/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_heap.h>
#include <libmctp.h>

static uint8_t MCTP_MEM[CONFIG_MCTP_HEAP_SIZE];

static struct {
	struct k_spinlock lock;
	struct sys_heap heap;
} mctp_heap;

static void *mctp_heap_alloc(size_t bytes)
{
	k_spinlock_key_t key = k_spin_lock(&mctp_heap.lock);

	void *ptr = sys_heap_alloc(&mctp_heap.heap, bytes);

	k_spin_unlock(&mctp_heap.lock, key);

	return ptr;
}

static void mctp_heap_free(void *ptr)
{
	k_spinlock_key_t key = k_spin_lock(&mctp_heap.lock);

	sys_heap_free(&mctp_heap.heap, ptr);

	k_spin_unlock(&mctp_heap.lock, key);
}

static void *mctp_heap_realloc(void *ptr, size_t bytes)
{
	k_spinlock_key_t key = k_spin_lock(&mctp_heap.lock);

	void *new_ptr = sys_heap_realloc(&mctp_heap.heap, ptr, bytes);

	k_spin_unlock(&mctp_heap.lock, key);

	return new_ptr;
}


static int mctp_heap_init(void)
{
	sys_heap_init(&mctp_heap.heap, MCTP_MEM, sizeof(MCTP_MEM));
	mctp_set_alloc_ops(mctp_heap_alloc, mctp_heap_free, mctp_heap_realloc);
	return 0;
}

SYS_INIT_NAMED(mctp_memory, mctp_heap_init, POST_KERNEL, 0);
