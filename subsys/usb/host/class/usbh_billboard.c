/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_usbh_billboard_device

#include <stdlib.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/class/usbh_billboard.h>
#include <zephyr/usb/bos.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"

LOG_MODULE_REGISTER(usbh_billboard, CONFIG_USBH_BILLBOARD_LOG_LEVEL);

/* String descriptor memory limit */
#define MAX_STRDESC_LEN (255)

/* NOTE: List of supported codes is at usb.org (LANGIDs) */
#ifndef USB_LANG_ENGLISH_USA
#define USB_LANG_ENGLISH_USA (0x0409)
#endif

/**
 * Driver data
 */
struct usbh_billboard_drv_data {
	/* Mutex */
	struct k_mutex lock;
	/* Device was probed */
	bool probed;
	struct usb_device *udev;
	uint16_t iface;
	/* Language code used for String descriptor requests */
	uint16_t lang_code;
};

/**
 * @brief Billboard driver initialization
 *
 * @param c_data  Pointer to USB class
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_billboard_class_init(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_billboard_drv_data *const drv_data = dev->data;

	k_mutex_init(&drv_data->lock);
	drv_data->probed = false;

	return 0;
}

/**
 * @brief Billboard driver probe function, invoked upon USB device attachment
 *
 * @param c_data        Pointer to USB class
 * @param udev          Pointer to USB device
 * @param iface         Interface number
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_billboard_class_probe(struct usbh_class_data *const c_data,
				      struct usb_device *const udev, uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct usbh_billboard_drv_data *drv_data = (void *)dev->data;
	int result = 0;

	LOG_DBG("Billboard device was attached");

	if ((udev == NULL) || (udev->state != USB_STATE_CONFIGURED)) {
		LOG_ERR("USB device not properly configured");
		return -ENODEV;
	}

	if (drv_data == NULL) {
		LOG_ERR("No Billboard device instance is available");
		return -ENODEV;
	}

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	drv_data->udev = udev;
	drv_data->iface = iface;

	/* Using English language as default one */
	drv_data->lang_code = USB_LANG_ENGLISH_USA;
	drv_data->probed = true;

	k_mutex_unlock(&drv_data->lock);

	return result;
}

/**
 * @brief Billboard driver remove function, invoked upon USB device removal
 *
 * @param c_data        Pointer to USB class
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_billboard_class_removed(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_billboard_drv_data *drv_data = (void *)dev->data;
	int result = 0;

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	drv_data->udev = NULL;
	drv_data->probed = false;
	k_mutex_unlock(&drv_data->lock);

	LOG_DBG("Billboard device was removed");

	return result;
}

/**
 * @brief Parses Billboard capability descriptors from BOS
 *
 * The BOS descriptor was fetched during device enumeration. The
 * function invokes a user callback for every billboard capability.
 *
 * @param dev             Pointer to device
 * @param billboard_cb    Pointer to callback
 * @param cb_arg          Pointer to callback argument
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_billboard_parse_api(struct device const *dev, usbh_billboard_cb_t billboard_cb,
				    void *cb_arg)
{
	struct usbh_billboard_drv_data *drv_data;
	const struct usb_bos_capability_header *cap_header;
	const struct usb_bos_descriptor *bos_desc = NULL;
	const void *seek_desc;
	int result = 0;

	if (dev == NULL || billboard_cb == NULL) {
		return -EINVAL;
	}

	drv_data = dev->data;

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			result = -EAGAIN;
			break;
		}

		bos_desc = usbh_desc_get_bos(drv_data->udev);
		if (bos_desc == NULL) {
			result = -ENOTSUP;
			break;
		}

		seek_desc = bos_desc;
		do {
			cap_header = usbh_desc_get_next_capability(bos_desc, seek_desc,
								   USB_BOS_CAPABILITY_BILLBOARD);
			if (cap_header) {
				billboard_cb(cb_arg, cap_header);
			}
			seek_desc = cap_header;
		} while (seek_desc != NULL);
	} while (0);

	k_mutex_unlock(&drv_data->lock);

	return result;
}

/**
 * @brief Perform full request to get string descriptor
 *
 * @param udev           Pointer to USB device
 * @param len            String descriptor length
 * @param str_desc       Output argument, pointer to string descriptor
 * @param str_id         String descriptor index
 * @param lang_code      Language code
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_billboard_string_wrapper(struct usb_device *udev, size_t len,
					 struct usb_string_descriptor *const str_desc,
					 uint8_t str_id, uint16_t lang_code)
{
	struct usb_string_descriptor str_hdr;
	int ret = 0;

	/* Request header */
	ret = usbh_req_desc_string(udev, 2, &str_hdr, str_id, lang_code);
	if (ret) {
		return ret;
	}
	len = MIN(str_hdr.bLength, len);
	/* Full request, respecting length constraint */
	ret = usbh_req_desc_string(udev, len, str_desc, str_id, lang_code);
	if (ret) {
		return ret;
	}

	if (str_desc->bLength != len) {
		return -EBADMSG;
	}

	return 0;
}

/**
 * @brief Fetch string descriptor based on string ID
 *
 * @param drv_data        Pointer to driver data
 * @param str_id          String index
 * @param str_desc        Output argument, pointer to string descriptor
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_billboard_fetch_string_desc_api(struct device const *dev, uint8_t str_id,
						struct usb_string_descriptor *str_desc, size_t len)
{
	struct usbh_billboard_drv_data *drv_data;
	int result = 0;

	do {
		if (dev == NULL || str_desc == NULL) {
			result = -EINVAL;
			break;
		}
		drv_data = dev->data;

		result = k_mutex_lock(&drv_data->lock, K_FOREVER);
		if (result) {
			break;
		}

		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			result = -EAGAIN;
		} else {
			result = usbh_billboard_string_wrapper(drv_data->udev, len, str_desc,
							       str_id, drv_data->lang_code);
		}
		k_mutex_unlock(&drv_data->lock);
	} while (0);

	return result;
}

/**
 * @brief Fetches available languages from the device
 *
 * @param drv_data        Pointer to driver data
 * @param str_desc        Output argument, pointer to string descriptor
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_billboard_fetch_langs_desc_api(struct device const *dev,
					       struct usb_string_descriptor *str_desc, size_t len)
{
	struct usbh_billboard_drv_data *drv_data;
	int result = 0;

	do {
		if (dev == NULL || str_desc == NULL) {
			result = -EINVAL;
			break;
		}
		drv_data = dev->data;

		result = k_mutex_lock(&drv_data->lock, K_FOREVER);
		if (result) {
			break;
		}

		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			result = -EAGAIN;
		} else {
			result = usbh_billboard_string_wrapper(drv_data->udev, len, str_desc, 0, 0);
		}
		k_mutex_unlock(&drv_data->lock);
	} while (0);

	return result;
}

/**
 * @brief Changes the language code used for fetching string descriptors
 *
 * @param drv_data        Pointer to driver data
 * @param lang_code       Language code
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_billboard_use_lang_api(struct device const *dev, uint16_t lang_code)
{
	const size_t string_hdr_size = 2;
	struct usbh_billboard_drv_data *drv_data;
	struct usb_string_descriptor *str_desc = NULL;
	size_t lang_count, i;
	int result = 0;

	if (dev == NULL) {
		return -EINVAL;
	}
	drv_data = dev->data;

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			result = -EAGAIN;
			break;
		}

		str_desc = k_malloc(MAX_STRDESC_LEN + 1);
		if (str_desc == NULL) {
			result = -ENOMEM;
			break;
		}

		result = usbh_billboard_string_wrapper(drv_data->udev, MAX_STRDESC_LEN + 1,
						       str_desc, 0, 0);
		if (result) {
			break;
		}

		if (str_desc->bLength <= string_hdr_size) {
			result = -EINVAL;
			break;
		}
		lang_count = (str_desc->bLength - string_hdr_size) / 2;

		/* Not found by default */
		result = -ENOTSUP;
		/* Search for requested 'lang_code' in device response */
		for (i = 0; i < lang_count; i++) {
			if (lang_code == sys_get_le16(&((uint8_t *)(&str_desc->bString))[i * 2])) {
				drv_data->lang_code = lang_code;
				result = 0;
				break;
			}
		}
	} while (0);

	if (str_desc != NULL) {
		k_free(str_desc);
	}

	k_mutex_unlock(&drv_data->lock);

	return result;
}

/**
 * @brief USB Host Billboard driver table
 */
static __maybe_unused DEVICE_API(usbh_billboard, usbh_billboard_api) = {
	.parse = usbh_billboard_parse_api,
	.fetch_string = usbh_billboard_fetch_string_desc_api,
	.fetch_langs = usbh_billboard_fetch_langs_desc_api,
	.use_lang = usbh_billboard_use_lang_api,
};

static __maybe_unused struct usbh_class_api usbh_billboard_class_api = {
	.init = usbh_billboard_class_init,
	.probe = usbh_billboard_class_probe,
	.removed = usbh_billboard_class_removed,
};

static __maybe_unused struct usbh_class_filter usbh_billboard_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_BILLBOARD,
		.sub = USB_BILLBOARD_SUBCLASS,
		.proto = USB_BILLBOARD_RUNTIME,
	},
	{0},
};

#define USBH_BILLBOARD_DEVICE_DEFINE(n)                                                            \
	static struct usbh_billboard_drv_data usbh_billboard_drv_data##n;                          \
	COND_CODE_0(DT_INST_PROP(n, match_class),                                             \
	(static struct usbh_class_filter const usbh_billboard_vid_pid_filters_##n[] = {       \
		{                                                                             \
			.flags = USBH_CLASS_MATCH_VID_PID,                                    \
			.vid = (DT_INST_REG_ADDR(n) >> 16u) & 0xFFFFu,                        \
			.pid = DT_INST_REG_ADDR(n) & 0xFFFFu,                                 \
		},                                                                            \
		{0u}                                                                          \
	};), ())    \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &usbh_billboard_drv_data##n, NULL, POST_KERNEL,       \
			      CONFIG_USBH_BILLBOARD_INIT_PRIORITY, &usbh_billboard_api);           \
	USBH_DEFINE_CLASS(billboard_host_c_data_##n, &usbh_billboard_class_api,                    \
			  (void *)DEVICE_DT_INST_GET(n),                                           \
			  COND_CODE_1(DT_INST_PROP(n, match_class),                           \
				(usbh_billboard_filters), (usbh_billboard_vid_pid_filters_##n)));

DT_INST_FOREACH_STATUS_OKAY(USBH_BILLBOARD_DEVICE_DEFINE)
