/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <kernel_internal.h>
#if defined(CONFIG_LOG_RUNTIME_FILTERING)
#include <zephyr/logging/log_ctrl.h>
#endif

#ifdef CONFIG_MINIMAL_LIBC
#include <stdlib.h>
#else
#include <malloc.h>
#endif /* CONFIG_MINIMAL_LIBC */

static int cmd_app_heap(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_BOARD_NATIVE_POSIX) && __GLIBC__ == 2 && __GLIBC_MINOR__ < 33
	/* mallinfo() was deprecated in glibc 2.33 and removed in 2.34. */
	struct mallinfo mi = mallinfo();
#else
	struct mallinfo2 mi = mallinfo2();
#endif /* CONFIG_BOARD_NATIVE_POSIX && __GLIBC__ == 2 && __GLIBC_MINOR__ < 33 */

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Heap size: %d bytes", mi.arena);
	shell_print(sh, "  used: %d bytes", mi.uordblks);
	shell_print(sh, "  free: %d bytes", mi.fordblks);
	shell_print(sh, "  max used: %d bytes", mi.usmblks);
	shell_print(sh, "  free fastbin: %d bytes", mi.fsmblks);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_app,
	SHELL_CMD(heap, NULL, "app heap", cmd_app_heap),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(app, &sub_app, "application commands", NULL);
