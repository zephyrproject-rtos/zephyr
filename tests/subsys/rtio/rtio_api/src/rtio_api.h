/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_RTIO_API_H_
#define ZEPHYR_TEST_RTIO_API_H_

#include <zephyr/kernel.h>

struct thread_info {
	k_tid_t tid;
	int executed;
	int priority;
	int cpu_id;
};

#endif /* ZEPHYR_TEST_RTIO_API_H_ */
