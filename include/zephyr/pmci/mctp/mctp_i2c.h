/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_MCTP_I2C_GPIO_H_
#define ZEPHYR_MCTP_I2C_GPIO_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/rtio/rtio.h>
#include <libmctp.h>

/** @cond INTERNAL_HIDDEN */

#define MCTP_I2C_GPIO_RX_MSG_LEN_ADDR  1
#define MCTP_I2C_GPIO_RX_MSG_ADDR  2
#define MCTP_I2C_GPIO_TX_MSG_LEN_ADDR 3
#define MCTP_I2C_GPIO_TX_MSG_ADDR 4

struct mctp_binding_i2c_controller_gpio;

struct mctp_i2c_controller_gpio_cb {

	struct gpio_callback callback;
	struct mctp_binding_i2c_controller_gpio *binding;
	uint8_t index;
};
/** @endcond INTERNAL_HIDDEN */

#define MCTP_I2C_MAX_PKT_SIZE 256

/**
 * @brief An MCTP binding for Zephyr's I2C interface using GPIO
 */
struct mctp_binding_i2c_controller_gpio {
	/** @cond INTERNAL_HIDDEN */
	struct mctp_binding binding;
	const struct device *i2c;

	uint8_t rx_buf_len;
	uint8_t rx_buf[MCTP_I2C_MAX_PKT_SIZE];
	uint8_t tx_buf[MCTP_I2C_MAX_PKT_SIZE];

	struct mctp_pktbuf *rx_pkt;

	/* Avoid needing a thread or work queue task by using RTIO */
	struct k_spinlock lock;
	struct rtio *r;

	/* Mapping of endpoints to i2c targets with gpios */
	size_t num_endpoints;
	const uint8_t *endpoint_ids;
	const struct rtio_iodev **endpoint_iodevs;
	const struct gpio_dt_spec *endpoint_gpios;
	struct mctp_i2c_controller_gpio_cb *endpoint_gpio_cbs;

	/** @endcond INTERNAL_HIDDEN */
};

/** @cond INTERNAL_HIDDEN */
int mctp_i2c_controller_gpio_start(struct mctp_binding *binding);
int mctp_i2c_controller_gpio_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt);

#define MCTP_I2C_IODEV_NAME(_idx, _name) _name##_idx

#define MCTP_I2C_IODEV_DEFINE(_node_id, addrs, _idx, _name)                                       \
	I2C_IODEV_DEFINE(MCTP_I2C_IODEV_NAME(_idx, _name),                                        \
		DT_PHANDLE(_node_id, i2c_controller),                                             \
		DT_PROP_BY_IDX(_node_id, addrs, _idx));

/** @endcond INTERNAL_HIDDEN */

#define MCTP_I2C_CONTROLLER_DEFINE_GPIOS(_node_id, _name)                                          \
	const struct gpio_dt_spec _name##_endpoint_gpios[] = {                                     \
		DT_FOREACH_PROP_ELEM_SEP(_node_id, endpoint_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))   \
	}

#define MCTP_I2C_CONTROLLER_DEFINE_IDS(_node_id, _name)                                            \
	const uint8_t _name##_endpoint_ids[] = DT_PROP(_node_id, endpoint_ids);

#define MCTP_I2C_CONTROLLER_DEFINE_GPIO_CBS(_node_id, _name)                                       \
	struct mctp_i2c_controller_gpio_cb                                                         \
		_name##_endpoint_gpio_cbs[DT_PROP_LEN(_node_id, endpoint_ids)]


#define MCTP_I2C_CONTROLLER_DEFINE_IODEVS(_node_id, _name)                                         \
	DT_FOREACH_PROP_ELEM_VARGS(_node_id, endpoint_addrs, MCTP_I2C_IODEV_DEFINE, _name)         \
		const struct rtio_iodev *_name##_endpoint_iodevs[] = {                             \
		LISTIFY(DT_PROP_LEN(_node_id, endpoint_ids), &MCTP_I2C_IODEV_NAME, (,), _name)      \
	}

#ifdef STUFFIT


#define MCTP_I2C_IODEV_DEFINE(_node_id, _addr, _idx, _name, ...)                                   \



	MCTP_I2C_CONTROLLER_DEFINE_IODEVS(_node_id, _name);                                        \
		.endpoint_iodevs = &_name##_endpoint_iodevs[0],                                    
#endif
	
/**
 * @brief Define a MCTP bus binding for I2C controller with GPIO
 *
 * Rather than mode switching as the MCTP standard wishes, this is a custom
 * binding. On the controller side each neighbor has an associated i2c address
 * and gpio it will listen for a trigger on. When triggered the controller will
 * read the target device. The target device will be expected to have what amounts
 * to two registers, one containing a message length and another containing a FIFO
 * like register producing the message bytes.
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
#define MCTP_I2C_CONTROLLER_GPIO_DT_DEFINE(_name, _node_id)                                        \
	RTIO_DEFINE(_name##_rtio, 5, 5);                                                           \
	MCTP_I2C_CONTROLLER_DEFINE_IODEVS(_node_id, _name);                                        \
	MCTP_I2C_CONTROLLER_DEFINE_GPIOS(_node_id, _name);                                         \
	MCTP_I2C_CONTROLLER_DEFINE_IDS(_node_id, _name);                                           \
        MCTP_I2C_CONTROLLER_DEFINE_GPIO_CBS(_node_id, _name);                                      \
	struct mctp_binding_i2c_controller_gpio _name = {                                          \
		.binding = {                                                                       \
			.name = STRINGIFY(_name), .version = 1,                                    \
			.start = mctp_i2c_controller_gpio_start,                                   \
			.tx = mctp_i2c_controller_gpio_tx,                                         \
			.pkt_size = MCTP_I2C_MAX_PKT_SIZE,                                         \
		},                                                                                 \
		.i2c = DEVICE_DT_GET(DT_PHANDLE(_node_id, i2c_controller)),                        \
		.num_endpoints = DT_PROP_LEN(_node_id, endpoint_ids),                              \
		.endpoint_ids = _name##_endpoint_ids,                                              \
		.endpoint_gpios = _name##_endpoint_gpios,                                          \
		.endpoint_gpio_cbs = _name##_endpoint_gpio_cbs,                                    \
		.r = &_name##_rtio,                                                                \
	};

#endif /* ZEPHYR_MCTP_I2C_H_ */
