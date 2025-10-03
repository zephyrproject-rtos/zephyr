/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MCTP over USB Protocol Endpoint Device Class public header
 *
 * This API is currently considered experimental.
 */

#include <zephyr/sys/iterable_sections.h>

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

/* MCTP class subclass options */
#define USBD_MCTP_SUBCLASS_MANAGEMENT_CONTROLLER   0
#define USBD_MCTP_SUBCLASS_MANAGED_DEVICE_ENDPOINT 0
#define USBD_MCTP_SUBCLASS_HOST_INTERFACE_ENDPOINT 1

/* MCTP class protocol options */
#define USBD_MCTP_PROTOCOL_1_X 1
#define USBD_MCTP_PROTOCOL_2_X 2

struct usbd_mctp_inst {
	const char *name;
	uint8_t sublcass;
	uint8_t mctp_protocol;
	void *priv;
	void (*data_recv_cb)(const void *const buf,
			     const uint16_t size,
			     const void *const priv);
	void (*error_cb)(const int err,
			 const uint8_t ep,
			 const void *const priv);
};

/**
 * @brief Define USB MCTP instance
 *
 * Use this macro to create set the parameters of an MCTP instance.
 *
 * @param id Identifier by which the linker sorts registered instances
 * @param subclass MCTP subclass used in the USB interfce descriptor
 * @param protocol MCTP protocol used in the USB interface descriptor
 * @param private Opaque private/user data
 * @param recv_cb Data received callback
 * @param err_cb Error callback
 */
#define USBD_DEFINE_MCTP_INSTANCE(id, subclass, protocol, private, recv_cb, err_cb)	\
	const STRUCT_SECTION_ITERABLE(usbd_mctp_inst, usbd_mctp_inst_##id) = {		\
		.name = STRINGIFY(id),							\
		.sublcass = subclass,							\
		.mctp_protocol = protocol,						\
		.priv = private,							\
		.data_recv_cb = recv_cb,						\
		.error_cb = err_cb							\
	}

/**
 * @brief Send data to the MCTP bus owner.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in
 * case of an error the caller retains the ownership of the buffer.
 *
 * @param inst USBD MCTP instance to send data through
 * @param buf Buffer containing outgoing data
 * @param size Number of bytes to send
 *
 * @return 0 on success, negative value on error
 */
int usbd_mctp_send(const struct usbd_mctp_inst *const inst,
		   const void *const buf,
		   const uint16_t size);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_MCTP_H_ */
