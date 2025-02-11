/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/uart/cdc_acm.h>
#include <zephyr/drivers/usb/usb_dc.h>
#include <zephyr/init.h>
#include <zephyr/usb/class/usb_cdc.h>

/*
 * Magic value that causes the bootloader to stay in bootloader mode instead of
 * starting the application.
 */
#if CONFIG_BOOTLOADER_BOSSA_ADAFRUIT_UF2
#define DOUBLE_TAP_MAGIC 0xf01669ef
#elif CONFIG_BOOTLOADER_BOSSA_ARDUINO
#define DOUBLE_TAP_MAGIC 0x07738135
#else
#error Unsupported BOSSA bootloader variant
#endif

#if defined(CONFIG_BOOTLOADER_BOSSA_DEVICE_NAME)

static void bossa_reset(const struct device *dev, uint32_t rate)
{
	uint32_t *top;

	if (rate != 1200) {
		return;
	}

	/* The programmer set the baud rate to 1200 baud.  Reset into the
	 * bootloader.
	 */
	usb_dc_detach();

	top = (uint32_t *)(DT_REG_ADDR(DT_NODELABEL(sram0)) +
			   DT_REG_SIZE(DT_NODELABEL(sram0)));
	top[-1] = DOUBLE_TAP_MAGIC;

	NVIC_SystemReset();
}

static int bossa_init(void)
{
	const struct device *dev =
		device_get_binding(CONFIG_BOOTLOADER_BOSSA_DEVICE_NAME);

	if (dev == NULL) {
		return -ENODEV;
	}

	return cdc_acm_dte_rate_callback_set(dev, bossa_reset);
}

SYS_INIT(bossa_init, APPLICATION, 0);

#endif /* CONFIG_BOOTLOADER_BOSSA_DEVICE_NAME */
