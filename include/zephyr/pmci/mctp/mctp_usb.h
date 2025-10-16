/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_MCTP_USB_H_
#define ZEPHYR_MCTP_USB_H_

#include <zephyr/sys/iterable_sections.h>
#include <libmctp.h>

/* MCTP class subclass options */
#define USBD_MCTP_SUBCLASS_MANAGEMENT_CONTROLLER   0
#define USBD_MCTP_SUBCLASS_MANAGED_DEVICE_ENDPOINT 0
#define USBD_MCTP_SUBCLASS_HOST_INTERFACE_ENDPOINT 1

/* MCTP class protocol options */
#define USBD_MCTP_PROTOCOL_1_X 1
#define USBD_MCTP_PROTOCOL_2_X 2

/* Binding-specific defines, internal use */
#define MCTP_USB_HEADER_SIZE       4
#define MCTP_USB_MAX_PACKET_LENGTH 255

/**
 * @brief An MCTP binding for Zephyr's USB device stack
 */
struct mctp_binding_usb {
	/** @cond INTERNAL_HIDDEN */
	struct mctp_binding binding;
	struct usbd_class_data *usb_class_data;
	uint8_t tx_buf[MCTP_USB_HEADER_SIZE + MCTP_USB_MAX_PACKET_LENGTH];
	struct k_sem tx_lock;
	struct mctp_pktbuf *rx_pkt;
	uint8_t rx_data_idx;
	enum {
		STATE_WAIT_HDR_DMTF0,
		STATE_WAIT_HDR_DMTF1,
		STATE_WAIT_HDR_RSVD0,
		STATE_WAIT_HDR_LEN,
		STATE_DATA
	} rx_state;
	/** @endcond INTERNAL_HIDDEN */
};

struct mctp_usb_class_inst {
	uint8_t sublcass;
	uint8_t mctp_protocol;
	struct mctp_binding_usb *mctp_binding;
};

/** @cond INTERNAL_HIDDEN */
int mctp_usb_start(struct mctp_binding *binding);
int mctp_usb_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt);
/** @endcond INTERNAL_HIDDEN */

/**
 * @brief Define a MCTP bus binding for USB
 *
 * @param _name Symbolic name of the bus binding variable
 * @param _subclass MCTP subclass used in the USB interfce descriptor
 * @param _protocol MCTP protocol used in the USB interface descriptor
 */
#define MCTP_USB_DEFINE(_name, _subclass, _protocol)						\
	struct mctp_binding_usb _name = {							\
		.binding = {									\
			.name = STRINGIFY(_name),						\
			.version = 1,								\
			.pkt_size = MCTP_PACKET_SIZE(MCTP_USB_MAX_PACKET_LENGTH),		\
			.pkt_header = 0,							\
			.pkt_trailer = 0,							\
			.start = mctp_usb_start,						\
			.tx = mctp_usb_tx							\
		},										\
		.usb_class_data = NULL,								\
		.rx_pkt = NULL,									\
		.rx_data_idx = 0,								\
		.rx_state = STATE_WAIT_HDR_DMTF0						\
	};											\
												\
	const STRUCT_SECTION_ITERABLE(mctp_usb_class_inst, mctp_usb_class_inst_##_name) = {	\
		.sublcass = _subclass,								\
		.mctp_protocol = _protocol,							\
		.mctp_binding = &_name,								\
	};

#endif /* ZEPHYR_MCTP_USB_H_ */
