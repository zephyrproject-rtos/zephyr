/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/retention/retention.h>
#include <zephyr/retention/bootloader.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bootloader_config, CONFIG_RETENTION_LOG_LEVEL);

static const struct device *bootloader_config_dev =
					DEVICE_DT_GET(DT_CHOSEN(zephyr_bootloader_config));

int bootloader_load_config(struct mcuboot_configuration *config)
{
	int rc;

	rc = retention_is_valid(bootloader_config_dev);

	if (rc == 1 || rc == -ENOTSUP) {
		rc = retention_read(bootloader_config_dev, 0, (void *)config,
				    sizeof(struct mcuboot_configuration));

		if (rc == 0 && config->configuration_version != MCUBOOT_CONFIGURATION_VERSION_1) {
			/* Unknown configuration version */
			rc = -EINVAL;
		}
	}

	return rc;
}
