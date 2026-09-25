/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBH_BILLBOARD_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBH_BILLBOARD_H_

#include <zephyr/device.h>
#include <zephyr/syscall.h>
#include <zephyr/usb/class/usb_billboard.h>
#include <zephyr/usb/bos.h>

/**
 * @file
 * @brief USB host Billboard class driver API
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief App callback to handle parsed capabilities descriptors
 *
 * @param cb_arg        Pointer to callback specific context structure
 * @param cap_header    Pointer to capability
 *
 */
typedef void (*usbh_billboard_cb_t)(void *cb_arg,
				    const struct usb_bos_capability_header *cap_header);

/**
 * @brief Parse billboard capabilities descriptors from BOS
 *
 * @param dev           Pointer to the device
 * @param billboard_cb  Application callback
 * @param cb_arg        Pointer to callback specific context structure
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_billboard_parse_t)(struct device const *dev, usbh_billboard_cb_t billboard_cb,
				      void *cb_arg);

/**
 * @brief Fetches string descriptors
 *
 * @param dev           Pointer to the device
 * @param str_idx       String descriptor index
 * @param str_desc      Output argument, double pointer to string descriptor
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_billboard_fetch_string_t)(struct device const *dev, uint8_t str_idx,
					     struct usb_string_descriptor *str_desc, size_t len);

/**
 * @brief Fetches the supported languages from the device
 *
 * @param dev           Pointer to the device
 * @param str_desc      Output argument, double pointer to string descriptor
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_billboard_fetch_langs_t)(struct device const *dev,
					    struct usb_string_descriptor *str_desc, size_t len);

/**
 * @brief Changes the language used for fetching string descriptors
 *
 * @param dev           Pointer to the device
 * @param lang_code     Language code
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_billboard_use_lang_t)(struct device const *dev, uint16_t lang_code);

/**
 * @brief USB host Billboard driver API
 */
__subsystem struct usbh_billboard_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief usbh_billboard_parse
	 */
	usbh_billboard_parse_t parse;
	/**
	 * @driver_ops_mandatory @copybrief usbh_billboard_fetch_string_desc
	 */
	usbh_billboard_fetch_string_t fetch_string;
	/**
	 * @driver_ops_mandatory @copybrief usbh_billboard_fetch_langs_desc
	 */
	usbh_billboard_fetch_langs_t fetch_langs;
	/**
	 * @driver_ops_mandatory @copybrief usbh_billboard_use_lang
	 */
	usbh_billboard_use_lang_t use_lang;
};

/**
 * @brief Fetches BOS and parse billboard capabilities descriptors
 *
 * @param dev           Pointer to the device
 * @param billboard_cb  Application callback
 * @param cb_arg        Pointer to callback specific context structure
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_billboard_parse(struct device const *dev, usbh_billboard_cb_t billboard_cb,
				   void *cb_arg);

static inline int z_impl_usbh_billboard_parse(struct device const *dev,
					      usbh_billboard_cb_t billboard_cb, void *cb_arg)
{
	struct usbh_billboard_driver_api const *api = DEVICE_API_GET(usbh_billboard, dev);

	if (api->parse == NULL) {
		return -ENOSYS;
	}
	return api->parse(dev, billboard_cb, cb_arg);
}

/**
 * @brief Fetches string descriptors
 *
 * @param dev           Pointer to the device
 * @param str_idx       String descriptor index
 * @param str_desc      Output argument, double pointer to string descriptor
 * @param len           Length of string descriptor
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_billboard_fetch_string_desc(struct device const *dev, uint8_t str_idx,
					       struct usb_string_descriptor *str_desc, size_t len);

static inline int z_impl_usbh_billboard_fetch_string_desc(struct device const *dev, uint8_t str_idx,
							  struct usb_string_descriptor *str_desc,
							  size_t len)
{
	struct usbh_billboard_driver_api const *api = DEVICE_API_GET(usbh_billboard, dev);

	if (api->fetch_string == NULL) {
		return -ENOSYS;
	}
	return api->fetch_string(dev, str_idx, str_desc, len);
}

/**
 * @brief Fetches the supported languages from the device
 *
 * @param dev           Pointer to the device
 * @param str_desc      Output argument, double pointer to string descriptor
 * @param len           Length of string descriptor
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_billboard_fetch_langs_desc(struct device const *dev,
					      struct usb_string_descriptor *str_desc, size_t len);

static inline int z_impl_usbh_billboard_fetch_langs_desc(struct device const *dev,
							 struct usb_string_descriptor *str_desc,
							 size_t len)
{
	struct usbh_billboard_driver_api const *api = DEVICE_API_GET(usbh_billboard, dev);

	if (api->fetch_langs == NULL) {
		return -ENOSYS;
	}
	return api->fetch_langs(dev, str_desc, len);
}

/**
 * @brief Changes the language used for fetching string descriptors
 *
 * @param dev           Pointer to the device
 * @param lang_code     Language code
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_billboard_use_lang(struct device const *dev, uint16_t lang_code);

static inline int z_impl_usbh_billboard_use_lang(struct device const *dev, uint16_t lang_code)
{
	struct usbh_billboard_driver_api const *api = DEVICE_API_GET(usbh_billboard, dev);

	if (api->use_lang == NULL) {
		return -ENOSYS;
	}
	return api->use_lang(dev, lang_code);
}

#include <zephyr/syscalls/usbh_billboard.h>

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBH_BILLBOARD_H_ */
