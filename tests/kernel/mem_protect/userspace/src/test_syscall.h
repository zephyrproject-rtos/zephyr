/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef USERSPACE_TEST_SYSCALL_H
#define USERSPACE_TEST_SYSCALL_H

__syscall void missing_syscall(void);
__syscall void check_syscall_context(void);

#include <syscalls/test_syscall.h>

#endif
