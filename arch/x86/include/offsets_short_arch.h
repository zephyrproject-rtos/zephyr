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

#define _kernel_offset_to_isf \
	(___kernel_t_arch_OFFSET + ___kernel_arch_t_isf_OFFSET)

/* end - kernel */

/* threads */

#define _thread_offset_to_excNestCount \
	(___thread_t_arch_OFFSET + ___thread_arch_t_excNestCount_OFFSET)

#define _thread_offset_to_esp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_esp_OFFSET)

#define _thread_offset_to_coopFloatReg \
	(___thread_t_arch_OFFSET + ___thread_arch_t_coopFloatReg_OFFSET)

#define _thread_offset_to_preempFloatReg \
	(___thread_t_arch_OFFSET + ___thread_arch_t_preempFloatReg_OFFSET)

/* end - threads */

#endif /* _offsets_short_arch__h_ */
