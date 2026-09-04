/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TRACING_CTF_SYSCALL_H_
#define ZEPHYR_TRACING_CTF_SYSCALL_H_

#include <zephyr/types.h>

void sys_trace_syscall_enter(uint32_t id, const char *name);
void sys_trace_syscall_exit(uint32_t id);

#define sys_port_trace_syscall_enter(id, name, ...) sys_trace_syscall_enter(id, #name)
#define sys_port_trace_syscall_exit(id, name, ...)  sys_trace_syscall_exit(id)

#endif /* ZEPHYR_TRACING_CTF_SYSCALL_H_ */
