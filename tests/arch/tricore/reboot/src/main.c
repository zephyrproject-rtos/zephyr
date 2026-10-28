/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>

#define REBOOT_MAGIC 0x52454254U

#if defined(CONFIG_TEST_REBOOT_COLD)
#define REBOOT_TYPE     SYS_REBOOT_COLD
#define REBOOT_TYPE_STR "cold"
#else
#define REBOOT_TYPE     SYS_REBOOT_WARM
#define REBOOT_TYPE_STR "warm"
#endif

/* A marker in no-init RAM tells the second boot from the first */
static __noinit uint32_t reboot_marker;

int main(void)
{
	if (reboot_marker == REBOOT_MAGIC) {
		reboot_marker = 0U;
		printk("reboot: restarted after the " REBOOT_TYPE_STR " reset request\n");
		return 0;
	}

	reboot_marker = REBOOT_MAGIC;
	printk("reboot: requesting a " REBOOT_TYPE_STR " reset\n");
	/* Let the UART send the line out of its FIFO before the reset */
	k_msleep(10);
	sys_reboot(REBOOT_TYPE);

	return 0;
}
