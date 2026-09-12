/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_M68K_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_M68K_INCLUDE_KERNEL_ARCH_FUNC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

void z_m68k_fatal_error(unsigned int reason, const struct arch_esf *esf);

static ALWAYS_INLINE void arch_kernel_init(void)
{
}

static ALWAYS_INLINE void arch_switch(void *switch_to, void **switched_from)
{
	extern void z_m68k_switch(struct k_thread *new,
				  struct k_thread *old);
	struct k_thread *new = switch_to;
	struct k_thread *old =
		CONTAINER_OF(switched_from, struct k_thread, switch_handle);

	z_m68k_switch(new, old);
}

static inline bool arch_is_in_isr(void)
{
	return !!_current_cpu->nested;
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_M68K_INCLUDE_KERNEL_ARCH_FUNC_H_ */
