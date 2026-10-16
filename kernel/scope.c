/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Compile-time enforcement of the kernel feature scope selected with
 * CONFIG_KERNEL_SCOPE_*. The excluded features depend on the scope in
 * Kconfig, which stops a prompt or a default from enabling them; a select
 * bypasses dependencies, and the assertions below reject such a build.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_KERNEL_SCOPE_SMP) || !IS_ENABLED(CONFIG_SMP),
	     "SMP is outside the selected kernel scope");
BUILD_ASSERT(IS_ENABLED(CONFIG_KERNEL_SCOPE_SMP) || (CONFIG_MP_MAX_NUM_CPUS == 1),
	     "more than one CPU is outside the selected kernel scope");
BUILD_ASSERT(IS_ENABLED(CONFIG_KERNEL_SCOPE_USERSPACE) || !IS_ENABLED(CONFIG_USERSPACE),
	     "user mode threads are outside the selected kernel scope");
BUILD_ASSERT(IS_ENABLED(CONFIG_KERNEL_SCOPE_MMU) || !IS_ENABLED(CONFIG_MMU),
	     "MMU-based virtual memory is outside the selected kernel scope");
BUILD_ASSERT(IS_ENABLED(CONFIG_KERNEL_SCOPE_DYNAMIC_OBJECTS) || !IS_ENABLED(CONFIG_DYNAMIC_OBJECTS),
	     "run-time kernel object allocation is outside the selected kernel scope");
