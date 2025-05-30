/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RX_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_RX_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

/* kernel */
#define KERNEL_OFFSET(field) _kernel_offset_to_##field

#define _kernel_offset_to_flags (___kernel_t_arch_OFFSET + ___kernel_arch_t_flags_OFFSET)

/* end - kernel */

/* threads */
#define THREAD_OFFSET(field) _thread_offset_to_##field

#define _thread_offset_to_sp (___thread_t_callee_saved_OFFSET + ___callee_saved_t_topOfStack_OFFSET)

#define _thread_offset_to_retval (___thread_t_callee_saved_OFFSET + ___callee_saved_t_retval_OFFSET)

#define _thread_offset_to_coopCoprocReg                                                            \
	(___thread_t_arch_OFFSET + ___thread_arch_t_coopCoprocReg_OFFSET)

#define _thread_offset_to_preempCoprocReg                                                          \
	(___thread_t_arch_OFFSET + ___thread_arch_t_preempCoprocReg_OFFSET)

#define _thread_offset_to_cpStack                                                                  \
	(_thread_offset_to_preempCoprocReg + __tPreempCoprocReg_cpStack_OFFSET)

#define _thread_offset_to_cpEnable (_thread_offset_to_cpStack + XT_CPENABLE)

/* end - threads */

#endif /* ZEPHYR_ARCH_RX_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
