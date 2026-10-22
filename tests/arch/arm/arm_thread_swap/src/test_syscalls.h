/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEST_SYSCALLS_H_
#define _TEST_SYSCALLS_H_
#include <zephyr/kernel.h>

__syscall void test_arm_user_syscall(void);
__syscall void test_arm_cpu_write_reg(void);

#include <zephyr/syscalls/test_syscalls.h>

#endif /* _TEST_SYSCALLS_H_ */
