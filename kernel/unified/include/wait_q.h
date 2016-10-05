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

#include <nano_private.h>

#ifdef CONFIG_KERNEL_V2
#include <misc/dlist.h>
#include <sched.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NANO_TIMEOUTS)
static inline void _timeout_remove_tcs_from_wait_q(struct tcs *tcs)
{
	_unpend_thread(tcs);
	tcs->timeout.wait_q = NULL;
}
#include <timeout_q.h>

#elif defined(CONFIG_NANO_TIMERS)
#include <timeout_q.h>
	#define _timeout_tcs_init(tcs) do { } while ((0))
	#define _abort_thread_timeout(tcs) do { } while ((0))

	#define _add_thread_timeout(thread, pq, ticks) do { } while (0)
#else
	#define _timeout_tcs_init(tcs) do { } while ((0))
	#define _abort_thread_timeout(tcs) do { } while ((0))
	#define _timeout_get_next_expiry() (K_FOREVER)

	#define _add_thread_timeout(thread, pq, ticks) do { } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _kernel_nanokernel_include_wait_q__h_ */
