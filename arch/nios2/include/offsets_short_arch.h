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

#define _thread_offset_to_r16 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r16_OFFSET)

#define _thread_offset_to_r17 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r17_OFFSET)

#define _thread_offset_to_r18 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r18_OFFSET)

#define _thread_offset_to_r19 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r19_OFFSET)

#define _thread_offset_to_r20 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r20_OFFSET)

#define _thread_offset_to_r21 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r21_OFFSET)

#define _thread_offset_to_r22 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r22_OFFSET)

#define _thread_offset_to_r23 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r23_OFFSET)

#define _thread_offset_to_r28 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r28_OFFSET)

#define _thread_offset_to_ra \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_ra_OFFSET)

#define _thread_offset_to_sp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_sp_OFFSET)

#define _thread_offset_to_key \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_key_OFFSET)

#define _thread_offset_to_retval \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_retval_OFFSET)

/* end - threads */

#endif /* _offsets_short_arch__h_ */
