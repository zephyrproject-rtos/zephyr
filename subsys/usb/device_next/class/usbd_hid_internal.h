/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/usb/class/usbd_hid.h>

/*
 * HID device driver API, we can keep internally as long as it is only used in
 * USB.
 */

struct hid_device_driver_api {
	int (*enable_output)(const struct device *dev, const bool enable);
	int (*submit_report)(const struct device *dev,
			     const uint16_t size, const uint8_t *const report);
	int (*dev_register)(const struct device *dev,
			    const uint8_t *const rdesc, const uint16_t rsize,
			    const struct hid_device_ops *const ops);
};
