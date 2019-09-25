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

__syscall int syscall_arg64(u64_t arg);

__syscall u64_t syscall_arg64_big(u32_t arg1, u32_t arg2, u64_t arg3,
				  u32_t arg4, u32_t arg5, u64_t arg6);

#include <syscalls/test_syscalls.h>

#endif /* _TEST_SYSCALLS_H_ */
