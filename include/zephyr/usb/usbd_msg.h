/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB support message types and structure
 */

#ifndef ZEPHYR_INCLUDE_USBD_MSG_H_
#define ZEPHYR_INCLUDE_USBD_MSG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup usbd_msg_api USB device core API
 * @ingroup usb
 * @{
 */

/**
 * @brief USB device support message types
 *
 * The first set of message types map to event types from the UDC driver API.
 */
enum usbd_msg_type {
	/** VBUS ready message (optional) */
	USBD_MSG_VBUS_READY,
	/** VBUS removed message (optional) */
	USBD_MSG_VBUS_REMOVED,
	/** Device resume message */
	USBD_MSG_RESUME,
	/** Device suspended message */
	USBD_MSG_SUSPEND,
	/** Bus reset detected */
	USBD_MSG_RESET,
	/** Non-correctable UDC error message  */
	USBD_MSG_UDC_ERROR,
	/** Unrecoverable device stack error message  */
	USBD_MSG_STACK_ERROR,
	/** CDC ACM Line Coding update */
	USBD_MSG_CDC_ACM_LINE_CODING,
	/** CDC ACM Line State update */
	USBD_MSG_CDC_ACM_CONTROL_LINE_STATE,
	/** Maximum number of message types */
	USBD_MSG_MAX_NUMBER,
};


static const char *const usbd_msg_type_list[] = {
	"VBUS ready",
	"VBUS removed",
	"Device resumed",
	"Device suspended",
	"Bus reset",
	"Controller error",
	"Stack error",
	"CDC ACM line coding",
	"CDC ACM control line state",
};

BUILD_ASSERT(ARRAY_SIZE(usbd_msg_type_list) == USBD_MSG_MAX_NUMBER,
	     "Number of entries in usbd_msg_type_list is not equal to USBD_MSG_MAX_NUMBER");

/**
 * @brief USB device message
 */
struct usbd_msg {
	/** Message type */
	enum usbd_msg_type type;
	/** Message status, value or data */
	union {
		int status;
		const struct device *dev;
	};
};

/**
 * @brief Callback type definition for USB device message delivery
 *
 * The implementation uses the system workqueue, and a callback provided and
 * registered by the application. The application callback is called in the
 * context of the system workqueue. Notification messages are stored in a queue
 * and delivered to the callback in sequence.
 *
 * @param[in] msg Pointer to USB device message
 */
typedef void (*usbd_msg_cb_t)(const struct usbd_msg *const msg);

/**
 * @brief Returns the message type as a constant string
 *
 * @param[in] type USBD message type
 *
 * @return Message type as a constant string
 */
static inline const char *usbd_msg_type_string(const enum usbd_msg_type type)
{
	if (type >= 0 && type < USBD_MSG_MAX_NUMBER) {
		return usbd_msg_type_list[type];
	}

	return "?";
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USBD_MSG_H_ */
