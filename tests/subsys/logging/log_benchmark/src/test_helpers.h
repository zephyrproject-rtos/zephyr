/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SRC_TEST_HELPERS_H__
#define SRC_TEST_HELPERS_H__

#include <zephyr/kernel.h>

__syscall void test_helpers_log_setup(void);
__syscall int test_helpers_cycle_get(void);
__syscall bool test_helpers_log_dropped_pending(void);

#include <syscalls/test_helpers.h>

#endif /* SRC_TEST_HELPERS_H__ */
