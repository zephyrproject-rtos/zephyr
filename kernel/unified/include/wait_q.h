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
#define _init_thread_timeout(thread) do { } while ((0))
#define _nano_timeout_thread_init(thread) _init_thread_timeout(thread)
#define _add_thread_timeout(thread, wait_q, timeout) do { } while (0)
static inline int _abort_thread_timeout(struct k_thread *thread) { return 0; }
#define _get_next_timeout_expiry() (K_FOREVER)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _kernel_nanokernel_include_wait_q__h_ */
