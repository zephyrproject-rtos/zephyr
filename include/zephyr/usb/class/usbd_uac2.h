/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Audio Class 2 device public header
 *
 * This header describes only class API interaction with application.
 * The audio device itself is modelled with devicetree zephyr,uac2 compatible.
 *
 * This API is currently considered experimental.
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_UAC2_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_UAC2_H_

#include <zephyr/device.h>

/**
 * @brief USB Audio Class 2 device API
 * @defgroup uac2_device USB Audio Class 2 device API
 * @ingroup usb
 * @{
 */

/**
 * @brief Get entity ID
 *
 * @param node node identifier
 */
#define UAC2_ENTITY_ID(node)							\
	({									\
		BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_PARENT(node), zephyr_uac2));	\
		UTIL_INC(DT_NODE_CHILD_IDX(node));				\
	})

/**
 * @brief USB Audio 2 application event handlers
 */
struct uac2_ops {
	/**
	 * @brief Start of Frame callback
	 *
	 * Notifies application about SOF event on the bus.
	 *
	 * @param dev USB Audio 2 device
	 * @param user_data Opaque user data pointer
	 */
	void (*sof_cb)(const struct device *dev, void *user_data);
	/**
	 * @brief Terminal update callback
	 *
	 * Notifies application that host has enabled or disabled a terminal.
	 *
	 * @param dev USB Audio 2 device
	 * @param terminal Terminal ID linked to AudioStreaming interface
	 * @param enabled True if host enabled terminal, False otherwise
	 * @param microframes True if USB connection speed uses microframes
	 * @param user_data Opaque user data pointer
	 */
	void (*terminal_update_cb)(const struct device *dev, uint8_t terminal,
				   bool enabled, bool microframes,
				   void *user_data);
	/**
	 * @brief Get receive buffer address
	 *
	 * USB stack calls this function to obtain receive buffer address for
	 * AudioStreaming interface. The buffer is owned by USB stack until
	 * @ref data_recv_cb callback is called. The buffer must be sufficiently
	 * aligned and otherwise suitable for use by UDC driver.
	 *
	 * @param dev USB Audio 2 device
	 * @param terminal Input Terminal ID linked to AudioStreaming interface
	 * @param size Maximum number of bytes USB stack will write to buffer.
	 * @param user_data Opaque user data pointer
	 */
	void *(*get_recv_buf)(const struct device *dev, uint8_t terminal,
			      uint16_t size, void *user_data);
	/**
	 * @brief Data received
	 *
	 * This function releases buffer obtained in @ref get_recv_buf after USB
	 * has written data to the buffer and/or no longer needs it.
	 *
	 * @param dev USB Audio 2 device
	 * @param terminal Input Terminal ID linked to AudioStreaming interface
	 * @param buf Buffer previously obtained via @ref get_recv_buf
	 * @param size Number of bytes written to buffer
	 * @param user_data Opaque user data pointer
	 */
	void (*data_recv_cb)(const struct device *dev, uint8_t terminal,
			     void *buf, uint16_t size, void *user_data);
	/**
	 * @brief Transmit buffer release callback
	 *
	 * This function releases buffer provided in @ref usbd_uac2_send when
	 * the class no longer needs it.
	 *
	 * @param dev USB Audio 2 device
	 * @param terminal Output Terminal ID linked to AudioStreaming interface
	 * @param buf Buffer previously provided via @ref usbd_uac2_send
	 * @param user_data Opaque user data pointer
	 */
	void (*buf_release_cb)(const struct device *dev, uint8_t terminal,
			       void *buf, void *user_data);
	/**
	 * @brief Get Explicit Feedback value
	 *
	 * Explicit feedback value format depends terminal connection speed.
	 * If device is High-Speed capable, it must use Q16.16 format if and
	 * only if the @ref terminal_update_cb was called with microframes
	 * parameter set to true. On Full-Speed only devices, or if High-Speed
	 * capable device is operating at Full-Speed (microframes was false),
	 * the format is Q10.14 stored on 24 least significant bits (i.e. 8 most
	 * significant bits are ignored).
	 *
	 * @param dev USB Audio 2 device
	 * @param terminal Input Terminal ID whose feedback should be returned
	 * @param user_data Opaque user data pointer
	 */
	uint32_t (*feedback_cb)(const struct device *dev, uint8_t terminal,
				void *user_data);
	/**
	 * @brief Get active sample rate
	 *
	 * USB stack calls this function when the host asks for active sample
	 * rate if the Clock Source entity supports more than one sample rate.
	 * This function won't ever be called (should be NULL) if all Clock
	 * Source entities support only one sample rate.
	 *
	 * @param dev USB Audio 2 device
	 * @param clock_id Clock Source ID whose sample rate should be returned
	 * @param user_data Opaque user data pointer
	 *
	 * @return Active sample rate in Hz
	 */
	uint32_t (*get_sample_rate)(const struct device *dev, uint8_t clock_id,
				    void *user_data);
	/**
	 * @brief Set active sample rate
	 *
	 * USB stack calls this function when the host sets active sample rate.
	 * This callback may be NULL if all Clock Source entities have only one
	 * sample rate. USB stack sanitizes the sample rate to closest valid
	 * rate for given Clock Source entity.
	 *
	 * @param dev USB Audio 2 device
	 * @param clock_id Clock Source ID whose sample rate should be set
	 * @param rate Sample rate in Hz
	 * @param user_data Opaque user data pointer
	 *
	 * @return 0 on success, negative value on error
	 */
	int (*set_sample_rate)(const struct device *dev, uint8_t clock_id,
			       uint32_t rate, void *user_data);
};

/**
 * @brief Register USB Audio 2 application callbacks.
 *
 * @param dev USB Audio 2 device instance
 * @param ops USB Audio 2 callback structure
 * @param user_data Opaque user data to pass to ops callbacks
 */
void usbd_uac2_set_ops(const struct device *dev,
		       const struct uac2_ops *ops, void *user_data);

/**
 * @brief Send audio data to output terminal
 *
 * Data buffer must be sufficiently aligned and otherwise suitable for use by
 * UDC driver.
 *
 * @param dev USB Audio 2 device
 * @param terminal Output Terminal ID linked to AudioStreaming interface
 * @param data Buffer containing outgoing data
 * @param size Number of bytes to send
 *
 * @return 0 on success, negative value on error
 */
int usbd_uac2_send(const struct device *dev, uint8_t terminal,
		   void *data, uint16_t size);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_UAC2_H_ */
