/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RH850_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_RH850_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE void arch_kernel_init(void)
{
}

static ALWAYS_INLINE bool arch_is_in_isr(void)
{
	return arch_curr_cpu()->nested != 0U;
}

static ALWAYS_INLINE bool arch_is_in_nested_exception(const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	return arch_curr_cpu()->nested > 1U;
}

void z_rh850_fault(struct arch_esf *esf, uint32_t cause, uint32_t level);

#ifdef CONFIG_FXU
void z_rh850_fxu_disable(struct k_thread *thread);
#endif

static ALWAYS_INLINE void arch_switch(void *switch_to, void **switched_from)
{
	register void *r6 __asm__("r6") = switch_to;
	register void **r7 __asm__("r7") = switched_from;

	__asm__ volatile("trap " STRINGIFY(RH850_TRAP_SWITCH) : : "r"(r6), "r"(r7) : "memory");
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_RH850_INCLUDE_KERNEL_ARCH_FUNC_H_ */
