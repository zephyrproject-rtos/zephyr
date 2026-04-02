/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file test per-thread malloc
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/sync_heap.h>
#include <stdlib.h>
#include <sys_malloc.h>

#define STACKSIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define HEAPSIZE  1024
#define PRIORITY  7
#ifdef CONFIG_USERSPACE
#define OPTIONS K_USER
#else
#define OPTIONS 0
#endif /* CONFIG_USERSPACE */

K_THREAD_STACK_DEFINE(thread_stack, STACKSIZE);
static struct k_thread thread;

static ZTEST_BMEM void *tls_buf;
static ZTEST_BMEM struct sys_sync_heap tls_heap;
static ZTEST_BMEM uint8_t tls_heap_mem[HEAPSIZE];

static void thread_entry(void *p1, void *p2, void *p3)
{
	char *temp_buf = NULL;

	tls_buf = malloc(100);

	if (tls_buf != NULL) {
		temp_buf = realloc(tls_buf, 200);

		if (temp_buf != NULL) {
			tls_buf = temp_buf;
		}

		free(tls_buf);
	}
}

#ifdef CONFIG_USERSPACE
static void enter_user_thread_entry(void *p1, void *p2, void *p3)
{
	int ret = 0;

	ret = k_thread_malloc_heap_assign(k_current_get(), &tls_heap);
	zassert_equal(ret, 0, "failed to assign heap");

	k_thread_user_mode_enter(thread_entry, p1, p2, p3);
}
#endif

/**
 *
 * @brief Test without assigned heap.
 *
 */
ZTEST(libc_tls_malloc, test_without_heap)
{
	int ret = 0;

	k_thread_create(&thread, thread_stack, K_THREAD_STACK_SIZEOF(thread_stack), thread_entry,
			NULL, NULL, NULL, PRIORITY, OPTIONS, K_NO_WAIT);
	ret = k_thread_join(&thread, K_FOREVER);
	zassert_equal(ret, 0, "failed to join thread");
	zassert_equal(tls_buf, NULL, "malloc did not return NULL without assigned heap");
}

/**
 *
 * @brief Test with assigned heap.
 *
 */
ZTEST(libc_tls_malloc, test_with_heap)
{
	int ret = 0;
	uint8_t *buf;

	ret = sys_sync_heap_init(&tls_heap, tls_heap_mem, sizeof(tls_heap_mem));
	zassert_equal(ret, 0, "failed to create heap");

	k_thread_create(&thread, thread_stack, K_THREAD_STACK_SIZEOF(thread_stack), thread_entry,
			NULL, NULL, NULL, PRIORITY, OPTIONS, K_FOREVER);
	ret = k_thread_malloc_heap_assign(&thread, &tls_heap);
	zassert_equal(ret, 0, "failed to assign heap");
	k_thread_start(&thread);
	ret = k_thread_join(&thread, K_FOREVER);
	zassert_equal(ret, 0, "failed to join thread");
	buf = (uint8_t *)tls_buf;
	zassert(buf >= tls_heap_mem && buf <= (tls_heap_mem + sizeof(tls_heap_mem)),
		"malloc did not return pointer within assigned heap");
}

/**
 *
 * @brief Test with assigned heap.
 *
 */
ZTEST(libc_tls_malloc, test_user_enter)
{
#ifdef CONFIG_USERSPACE
	int ret = 0;
	uint8_t *buf;

	ret = sys_sync_heap_init(&tls_heap, tls_heap_mem, sizeof(tls_heap_mem));
	zassert_equal(ret, 0, "failed to create heap");

	k_thread_create(&thread, thread_stack, K_THREAD_STACK_SIZEOF(thread_stack),
			enter_user_thread_entry, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
	ret = k_thread_join(&thread, K_FOREVER);
	zassert_equal(ret, 0, "failed to join thread");
	buf = (uint8_t *)tls_buf;
	zassert(buf >= tls_heap_mem && buf <= (tls_heap_mem + sizeof(tls_heap_mem)),
		"malloc did not return pointer within assigned heap");
#else
	ztest_test_skip();
#endif /* CONFIG_USERSPACE */
}

ZTEST_SUITE(libc_tls_malloc, NULL, NULL, NULL, NULL, NULL);
