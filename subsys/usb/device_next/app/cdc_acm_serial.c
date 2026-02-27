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
LOG_MODULE_REGISTER(cdc_acm_serial, CONFIG_USBD_LOG_LEVEL);

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

/*
 * The CDC ACM instances are registered in the same order that they appear in
 * this array. Therefore, they are always presented in the same order on the
 * host side. Add new entries at the end.
 */
const static struct device *uart_devs[] = {
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_console)),
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_shell_uart)),
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_uart_mcumgr)),
};

/*
 * The class data stores a reference to the implemented UART device for
 * internal purposes. Iterate over all class instances and compare the dev
 * parameter with the referenced UART device. Then, try to register the
 * instance. If multiple chosen node properties reference the same device,
 * usbd_register_class() will fail with -EALREADY.
 */
static int register_chosen(struct usbd_context *const uds_ctx,
			   const enum usbd_speed speed,
			   const struct device *const dev)
{
	int err;

	if (speed == USBD_SPEED_HS) {
		STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_hs, usbd_class_node, c_nd) {
			struct usbd_class_data *c_data = c_nd->c_data;

			if (usbd_class_get_private(c_data) == dev) {
				err = usbd_register_class(&cdc_acm_serial,
							  c_data->name, speed, 1);
				if (err != 0 && err != -EALREADY) {
					LOG_ERR("Failed to register %s", c_data->name);
					return err;
				}

				break;
			}
		}
	}

	if (speed == USBD_SPEED_FS) {
		STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs, usbd_class_node, c_nd) {
			struct usbd_class_data *c_data = c_nd->c_data;

			if (usbd_class_get_private(c_data) == dev) {
				err = usbd_register_class(&cdc_acm_serial,
							  c_data->name, speed, 1);
				if (err != 0 && err != -EALREADY) {
					LOG_ERR("Failed to register %s", c_data->name);
					return err;
				}

				break;
			}
		}
	}

	return 0;
}

static int register_cdc_acm(struct usbd_context *const uds_ctx,
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

	if (IS_ENABLED(CONFIG_CDC_ACM_SERIAL_MULTIPLE_INSTANCES)) {
		for (int n = 0; n < ARRAY_SIZE(uart_devs); n++) {
			if (uart_devs[n] == NULL) {
				continue;
			}

			err = register_chosen(uds_ctx, speed, uart_devs[n]);
			if (err) {
				return err;
			}
		}
	} else {
		/*
		 * Only register the first instance to maintain the legacy
		 * stack behavior when using CDC ACM as a serial backend for
		 * different applications.
		 */
		err = usbd_register_class(&cdc_acm_serial, "cdc_acm_0", speed, 1);
		if (err) {
			LOG_ERR("Failed to register %s", "cdc_acm_0");
			return err;
		}
	}

	return usbd_device_set_code_triple(uds_ctx, speed,
					   USB_BCC_MISCELLANEOUS, 0x02, 0x01);
}


static int cdc_acm_serial_init_device(void)
{
	int err;

	err = usbd_add_descriptor(&cdc_acm_serial, &cdc_acm_serial_lang);
	if (err) {
		LOG_ERR("Failed to initialize %s (%d)", "language descriptor", err);
		return err;
	}

	err = usbd_add_descriptor(&cdc_acm_serial, &cdc_acm_serial_mfr);
	if (err) {
		LOG_ERR("Failed to initialize %s (%d)", "manufacturer descriptor", err);
		return err;
	}

	err = usbd_add_descriptor(&cdc_acm_serial, &cdc_acm_serial_product);
	if (err) {
		LOG_ERR("Failed to initialize %s (%d)", "product descriptor", err);
		return err;
	}

	IF_ENABLED(CONFIG_HWINFO, (
		err = usbd_add_descriptor(&cdc_acm_serial, &cdc_acm_serial_sn);
	))
	if (err) {
		LOG_ERR("Failed to initialize %s (%d)", "SN descriptor", err);
		return err;
	}

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_caps_speed(&cdc_acm_serial) == USBD_SPEED_HS) {
		err = register_cdc_acm(&cdc_acm_serial, USBD_SPEED_HS);
		if (err) {
			return err;
		}
	}

	err = register_cdc_acm(&cdc_acm_serial, USBD_SPEED_FS);
	if (err) {
		return err;
	}

	err = usbd_init(&cdc_acm_serial);
	if (err) {
		LOG_ERR("Failed to initialize %s (%d)", "device support", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_CDC_ACM_SERIAL_ENABLE_AT_BOOT)) {
		err = usbd_enable(&cdc_acm_serial);
		if (err) {
			LOG_ERR("Failed to enable %s (%d)", "device support", err);
			return err;
		}
	}

	return 0;
}

SYS_INIT(cdc_acm_serial_init_device, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
