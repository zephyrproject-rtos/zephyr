/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/sys/iterable_sections.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_sample_config);

#define ZEPHYR_PROJECT_USB_VID		0x2fe3

USBD_DEVICE_DEFINE(sample_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   ZEPHYR_PROJECT_USB_VID, CONFIG_SAMPLE_USBD_PID);

USBD_DESC_LANG_DEFINE(sample_lang);
USBD_DESC_MANUFACTURER_DEFINE(sample_mfr, CONFIG_SAMPLE_USBD_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(sample_product, CONFIG_SAMPLE_USBD_PRODUCT);
USBD_DESC_SERIAL_NUMBER_DEFINE(sample_sn, "0123456789ABCDEF");

static const uint8_t attributes = (IS_ENABLED(CONFIG_SAMPLE_USBD_SELF_POWERED) ?
				   USB_SCD_SELF_POWERED : 0) |
				  (IS_ENABLED(CONFIG_SAMPLE_USBD_REMOTE_WAKEUP) ?
				   USB_SCD_REMOTE_WAKEUP : 0);

USBD_CONFIGURATION_DEFINE(sample_fs_config,
			  attributes,
			  CONFIG_SAMPLE_USBD_MAX_POWER);

USBD_CONFIGURATION_DEFINE(sample_hs_config,
			  attributes,
			  CONFIG_SAMPLE_USBD_MAX_POWER);

static int register_fs_classes(struct usbd_contex *uds_ctx)
{
	int err = 0;

	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs, usbd_class_iter, iter) {
		/* Pull everything that is enabled in our configuration. */
		err = usbd_register_class(uds_ctx, iter->c_nd->name,
					  USBD_SPEED_FS, 1);
		if (err) {
			LOG_ERR("Failed to register FS %s (%d)",
				iter->c_nd->name, err);
			return err;
		}

		LOG_DBG("Register FS %s", iter->c_nd->name);
	}

	return err;
}

static int register_hs_classes(struct usbd_contex *uds_ctx)
{
	int err = 0;

	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_hs, usbd_class_iter, iter) {
		/* Pull everything that is enabled in our configuration. */
		err = usbd_register_class(uds_ctx, iter->c_nd->name,
					  USBD_SPEED_HS, 1);
		if (err) {
			LOG_ERR("Failed to register HS %s (%d)",
				iter->c_nd->name, err);
			return err;
		}

		LOG_DBG("Register HS %s", iter->c_nd->name);
	}

	return err;
}

static int sample_add_configuration(struct usbd_contex *uds_ctx,
				    const enum usbd_speed speed,
				    struct usbd_config_node *config)
{
	int err;

	err = usbd_add_configuration(uds_ctx, speed, config);
	if (err) {
		LOG_ERR("Failed to add configuration (%d)", err);
		return err;
	}

	if (speed == USBD_SPEED_FS) {
		err = register_fs_classes(uds_ctx);
	} else if (speed == USBD_SPEED_HS) {
		err = register_hs_classes(uds_ctx);
	}

	if (err) {
		return err;
	}

	/* Always use class code information from Interface Descriptors */
	if (IS_ENABLED(CONFIG_USBD_CDC_ACM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_CDC_ECM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS)) {
		/*
		 * Class with multiple interfaces have an Interface
		 * Association Descriptor available, use an appropriate triple
		 * to indicate it.
		 */
		usbd_device_set_code_triple(uds_ctx, speed,
					    USB_BCC_MISCELLANEOUS, 0x02, 0x01);
	} else {
		usbd_device_set_code_triple(uds_ctx, speed, 0, 0, 0);
	}

	return 0;
}

struct usbd_contex *sample_usbd_init_device(usbd_msg_cb_t msg_cb)
{
	int err;

	err = usbd_add_descriptor(&sample_usbd, &sample_lang);
	if (err) {
		LOG_ERR("Failed to initialize language descriptor (%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&sample_usbd, &sample_mfr);
	if (err) {
		LOG_ERR("Failed to initialize manufacturer descriptor (%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&sample_usbd, &sample_product);
	if (err) {
		LOG_ERR("Failed to initialize product descriptor (%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&sample_usbd, &sample_sn);
	if (err) {
		LOG_ERR("Failed to initialize SN descriptor (%d)", err);
		return NULL;
	}

	if (usbd_caps_speed(&sample_usbd) == USBD_SPEED_HS) {
		err = sample_add_configuration(&sample_usbd, USBD_SPEED_HS,
					       &sample_hs_config);
		if (err) {
			LOG_ERR("Failed to add High-Speed configuration");
			return NULL;
		}
	}

	err = sample_add_configuration(&sample_usbd, USBD_SPEED_FS,
				       &sample_fs_config);
	if (err) {
		LOG_ERR("Failed to add Full-Speed configuration");
		return NULL;
	}

	if (msg_cb != NULL) {
		err = usbd_msg_register_cb(&sample_usbd, msg_cb);
		if (err) {
			LOG_ERR("Failed to register message callback");
			return NULL;
		}
	}

	err = usbd_init(&sample_usbd);
	if (err) {
		LOG_ERR("Failed to initialize device support");
		return NULL;
	}

	return &sample_usbd;
}
