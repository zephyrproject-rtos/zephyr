/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEST_SYSCALLS_H_
#define _TEST_SYSCALLS_H_
#include <zephyr.h>

__syscall void test_cpu_write_reg(void);

#include <syscalls/test_syscalls.h>

#endif /* _TEST_SYSCALLS_H_ */
