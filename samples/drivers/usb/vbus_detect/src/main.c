/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB VBUS Detection Sample
 *
 * This sample demonstrates the USB VBUS detection driver. It initializes
 * the USB device controller and monitors VBUS state changes via GPIO.
 *
 * Connect a VBUS sense signal to the configured GPIO pin (PA9 on
 * disco_l475_iot1) to see VBUS ready/removed events in the log output.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct device *udc_dev = DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0));

static int udc_event_handler(const struct device *dev,
			     const struct udc_event *const event)
{
	switch (event->type) {
	case UDC_EVT_VBUS_READY:
		LOG_INF("VBUS ready - USB cable connected");
		break;
	case UDC_EVT_VBUS_REMOVED:
		LOG_INF("VBUS removed - USB cable disconnected");
		break;
	case UDC_EVT_RESET:
		LOG_INF("USB bus reset");
		break;
	case UDC_EVT_SUSPEND:
		LOG_INF("USB suspended");
		break;
	case UDC_EVT_RESUME:
		LOG_INF("USB resumed");
		break;
	default:
		LOG_DBG("UDC event: %d", event->type);
		break;
	}

	return 0;
}

int main(void)
{
	int ret;

	LOG_INF("USB VBUS Detection Sample");

	if (!device_is_ready(udc_dev)) {
		LOG_ERR("UDC device not ready");
		return -ENODEV;
	}

	ret = udc_init(udc_dev, udc_event_handler, NULL);
	if (ret) {
		LOG_ERR("Failed to initialize UDC: %d", ret);
		return ret;
	}

	LOG_INF("UDC initialized, waiting for VBUS events...");
	LOG_INF("Connect/disconnect USB cable to see VBUS state changes");

	/* The VBUS detect driver runs automatically in the background */
	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
