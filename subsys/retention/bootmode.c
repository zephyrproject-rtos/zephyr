/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/retention/retention.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bootmode, CONFIG_RETENTION_LOG_LEVEL);

static const struct device *boot_mode_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_boot_mode));

int bootmode_check(uint8_t boot_mode)
{
	int rc;

	rc = retention_is_valid(boot_mode_dev);

	if (rc == 1) {
		uint8_t stored_mode;

		rc = retention_read(boot_mode_dev, 0, &stored_mode, sizeof(stored_mode));

		/* Only check if modes match if there was no error, otherwise return the error */
		if (rc == 0) {
			if (stored_mode == boot_mode) {
				rc = 1;
			}
		}
	}

	return rc;
}

int bootmode_set(uint8_t boot_mode)
{
	return retention_write(boot_mode_dev, 0, &boot_mode, sizeof(boot_mode));
}

int bootmode_clear(void)
{
	return retention_clear(boot_mode_dev);
}
