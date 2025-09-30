/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_MCTP_I3C_CONTROLLER_H_
#define ZEPHYR_MCTP_I3C_CONTROLLER_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/pmci/mctp/mctp_i3c_common.h>

#include <libmctp.h>
#include <stdatomic.h>

/** @cond INTERNAL_HIDDEN */

struct mctp_binding_i3c_controller;

struct mctp_i3c_endpoint {
	struct i3c_device_desc i3c_device;
	struct mctp_binding_i3c_controller *binding;
};

/** @endcond INTERNAL_HIDDEN */


/**
 * @brief An MCTP binding for Zephyr's I3C interface using IBI interrupts for signaling
 */
struct mctp_binding_i3c_controller {
	/** @cond INTERNAL_HIDDEN */
	struct mctp_binding binding;
	const struct device *i3c;

	uint8_t rx_buf_len;
	uint8_t rx_buf[MCTP_I3C_MAX_PKT_SIZE];
	uint8_t tx_buf[MCTP_I3C_MAX_PKT_SIZE];

	struct mctp_pktbuf *rx_pkt;

	const size_t num_endpoints;
	const uint8_t *endpoint_ids;
	const uint8_t *endpoint_addrs;
	struct mctp_i3c_endpoint *endpoints;

	/** @endcond INTERNAL_HIDDEN */
};

/** @cond INTERNAL_HIDDEN */
int mctp_i3c_controller_start(struct mctp_binding *binding);
int mctp_i3c_controller_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt);

#define MCTP_I3C_CONTROLLER_DEFINE_IDS(_node_id, _name)                                        \
	const uint8_t _name##_endpoint_ids[] = DT_PROP(_node_id, endpoint_ids);

#define MCTP_I3C_CONTROLLER_DEFINE_I3C_ADDRESSES(_node_id, _name)                              \
	const uint8_t _name##_endpoint_addrs[] = DT_PROP(_node_id, endpoint_addrs);

#define MCTP_I3C_CONTROLLER_DEFINE_I3C_ENDPOINTS(_node_id, _name)                              \
	struct mctp_i3c_endpoint \
		_name##_endpoints[DT_PROP_LEN(_node_id, endpoint_ids)]

/** @endcond INTERNAL_HIDDEN */

/**
 * @brief Define a MCTP bus binding for i3c controller
 *
 * This bus binding make use of IBI interrupt signaling from targets to signal their desire
 * to send a message. The binding specification (dsp0233 v1.0.1) offers alternative modes of
 * operaton such as polling or directly reading but these are not implemented.
 *
 * @param _name Symbolic name of the bus binding variable
 * @param _node_id DeviceTree Node containing the configuration of this MCTP binding
 */
#define MCTP_I3C_CONTROLLER_DT_DEFINE(_name, _node_id)                                        \
	MCTP_I3C_CONTROLLER_DEFINE_IDS(_node_id, _name);                                      \
	MCTP_I3C_CONTROLLER_DEFINE_I3C_ENDPOINTS(_node_id, _name);                            \
	MCTP_I3C_CONTROLLER_DEFINE_I3C_ADDRESSES(_node_id, _name);                            \
	struct mctp_binding_i3c_controller _name = {                                          \
		.binding = {                                                                  \
			.name = STRINGIFY(_name), .version = 1,                               \
			.start = mctp_i3c_controller_start,                                   \
			.tx = mctp_i3c_controller_tx,                                         \
			.pkt_size = MCTP_I3C_MAX_PKT_SIZE,                                    \
		},                                                                            \
		.i3c = DEVICE_DT_GET(DT_PHANDLE(_node_id, i3c)),                              \
		.num_endpoints = DT_PROP_LEN(_node_id, endpoint_ids),                         \
		.endpoint_ids = _name##_endpoint_ids,                                         \
		.endpoint_addrs = _name##_endpoint_addrs,                                     \
		.endpoints = _name##_endpoints,                                               \
	};

#endif /* ZEPHYR_MCTP_I3C_CONTROLLER_H_ */
