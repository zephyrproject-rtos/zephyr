/*
 * Copyright (c) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/drivers/firmware/tisci/tisci.h"
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ti_k3_ospi_boot_reset, CONFIG_SOC_LOG_LEVEL);

/* Generic 8D-8D-8D to 1S-1S-1S reset for any OSPI flash */
static int ti_k3_reset_flash_from_8d8d8d(const struct device *mspi_dev,
					 const struct mspi_dev_id *dev_id,
					 uint32_t reset_recovery_us)
{
	int rc;

	struct mspi_dev_cfg boot_8d_cfg = {
		.io_mode = MSPI_IO_MODE_OCTAL,
		.data_rate = MSPI_DATA_RATE_DUAL,
		.cmd_length = 2,
	};

	struct mspi_dev_cfg boot_1s_cfg = {
		.io_mode = MSPI_IO_MODE_SINGLE,
		.data_rate = MSPI_DATA_RATE_SINGLE,
		.cmd_length = 1,
	};

	struct mspi_xfer_packet reset_packet = {.dir = MSPI_TX};

	struct mspi_xfer reset_xfer = {
		.xfer_mode = MSPI_PIO,
		.packets = &reset_packet,
		.num_packet = 1,
		.cmd_length = 2,
	};

	rc = mspi_dev_config(mspi_dev, dev_id,
			     MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_CMD_LEN |
				     MSPI_DEVICE_CONFIG_DATA_RATE,
			     &boot_8d_cfg);
	if (rc < 0) {
		LOG_ERR("Failed to configure MSPI for 8D-8D-8D mode: %d", rc);
		return rc;
	}

	/* Reset enable (0x66) */
	reset_packet.cmd = 0x6666;
	rc = mspi_transceive(mspi_dev, dev_id, &reset_xfer);
	if (rc < 0) {
		LOG_ERR("Reset enable command failed: %d", rc);
		return rc;
	}

	/* Reset (0x99) */
	reset_packet.cmd = 0x9999;
	rc = mspi_transceive(mspi_dev, dev_id, &reset_xfer);
	if (rc < 0) {
		LOG_ERR("Reset command failed: %d", rc);
		return rc;
	}

	rc = mspi_dev_config(mspi_dev, dev_id,
			     MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_CMD_LEN |
				     MSPI_DEVICE_CONFIG_DATA_RATE,
			     &boot_1s_cfg);
	if (rc < 0) {
		LOG_ERR("Failed to configure MSPI for 1S-1S-1S mode: %d", rc);
		return rc;
	}

	if (reset_recovery_us > 0) {
		k_busy_wait(reset_recovery_us);
	}

	/* This releases the MSPI controller. */
	(void)mspi_get_channel_status(mspi_dev, 0);

	return 0;
}

/* Platform initialization for chosen flash devices */
static int ti_k3_ospi_flash_boot_reset_init(void)
{
	LOG_INF("TI K3 OSPI flash boot mode reset initialization");

	/* Get flash controller from chosen node */
#if DT_HAS_CHOSEN(zephyr_flash_controller)
	const struct device *flash_controller = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

	if (!device_is_ready(flash_controller)) {
		LOG_WRN("Flash controller not ready");
		return -ENODEV;
	}

	/* Loop through all children of the MSPI controller and apply 8D-8D-8D reset */
#define APPLY_RESET_TO_MSPI_CHILD(node_id)                                                         \
	do {                                                                                       \
		struct mspi_dev_id dev_id = {                                                      \
			.dev_idx = DT_REG_ADDR(node_id),                                           \
		};                                                                                 \
		LOG_INF("Attempting to reset flash from 8D-8D-8D mode for MSPI device at idx %d",  \
			dev_id.dev_idx);                                                           \
		uint32_t reset_recovery_us = DT_PROP_OR(node_id, t_reset_recovery, 0);             \
		int ret = ti_k3_reset_flash_from_8d8d8d(flash_controller, &dev_id,                 \
							reset_recovery_us);                        \
		if (ret < 0) {                                                                     \
			LOG_WRN("Failed to reset MSPI device at idx %d: %d", dev_id.dev_idx, ret); \
		} else {                                                                           \
			LOG_INF("Successfully reset MSPI device at idx %d", dev_id.dev_idx);       \
		}                                                                                  \
	} while (0)

	/* Apply reset to all children under the flash controller */
	DT_FOREACH_CHILD_SEP(DT_CHOSEN(zephyr_flash_controller), APPLY_RESET_TO_MSPI_CHILD, (;));

#undef APPLY_RESET_TO_MSPI_CHILD
#else
#warning "No flash controller chosen"
#endif /* DT_HAS_CHOSEN(zephyr_flash_controller) */

	return 0;
}

#define TI_K3_FLASH_RESET_INIT_PRIORITY UTIL_DEC(CONFIG_FLASH_INIT_PRIORITY)

SYS_INIT(ti_k3_ospi_flash_boot_reset_init, POST_KERNEL, TI_K3_FLASH_RESET_INIT_PRIORITY);
