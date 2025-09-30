/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_MCTP_I3C_TARGET_H_
#define ZEPHYR_MCTP_I3C_TARGET_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/pmci/mctp/mctp_i3c_common.h>
#include <libmctp.h>

/**
 * @brief An MCTP binding for Zephyr's I3C target interface using GPIO
 */
struct mctp_binding_i3c_target {
	/** @cond INTERNAL_HIDDEN */
	struct mctp_binding binding;
	const struct device *i3c;
	struct i3c_target_config i3c_target_cfg;
	uint8_t endpoint_id;
	struct mctp_pktbuf *tx_pkt;
	struct k_sem *tx_lock;
	struct k_sem *tx_complete;
	struct mctp_pktbuf *rx_pkt;
	/** @endcond INTERNAL_HIDDEN */
};

/** @cond INTERNAL_HIDDEN */
extern const struct i3c_target_callbacks mctp_i3c_target_callbacks;
int mctp_i3c_target_start(struct mctp_binding *binding);
int mctp_i3c_target_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt);
/** @endcond INTERNAL_HIDDEN */

/**
 * @brief Define a MCTP bus binding for I3C target mode using IBI for signaling
 *
 * @param _name Symbolic name of the bus binding variable
 * @param _node_id DeviceTree Node containing the configuration of this MCTP binding
 */
#define MCTP_I3C_TARGET_DT_DEFINE(_name, _node_id)                                                 \
	K_SEM_DEFINE(_name##_tx_lock, 1, 1);                                                       \
	K_SEM_DEFINE(_name##_tx_complete, 0, 1);                                                   \
	struct mctp_binding_i3c_target _name = {                                                   \
		.binding = {                                                                       \
			.name = STRINGIFY(_name), .version = 1,                                    \
			.start = mctp_i3c_target_start,                                            \
			.tx = mctp_i3c_target_tx,                                                  \
			.pkt_size = MCTP_I3C_MAX_PKT_SIZE,                                         \
		},                                                                                 \
		.i3c = DEVICE_DT_GET(DT_PHANDLE(_node_id, i3c)),                                   \
		.i3c_target_cfg = {                                                                \
			.callbacks = &mctp_i3c_target_callbacks,                                   \
		},                                                                                 \
		.endpoint_id = DT_PROP(_node_id, endpoint_id),                                     \
		.tx_lock = &_name##_tx_lock,                                                       \
		.tx_complete = &_name##_tx_complete,                                               \
	};

#endif /* ZEPHYR_MCTP_I3C_TARGET_H_ */
