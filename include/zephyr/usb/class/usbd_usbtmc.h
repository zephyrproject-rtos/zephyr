/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USBD USBTMC device class API header
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_USBTMC_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_USBTMC_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USBD USBTMC device class API
 * @defgroup usbd_usbtmc USBD USBTMC device class API
 * @ingroup usb
 * @since 4.5
 * @version 0.1.0
 * @see usbtmc: "Universal Serial Bus Test and Measurement Class Specification
 *              (USBTMC)" Revision 1.0 (April 14, 2003)
 * @{
 */

/**
 * @brief USBTMC application event handlers
 *
 * The USBTMC class implements the transport only, interpretation of the
 * device dependent message payload, typically IEEE 488.2 or SCPI commands,
 * is left to the application. All callbacks are invoked from the USB device
 * stack thread and must not block.
 */
struct usbd_usbtmc_ops {
	/**
	 * @brief Interface ready event handler
	 *
	 * This callback is called with the ready argument set to true when
	 * the interface is part of the active configuration and the device
	 * can exchange USBTMC messages with the host, and with the argument
	 * set to false when the interface is no longer active. This callback
	 * is optional.
	 */
	void (*ready)(const struct device *dev, const bool ready);

	/**
	 * @brief Message data received event handler
	 *
	 * Device dependent message data received from the host. Messages are
	 * delivered in chunks, the begin argument marks the first chunk of a
	 * new message and the eom argument marks the last chunk of a message.
	 * The data buffer is only valid during the execution of the callback,
	 * the application must copy the data if it needs it later.
	 */
	void (*msg_out)(const struct device *dev, const uint8_t *const data,
			const size_t len, const bool begin, const bool eom);

	/**
	 * @brief Message data sent event handler
	 *
	 * Message data submitted with usbd_usbtmc_msg_write() has been sent
	 * to the host and buffer space is available again. This callback is
	 * optional, the application can use it to submit further data of a
	 * message that does not fit into the buffers at once.
	 */
	void (*msg_in_done)(const struct device *dev);

	/**
	 * @brief Interface clear event handler
	 *
	 * The host initiated a clear of the interface using the INITIATE_CLEAR
	 * request. All buffered message data has been discarded and the
	 * application must reset any message parser state. This callback is
	 * optional.
	 */
	void (*clear)(const struct device *dev);

	/**
	 * @brief Indicator pulse request handler
	 *
	 * The host requested an indicator pulse. If provided, the device
	 * reports INDICATOR_PULSE support in the GET_CAPABILITIES response
	 * and the application should turn on an indicator, for example an
	 * LED, for a human recognizable amount of time. If not provided,
	 * the request is not supported. This callback is optional.
	 */
	int (*indicator_pulse)(const struct device *dev);
};

/**
 * @brief Register USBTMC application event handlers
 *
 * The application must register the event handlers before the USB device
 * support is initialized and enabled.
 *
 * @param[in] dev Pointer to USBTMC device
 * @param[in] ops Pointer to USBTMC application event handlers
 *
 * @return 0 on success, negative errno code on failure.
 */
int usbd_usbtmc_register(const struct device *dev,
			 const struct usbd_usbtmc_ops *const ops);

/**
 * @brief Write device dependent message data
 *
 * Submit device dependent message data to be sent to the host in response to
 * a REQUEST_DEV_DEP_MSG_IN transfer. A message can be submitted in multiple
 * chunks, the end argument marks the end of the message. The data is copied
 * to internal buffers and the call does not block. If there is not enough
 * buffer space, less than len bytes are copied and the application should
 * submit the remaining data, and the end of message flag, again after the
 * msg_in_done() callback.
 *
 * @param[in] dev  Pointer to USBTMC device
 * @param[in] data Pointer to message data
 * @param[in] len  Length of message data
 * @param[in] end  End of message. Only taken into account when all len bytes
 *                 are copied.
 *
 * @return Number of bytes copied on success, negative errno code on failure.
 * @retval -EACCES if the interface is not enabled by the host
 */
int usbd_usbtmc_msg_write(const struct device *dev, const uint8_t *const data,
			  const size_t len, const bool end);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_USBTMC_H_ */
