/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/symbol.h>
#include <zephyr/kernel.h>

extern struct k_thread my_thread;
#define MY_THREAD_STACK_SIZE 1024
extern struct z_thread_stack_element my_thread_stack[];

extern struct k_sem my_sem;

#ifdef CONFIG_USERSPACE
#define MY_THREAD_PRIO 1
#define MY_THREAD_OPTIONS (K_USER | K_INHERIT_PERMS)
#else
#define MY_THREAD_PRIO 0
#define MY_THREAD_OPTIONS 0
#endif
