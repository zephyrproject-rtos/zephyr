/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_X86_INCLUDE_OFFSETS_SHORT_ARCH_H_

#ifdef CONFIG_X86_64
#include <intel64/offsets_short_arch.h>
#else
#include <ia32/offsets_short_arch.h>
#endif

#define _thread_offset_to_flags \
	(___thread_t_arch_OFFSET + ___thread_arch_t_flags_OFFSET)

#ifdef CONFIG_USERSPACE
#define _thread_offset_to_psp \
	(___thread_t_arch_OFFSET + ___thread_arch_t_psp_OFFSET)

#ifdef CONFIG_PROCESS_VM
#define _thread_offset_to_fork_esi \
	(___thread_t_arch_OFFSET + ___thread_arch_t_fork_esi_OFFSET)
#endif /* CONFIG_PROCESS_VM */

#ifdef CONFIG_SYSCALL_RETURN_HOOK
#define _thread_offset_to_sig_eax \
	(___thread_t_arch_OFFSET + ___thread_arch_t_sig_eax_OFFSET)
#endif

#define _thread_offset_to_ptables \
	(___thread_t_arch_OFFSET + ___thread_arch_t_ptables_OFFSET)
#endif /* CONFIG_USERSPACE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
