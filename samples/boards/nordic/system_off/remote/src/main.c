/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>
#include <hal/nrf_memconf.h>

int main(void)
{
	int rc;
	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	printf("%s system off demo. Ready for system off.\n", CONFIG_BOARD);

	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	if (rc < 0) {
		printf("Could not suspend console (%d)\n", rc);
		return 0;
	}

	/* Turn off retention of the local radio domain RAM. */
	nrf_memconf_ramblock_ret_mask_enable_set(NRF_MEMCONF, 0, RAMBLOCK_RET_MASK, false);
	nrf_memconf_ramblock_ret_mask_enable_set(NRF_MEMCONF, 1, RAMBLOCK_RET_MASK, false);

	sys_poweroff();

	return 0;
}
