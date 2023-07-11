/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <malloc.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>

static int cmd_app_heap(const struct shell *sh, size_t argc, char **argv)
{
#ifdef CONFIG_BOARD_NATIVE_POSIX
#if defined(__GLIBC__)
#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 33
	/* mallinfo() was deprecated in glibc 2.33 and removed in 2.34. */
	struct mallinfo mi = mallinfo();
#else
	struct mallinfo2 mi = mallinfo2();
#endif /* __GLIBC__ == 2 && __GLIBC_MINOR__ < 33 */
#else  /* __GLIBC__ */
	struct mallinfo mi = mallinfo();
#endif /* __GLIBC__ */
#else  /* CONFIG_BOARD_NATIVE_POSIX */
	struct mallinfo mi = mallinfo();
#endif /* CONFIG_BOARD_NATIVE_POSIX */

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Heap size: %zu bytes", (size_t)mi.arena);
	shell_print(sh, "  used: %zu bytes", (size_t)mi.uordblks);
	shell_print(sh, "  free: %zu bytes", (size_t)mi.fordblks);
	shell_print(sh, "  max used: %zu bytes", (size_t)mi.usmblks);
	shell_print(sh, "  free fastbin: %zu bytes", (size_t)mi.fsmblks);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_app,
	SHELL_CMD(heap, NULL, "app heap", cmd_app_heap),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(app, &sub_app, "application commands", NULL);
