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

__syscall int syscall_arg64(uint64_t arg);

__syscall uint64_t syscall_arg64_big(uint32_t arg1, uint32_t arg2, uint64_t arg3,
				  uint32_t arg4, uint32_t arg5, uint64_t arg6);

__syscall bool syscall_context(void);

__syscall uint32_t more_args(uint32_t arg1, uint32_t arg2, uint32_t arg3,
			     uint32_t arg4, uint32_t arg5, uint32_t arg6,
			     uint32_t arg7);

#include <syscalls/test_syscalls.h>

#endif /* _TEST_SYSCALLS_H_ */
