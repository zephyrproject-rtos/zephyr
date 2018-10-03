/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEST_SYSCALLS_H_
#define _TEST_SYSCALLS_H_
#include <zephyr.h>

__syscall int string_alloc_copy(char *src);

__syscall int string_copy(char *src);

__syscall int to_copy(char *dest);

__syscall size_t string_nlen(char *src, size_t maxlen, int *err);

#include <syscalls/test_syscalls.h>

#endif /* _TEST_SYSCALLS_H_ */
