/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEST_SYSCALLS_H_
#define _TEST_SYSCALLS_H_
#include <zephyr/kernel.h>

__syscall void blow_up_priv_stack(void);

#include <zephyr/syscalls/test_syscalls.h>

#endif /* _TEST_SYSCALLS_H_ */
