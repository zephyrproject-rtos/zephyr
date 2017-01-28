/* wait queue for multiple threads on kernel objects */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _kernel_include_wait_q__h_
#define _kernel_include_wait_q__h_

#include <kernel_structs.h>
#include <misc/dlist.h>
#include <ksched.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS
#include <timeout_q.h>
#else
static ALWAYS_INLINE void _init_thread_timeout(struct _thread_base *thread_base)
{
	ARG_UNUSED(thread_base);
}

static ALWAYS_INLINE void
_add_thread_timeout(struct k_thread *thread, _wait_q_t *wait_q, int32_t timeout)
{
	ARG_UNUSED(thread);
	ARG_UNUSED(wait_q);
	ARG_UNUSED(timeout);
}

static ALWAYS_INLINE int _abort_thread_timeout(struct k_thread *thread)
{
	ARG_UNUSED(thread);

	return 0;
}
#define _get_next_timeout_expiry() (K_FOREVER)
#endif

#define _WAIT_Q_INIT(wait_q) SYS_DLIST_STATIC_INIT(wait_q)

#ifdef __cplusplus
}
#endif

#endif /* _kernel_include_wait_q__h_ */
