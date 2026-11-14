/*
 * Copyright (c) 2026, Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/sys_heap.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memref_sys_heap, LOG_LEVEL_DBG);

#include "memref_testcases.h"

#define HEAP_SIZE 4096
static uint8_t heap_mem[HEAP_SIZE];
static struct sys_heap heap;

static int sys_heap_backend_setup(void)
{
	sys_heap_init(&heap, heap_mem, HEAP_SIZE);

	return 0;
}
SYS_INIT(sys_heap_backend_setup, APPLICATION, 0);

static void *sys_heap_alloc_wrapper(void *ctx, size_t size)
{
	return sys_heap_alloc((struct sys_heap *)ctx, size);
}

static void sys_heap_free_wrapper(void *ctx, void *mem)
{
	sys_heap_free((struct sys_heap *)ctx, mem);
}

MEMREF_BACKEND_DEFINE(sys_heap_backend, sys_heap_alloc_wrapper, sys_heap_free_wrapper, &heap);

static void *sys_heap_backend_alloc(void)
{
	return memref_alloc(&sys_heap_backend, 64);
}

#ifdef CONFIG_MEMREF_DESTROY_CB
static void *sys_heap_backend_alloc_cb(memref_destroy_cb_t destroy)
{
	return memref_alloc_with_destroy(&sys_heap_backend, 64, destroy);
}
#endif

MEMREF_VARLEN_TESTCASES(memref_sys_heap, &sys_heap_backend, sys_heap_backend_alloc,
			sys_heap_backend_alloc_cb);
