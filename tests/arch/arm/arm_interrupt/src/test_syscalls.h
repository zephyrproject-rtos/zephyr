/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEST_SYSCALLS_H_
#define _TEST_SYSCALLS_H_
#include <zephyr/kernel.h>

__syscall void test_arm_user_interrupt_syscall(void);

#include <syscalls/test_syscalls.h>

#endif /* _TEST_SYSCALLS_H_ */
