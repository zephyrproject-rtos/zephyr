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

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS

static inline void z_init_timeout(struct _timeout *t)
{
	sys_dnode_init(&t->node);
}

void z_add_timeout(struct _timeout *to, _timeout_func_t fn,
		   k_timeout_t expires);

int z_abort_timeout(struct _timeout *to);

static inline bool z_is_inactive_timeout(struct _timeout *t)
{
	return !sys_dnode_is_linked(&t->node);
}

static inline void z_init_thread_timeout(struct _thread_base *thread_base)
{
	z_init_timeout(&thread_base->timeout);
}

extern void z_thread_timeout(struct _timeout *to);

static inline void z_add_thread_timeout(struct k_thread *th,
					k_timeout_t expires)
{
	z_add_timeout(&th->base.timeout, z_thread_timeout, expires);
}

static inline int z_abort_thread_timeout(struct k_thread *thread)
{
	return z_abort_timeout(&thread->base.timeout);
}

k_ticks_t z_get_next_timeout_expiry(void);

void z_set_timeout_expiry(k_ticks_t ticks, bool idle);

k_ticks_t z_timeout_end(struct _timeout *timeout);
k_ticks_t z_timeout_remaining(struct _timeout *timeout);

#else

/* Stubs when !CONFIG_SYS_CLOCK_EXISTS */
#define z_init_thread_timeout(t) do {} while (false)
#define z_abort_thread_timeout(t) (0)
#define z_is_inactive_timeout(t) 0
#define z_get_next_timeout_expiry() (K_FOREVER_TICKS)
#define z_set_timeout_expiry(t, i) do {} while (false)

static inline void z_add_thread_timeout(struct k_thread *th,
					k_timeout_t expires)
{
	ARG_UNUSED(th);
	ARG_UNUSED(expires);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_KERNEL_INCLUDE_TIMEOUT_Q_H_ */
