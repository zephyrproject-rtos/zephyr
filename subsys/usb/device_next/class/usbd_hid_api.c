/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usbd_hid_internal.h"

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/usb/class/usbd_hid.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hid_api, CONFIG_USBD_HID_LOG_LEVEL);

int hid_device_submit_report(const struct device *dev,
			     const uint16_t size, const uint8_t *const report)
{
	const struct hid_device_driver_api *api = dev->api;

	return api->submit_report(dev, size, report);
}

int hid_device_register(const struct device *dev,
			const uint8_t *const rdesc, const uint16_t rsize,
			const struct hid_device_ops *const ops)
{
	const struct hid_device_driver_api *api = dev->api;

	return api->dev_register(dev, rdesc, rsize, ops);
}

int hid_device_set_in_polling(const struct device *dev, const unsigned int period_us)
{
	const struct hid_device_driver_api *const api = dev->api;

	if (IS_ENABLED(CONFIG_USBD_HID_SET_POLLING_PERIOD)) {
		return api->set_in_polling(dev, period_us);
	}

	return -ENOTSUP;
}

int hid_device_set_out_polling(const struct device *dev, const unsigned int period_us)
{
	const struct hid_device_driver_api *const api = dev->api;

	if (IS_ENABLED(CONFIG_USBD_HID_SET_POLLING_PERIOD)) {
		return api->set_out_polling(dev, period_us);
	}

	return -ENOTSUP;
}
