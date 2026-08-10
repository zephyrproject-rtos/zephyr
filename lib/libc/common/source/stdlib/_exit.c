/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/toolchain.h>

/*
 * Common _exit(): with process support, terminate the calling thread's
 * process; otherwise there is no process to end, so report and spin as the
 * per-libc stubs did. This strong definition overrides the weak picolibc and
 * newlib stubs; the minimal libc's non-weak stub is gated out at build time.
 */
FUNC_NORETURN void _exit(int status)
{
#ifdef CONFIG_PROCESS
	k_exit(status);
#else
	ARG_UNUSED(status);

	printk("exit\n");
#endif

	while (1) {
		Z_SPIN_DELAY(100);
	}
}
