/*
 * Copyright (c) 2016 Wind River Systems, Inc.
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

#ifndef _offsets_short__h_
#define _offsets_short__h_

#include <offsets.h>
#include <offsets_short_arch.h>

/* kernel */

/* main */

#define _kernel_offset_to_nested \
	(___kernel_t_nested_OFFSET)

#define _kernel_offset_to_irq_stack \
	(___kernel_t_irq_stack_OFFSET)

#define _kernel_offset_to_current \
	(___kernel_t_current_OFFSET)

#define _kernel_offset_to_idle \
	(___kernel_t_idle_OFFSET)

#define _kernel_offset_to_current_fp \
	(___kernel_t_current_fp_OFFSET)

/* end - kernel */

/* threads */

/* main */

#define _thread_offset_to_callee_saved \
	(___thread_t_callee_saved_OFFSET)

/* base */

#define _thread_offset_to_flags \
	(___thread_t_base_OFFSET + ___thread_base_t_flags_OFFSET)

#define _thread_offset_to_prio \
	(___thread_t_base_OFFSET + ___thread_base_t_prio_OFFSET)

#define _thread_offset_to_sched_locked \
	(___thread_t_base_OFFSET + ___thread_base_t_sched_locked_OFFSET)

#define _thread_offset_to_esf \
	(___thread_t_arch_OFFSET + ___thread_arch_t_esf_OFFSET)


/* end - threads */

#endif /* _offsets_short__h_ */
