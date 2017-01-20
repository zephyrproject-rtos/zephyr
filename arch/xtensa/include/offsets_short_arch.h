/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
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

#define _kernel_offset_to_flags \
	(___kernel_t_arch_OFFSET + ___kernel_arch_t_flags_OFFSET)

/* end - kernel */

/* threads */

#define _thread_offset_to_sp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_topOfStack_OFFSET)

#define _thread_offset_to_retval \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_retval_OFFSET)

#define _thread_offset_to_coopCoprocReg \
	(___thread_t_arch_OFFSET + ___thread_arch_t_coopCoprocReg_OFFSET)

#define _thread_offset_to_preempCoprocReg \
	(___thread_t_arch_OFFSET + ___thread_arch_t_preempCoprocReg_OFFSET)

/* end - threads */

#endif /* _offsets_short_arch__h_ */
