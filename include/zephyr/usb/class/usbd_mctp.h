/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB MCTP over USB Protocol Endpoint Device Class (MCTP) public header
 *
 * This header describes MCTP class interactions desgined to be used by libMCTP.
 * The MCTP device itself is modelled with devicetree zephyr,mctp-usb compatible.
 *
 * This API is currently considered experimental.
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_MCTP_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_MCTP_H_

/**
 * @brief USB MCTP device API
 * @defgroup usbd_mctp USB MCTP device API
 * @ingroup usb
 * @since 4.2
 * @version 0.1.0
 * @{
 */

/**
 * @brief USB MCTP application event handlers
 */
struct mctp_ops {
	/**
	 * @brief Data received
	 *
	 * This function is called after the USB has received a packet of data, up to max packet
	 * size. The MCTP USB binding for libMCTP is responsible for copying the data from the USB
	 * buffer and packetizing the data for the MCTP core. This callback is mandatory.
	 *
	 * @param dev MCTP USB device instance
	 * @param buf Buffer containing USB packet data
	 * @param size Number of bytes in the buffer
	 * @param user_data Opaque user data pointer
	 */
	void (*data_recv_cb)(const struct device *dev, void *buf, uint16_t size, void *user_data);
	/**
	 * @brief TX complete
	 *
	 * This function is called after the USB has completed sending data to the MCTP bus owner.
	 * This function is optional in general, but required if using the libMCTP USB binding.
	 *
	 * @param dev MCTP USB device instance
	 * @param buf Buffer containing USB packet data
	 * @param size Number of bytes in the buffer
	 * @param user_data Opaque user data pointer
	 */
	void (*tx_complete_cb)(const struct device *dev, void *user_data);
};

/**
 * @brief Register MCTP USB application callbacks.
 *
 * @param dev MCTP USB device instance
 * @param ops MCTP callback structure
 * @param user_data Opaque user data to pass to ops callbacks
 */
void usbd_mctp_set_ops(const struct device *dev, const struct mctp_ops *ops, void *user_data);

/**
 * @brief Send data to the MCTP bus owner.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in
 * case of an error the caller retains the ownership of the buffer.
 *
 * @param dev MCTP USB device instance
 * @param buf Buffer containing outgoing data
 * @param size Number of bytes to send
 *
 * @return 0 on success, negative value on error
 */
int usbd_mctp_send(const struct device *dev, void *buf, uint16_t size);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_MCTP_H_ */
