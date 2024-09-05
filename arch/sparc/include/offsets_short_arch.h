/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_SPARC_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_SPARC_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <zephyr/offsets.h>

#define _thread_offset_to_y \
	(___thread_t_callee_saved_OFFSET + __struct_arch_csf_y_OFFSET)

#define _thread_offset_to_l0_and_l1 \
	(___thread_t_callee_saved_OFFSET + __struct_arch_csf_l0_and_l1_OFFSET)

#define _thread_offset_to_l2 \
	(___thread_t_callee_saved_OFFSET + __struct_arch_csf_l2_OFFSET)

#define _thread_offset_to_l4 \
	(___thread_t_callee_saved_OFFSET + __struct_arch_csf_l4_OFFSET)

#define _thread_offset_to_l6 \
	(___thread_t_callee_saved_OFFSET + __struct_arch_csf_l6_OFFSET)

#define _thread_offset_to_i0 \
	(___thread_t_callee_saved_OFFSET + __struct_arch_csf_i0_OFFSET)

#define _thread_offset_to_i2 \
	(___thread_t_callee_saved_OFFSET + __struct_arch_csf_i2_OFFSET)

#define _thread_offset_to_i4 \
	(___thread_t_callee_saved_OFFSET + __struct_arch_csf_i4_OFFSET)

#define _thread_offset_to_i6 \
	(___thread_t_callee_saved_OFFSET + __struct_arch_csf_i6_OFFSET)

#define _thread_offset_to_o6 \
	(___thread_t_callee_saved_OFFSET + __struct_arch_csf_o6_OFFSET)

#define _thread_offset_to_psr \
	(___thread_t_callee_saved_OFFSET + __struct_arch_csf_psr_OFFSET)

#endif /* ZEPHYR_ARCH_SPARC_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
