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
#include <zephyr/pmci/mctp/mctp_i3c_common.h>
#include <libmctp.h>

/**
 * @brief An MCTP binding for Zephyr's I3C interface using IBI interrupts for signaling
 */
struct mctp_binding_i3c_controller {
	/** @cond INTERNAL_HIDDEN */
	struct mctp_binding binding;
	size_t num_endpoints;
	const struct device **devices;
	uint8_t *endpoint_ids;
	struct i3c_device_desc **endpoint_i3c_devs;
	enum mctp_i3c_endpoint_state *endpoint_states;
	size_t rx_buf_len;
	uint8_t *rx_buf;
	/** @endcond INTERNAL_HIDDEN */
};

/** @cond INTERNAL_HIDDEN */
int mctp_i3c_controller_start(struct mctp_binding *binding);
int mctp_i3c_controller_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt);
void mctp_i3c_controller_ibi_cb(struct i3c_device_desc *target, struct i3c_ibi_payload *payload);

#define MCTP_I3C_ENDPOINT_IDS(_node_id, _name)					\
	static uint8_t _name##_endpoint_ids[DT_PROP_LEN(_node_id, endpoints)]	\
		= DT_PROP(_node_id, endpoint_ids)

#define MCTP_I3C_ENDPOINT_DEVICE(_idx, _node_id, ...)				\
	DEVICE_DT_GET_BY_IDX(_node_id, endpoints, _idx)

#define MCTP_I3C_CONTROLLER_DEFINE_DEVICES(_node_id, _name)			\
	const struct device *_name##_endpoints[] = {				\
		LISTIFY(DT_PROP_LEN(_node_id, endpoints),			\
			MCTP_I3C_ENDPOINT_DEVICE, (,), _node_id)		\
	}

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
#define MCTP_I3C_CONTROLLER_DT_DEFINE(_name, _node_id)						\
	MCTP_I3C_CONTROLLER_DEFINE_DEVICES(_node_id, _name);					\
	MCTP_I3C_ENDPOINT_IDS(_node_id, _name);							\
	static struct i3c_device_desc								\
		*_name##_endpoint_i3c_devs[DT_PROP_LEN(_node_id, endpoints)];			\
	static struct mctp_binding_i3c_controller _name = {					\
		.binding = {									\
			.name = STRINGIFY(_name), .version = 1,					\
			.start = mctp_i3c_controller_start,					\
			.tx = mctp_i3c_controller_tx,						\
			.pkt_size = MCTP_I3C_MAX_PKT_SIZE,					\
		},										\
		.num_endpoints = DT_PROP_LEN(_node_id, endpoints),				\
		.devices = _name##_endpoints,							\
		.endpoint_ids = _name##_endpoint_ids,						\
		.endpoint_i3c_devs = _name##_endpoint_i3c_devs,					\
	};

#endif /* ZEPHYR_MCTP_I3C_CONTROLLER_H_ */
