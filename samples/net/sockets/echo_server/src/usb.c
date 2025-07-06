/*
 * Copyright (c) 2021 TiaC Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_echo_server_sample, LOG_LEVEL_DBG);

#include <zephyr/usb/usb_device.h>
#include <zephyr/net/net_config.h>

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
#include <sample_usbd.h>

static struct usbd_context *sample_usbd;

static int enable_usb_device_next(void)
{
	int err;

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		return -ENODEV;
	}

	err = usbd_enable(sample_usbd);
	if (err) {
		return err;
	}

	return 0;
}
#endif /* CONFIG_USB_DEVICE_STACK_NEXT */

int init_usb(void)
{
#if defined(CONFIG_USB_DEVICE_STACK)
	int ret;

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Cannot enable USB (%d)", ret);
		return ret;
	}
#endif /* CONFIG_USB_DEVICE_STACK */

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
	if (enable_usb_device_next()) {
		return 0;
	}
#endif /* CONFIG_USB_DEVICE_STACK_NEXT */

	(void)net_config_init_app(NULL, "Initializing network");

	return 0;
}
