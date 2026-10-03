/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Validate CONFIG_OMIT_ZEPHYR_STDINT_FOR_APPS.
 *
 * When the option is enabled, application code gets the toolchain's native
 * integer type definitions. The correct printf format specifiers then
 * depend on the data model:
 *
 *   ILP32 (32-bit targets): int32_t is "long"      -> %ld
 *   LP64  (64-bit targets): int64_t is "long"      -> %ld
 *
 * Under Zephyr's enforced convention (the default), the mapping is:
 *
 *   int32_t is always "int"       -> %d
 *   int64_t is always "long long" -> %lld
 *
 * This test uses the format specifiers matching each convention and
 * must compile cleanly (no -Wformat warnings, which are errors with
 * -Werror) to pass.
 */

#include <zephyr/kernel.h>
#include <stdint.h>

int main(void)
{
	int32_t val32 = 32;
	int64_t val64 = 64;

#ifdef CONFIG_OMIT_ZEPHYR_STDINT_FOR_APPS
	/* Native toolchain types */
#if __SIZEOF_POINTER__ == 4
	/* ILP32: int32_t is long */
	printk("int32_t = %ld\n", val32);
	printk("int64_t = %lld\n", val64);
#else
	/* LP64: int64_t is long */
	printk("int32_t = %d\n", val32);
	printk("int64_t = %ld\n", val64);
#endif
#else
	/* Zephyr convention: int32_t = int, int64_t = long long */
	printk("int32_t = %d\n", val32);
	printk("int64_t = %lld\n", val64);
#endif

	return 0;
}
