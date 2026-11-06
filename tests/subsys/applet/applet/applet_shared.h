/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_SUBSYS_APPLET_APPLET_SHARED_H_
#define ZEPHYR_TESTS_SUBSYS_APPLET_APPLET_SHARED_H_

#include <stdint.h>

/* Upper bound on the number of threads any single applet in this suite uses. */
#define APPLET_TEST_MAX_THREADS 6

/* Commands understood by worker_main(), passed as the thread argument. */
#define APPLET_TEST_CMD_QUICK 0U
#define APPLET_TEST_CMD_SPIN  1U
#define APPLET_TEST_CMD_SLEEP 2U

#define APPLET_TEST_SPIN_MS  5
#define APPLET_TEST_SLEEP_MS 50

/* Written by the test before the applet runs, checked by shared_mem_main(). */
#define APPLET_TEST_PATTERN 0xA5A5A5A5U
/* Written back by shared_mem_main() once it has read the whole buffer. */
#define APPLET_TEST_MAGIC   0x600DF00DU

struct applet_test_shared;

/*
 * Per-thread argument. It lives inside the shared buffer so that an
 * unprivileged applet thread can dereference it: the only memory such a
 * thread is granted is the partition the buffer sits in.
 */
struct applet_test_slot {
	struct applet_test_shared *shared;
	uint32_t index;
};

struct applet_test_shared {
	uint32_t pattern[APPLET_TEST_MAX_THREADS];
	volatile uint32_t result[APPLET_TEST_MAX_THREADS];
	struct applet_test_slot slot[APPLET_TEST_MAX_THREADS];
};

#endif /* ZEPHYR_TESTS_SUBSYS_APPLET_APPLET_SHARED_H_ */
