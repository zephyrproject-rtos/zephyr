/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Generic applet body used by most of the applet tests. Everything here must
 * stay callable from an unprivileged thread, so it only uses syscalls and
 * memory the test explicitly grants through a partition.
 */

#include <zephyr/kernel.h>
#include <zephyr/llext/symbol.h>

#include "applet_shared.h"

/**
 * @param arg  One of the APPLET_TEST_CMD_* values.
 */
void worker_main(void *arg)
{
	uintptr_t cmd = (uintptr_t)arg;

	if (cmd == APPLET_TEST_CMD_SPIN) {
		while (true) {
			k_sleep(K_MSEC(APPLET_TEST_SPIN_MS));
		}
	} else if (cmd == APPLET_TEST_CMD_SLEEP) {
		k_sleep(K_MSEC(APPLET_TEST_SLEEP_MS));
	}
}
LL_EXTENSION_SYMBOL(worker_main);

/**
 * Entry point under the name applet_spawn() looks for by default.
 *
 * @param arg  One of the APPLET_TEST_CMD_* values.
 */
void applet_main(void *arg)
{
	worker_main(arg);
}
LL_EXTENSION_SYMBOL(applet_main);

/**
 * Reads the whole shared buffer and reports success in its own result slot.
 * Under CONFIG_USERSPACE the read only succeeds if the buffer's partition is
 * in this thread's memory domain, which is what makes this a domain test.
 *
 * @param arg  Pointer to this thread's struct applet_test_slot.
 */
void shared_mem_main(void *arg)
{
	struct applet_test_slot *slot = arg;
	struct applet_test_shared *shared = slot->shared;
	uint32_t result = APPLET_TEST_MAGIC;

	for (unsigned int i = 0; i < APPLET_TEST_MAX_THREADS; i++) {
		if (shared->pattern[i] != APPLET_TEST_PATTERN) {
			result = 0U;
		}
	}

	shared->result[slot->index] = result;
}
LL_EXTENSION_SYMBOL(shared_mem_main);
