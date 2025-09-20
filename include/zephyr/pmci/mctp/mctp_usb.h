/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_MCTP_USB_H_
#define ZEPHYR_MCTP_USB_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <libmctp.h>

#define MCTP_USB_HEADER_SIZE       4
#define MCTP_USB_MAX_PACKET_LENGTH 255

/**
 * @brief An MCTP binding for Zephyr's USB device stack
 */
struct mctp_binding_usb {
	/** @cond INTERNAL_HIDDEN */
	struct mctp_binding binding;
	const struct device *usb_dev;
	uint8_t endpoint_id;
	uint8_t tx_buf[MCTP_USB_HEADER_SIZE + MCTP_USB_MAX_PACKET_LENGTH];
	struct k_sem tx_sem;
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

/** @cond INTERNAL_HIDDEN */
int mctp_usb_start(struct mctp_binding *binding);
int mctp_usb_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt);
/** @endcond INTERNAL_HIDDEN */

/**
 * @brief Define a MCTP bus binding for USB
 *
 * @param _name Symbolic name of the bus binding variable
 * @param _dev DeviceTree Node containing the configuration of this MCTP binding
 */
#define MCTP_USB_DT_DEFINE(_name, _dev)                                                            \
	struct mctp_binding_usb _name = {                                                          \
		.binding = {                                                                       \
			.name = STRINGIFY(_name), .version = 1,                                    \
					  .pkt_size =                                              \
						  MCTP_PACKET_SIZE(MCTP_USB_MAX_PACKET_LENGTH),    \
					  .pkt_header = 0, .pkt_trailer = 0,                       \
					  .start = mctp_usb_start, .tx = mctp_usb_tx,              \
			},                                                                         \
			.usb_dev = DEVICE_DT_GET(_dev),                                            \
			.endpoint_id = DT_PROP_OR(_dev, endpoint_id, 0),                           \
			.rx_pkt = NULL,                                                            \
			.rx_data_idx = 0,                                                          \
			.rx_state = STATE_WAIT_HDR_DMTF0};

#endif /* ZEPHYR_MCTP_USB_H_ */
