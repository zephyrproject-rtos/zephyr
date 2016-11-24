/* wait queue for multiple fibers on nanokernel objects */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _kernel_nanokernel_include_wait_q__h_
#define _kernel_nanokernel_include_wait_q__h_

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

#ifdef __cplusplus
}
#endif

#endif /* _kernel_nanokernel_include_wait_q__h_ */
