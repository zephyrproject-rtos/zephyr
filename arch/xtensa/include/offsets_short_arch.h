/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _offsets_short_arch__h_
#define _offsets_short_arch__h_

#include <offsets.h>

/* kernel */
#define KERNEL_OFFSET(field) _kernel_offset_to_##field

#define _kernel_offset_to_flags \
	(___kernel_t_arch_OFFSET + ___kernel_arch_t_flags_OFFSET)

/* end - kernel */

/* threads */
#define THREAD_OFFSET(field) _thread_offset_to_##field

#define _thread_offset_to_sp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_topOfStack_OFFSET)

#define _thread_offset_to_retval \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_retval_OFFSET)

#define _thread_offset_to_coopCoprocReg \
	(___thread_t_arch_OFFSET + ___thread_arch_t_coopCoprocReg_OFFSET)

#define _thread_offset_to_preempCoprocReg \
	(___thread_t_arch_OFFSET + ___thread_arch_t_preempCoprocReg_OFFSET)

#define _thread_offset_to_cpStack \
	(_thread_offset_to_preempCoprocReg + __tPreempCoprocReg_cpStack_OFFSET)

/* end - threads */

#endif /* _offsets_short_arch__h_ */
