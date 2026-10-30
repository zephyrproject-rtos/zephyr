/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright The Zephyr Project Contributors
 */

#ifndef ZEPHYR_INCLUDE_ARCH_COMMON_INIT_H_
#define ZEPHYR_INCLUDE_ARCH_COMMON_INIT_H_

#include <zephyr/toolchain.h>
#include <stddef.h>

FUNC_NORETURN void z_cstart(void);

/**
 * @brief equivalent of memset() for early boot usage
 *
 * Architectures that can't safely use the regular (optimized) memset very
 * early during boot because e.g. hardware isn't yet sufficiently initialized
 * may override this with their own safe implementation.
 *
 * @param dst Destination buffer
 * @param c value to fill with, usually 0x00
 * @param n number of bytes to fill
 */
void arch_early_memset(void *dst, int c, size_t n);

/**
 * @brief equivalent of memcpy() for early boot usage
 *
 * Architectures that can't safely use the regular (optimized) memcpy very
 * early during boot because e.g. hardware isn't yet sufficiently initialized
 * may override this with their own safe implementation.
 *
 * @param dst Destination buffer
 * @param src Source buffer
 * @param n number of bytes to copy
 */
void arch_early_memcpy(void *dst, const void *src, size_t n);

/**
 * @brief equivalent of memset() for late relocation usage
 *
 * Relocate code and/or data segments and flush the cache, to support relocation later in the boot
 * stage, such as when a memory controller got initialized.
 *
 * @param dst Destination buffer
 * @param c value to fill with, usually 0x00
 * @param n number of bytes to fill
 */
void arch_late_memset(void *dst, int c, size_t n);

/**
 * @brief equivalent of memcpy() for late relocation usage
 *
 * Relocate code and/or data segments and flush the cache, to support relocation later in the boot
 * stage, such as when a memory controller got initialized.
 *
 * @param dst Destination buffer
 * @param src Source buffer
 * @param n number of bytes to copy
 */
void arch_late_memcpy(void *dst, const void *src, size_t n);

#ifdef CONFIG_LINKER_USE_BOOT_SECTION
/**
 * @brief Clear BSS within the boot region
 *
 * This routine clears the BSS within the boot region.
 * This is separate from arch_bss_zero() as boot region may
 * contain symbols required for the boot process before
 * paging is initialized.
 */
void arch_bss_zero_boot(void);
#else
static inline void arch_bss_zero_boot(void)
{
	/* Do nothing */
}
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */

/**
 * @brief Clear BSS
 *
 * This routine clears the BSS region, so all bytes are 0.
 */
void arch_bss_zero(void);

#endif /* ZEPHYR_INCLUDE_ARCH_COMMON_INIT_H_ */
