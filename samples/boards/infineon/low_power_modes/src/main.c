/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 * app_main (from main_dut.c) runs in a dedicated worker thread: its deep DS-RAM
 * entry path and large on-stack driver-config structs need more than the
 * default main stack.
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
