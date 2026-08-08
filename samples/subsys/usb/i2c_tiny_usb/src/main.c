/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>

#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include "i2c_tiny_usb.h"

static void msg_cb(struct usbd_context *const usbd_ctx, const struct usbd_msg *const msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(usbd_ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(usbd_ctx)) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(usbd_ctx)) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}
}

int main(void)
{
	struct usbd_context *sample_usbd;
	int ret;

	sample_usbd = sample_usbd_setup_device(msg_cb);
	if (sample_usbd == NULL) {
		LOG_ERR_DEVICE_NOT_READY(sample_usbd->dev);
		return -ENODEV;
	}

	ret = i2c_tiny_usb_register_vreqs(sample_usbd);
	if (ret) {
		LOG_ERR("Failed to register vendor requests");
		return ret;
	}

	ret = usbd_init(sample_usbd);
	if (ret) {
		LOG_ERR("Failed to initialize device support");
		return ret;
	}

	if (!usbd_can_detect_vbus(sample_usbd)) {
		ret = usbd_enable(sample_usbd);
		if (ret) {
			LOG_ERR("Failed to enable device support");
			return ret;
		}
	}

	LOG_INF("USB to I2C bridge sample is initialized");

	return 0;
}
