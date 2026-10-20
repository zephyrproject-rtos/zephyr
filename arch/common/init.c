/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright The Zephyr Project Contributors
 */

#include <stddef.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/linker/linker-defs.h>

/* LCOV_EXCL_START
 *
 * This code is called so early in the boot process that code coverage
 * doesn't work properly. In addition, not all arches call this code,
 * some like x86 do this with optimized assembly
 */

__boot_func
void __weak arch_early_memset(void *dst, int c, size_t n)
{
	(void) memset(dst, c, n);
}

__boot_func
void __weak arch_early_memcpy(void *dst, const void *src, size_t n)
{
	(void) memcpy(dst, src, n);
}

void __weak arch_late_memset(void *dst, int c, size_t n)
{
	(void) memset(dst, c, n);

	sys_cache_data_flush_range(dst, n);
}

void __weak arch_late_memcpy(void *dst, const void *src, size_t n)
{
	(void) memcpy(dst, src, n);

	sys_cache_data_flush_range(dst, n);
	sys_cache_instr_invd_range(dst, n);
}

__boot_func
void arch_bss_zero(void)
{
	if (IS_ENABLED(CONFIG_SKIP_BSS_CLEAR)) {
		return;
	}

	arch_early_memset(__bss_start, 0, __bss_end - __bss_start);
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

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <zephyr/cache.h>

extern void late_relocation(void);

int late_relocation_init(void)
{
	late_relocation();
	return 0;
}

#ifdef CONFIG_LATE_RELOCATION_EARLY
SYS_INIT(late_relocation_init, EARLY, CONFIG_LATE_RELOCATION_INIT_PRIORITY);

#elif CONFIG_LATE_RELOCATION_PRE_KERNEL_1
SYS_INIT(late_relocation_init, PRE_KERNEL_1, CONFIG_LATE_RELOCATION_INIT_PRIORITY);

#elif CONFIG_LATE_RELOCATION_PRE_KERNEL_2
SYS_INIT(late_relocation_init, PRE_KERNEL_2, CONFIG_LATE_RELOCATION_INIT_PRIORITY);

#elif CONFIG_LATE_RELOCATION_POST_KERNEL
SYS_INIT(late_relocation_init, POST_KERNEL, CONFIG_LATE_RELOCATION_INIT_PRIORITY);

#elif CONFIG_LATE_RELOCATION_APPLICATION
SYS_INIT(late_relocation_init, APPLICATION, CONFIG_LATE_RELOCATION_INIT_PRIORITY);

#elif CONFIG_LATE_RELOCATION_SMP
SYS_INIT(late_relocation_init, SMP, CONFIG_LATE_RELOCATION_INIT_PRIORITY);

#else
#error No code relocation init level selected
#endif

#endif	/* CONFIG_CODE_DATA_RELOCATION */

#ifdef CONFIG_LINKER_USE_BOOT_SECTION
__boot_func
void arch_bss_zero_boot(void)
{
	arch_early_memset(&lnkr_boot_bss_start, 0,
		       (uintptr_t)&lnkr_boot_bss_end
		       - (uintptr_t)&lnkr_boot_bss_start);
}
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */

/* LCOV_EXCL_STOP */
