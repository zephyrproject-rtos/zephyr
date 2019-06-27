/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_SPARC_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_SPARC_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

#define _esf_reg(reg)	\
	(__z_arch_esf_t_## reg ##_OFFSET)

#define _thread_offset_to_callee_saved_reg(reg)	\
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_## reg ##_OFFSET)

#define _switch_handle ___thread_t_switch_handle_OFFSET

#define _kernel_to_current _kernel_offset_to_current

#endif /* ZEPHYR_ARCH_SPARC_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
