/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright The Zephyr Project Contributors
 */

#ifndef ZEPHYR_ARCH_COMMON_INIT_H_
#define ZEPHYR_ARCH_COMMON_INIT_H_

FUNC_NORETURN void z_cstart(void);

/* Early boot functions */
void arch_early_memset(void *dst, int c, size_t n);
void arch_early_memcpy(void *dst, const void *src, size_t n);

void arch_bss_zero(void);

#ifdef CONFIG_LINKER_USE_BOOT_SECTION
void arch_bss_zero_boot(void);
#else
static inline void arch_bss_zero_boot(void)
{
	/* Do nothing */
}
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */

#ifdef CONFIG_LINKER_USE_PINNED_SECTION
void arch_bss_zero_pinned(void);
#else
static inline void arch_bss_zero_pinned(void)
{
	/* Do nothing */
}
#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

#endif /* ZEPHYR_ARCH_COMMON_INIT_H_ */
