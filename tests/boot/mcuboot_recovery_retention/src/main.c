/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/retention/bootmode.h>
#include <zephyr/kernel.h>
#include <stdio.h>

void main(void)
{
	printf("Waiting...\n");
	k_sleep(K_SECONDS(1));

	int rc = bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);

	if (rc == 0) {
		sys_reboot(SYS_REBOOT_WARM);
	} else {
		printf("Error, failed to set boot mode: %d\n", rc);
	}
}
