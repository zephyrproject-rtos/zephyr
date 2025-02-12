/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_IA32_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_X86_INCLUDE_IA32_OFFSETS_SHORT_ARCH_H_

#include <zephyr/offsets.h>

/* kernel */

#define _kernel_offset_to_isf \
	(___kernel_t_arch_OFFSET + ___kernel_arch_t_isf_OFFSET)

/* end - kernel */

/* threads */

#define _thread_offset_to_excNestCount \
	(___thread_t_arch_OFFSET + ___thread_arch_t_excNestCount_OFFSET)

#define _thread_offset_to_esp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_esp_OFFSET)

#define _thread_offset_to_preempFloatReg \
	(___thread_t_arch_OFFSET + ___thread_arch_t_preempFloatReg_OFFSET)

/* end - threads */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_IA32_OFFSETS_SHORT_ARCH_H_ */
