/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdc_acm_serial, LOG_LEVEL_DBG);

/*
 * This is intended for use with cdc-acm-snippet or as a default serial backend
 * only in applications where no other USB features are required, configured,
 * and enabled. This code only registers the first CDC-ACM instance.
 */

USBD_DEVICE_DEFINE(cdc_acm_serial,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   CONFIG_CDC_ACM_SERIAL_VID, CONFIG_CDC_ACM_SERIAL_PID);

USBD_DESC_LANG_DEFINE(cdc_acm_serial_lang);
USBD_DESC_MANUFACTURER_DEFINE(cdc_acm_serial_mfr, CONFIG_CDC_ACM_SERIAL_MANUFACTURER_STRING);
USBD_DESC_PRODUCT_DEFINE(cdc_acm_serial_product, CONFIG_CDC_ACM_SERIAL_PRODUCT_STRING);
IF_ENABLED(CONFIG_HWINFO, (USBD_DESC_SERIAL_NUMBER_DEFINE(cdc_acm_serial_sn)));

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");

static const uint8_t attributes = IS_ENABLED(CONFIG_CDC_ACM_SERIAL_SELF_POWERED) ?
				  USB_SCD_SELF_POWERED : 0;

USBD_CONFIGURATION_DEFINE(cdc_acm_serial_fs_config,
			  attributes,
			  CONFIG_CDC_ACM_SERIAL_MAX_POWER, &fs_cfg_desc);

USBD_CONFIGURATION_DEFINE(cdc_acm_serial_hs_config,
			  attributes,
			  CONFIG_CDC_ACM_SERIAL_MAX_POWER, &hs_cfg_desc);


static int register_cdc_acm_0(struct usbd_context *const uds_ctx,
			      const enum usbd_speed speed)
{
	struct usbd_config_node *cfg_nd;
	int err;

	if (speed == USBD_SPEED_HS) {
		cfg_nd = &cdc_acm_serial_hs_config;
	} else {
		cfg_nd = &cdc_acm_serial_fs_config;
	}

	err = usbd_add_configuration(uds_ctx, speed, cfg_nd);
	if (err) {
		LOG_ERR("Failed to add configuration");
		return err;
	}

	err = usbd_register_class(&cdc_acm_serial, "cdc_acm_0", speed, 1);
	if (err) {
		LOG_ERR("Failed to register classes");
		return err;
	}

	return usbd_device_set_code_triple(uds_ctx, speed,
					   USB_BCC_MISCELLANEOUS, 0x02, 0x01);
}


static int cdc_acm_serial_init_device(void)
{
	int err;

	err = usbd_add_descriptor(&cdc_acm_serial, &cdc_acm_serial_lang);
	if (err) {
		LOG_ERR("Failed to initialize language descriptor (%d)", err);
		return err;
	}

	err = usbd_add_descriptor(&cdc_acm_serial, &cdc_acm_serial_mfr);
	if (err) {
		LOG_ERR("Failed to initialize manufacturer descriptor (%d)", err);
		return err;
	}

	err = usbd_add_descriptor(&cdc_acm_serial, &cdc_acm_serial_product);
	if (err) {
		LOG_ERR("Failed to initialize product descriptor (%d)", err);
		return err;
	}

	IF_ENABLED(CONFIG_HWINFO, (
		err = usbd_add_descriptor(&cdc_acm_serial, &cdc_acm_serial_sn);
	))
	if (err) {
		LOG_ERR("Failed to initialize SN descriptor (%d)", err);
		return err;
	}

	if (usbd_caps_speed(&cdc_acm_serial) == USBD_SPEED_HS) {
		err = register_cdc_acm_0(&cdc_acm_serial, USBD_SPEED_HS);
		if (err) {
			return err;
		}
	}

	err = register_cdc_acm_0(&cdc_acm_serial, USBD_SPEED_FS);
	if (err) {
		return err;
	}

	err = usbd_init(&cdc_acm_serial);
	if (err) {
		LOG_ERR("Failed to initialize device support");
		return err;
	}

	err = usbd_enable(&cdc_acm_serial);
	if (err) {
		LOG_ERR("Failed to enable device support");
		return err;
	}

	return 0;
}

SYS_INIT(cdc_acm_serial_init_device, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
