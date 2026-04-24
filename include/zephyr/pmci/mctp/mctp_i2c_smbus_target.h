/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PMCI_MCTP_I2C_SMBUS_TARGET_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PMCI_MCTP_I2C_SMBUS_TARGET_H_

/**
 * @file
 * @brief MCTP over I2C SMBus target binding API.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include <libmctp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SMBus command code used for MCTP over I2C binding.
 */
#define MCTP_SMBUS_CMD_CODE 0x0f

/**
 * @brief Length of the SMBus source address field.
 */
#define MCTP_I2C_SMBUS_SRC_ADDR_LEN 1U

/**
 * @brief Maximum MCTP payload size carried by one SMBus block transaction.
 */
#define MCTP_I2C_SMBUS_MAX_MCTP_BYTES \
	(CONFIG_MCTP_I2C_SMBUS_BLOCK_MAX - MCTP_I2C_SMBUS_SRC_ADDR_LEN)

/**
 * @brief MCTP I2C SMBus target binding context.
 */
struct mctp_binding_i2c_smbus_target {
	/** Base MCTP binding object. */
	struct mctp_binding binding;

	/** I2C controller device used by this binding. */
	const struct device *i2c;

	/** I2C target configuration registered with the controller. */
	struct i2c_target_config i2c_target_cfg;

	/** Local MCTP endpoint identifier. */
	uint8_t endpoint_id;

	/** Local I2C target address used by this endpoint. */
	uint8_t ep_i2c_addr;

	/** Remote I2C address used when transmitting responses. */
	uint8_t remote_i2c_addr;

	/** Current receive state for SMBus block transaction parsing. */
	uint8_t rx_state;

	/** Received SMBus command code. */
	uint8_t rx_cmd;

	/** Expected SMBus block byte count. */
	uint8_t rx_count;

	/** Current receive buffer index. */
	uint8_t rx_idx;

	/** Receive buffer for one SMBus block transaction. */
	uint8_t rx_buf[CONFIG_MCTP_I2C_SMBUS_BLOCK_MAX];

	/** Lock protecting transmit state. */
	struct k_sem *tx_lock;

	/** Work item used to transmit pending response data. */
	struct k_work tx_work;

	/** True when a response transmit operation is pending. */
	bool tx_pending;

	/** Length of data pending in @ref tx_buf. */
	uint8_t tx_len;

	/** Transmit buffer for response payload. */
	uint8_t tx_buf[CONFIG_MCTP_I2C_SMBUS_BLOCK_MAX];
};

/**
 * @brief Start an MCTP I2C SMBus target binding.
 *
 * This registers the endpoint as an I2C target and prepares it to receive
 * MCTP packets carried over SMBus block transactions.
 *
 * @param binding Base MCTP binding object.
 *
 * @retval 0 Binding started successfully.
 * @retval -errno Failed to start the binding.
 */
int mctp_i2c_smbus_target_start(struct mctp_binding *binding);

/**
 * @brief Transmit an MCTP packet over an I2C SMBus target binding.
 *
 * The packet is transmitted using controller-mode I2C access on the same
 * controller that is normally registered as an I2C target.
 *
 * @param binding Base MCTP binding object.
 * @param pkt Packet buffer to transmit.
 *
 * @retval 0 Packet queued or transmitted successfully.
 * @retval -errno Failed to transmit the packet.
 */
int mctp_i2c_smbus_target_tx(struct mctp_binding *binding,
			     struct mctp_pktbuf *pkt);

/** @cond INTERNAL_HIDDEN */
extern const struct i2c_target_callbacks mctp_i2c_smbus_target_callbacks;
/** INTERNAL_HIDDEN @endcond */

/**
 * @brief Define an MCTP I2C SMBus target binding instance from devicetree.
 *
 * This macro instantiates and initializes an MCTP over I2C SMBus target
 * binding using the provided devicetree node.
 *
 * It sets up:
 * - I2C device reference
 * - Target configuration
 * - Endpoint addressing
 * - Internal buffers and work structures
 *
 * @param _name Name of the binding instance.
 * @param _node_id Devicetree node identifier.
 */
#define MCTP_I2C_SMBUS_TARGET_DT_DEFINE(_name, _node_id)		\
	K_SEM_DEFINE(_name##_tx_lock, 1, 1);				\
	static struct mctp_binding_i2c_smbus_target _name = {		\
		.binding = {						\
			.name = STRINGIFY(_name),			\
			.version = 1,					\
			.start = mctp_i2c_smbus_target_start,		\
			.tx = mctp_i2c_smbus_target_tx,			\
			.pkt_size = MCTP_I2C_SMBUS_MAX_MCTP_BYTES,	\
		},							\
		.i2c = DEVICE_DT_GET(DT_PHANDLE(_node_id, i2c)),	\
		.i2c_target_cfg = {					\
			.address = DT_PROP(_node_id, i2c_addr),		\
			.callbacks = &mctp_i2c_smbus_target_callbacks,	\
		},							\
		.endpoint_id = DT_PROP(_node_id, endpoint_id),		\
		.ep_i2c_addr = DT_PROP(_node_id, i2c_addr),		\
		.remote_i2c_addr = DT_PROP(_node_id, remote_i2c_addr),	\
		.tx_lock = &_name##_tx_lock,				\
	}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PMCI_MCTP_I2C_SMBUS_TARGET_H_ */
