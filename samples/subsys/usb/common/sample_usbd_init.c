/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/bos.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_sample_config);

/* Use zephyr_udc compatible for auto-discovery */
#define DT_DRV_COMPAT zephyr_udc

/* By default, do not register the USB DFU class DFU mode instance. */
static const char *const blocklist[] = {
	"dfu_dfu",
	NULL,
};

/* doc device instantiation start */
/*
 * Instantiate a USBD context for each device with zephyr,udc compatible.
 * This enables auto-discovery of all UDC devices in the system.
 */
#define USBD_DEVICE_DEFINE_INST(inst)					\
	USBD_DEVICE_DEFINE(sample_usbd_##inst,				\
			   DEVICE_DT_INST_GET(inst),			\
			   CONFIG_SAMPLE_USBD_VID,			\
			   CONFIG_SAMPLE_USBD_PID);

DT_INST_FOREACH_STATUS_OKAY(USBD_DEVICE_DEFINE_INST)

/*
 * Backward compatibility: Add zephyr_udc0 nodelabel device if it exists
 * but does NOT have zephyr,udc compatible (legacy boards).
 */
#if DT_NODE_EXISTS(DT_NODELABEL(zephyr_udc0)) && \
    !DT_NODE_HAS_COMPAT(DT_NODELABEL(zephyr_udc0), zephyr_udc)
USBD_DEVICE_DEFINE(sample_usbd_fallback,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   CONFIG_SAMPLE_USBD_VID,
		   CONFIG_SAMPLE_USBD_PID);
#define USBD_FALLBACK_CTX_PTR &sample_usbd_fallback,
#else
#define USBD_FALLBACK_CTX_PTR
#endif

/* Array of pointers to all USBD contexts */
#define USBD_CTX_PTR_INST(inst) &sample_usbd_##inst,

static struct usbd_context *const sample_usbd_contexts[] = {
	DT_INST_FOREACH_STATUS_OKAY(USBD_CTX_PTR_INST)
	USBD_FALLBACK_CTX_PTR
};

#define SAMPLE_USBD_DEVICE_COUNT ARRAY_SIZE(sample_usbd_contexts)

/* For backward compatibility, reference first device as sample_usbd */
#define sample_usbd (*sample_usbd_contexts[0])
/* doc device instantiation end */

/* doc string instantiation start */
USBD_DESC_LANG_DEFINE(sample_lang);
USBD_DESC_MANUFACTURER_DEFINE(sample_mfr, CONFIG_SAMPLE_USBD_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(sample_product, CONFIG_SAMPLE_USBD_PRODUCT);
IF_ENABLED(CONFIG_HWINFO, (USBD_DESC_SERIAL_NUMBER_DEFINE(sample_sn)));

/* doc string instantiation end */

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");

/* doc configuration instantiation start */
static const uint8_t attributes = (IS_ENABLED(CONFIG_SAMPLE_USBD_SELF_POWERED) ?
				   USB_SCD_SELF_POWERED : 0) |
				  (IS_ENABLED(CONFIG_SAMPLE_USBD_REMOTE_WAKEUP) ?
				   USB_SCD_REMOTE_WAKEUP : 0);

/* Full speed configuration */
USBD_CONFIGURATION_DEFINE(sample_fs_config,
			  attributes,
			  CONFIG_SAMPLE_USBD_MAX_POWER, &fs_cfg_desc);

/* High speed configuration */
USBD_CONFIGURATION_DEFINE(sample_hs_config,
			  attributes,
			  CONFIG_SAMPLE_USBD_MAX_POWER, &hs_cfg_desc);
/* doc configuration instantiation end */

#if CONFIG_SAMPLE_USBD_20_EXTENSION_DESC
/*
 * This does not yet provide valuable information, but rather serves as an
 * example, and will be improved in the future.
 */
static const struct usb_bos_capability_lpm bos_cap_lpm = {
	.bLength = sizeof(struct usb_bos_capability_lpm),
	.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
	.bDevCapabilityType = USB_BOS_CAPABILITY_EXTENSION,
	.bmAttributes = 0UL,
};

USBD_DESC_BOS_DEFINE(sample_usbext, sizeof(bos_cap_lpm), &bos_cap_lpm);
#endif

static void sample_fix_code_triple(struct usbd_context *uds_ctx,
				   const enum usbd_speed speed)
{
	/* Always use class code information from Interface Descriptors */
	if (IS_ENABLED(CONFIG_USBD_CDC_ACM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_CDC_ECM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_CDC_NCM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_MIDI2_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_VIDEO_CLASS)) {
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
}

struct usbd_context *sample_usbd_setup_device(usbd_msg_cb_t msg_cb)
{
	int err;

	/* doc add string descriptor start */
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

	IF_ENABLED(CONFIG_HWINFO, (
		err = usbd_add_descriptor(&sample_usbd, &sample_sn);
	))
	if (err) {
		LOG_ERR("Failed to initialize SN descriptor (%d)", err);
		return NULL;
	}
	/* doc add string descriptor end */

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_caps_speed(&sample_usbd) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&sample_usbd, USBD_SPEED_HS,
					     &sample_hs_config);
		if (err) {
			LOG_ERR("Failed to add High-Speed configuration");
			return NULL;
		}

		err = usbd_register_all_classes(&sample_usbd, USBD_SPEED_HS, 1,
						blocklist);
		if (err) {
			LOG_ERR("Failed to add register classes");
			return NULL;
		}

		sample_fix_code_triple(&sample_usbd, USBD_SPEED_HS);
	}

	/* doc configuration register start */
	err = usbd_add_configuration(&sample_usbd, USBD_SPEED_FS,
				     &sample_fs_config);
	if (err) {
		LOG_ERR("Failed to add Full-Speed configuration");
		return NULL;
	}
	/* doc configuration register end */

	/* doc functions register start */
	err = usbd_register_all_classes(&sample_usbd, USBD_SPEED_FS, 1, blocklist);
	if (err) {
		LOG_ERR("Failed to add register classes");
		return NULL;
	}
	/* doc functions register end */

	sample_fix_code_triple(&sample_usbd, USBD_SPEED_FS);
	usbd_self_powered(&sample_usbd, attributes & USB_SCD_SELF_POWERED);

	if (msg_cb != NULL) {
		/* doc device init-and-msg start */
		err = usbd_msg_register_cb(&sample_usbd, msg_cb);
		if (err) {
			LOG_ERR("Failed to register message callback");
			return NULL;
		}
		/* doc device init-and-msg end */
	}

#if CONFIG_SAMPLE_USBD_20_EXTENSION_DESC
	(void)usbd_device_set_bcd_usb(&sample_usbd, USBD_SPEED_FS, 0x0201);
	(void)usbd_device_set_bcd_usb(&sample_usbd, USBD_SPEED_HS, 0x0201);

	err = usbd_add_descriptor(&sample_usbd, &sample_usbext);
	if (err) {
		LOG_ERR("Failed to add USB 2.0 Extension Descriptor");
		return NULL;
	}
#endif

	return &sample_usbd;
}

struct usbd_context *sample_usbd_init_device(usbd_msg_cb_t msg_cb)
{
	int err;

	if (sample_usbd_setup_device(msg_cb) == NULL) {
		return NULL;
	}

	/* doc device init start */
	err = usbd_init(&sample_usbd);
	if (err) {
		LOG_ERR("Failed to initialize device support");
		return NULL;
	}
	/* doc device init end */

	return &sample_usbd;
}

/*
 * Get the number of UDC devices discovered via zephyr,udc compatible.
 */
size_t sample_usbd_get_device_count(void)
{
	return SAMPLE_USBD_DEVICE_COUNT;
}

/*
 * Get a USBD context by index.
 */
struct usbd_context *sample_usbd_get_context(size_t idx)
{
	if (idx >= SAMPLE_USBD_DEVICE_COUNT) {
		return NULL;
	}

	return sample_usbd_contexts[idx];
}

/*
 * Setup and initialize all UDC devices discovered via zephyr,udc compatible.
 * Returns the number of successfully initialized devices.
 */
static int sample_usbd_setup_single(struct usbd_context *ctx, usbd_msg_cb_t msg_cb)
{
	int err;

	err = usbd_add_descriptor(ctx, &sample_lang);
	if (err) {
		LOG_ERR("Failed to add language descriptor (%d)", err);
		return err;
	}

	err = usbd_add_descriptor(ctx, &sample_mfr);
	if (err) {
		LOG_ERR("Failed to add manufacturer descriptor (%d)", err);
		return err;
	}

	err = usbd_add_descriptor(ctx, &sample_product);
	if (err) {
		LOG_ERR("Failed to add product descriptor (%d)", err);
		return err;
	}

	IF_ENABLED(CONFIG_HWINFO, (
		err = usbd_add_descriptor(ctx, &sample_sn);
		if (err) {
			LOG_ERR("Failed to add SN descriptor (%d)", err);
			return err;
		}
	))

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_caps_speed(ctx) == USBD_SPEED_HS) {
		err = usbd_add_configuration(ctx, USBD_SPEED_HS,
					     &sample_hs_config);
		if (err) {
			LOG_ERR("Failed to add HS configuration (%d)", err);
			return err;
		}

		err = usbd_register_all_classes(ctx, USBD_SPEED_HS, 1,
						blocklist);
		if (err) {
			LOG_ERR("Failed to register HS classes (%d)", err);
			return err;
		}

		sample_fix_code_triple(ctx, USBD_SPEED_HS);
	}

	err = usbd_add_configuration(ctx, USBD_SPEED_FS, &sample_fs_config);
	if (err) {
		LOG_ERR("Failed to add FS configuration (%d)", err);
		return err;
	}

	err = usbd_register_all_classes(ctx, USBD_SPEED_FS, 1, blocklist);
	if (err) {
		LOG_ERR("Failed to register FS classes (%d)", err);
		return err;
	}

	sample_fix_code_triple(ctx, USBD_SPEED_FS);
	usbd_self_powered(ctx, attributes & USB_SCD_SELF_POWERED);

	if (msg_cb != NULL) {
		err = usbd_msg_register_cb(ctx, msg_cb);
		if (err) {
			LOG_ERR("Failed to register message callback (%d)", err);
			return err;
		}
	}

	return 0;
}

int sample_usbd_init_all_devices(usbd_msg_cb_t msg_cb)
{
	int initialized = 0;
	int err;

	for (size_t i = 0; i < SAMPLE_USBD_DEVICE_COUNT; i++) {
		struct usbd_context *ctx = sample_usbd_contexts[i];

		err = sample_usbd_setup_single(ctx, msg_cb);
		if (err) {
			LOG_ERR("Failed to setup device %zu (%d)", i, err);
			continue;
		}

		err = usbd_init(ctx);
		if (err) {
			LOG_ERR("Failed to init device %zu (%d)", i, err);
			continue;
		}

		LOG_INF("Initialized UDC device %zu", i);
		initialized++;
	}

	return initialized;
}
