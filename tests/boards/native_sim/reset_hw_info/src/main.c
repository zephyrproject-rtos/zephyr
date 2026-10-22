/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/reboot.h>
#include <nsi_main.h>

int main(void)
{
	uint32_t cause;
	int err;

	err = hwinfo_get_reset_cause(&cause);
	if (err != 0) {
		posix_print_error_and_exit("hwinfo_get_reset_cause() failed %i\n", err);
	}

	if (cause == RESET_POR) {
		printf("This seems like the first start => Resetting\n");
		sys_reboot(SYS_REBOOT_WARM);
	} else if (cause == RESET_SOFTWARE) {
		printf("Booted after SOFTWARE reset => we are done\n");
	}
	nsi_exit(0);
}
