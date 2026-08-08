/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 * The per-core application (app_main) runs in a dedicated worker thread rather
 * than on the main thread.  The DUT application builds several large
 * driver-config structs on the stack and its DS-RAM entry path is deep, so it
 * needs more stack than the default main thread; giving the worker its own
 * 8 KiB stack keeps the main stack at its default size.  app_main is provided
 * by the active target's source file (main_dut.c or main_companion.c).
 */
#define APP_THREAD_STACK_SIZE 8192
#define APP_THREAD_PRIORITY   5

void app_main(void);

K_THREAD_STACK_DEFINE(app_thread_stack, APP_THREAD_STACK_SIZE);
static struct k_thread app_thread;

static void app_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	app_main();
}

int main(void)
{
	k_thread_create(&app_thread, app_thread_stack, K_THREAD_STACK_SIZEOF(app_thread_stack),
			app_thread_entry, NULL, NULL, NULL, APP_THREAD_PRIORITY, 0, K_NO_WAIT);

	return 0;
}
