/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018,2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB HID Class device API header
 */

#ifndef ZEPHYR_INCLUDE_USB_HID_CLASS_DEVICE_H_
#define ZEPHYR_INCLUDE_USB_HID_CLASS_DEVICE_H_

#include <usb/class/hid.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief usb_hid.h API
 * @defgroup usb_hid_class USB HID class API
 * @{
 * @}
 */

/**
 * @defgroup usb_hid_device_api HID class USB specific definitions
 * @ingroup usb_hid_class
 * @{
 */

typedef int (*hid_cb_t)(const struct device *dev,
			struct usb_setup_packet *setup, int32_t *len,
			uint8_t **data);
typedef void (*hid_int_ready_callback)(const struct device *dev);
typedef void (*hid_protocol_cb_t)(const struct device *dev, uint8_t protocol);
typedef void (*hid_idle_cb_t)(const struct device *dev, uint16_t report_id);

/**
 * @brief USB HID device interface
 */
struct hid_ops {
	hid_cb_t get_report;
	hid_cb_t get_idle;
	hid_cb_t get_protocol;
	hid_cb_t set_report;
	hid_cb_t set_idle;
	hid_cb_t set_protocol;
	hid_protocol_cb_t protocol_change;
	hid_idle_cb_t on_idle;
	/*
	 * int_in_ready is an optional callback that is called when
	 * the current interrupt IN transfer has completed.  This can
	 * be used to wait for the endpoint to go idle or to trigger
	 * the next transfer.
	 */
	hid_int_ready_callback int_in_ready;
#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
	hid_int_ready_callback int_out_ready;
#endif
};

/**
 * @brief Register HID device
 *
 * @param[in]  dev          Pointer to USB HID device
 * @param[in]  desc         Pointer to HID report descriptor
 * @param[in]  size         Size of HID report descriptor
 * @param[in]  op           Pointer to USB HID device interrupt struct
 */
void usb_hid_register_device(const struct device *dev,
			     const uint8_t *desc,
			     size_t size,
			     const struct hid_ops *op);

/**
 * @brief Write to USB HID interrupt endpoint buffer
 *
 * @param[in]  dev          Pointer to USB HID device
 * @param[in]  data         Pointer to data buffer
 * @param[in]  data_len     Length of data to copy
 * @param[out] bytes_ret    Bytes written to the EP buffer.
 *
 * @return 0 on success, negative errno code on fail.
 */
int hid_int_ep_write(const struct device *dev,
		     const uint8_t *data,
		     uint32_t data_len,
		     uint32_t *bytes_ret);

/**
 * @brief Read from USB HID interrupt endpoint buffer
 *
 * @param[in]  dev          Pointer to USB HID device
 * @param[in]  data         Pointer to data buffer
 * @param[in]  max_data_len Max length of data to copy
 * @param[out] ret_bytes    Number of bytes to copy.  If data is NULL and
 *                          ret_bytes is 0 the number of bytes
 *                          available in the buffer will be returned.
 *
 * @return 0 on success, negative errno code on fail.
 */
int hid_int_ep_read(const struct device *dev,
		    uint8_t *data,
		    uint32_t max_data_len,
		    uint32_t *ret_bytes);

/**
 * @brief Initialize USB HID class support
 *
 * @param[in]  dev          Pointer to USB HID device
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_hid_init(const struct device *dev);

/**
 * @}
 */

/*
 * Macros below are deprecated in 2.6 release.
 * Please replace with macros from common HID header, include/usb/class/hid.h
 */

#define HID_CLASS_DESCRIPTOR_HID	__DEPRECATED_MACRO USB_DESC_HID
#define HID_CLASS_DESCRIPTOR_REPORT	__DEPRECATED_MACRO USB_DESC_HID_REPORT

#define HID_GET_REPORT			__DEPRECATED_MACRO USB_HID_GET_REPORT
#define HID_GET_IDLE			__DEPRECATED_MACRO USB_HID_GET_IDLE
#define HID_GET_PROTOCOL		__DEPRECATED_MACRO USB_HID_GET_PROTOCOL
#define HID_SET_REPORT			__DEPRECATED_MACRO USB_HID_SET_REPORT
#define HID_SET_IDLE			__DEPRECATED_MACRO USB_HID_SET_IDLE
#define HID_SET_PROTOCOL		__DEPRECATED_MACRO USB_HID_SET_PROTOCOL

#define ITEM_MAIN			__DEPRECATED_MACRO 0x0
#define ITEM_GLOBAL			__DEPRECATED_MACRO 0x1
#define ITEM_LOCAL			__DEPRECATED_MACRO 0x2

#define ITEM_TAG_INPUT			__DEPRECATED_MACRO 0x8
#define ITEM_TAG_OUTPUT			__DEPRECATED_MACRO 0x9
#define ITEM_TAG_COLLECTION		__DEPRECATED_MACRO 0xA
#define ITEM_TAG_COLLECTION_END		__DEPRECATED_MACRO 0xC

#define ITEM_TAG_USAGE_PAGE		__DEPRECATED_MACRO 0x0
#define ITEM_TAG_LOGICAL_MIN		__DEPRECATED_MACRO 0x1
#define ITEM_TAG_LOGICAL_MAX		__DEPRECATED_MACRO 0x2
#define ITEM_TAG_REPORT_SIZE		__DEPRECATED_MACRO 0x7
#define ITEM_TAG_REPORT_ID		__DEPRECATED_MACRO 0x8
#define ITEM_TAG_REPORT_COUNT		__DEPRECATED_MACRO 0x9

#define ITEM_TAG_USAGE			__DEPRECATED_MACRO 0x0
#define ITEM_TAG_USAGE_MIN		__DEPRECATED_MACRO 0x1
#define ITEM_TAG_USAGE_MAX		__DEPRECATED_MACRO 0x2

#define HID_MAIN_ITEM(bTag, bSize)	\
	__DEPRECATED_MACRO HID_ITEM(bTag, ITEM_MAIN, bSize)
#define HID_GLOBAL_ITEM(bTag, bSize)	\
	__DEPRECATED_MACRO HID_ITEM(bTag, ITEM_GLOBAL, bSize)
#define HID_LOCAL_ITEM(bTag, bSize)	\
	__DEPRECATED_MACRO HID_ITEM(bTag, ITEM_LOCAL, bSize)

#define HID_MI_COLLECTION		\
	__DEPRECATED_MACRO HID_MAIN_ITEM(ITEM_TAG_COLLECTION, 1)
#define HID_MI_COLLECTION_END		\
	__DEPRECATED_MACRO HID_MAIN_ITEM(ITEM_TAG_COLLECTION_END, 0)
#define HID_MI_INPUT			\
	__DEPRECATED_MACRO HID_MAIN_ITEM(ITEM_TAG_INPUT, 1)
#define HID_MI_OUTPUT			\
	__DEPRECATED_MACRO HID_MAIN_ITEM(ITEM_TAG_OUTPUT, 1)
#define HID_GI_USAGE_PAGE		\
	__DEPRECATED_MACRO HID_GLOBAL_ITEM(ITEM_TAG_USAGE_PAGE, 1)
#define HID_GI_LOGICAL_MIN(size)	\
	__DEPRECATED_MACRO HID_GLOBAL_ITEM(ITEM_TAG_LOGICAL_MIN, size)
#define HID_GI_LOGICAL_MAX(size)	\
	__DEPRECATED_MACRO HID_GLOBAL_ITEM(ITEM_TAG_LOGICAL_MAX, size)
#define HID_GI_REPORT_SIZE		\
	__DEPRECATED_MACRO HID_GLOBAL_ITEM(ITEM_TAG_REPORT_SIZE, 1)
#define HID_GI_REPORT_ID		\
	__DEPRECATED_MACRO HID_GLOBAL_ITEM(ITEM_TAG_REPORT_ID, 1)
#define HID_GI_REPORT_COUNT		\
	__DEPRECATED_MACRO HID_GLOBAL_ITEM(ITEM_TAG_REPORT_COUNT, 1)

#define HID_LI_USAGE			\
	__DEPRECATED_MACRO HID_LOCAL_ITEM(ITEM_TAG_USAGE, 1)
#define HID_LI_USAGE_MIN(size)		\
	__DEPRECATED_MACRO HID_LOCAL_ITEM(ITEM_TAG_USAGE_MIN, size)
#define HID_LI_USAGE_MAX(size)		\
	__DEPRECATED_MACRO HID_LOCAL_ITEM(ITEM_TAG_USAGE_MAX, size)

#define USAGE_GEN_DESKTOP		__DEPRECATED_MACRO 0x01
#define USAGE_GEN_KEYBOARD		__DEPRECATED_MACRO 0x07
#define USAGE_GEN_LEDS			__DEPRECATED_MACRO 0x08
#define USAGE_GEN_BUTTON		__DEPRECATED_MACRO 0x09

#define USAGE_GEN_DESKTOP_UNDEFINED	__DEPRECATED_MACRO 0x00
#define USAGE_GEN_DESKTOP_POINTER	__DEPRECATED_MACRO 0x01
#define USAGE_GEN_DESKTOP_MOUSE		__DEPRECATED_MACRO 0x02
#define USAGE_GEN_DESKTOP_JOYSTICK	__DEPRECATED_MACRO 0x04
#define USAGE_GEN_DESKTOP_GAMEPAD	__DEPRECATED_MACRO 0x05
#define USAGE_GEN_DESKTOP_KEYBOARD	__DEPRECATED_MACRO 0x06
#define USAGE_GEN_DESKTOP_KEYPAD	__DEPRECATED_MACRO 0x07
#define USAGE_GEN_DESKTOP_X		__DEPRECATED_MACRO 0x30
#define USAGE_GEN_DESKTOP_Y		__DEPRECATED_MACRO 0x31
#define USAGE_GEN_DESKTOP_WHEEL		__DEPRECATED_MACRO 0x38

#define COLLECTION_PHYSICAL		__DEPRECATED_MACRO 0x00
#define COLLECTION_APPLICATION		__DEPRECATED_MACRO 0x01

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_HID_CLASS_DEVICE_H_ */
