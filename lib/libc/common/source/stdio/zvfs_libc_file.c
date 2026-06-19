/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief The C library's FILE-object store.
 *
 * Raw, FILE-sized slots that the per-libc zvfs_libc_fdopen() hooks initialize
 * as a FILE bound to a ZVFS descriptor, and that fclose() returns. The store is
 * the C library's: it lives in the C library's memory partition and is claimed
 * with atomics, so a user-mode process opens and closes streams without a
 * kernel object in the way. Descriptors are the kernel's; the store never
 * learns what they name.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/zvfs_libc.h>

BUILD_ASSERT(ZVFS_LIBC_FILE_SIZE > 0, "ZVFS_LIBC_FILE_SIZE must be positive");
BUILD_ASSERT((ZVFS_LIBC_FILE_SIZE % ZVFS_LIBC_FILE_ALIGN) == 0,
	     "ZVFS_LIBC_FILE_SIZE must be a multiple of ZVFS_LIBC_FILE_ALIGN");

static Z_LIBC_DATA uint8_t store[ZVFS_OPEN_SIZE][ZVFS_LIBC_FILE_SIZE]
	__aligned(ZVFS_LIBC_FILE_ALIGN);
static Z_LIBC_DATA ATOMIC_DEFINE(used, ZVFS_OPEN_SIZE);

static inline bool store_contains(const void *fp)
{
	const uint8_t *p = fp;

	return (p >= store[0]) && (p < store[0] + sizeof(store));
}

void *zvfs_libc_file_alloc(int fd)
{
	ARG_UNUSED(fd);

	for (int i = 0; i < ZVFS_OPEN_SIZE; i++) {
		if (!atomic_test_and_set_bit(used, i)) {
			return store[i];
		}
	}

	return NULL;
}

void zvfs_libc_file_free(void *fp)
{
	if ((fp == NULL) || !store_contains(fp)) {
		return;
	}
	atomic_clear_bit(used, ((const uint8_t *)fp - store[0]) / ZVFS_LIBC_FILE_SIZE);
}
