/*
 * Copyright (c) 2025 Navimatix GmbH.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/uart/cdc_acm.h>
#include <zephyr/drivers/usb/usb_dc.h>
#include <zephyr/init.h>
#include <zephyr/usb/class/usb_cdc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(reset, CONFIG_SOC_LOG_LEVEL);

#define TEENSY_RESET_BAUDRATE 134
#define TEENSY_RESET_COMMAND  "bkpt #251"
#define TEENSY_VID 0x16c0
#define TEENSY_PID 0x0483

static void teensy_reset(const struct device *dev, uint32_t rate)
{
	if (rate != TEENSY_RESET_BAUDRATE) {
		return;
	}

	/* The programmer set the baud rate to 134 baud.  Reset into the
	 * bootloader.
	 */
	__ASM volatile(TEENSY_RESET_COMMAND);
}

void board_late_init_hook(void)
{
	const struct device *dev = device_get_binding(CONFIG_BOOTLOADER_TEENSY_DEVICE_NAME);

	if (dev == NULL) {
		LOG_ERR("Could not find reset signal device with name %s",
			CONFIG_BOOTLOADER_TEENSY_DEVICE_NAME);
		return;
	}

	if ((CONFIG_USB_DEVICE_VID != TEENSY_VID) || (CONFIG_USB_DEVICE_PID != TEENSY_PID)) {
		LOG_ERR("Incorrect USB VID or PID. CONFIG_USB_DEVICE_VID should be 0x%x and "
			"CONFIG_USB_DEVICE_PID should be 0x%x.",
			TEENSY_VID, TEENSY_PID);
		return;
	}

	cdc_acm_dte_rate_callback_set(dev, teensy_reset);
}
