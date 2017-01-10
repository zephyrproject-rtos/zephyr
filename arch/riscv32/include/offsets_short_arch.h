/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
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

#define _thread_offset_to_sp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_sp_OFFSET)

#define _thread_offset_to_s0 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s0_OFFSET)

#define _thread_offset_to_s1 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s1_OFFSET)

#define _thread_offset_to_s2 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s2_OFFSET)

#define _thread_offset_to_s3 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s3_OFFSET)

#define _thread_offset_to_s4 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s4_OFFSET)

#define _thread_offset_to_s5 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s5_OFFSET)

#define _thread_offset_to_s6 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s6_OFFSET)

#define _thread_offset_to_s7 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s7_OFFSET)

#define _thread_offset_to_s8 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s8_OFFSET)

#define _thread_offset_to_s9 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s9_OFFSET)

#define _thread_offset_to_s10 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s10_OFFSET)

#define _thread_offset_to_s11 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s11_OFFSET)

#define _thread_offset_to_swap_return_value \
	(___thread_t_arch_OFFSET + ___thread_arch_t_swap_return_value_OFFSET)

/* end - threads */

#endif /* _offsets_short_arch__h_ */
