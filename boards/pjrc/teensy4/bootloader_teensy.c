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

static void teensy_reset(const struct device *dev, uint32_t rate)
{
	if (rate != 134) {
		return;
	}

	/* The programmer set the baud rate to 134 baud.  Reset into the
	 * bootloader.
	 */
	__ASM volatile("bkpt #251");
}

static int teensy_init(void)
{
	const struct device *dev = device_get_binding(CONFIG_BOOTLOADER_TEENSY_DEVICE_NAME);

	if (dev == NULL) {
		return -ENODEV;
	}

	return cdc_acm_dte_rate_callback_set(dev, teensy_reset);
}

SYS_INIT(teensy_init, APPLICATION, 0);
