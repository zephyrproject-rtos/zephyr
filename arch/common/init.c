/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright The Zephyr Project Contributors
 */

#include <stddef.h>
#include <string.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/linker/linker-defs.h>

/* LCOV_EXCL_START
 *
 * This code is called so early in the boot process that code coverage
 * doesn't work properly. In addition, not all arches call this code,
 * some like x86 do this with optimized assembly
 */

/**
 * @brief equivalent of memset() for early boot usage
 *
 * Architectures that can't safely use the regular (optimized) memset very
 * early during boot because e.g. hardware isn't yet sufficiently initialized
 * may override this with their own safe implementation.
 */
__boot_func
void __weak arch_early_memset(void *dst, int c, size_t n)
{
	(void) memset(dst, c, n);
}

/**
 * @brief equivalent of memcpy() for early boot usage
 *
 * Architectures that can't safely use the regular (optimized) memcpy very
 * early during boot because e.g. hardware isn't yet sufficiently initialized
 * may override this with their own safe implementation.
 */
__boot_func
void __weak arch_early_memcpy(void *dst, const void *src, size_t n)
{
	(void) memcpy(dst, src, n);
}

/**
 * @brief Clear BSS
 *
 * This routine clears the BSS region, so all bytes are 0.
 */
__boot_func
void arch_bss_zero(void)
{
	if (IS_ENABLED(CONFIG_SKIP_BSS_CLEAR)) {
		return;
	}

	arch_early_memset(__bss_start, 0, __bss_end - __bss_start);
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_ccm))
	arch_early_memset(&__ccm_bss_start, 0,
		       (uintptr_t) &__ccm_bss_end
		       - (uintptr_t) &__ccm_bss_start);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm))
	arch_early_memset(&__dtcm_bss_start, 0,
		       (uintptr_t) &__dtcm_bss_end
		       - (uintptr_t) &__dtcm_bss_start);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_ocm))
	arch_early_memset(&__ocm_bss_start, 0,
		       (uintptr_t) &__ocm_bss_end
		       - (uintptr_t) &__ocm_bss_start);
#endif
#ifdef CONFIG_CODE_DATA_RELOCATION
	extern void bss_zeroing_relocation(void);

	bss_zeroing_relocation();
#endif	/* CONFIG_CODE_DATA_RELOCATION */
#ifdef CONFIG_COVERAGE_GCOV
	arch_early_memset(&__gcov_bss_start, 0,
		       ((uintptr_t) &__gcov_bss_end - (uintptr_t) &__gcov_bss_start));
#endif /* CONFIG_COVERAGE_GCOV */
#ifdef CONFIG_NOCACHE_MEMORY
	arch_early_memset(&_nocache_ram_start, 0,
			(uintptr_t) &_nocache_ram_end - (uintptr_t) &_nocache_ram_start);
#endif
}

#ifdef CONFIG_LINKER_USE_BOOT_SECTION
/**
 * @brief Clear BSS within the boot region
 *
 * This routine clears the BSS within the boot region.
 * This is separate from arch_bss_zero() as boot region may
 * contain symbols required for the boot process before
 * paging is initialized.
 */
__boot_func
void arch_bss_zero_boot(void)
{
	arch_early_memset(&lnkr_boot_bss_start, 0,
		       (uintptr_t)&lnkr_boot_bss_end
		       - (uintptr_t)&lnkr_boot_bss_start);
}
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */

#ifdef CONFIG_LINKER_USE_PINNED_SECTION
/**
 * @brief Clear BSS within the pinned region
 *
 * This routine clears the BSS within the pinned region.
 * This is separate from arch_bss_zero() as pinned region may
 * contain symbols required for the boot process before
 * paging is initialized.
 */
#ifdef CONFIG_LINKER_USE_BOOT_SECTION
__boot_func
#else
__pinned_func
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */
void arch_bss_zero_pinned(void)
{
	arch_early_memset(&lnkr_pinned_bss_start, 0,
		       (uintptr_t)&lnkr_pinned_bss_end
		       - (uintptr_t)&lnkr_pinned_bss_start);
}
#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

/* LCOV_EXCL_STOP */
