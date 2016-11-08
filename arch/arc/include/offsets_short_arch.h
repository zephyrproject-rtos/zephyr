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

#ifndef _offsets_short_arch__h_
#define _offsets_short_arch__h_

#include <offsets.h>

/* kernel */

/* nothing for now */

/* end - kernel */

/* threads */

#define _thread_offset_to_intlock_key \
	(___thread_t_arch_OFFSET + ___thread_arch_t_intlock_key_OFFSET)

#define _thread_offset_to_relinquish_cause \
	(___thread_t_arch_OFFSET + ___thread_arch_t_relinquish_cause_OFFSET)

#define _thread_offset_to_return_value \
	(___thread_t_arch_OFFSET + ___thread_arch_t_return_value_OFFSET)

#define _thread_offset_to_stack_top \
	(___thread_t_arch_OFFSET + ___thread_arch_t_stack_top_OFFSET)

#define _thread_offset_to_sp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_sp_OFFSET)


/* end - threads */

#endif /* _offsets_short_arch__h_ */
