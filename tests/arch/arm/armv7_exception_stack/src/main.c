/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/cortex_a_r/exception_stack.h>

#if defined(Z_ARMV7_EXCEPTION_ENTRY_STACK_BYTES)
BUILD_ASSERT(CONFIG_ARMV7_EXCEPTION_STACK_SIZE >= Z_ARMV7_EXCEPTION_ENTRY_STACK_BYTES,
	     "application build: exception stack smaller than required entry frame");
#endif

int main(void)
{
	return 0;
}
