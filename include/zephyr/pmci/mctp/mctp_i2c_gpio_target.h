/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_MCTP_I2C_GPIO_TARGET_H_
#define ZEPHYR_MCTP_I2C_GPIO_TARGET_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pmci/mctp/mctp_i2c_gpio_common.h>
#include <libmctp.h>

/**
 * @brief An MCTP binding for Zephyr's I2C target interface using GPIO
 */
struct mctp_binding_i2c_gpio_target {
	/** @cond INTERNAL_HIDDEN */
	struct mctp_binding binding;
	const struct device *i2c;
	struct i2c_target_config i2c_target_cfg;
	uint8_t endpoint_id;
	const struct gpio_dt_spec endpoint_gpio;
	uint8_t reg_addr;
	bool rxtx;
	uint8_t rx_idx;
	struct mctp_pktbuf *rx_pkt;
	struct k_sem *tx_lock;
	struct k_sem *tx_complete;
	uint8_t tx_idx;
	struct mctp_pktbuf *tx_pkt;
	/** INTERNAL_HIDDEN @endcond */
};

/** @cond INTERNAL_HIDDEN */
extern const struct i2c_target_callbacks mctp_i2c_gpio_target_callbacks;
int mctp_i2c_gpio_target_start(struct mctp_binding *binding);
int mctp_i2c_gpio_target_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt);
/** INTERNAL_HIDDEN @endcond */

/**
 * @brief Define a MCTP bus binding for I2C target with GPIO
 *
 * Rather than mode switching as the MCTP standard wishes, this is a custom
 * binding. On the target side a gpio pin is set active (could be active low or
 * active high) when writes are pending. It's expected the controller then
 * reads from the I2C target device at some point in the future. Reads are
 * accepted at any time and are expected to contain full mctp packets.
 *
 * In effect each device has a pair of pseudo registers for reading or writing
 * as a packet FIFO.
 *
 * Thus the sequence for a I2C target to send a message would be...
 *
 * 1. Setup a length and message value (pseudo registers).
 * 2. Signal the controller with a GPIO level set
 * 3. The controller then is expected to read the length (write of addr 0 + read 1 byte)
 * 4. The controller then is expected to read the message (write of addr 1 + read N bytes)
 * 5. The target clears the pin set
 * 6. The controller reports a message receive to MCTP
 *
 * @param _name Symbolic name of the bus binding variable
 * @param _node_id DeviceTree Node containing the configuration of this MCTP binding
 */
#define MCTP_I2C_GPIO_TARGET_DT_DEFINE(_name, _node_id)                                            \
	K_SEM_DEFINE(_name ##_tx_lock, 1, 1);                                                      \
	K_SEM_DEFINE(_name ##_tx_complete, 0, 1);                                                  \
	struct mctp_binding_i2c_gpio_target _name = {                                              \
		.binding = {                                                                       \
			.name = STRINGIFY(_name), .version = 1,                                    \
			.start = mctp_i2c_gpio_target_start,                                       \
			.tx = mctp_i2c_gpio_target_tx,                                             \
			.pkt_size = MCTP_I2C_GPIO_MAX_PKT_SIZE,                                    \
		},                                                                                 \
		.i2c = DEVICE_DT_GET(DT_PHANDLE(_node_id, i2c)),                                   \
		.i2c_target_cfg = {                                                                \
			.address = DT_PROP(_node_id, i2c_addr),                                    \
			.callbacks = &mctp_i2c_gpio_target_callbacks,                              \
		},                                                                                 \
		.endpoint_id = DT_PROP(_node_id, endpoint_id),                                     \
		.endpoint_gpio = GPIO_DT_SPEC_GET(_node_id, endpoint_gpios),                       \
		.tx_lock = &_name##_tx_lock,                                                       \
		.tx_complete = &_name##_tx_complete,                                               \
	};

#endif /* ZEPHYR_MCTP_I2C_GPIO_TARGET_H_ */
