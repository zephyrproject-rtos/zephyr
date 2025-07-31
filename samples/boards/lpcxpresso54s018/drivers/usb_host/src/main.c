/*
 * Copyright (c) 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(usb_host_sample, LOG_LEVEL_INF);

static void usb_host_status_cb(struct usb_device *udev,
			       const struct usbh_message *const msg)
{
	if (msg->type == USBH_MSG_DEVICE_ADDED) {
		LOG_INF("USB device connected");
		LOG_INF("  VID: 0x%04x, PID: 0x%04x",
			le16_to_cpu(udev->descriptor.idVendor),
			le16_to_cpu(udev->descriptor.idProduct));
		LOG_INF("  Device class: 0x%02x", udev->descriptor.bDeviceClass);
	} else if (msg->type == USBH_MSG_DEVICE_REMOVED) {
		LOG_INF("USB device disconnected");
	}
}

int main(void)
{
	int ret;

	LOG_INF("USB Host sample application");
	LOG_INF("Waiting for USB device to be connected...");

	/* Initialize USB host stack */
	ret = usbh_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize USB host stack: %d", ret);
		return ret;
	}

	/* Register callback for USB device events */
	ret = usbh_register_callback(usb_host_status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to register USB host callback: %d", ret);
		return ret;
	}

	/* Enable USB host controller */
	ret = usbh_enable();
	if (ret != 0) {
		LOG_ERR("Failed to enable USB host: %d", ret);
		return ret;
	}

	LOG_INF("USB host initialized successfully");

	/* Main loop */
	while (1) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}