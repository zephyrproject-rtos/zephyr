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

#define _thread_offset_to_return_ps \
	(___thread_t_arch_OFFSET + ___thread_arch_t_return_ps_OFFSET)

#define _thread_offset_to_ptables \
	(___thread_t_arch_OFFSET + ___thread_arch_t_ptables_OFFSET)

#define _thread_offset_to_mem_domain \
	(___thread_t_mem_domain_info_OFFSET + ___mem_domain_info_t_mem_domain_OFFSET)

#define _k_mem_domain_offset_to_arch_reg_asid \
	(__k_mem_domain_t_arch_OFFSET + __arch_mem_domain_t_reg_asid_OFFSET)

#define _k_mem_domain_offset_to_arch_reg_ptevaddr \
	(__k_mem_domain_t_arch_OFFSET + __arch_mem_domain_t_reg_ptevaddr_OFFSET)

#define _k_mem_domain_offset_to_arch_reg_ptepin_as \
	(__k_mem_domain_t_arch_OFFSET + __arch_mem_domain_t_reg_ptepin_as_OFFSET)

#define _k_mem_domain_offset_to_arch_reg_ptepin_at \
	(__k_mem_domain_t_arch_OFFSET + __arch_mem_domain_t_reg_ptepin_at_OFFSET)

#define _k_mem_domain_offset_to_arch_reg_vecpin_as \
	(__k_mem_domain_t_arch_OFFSET + __arch_mem_domain_t_reg_vecpin_as_OFFSET)

#define _k_mem_domain_offset_to_arch_reg_vecpin_at \
	(__k_mem_domain_t_arch_OFFSET + __arch_mem_domain_t_reg_vecpin_at_OFFSET)

#endif /* CONFIG_USERSPACE */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
