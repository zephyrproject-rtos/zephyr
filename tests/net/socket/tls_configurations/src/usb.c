/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_config.h>
#include <zephyr/usb/usbd.h>
#include <sample_usbd.h>

LOG_MODULE_DECLARE(tls_configuration_sample, LOG_LEVEL_INF);

static int init_usb(void)
{
	struct usbd_context *usbd;
	int err;

	usbd = sample_usbd_init_device(NULL);
	if (usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	err = usbd_enable(usbd);
	if (err != 0) {
		LOG_ERR("Failed to enable USB device (%d)", err);
		return err;
	}

	(void)net_config_init_app(NULL, "Initializing network");

	return 0;
}

SYS_INIT(init_usb, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
