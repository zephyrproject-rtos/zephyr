/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample app for USB MTP class
 *
 * Sample app for USB MTP class driver.
 */

#include <sample_usbd.h>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/class/usbd_mtp.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_mtp_sample, LOG_LEVEL_INF);

USBD_MTP_DEFINE_INSTANCE(ram, "ZEPHYR", "Zephyr MTP", "1.0",
			 "0123456789ABCDEF0123456789ABCDEF",
			 USBD_MTP_STORAGE_ENTRY(DT_PROP(DT_NODELABEL(ffs1), mount_point),
						DT_PROP(DT_NODELABEL(ffs1), read_only)));

static void sample_msg_cb(struct usbd_context *const ctx, const struct usbd_msg *msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(ctx)) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(ctx)) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}
}

int main(void)
{
	struct usbd_context *sample_usbd;
	int ret;

	sample_usbd = sample_usbd_init_device(sample_msg_cb);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	if (!usbd_can_detect_vbus(sample_usbd)) {
		/* doc device enable start */
		ret = usbd_enable(sample_usbd);
		if (ret) {
			LOG_ERR("Failed to enable device support");
			return ret;
		}
		/* doc device enable end */
	}

	LOG_INF("USB device support enabled\n");
	return 0;
}
