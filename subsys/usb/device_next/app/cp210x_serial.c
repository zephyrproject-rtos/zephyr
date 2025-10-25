/*
 * Copyright (c) 2025 Sergey Matsievskiy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cp210x_serial, CONFIG_USBD_LOG_LEVEL);

/*
 * This is intended for use with cp210x-snippet or as a default serial backend
 * only in applications where no other USB features are required, configured,
 * and enabled. This code only registers the first CP210X instance.
 */

USBD_DEVICE_DEFINE(cp210x_serial,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   CONFIG_CP210X_SERIAL_VID, CONFIG_CP210X_SERIAL_PID);

USBD_DESC_LANG_DEFINE(cp210x_serial_lang);
USBD_DESC_MANUFACTURER_DEFINE(cp210x_serial_mfr, CONFIG_CP210X_SERIAL_MANUFACTURER_STRING);
USBD_DESC_PRODUCT_DEFINE(cp210x_serial_product, CONFIG_CP210X_SERIAL_PRODUCT_STRING);
IF_ENABLED(CONFIG_HWINFO, (USBD_DESC_SERIAL_NUMBER_DEFINE(cp210x_serial_sn)));

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");

static const uint8_t attributes = IS_ENABLED(CONFIG_CP210X_SERIAL_SELF_POWERED) ?
				  USB_SCD_SELF_POWERED : 0;

USBD_CONFIGURATION_DEFINE(cp210x_serial_fs_config,
			  attributes,
			  CONFIG_CP210X_SERIAL_MAX_POWER, &fs_cfg_desc);

static int register_cp210x_0(struct usbd_context *const uds_ctx,
			     const enum usbd_speed speed)
{
	struct usbd_config_node *cfg_nd;
	int err;

	cfg_nd = &cp210x_serial_fs_config;

	err = usbd_add_configuration(uds_ctx, speed, cfg_nd);
	if (err) {
		LOG_ERR("Failed to add configuration");
		return err;
	}

	err = usbd_register_class(&cp210x_serial, "cp210x_0", speed, 1);
	if (err) {
		LOG_ERR("Failed to register classes");
		return err;
	}

	return usbd_device_set_code_triple(uds_ctx, speed, 0, 0, 0);
}

static int cp210x_serial_init_device(void)
{
	int err;

	err = usbd_add_descriptor(&cp210x_serial, &cp210x_serial_lang);
	if (err) {
		LOG_ERR("Failed to initialize %s (%d)", "language descriptor", err);
		return err;
	}

	err = usbd_add_descriptor(&cp210x_serial, &cp210x_serial_mfr);
	if (err) {
		LOG_ERR("Failed to initialize %s (%d)", "manufacturer descriptor", err);
		return err;
	}

	err = usbd_add_descriptor(&cp210x_serial, &cp210x_serial_product);
	if (err) {
		LOG_ERR("Failed to initialize %s (%d)", "product descriptor", err);
		return err;
	}

	IF_ENABLED(CONFIG_HWINFO, (
		err = usbd_add_descriptor(&cp210x_serial, &cp210x_serial_sn);
	))
	if (err) {
		LOG_ERR("Failed to initialize %s (%d)", "SN descriptor", err);
		return err;
	}

	err = register_cp210x_0(&cp210x_serial, USBD_SPEED_FS);
	if (err) {
		return err;
	}

	err = usbd_init(&cp210x_serial);
	if (err) {
		LOG_ERR("Failed to initialize %s (%d)", "device support", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_CP210X_SERIAL_ENABLE_AT_BOOT)) {
		err = usbd_enable(&cp210x_serial);
		if (err) {
			LOG_ERR("Failed to enable %s (%d)", "device support", err);
			return err;
		}
	}

	return 0;
}

SYS_INIT(cp210x_serial_init_device, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
