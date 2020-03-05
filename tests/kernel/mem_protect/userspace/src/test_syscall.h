/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef USERSPACE_TEST_SYSCALL_H
#define USERSPACE_TEST_SYSCALL_H

__syscall void stack_info_get(char **start_addr, size_t *size);
__syscall int check_perms(void *addr, size_t size, int write);
__syscall void missing_syscall(void);
__syscall void check_syscall_context(void);

#include <syscalls/test_syscall.h>

#endif
