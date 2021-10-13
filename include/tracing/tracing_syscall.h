/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TRACING_SYSCALL_H_
#define ZEPHYR_INCLUDE_TRACING_SYSCALL_H_

#ifndef CONFIG_TRACING

#define sys_port_tracing_syscall_enter(func) do { } while (false)
#define sys_port_tracing_syscall_exit(func) do { } while (false)

#endif


#endif /* ZEPHYR_INCLUDE_TRACING_SYSCALL_H_ */
