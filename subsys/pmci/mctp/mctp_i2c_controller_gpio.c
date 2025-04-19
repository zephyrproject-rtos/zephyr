/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "zephyr/rtio/rtio.h"
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pmci/mctp/mctp_i2c.h>
#include <crc-16-ccitt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_i2c_controller_gpio, CONFIG_MCTP_LOG_LEVEL);

static void mctp_tx_requested_isr(const struct device *port, struct gpio_callback *cb,
				 gpio_port_pins_t pins)
{
	struct mctp_i2c_controller_gpio_cb *cb_data =
		CONTAINER_OF(cb, struct mctp_i2c_controller_gpio_cb, callback);
	struct mctp_binding_i2c_controller_gpio *b = cb_data->binding;
	const struct rtio_iodev *iodev = b->endpoint_iodevs[cb_data->index];


	/* Multiple GPIOs may come in, so we need to lock around our I2C setup */
	k_spinlock_key_t key = k_spin_lock(&b->lock);

	/* TODO mark this GPIO as ready in a bit array, and disable the interrupt.
	 * Then check if we are already doing a target device read, if so move on,
	 * if start doing one continue
	 */
	 
	/* Asynchronous bindings may return -EBUSY */
	struct rtio_sqe *write_len_addr_sqe = rtio_sqe_acquire(b->r);
	struct rtio_sqe *read_len_sqe = rtio_sqe_acquire(b->r);
	struct rtio_sqe *write_msg_addr_sqe = rtio_sqe_acquire(b->r);
	struct rtio_sqe *read_msg_sqe = rtio_sqe_acquire(b->r);
	uint8_t addr;

	addr = MCTP_I2C_GPIO_RX_MSG_LEN_ADDR;
	rtio_sqe_prep_tiny_write(write_len_addr_sqe, iodev, RTIO_PRIO_NORM,
			&addr, 1, NULL);
	write_len_addr_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_len_sqe, iodev, RTIO_PRIO_NORM, &b->rx_buf_len, 1, NULL);
	read_len_sqe->flags |= RTIO_SQE_CHAINED;
	addr = MCTP_I2C_GPIO_RX_MSG_ADDR;
	rtio_sqe_prep_tiny_write(write_msg_addr_sqe, iodev, RTIO_PRIO_NORM,
			&addr, 1, NULL);
	write_msg_addr_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_tiny_write(read_msg_sqe, iodev, RTIO_PRIO_NORM, b->rx_buf, 1, NULL);

	/* TODO add a completion callback here such so on completion of one target read we
	 * can check if any others (in a bit array) have requested a message read as
	 * well as re-enabling the GPIO interrupt
	 */

	rtio_submit(b->r, 0);

	k_spin_unlock(&b->lock, key);
}

int mctp_i2c_controller_gpio_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt)
{
	/* Which i2c device am I sending this to? */
	int i;
	struct mctp_hdr *hdr = mctp_pktbuf_hdr(pkt);
	struct mctp_binding_i2c_controller_gpio *b =
		CONTAINER_OF(binding, struct mctp_binding_i2c_controller_gpio, binding);

	for (i = 0; i < b->num_endpoints; i++) {
		if (b->endpoint_ids[i] == hdr->dest) {
			break;
		}
	}

	const struct rtio_iodev *iodev = b->endpoint_iodevs[i];

	/* Multiple GPIOs may come in, so we need to lock around our I2C setup */
	k_spinlock_key_t key = k_spin_lock(&b->lock);

	/* Asynchronous bindings may return -EBUSY */
	if (rtio_sqe_acquirable(b->r) < 4) {
		return -EBUSY;
	}


	struct rtio_sqe *write_len_addr_sqe = rtio_sqe_acquire(b->r);
	struct rtio_sqe *write_len_sqe = rtio_sqe_acquire(b->r);
	struct rtio_sqe *write_addr_sqe = rtio_sqe_acquire(b->r);
	struct rtio_sqe *write_data_sqe = rtio_sqe_acquire(b->r);
	uint8_t addr;

	addr = MCTP_I2C_GPIO_TX_MSG_LEN_ADDR;
	rtio_sqe_prep_tiny_write(write_len_addr_sqe, iodev, RTIO_PRIO_NORM,
			&addr, 1, NULL);
	write_len_addr_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_tiny_write(write_len_sqe, iodev, RTIO_PRIO_NORM,
			(uint8_t *)&pkt->size, 1, NULL);
	write_len_sqe->flags |= RTIO_SQE_CHAINED;

	addr = MCTP_I2C_GPIO_TX_MSG_ADDR;
	rtio_sqe_prep_tiny_write(write_addr_sqe, iodev, RTIO_PRIO_NORM,
			&addr, 1, NULL);
	write_addr_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_write(write_data_sqe, iodev, RTIO_PRIO_NORM,
			pkt->data, pkt->size, NULL);

	rtio_submit(b->r, 0);

	k_spin_unlock(&b->lock, key);

	return 0;
}

int mctp_i2c_controller_gpio_start(struct mctp_binding *binding)
{
	struct mctp_binding_i2c_controller_gpio *b =
		CONTAINER_OF(binding, struct mctp_binding_i2c_controller_gpio, binding);

	for (int i = 0; i < b->num_endpoints; i++) {
		gpio_init_callback(&b->endpoint_gpio_cbs[i].callback, mctp_tx_requested_isr,
		                   b->endpoint_gpios[i].pin);
		b->endpoint_gpio_cbs[i].binding = b;
		b->endpoint_gpio_cbs[i].index = i;
		gpio_add_callback_dt(&b->endpoint_gpios[i], &b->endpoint_gpio_cbs[i].callback);
		gpio_pin_configure_dt(&b->endpoint_gpios[i], GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&b->endpoint_gpios[i], GPIO_INT_LEVEL_ACTIVE);
	}

	return 0;
}
