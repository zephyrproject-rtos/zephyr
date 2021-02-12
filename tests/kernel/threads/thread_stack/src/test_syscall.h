/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef USERSPACE_TEST_SYSCALL_H
#define USERSPACE_TEST_SYSCALL_H

__syscall void stack_info_get(char **start_addr, size_t *size);
#ifdef CONFIG_USERSPACE
__syscall int check_perms(void *addr, size_t size, int write);
__syscall struct z_object *find_by_name(void *name);
#endif
#include <syscalls/test_syscall.h>

#endif
