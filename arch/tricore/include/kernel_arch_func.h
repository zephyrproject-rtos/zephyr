/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_TRICORE_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_TRICORE_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

static ALWAYS_INLINE void arch_kernel_init(void)
{
	/* No per-arch kernel init: TriCore boot path (prep_c + trap
	 * vectors) finishes all arch setup before this hook runs.
	 */
}

static ALWAYS_INLINE bool arch_is_in_isr(void)
{
	return arch_curr_cpu()->nested != 0U;
}

static ALWAYS_INLINE void arch_switch(void *switch_to, void **switched_from)
{
	extern void z_tricore_switch(struct k_thread *to, struct k_thread *from);
	register struct k_thread *to __asm__("a4") = switch_to;
	register struct k_thread *from __asm__("a5") =
		CONTAINER_OF(switched_from, struct k_thread, switch_handle);

	/* Use syscall for context switch and restore */
	__asm("syscall " STRINGIFY(TRICORE_SYSCALL_SWITCH) ::"a"(to), "a"(from));
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_TRICORE_INCLUDE_KERNEL_ARCH_FUNC_H_ */
