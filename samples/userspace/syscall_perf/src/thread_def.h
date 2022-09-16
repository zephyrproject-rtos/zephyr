/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef THREAD_DEF_H
#define THREAD_DEF_H

#include <zephyr/kernel.h>

#define THREAD_STACKSIZE	2048

struct k_thread user_thread;
K_THREAD_STACK_DEFINE(user_stack, THREAD_STACKSIZE);
void supervisor_thread_function(void *p1, void *p2, void *p3);

struct k_thread supervisor_thread;
K_THREAD_STACK_DEFINE(supervisor_stack, THREAD_STACKSIZE);
void user_thread_function(void *p1, void *p2, void *p3);

#endif /* THREAD_DEF_H */
