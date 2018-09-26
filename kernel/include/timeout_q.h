/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_TIMEOUT_Q_H_
#define ZEPHYR_KERNEL_INCLUDE_TIMEOUT_Q_H_

/**
 * @file
 * @brief timeout queue for threads on kernel objects
 */

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS

extern u64_t z_last_tick_announced;

void _init_timeout(struct _timeout *t, _timeout_func_t func);

void _add_timeout(struct k_thread *thread, struct _timeout *timeout,
                  s32_t timeout_in_ticks);

int _abort_timeout(struct _timeout *timeout);

void _init_thread_timeout(struct _thread_base *thread_base);

void _add_thread_timeout(struct k_thread *thread, s32_t timeout_in_ticks);

int _abort_thread_timeout(struct k_thread *thread);

s32_t _get_next_timeout_expiry(void);

#else

/* Stubs when !CONFIG_SYS_CLOCK_EXISTS */
#define _init_thread_timeout(t) do{}while(0)
#define _add_thread_timeout(th,to) do{}while(0 && (void*)to && (void*)th)
#define _abort_thread_timeout(t) (0)
#define _get_next_timeout_expiry() (K_FOREVER)

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_KERNEL_INCLUDE_TIMEOUT_Q_H_ */
