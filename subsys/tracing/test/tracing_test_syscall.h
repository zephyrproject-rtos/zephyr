/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TRACING_TEST_SYSCALL_H_
#define ZEPHYR_TRACING_TEST_SYSCALL_H_

#include <stdint.h>

void sys_trace_syscall_enter(uint32_t syscall_id, const char *syscall_name);
void sys_trace_syscall_exit(uint32_t syscall_id, const char *sycall_name);

#define sys_port_trace_syscall_enter(id, name, ...)	\
	sys_trace_syscall_enter(id, #name)

#define sys_port_trace_syscall_exit(id, name, ...)	\
	sys_trace_syscall_exit(id, #name)

#endif /* ZEPHYR_TRACING_TEST_SYSCALL_H_ */
