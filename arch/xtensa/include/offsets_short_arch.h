/*
 * Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_OFFSETS_SHORT_ARCH_H_

#define _thread_offset_to_flags \
	(___thread_t_arch_OFFSET + ___thread_arch_t_flags_OFFSET)

#ifdef CONFIG_USERSPACE
#define _thread_offset_to_psp \
	(___thread_t_arch_OFFSET + ___thread_arch_t_psp_OFFSET)

#define _thread_offset_to_ptables \
	(___thread_t_arch_OFFSET + ___thread_arch_t_ptables_OFFSET)
#endif /* CONFIG_USERSPACE */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
