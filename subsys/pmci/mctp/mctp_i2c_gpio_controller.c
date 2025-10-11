/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "zephyr/drivers/gpio.h"
#include "zephyr/pmci/mctp/mctp_i2c_gpio_common.h"
#include "zephyr/rtio/rtio.h"
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pmci/mctp/mctp_i2c_gpio_controller.h>
#include <crc-16-ccitt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_i2c_gpio_controller, CONFIG_MCTP_LOG_LEVEL);


static void mctp_start_rx(struct mctp_binding_i2c_gpio_controller *b,
			  bool chained_rx);

static void rx_completion(struct rtio *r, const struct rtio_sqe *sqe, int result, void *arg0)
{
	ARG_UNUSED(result);

	struct mctp_binding_i2c_gpio_controller *b = arg0;
	struct rtio_cqe *cqe;

	while ((cqe = rtio_cqe_consume(r))) {
		rtio_cqe_release(r, cqe);
	}

	struct mctp_pktbuf *pkt = mctp_pktbuf_alloc(&b->binding, b->rx_buf_len);

	memcpy(pkt->data, b->rx_buf, b->rx_buf_len);

	LOG_DBG("giving pkt to mctp, len %d", b->rx_buf_len);
	mctp_bus_rx(&b->binding, pkt);

	/* Walk our pending bits from the current index, and start the next one if needed */
	struct mctp_i2c_gpio_controller_cb *cb = b->inflight_rx;

	/* Re-enable the GPIO interrupt */
	gpio_pin_interrupt_configure_dt(&b->endpoint_gpios[cb->index], GPIO_INT_ENABLE);

	/* Try and start the next transfer if one is pending */
	mctp_start_rx(b, true);
}


static void rx_len_completion(struct rtio *r, const struct rtio_sqe *sqe, int result, void *arg0)
{
	ARG_UNUSED(result);

	const uint8_t mctp_tx_msg_addr = MCTP_I2C_GPIO_TX_MSG_ADDR;

	struct mctp_binding_i2c_gpio_controller *b = arg0;
	const struct rtio_iodev *iodev = b->endpoint_iodevs[b->inflight_rx->index];
	struct rtio_cqe *cqe;

	while ((cqe = rtio_cqe_consume(r))) {
		rtio_cqe_release(r, cqe);
	}
	struct rtio_sqe *write_msg_addr_sqe = rtio_sqe_acquire(b->r_rx);
	struct rtio_sqe *read_msg_sqe = rtio_sqe_acquire(b->r_rx);
	struct rtio_sqe *callback_sqe = rtio_sqe_acquire(b->r_rx);

	LOG_DBG("reading buf %d", b->rx_buf_len);
	rtio_sqe_prep_tiny_write(write_msg_addr_sqe, iodev, RTIO_PRIO_NORM,
			&mctp_tx_msg_addr, 1, NULL);
	write_msg_addr_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_msg_sqe, iodev, RTIO_PRIO_NORM, b->rx_buf, b->rx_buf_len, NULL);
	read_msg_sqe->flags |= RTIO_SQE_CHAINED;
	read_msg_sqe->iodev_flags |= RTIO_IODEV_I2C_RESTART | RTIO_IODEV_I2C_STOP;

	rtio_sqe_prep_callback(callback_sqe, rx_completion, b, NULL);
	callback_sqe->flags |= RTIO_SQE_NO_RESPONSE;

	rtio_submit(b->r_rx, 0);
}

/*
 * Atomically start or mark as pending the next receive
 */
static void mctp_start_rx(struct mctp_binding_i2c_gpio_controller *b, bool chained_rx)
{
	/* Critical section to set the next inflight transmit */
	k_spinlock_key_t key = k_spin_lock(&b->rx_lock);

	/* Ongoing transfer already in fight */
	if (!chained_rx && b->inflight_rx != NULL) {
		k_spin_unlock(&b->rx_lock, key);
		return;
	}

	struct mpsc_node *node = mpsc_pop(&b->rx_q);

	if (node == NULL) {
		b->inflight_rx = NULL;
		k_spin_unlock(&b->rx_lock, key);
		return;
	}

	struct mctp_i2c_gpio_controller_cb *next =
		CONTAINER_OF(node, struct mctp_i2c_gpio_controller_cb, q);

	b->inflight_rx = next;
	k_spin_unlock(&b->rx_lock, key);

	const uint8_t mctp_tx_msg_len_addr = MCTP_I2C_GPIO_TX_MSG_LEN_ADDR;
	const struct rtio_iodev *iodev = b->endpoint_iodevs[next->index];
	struct rtio_sqe *write_len_addr_sqe = rtio_sqe_acquire(b->r_rx);
	struct rtio_sqe *read_len_sqe = rtio_sqe_acquire(b->r_rx);
	struct rtio_sqe *callback_sqe = rtio_sqe_acquire(b->r_rx);

	rtio_sqe_prep_tiny_write(write_len_addr_sqe, iodev, RTIO_PRIO_NORM,
			&mctp_tx_msg_len_addr, 1, NULL);
	write_len_addr_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_len_sqe, iodev, RTIO_PRIO_NORM, &b->rx_buf_len, 1, NULL);
	read_len_sqe->flags |= RTIO_SQE_CHAINED;
	read_len_sqe->iodev_flags |= RTIO_IODEV_I2C_RESTART | RTIO_IODEV_I2C_STOP;

	rtio_sqe_prep_callback(callback_sqe, rx_len_completion, b, NULL);
	callback_sqe->flags |= RTIO_SQE_NO_RESPONSE;

	rtio_submit(b->r_rx, 0);
}

void mctp_tx_requested_isr(const struct device *port, struct gpio_callback *cb,
				 gpio_port_pins_t pins)
{
	struct mctp_i2c_gpio_controller_cb *cb_data =
		CONTAINER_OF(cb, struct mctp_i2c_gpio_controller_cb, callback);
	struct mctp_binding_i2c_gpio_controller *b = cb_data->binding;

	LOG_DBG("disable int");
	gpio_pin_interrupt_configure_dt(&b->endpoint_gpios[cb_data->index], GPIO_INT_DISABLE);

	mpsc_push(&b->rx_q, &cb_data->q);

	/* Atomically start the transfer if nothing is ongoing, if the current tx isn't NULL
	 * a bit is set instead marking the transfer as pending for this matching GPIO and
	 * i2c target
	 */
	mctp_start_rx(b, false);
}

int mctp_i2c_gpio_controller_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt)
{
	int rc = 0;

	/* Which i2c device am I sending this to? */
	int i;
	struct mctp_hdr *hdr = mctp_pktbuf_hdr(pkt);
	struct mctp_binding_i2c_gpio_controller *b =
		CONTAINER_OF(binding, struct mctp_binding_i2c_gpio_controller, binding);
	uint8_t pktsize = pkt->end - pkt->start;

	for (i = 0; i < b->num_endpoints; i++) {
		if (b->endpoint_ids[i] == hdr->dest) {
			break;
		}
	}
	k_sem_take(b->tx_lock, K_FOREVER);

	const struct rtio_iodev *iodev = b->endpoint_iodevs[i];
	struct rtio_sqe *write_len_addr_sqe = rtio_sqe_acquire(b->r_tx);
	struct rtio_sqe *write_len_sqe = rtio_sqe_acquire(b->r_tx);
	struct rtio_sqe *write_addr_sqe = rtio_sqe_acquire(b->r_tx);
	struct rtio_sqe *write_data_sqe = rtio_sqe_acquire(b->r_tx);
	uint8_t addr;

	addr = MCTP_I2C_GPIO_RX_MSG_LEN_ADDR;
	rtio_sqe_prep_tiny_write(write_len_addr_sqe, iodev, RTIO_PRIO_NORM,
			&addr, 1, NULL);
	write_len_addr_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_tiny_write(write_len_sqe, iodev, RTIO_PRIO_NORM,
			(uint8_t *)&pktsize, 1, NULL);
	write_len_sqe->flags |= RTIO_SQE_TRANSACTION;
	addr = MCTP_I2C_GPIO_RX_MSG_ADDR;
	rtio_sqe_prep_tiny_write(write_addr_sqe, iodev, RTIO_PRIO_NORM,
			&addr, 1, NULL);
	write_addr_sqe->flags |= RTIO_SQE_TRANSACTION;
	write_addr_sqe->iodev_flags |= RTIO_IODEV_I2C_RESTART;
	rtio_sqe_prep_write(write_data_sqe, iodev, RTIO_PRIO_NORM,
			&pkt->data[pkt->start], pktsize, NULL);
	write_data_sqe->iodev_flags |=  RTIO_IODEV_I2C_STOP;

	rtio_submit(b->r_tx, 4);

	struct rtio_cqe *cqe;

	while ((cqe = rtio_cqe_consume(b->r_tx))) {
		if (cqe->result != 0 && rc == 0) {
			rc = cqe->result;
		}
		rtio_cqe_release(b->r_tx, cqe);
	}

	if (rc != 0) {
		LOG_WRN("failed sending mctp message %p", (void *)pkt);
	}

	k_sem_give(b->tx_lock);

	/* We must *always* return 0 despite errors, otherwise libmctp does not free the packet! */
	return 0;
}

int mctp_i2c_gpio_controller_start(struct mctp_binding *binding)
{
	struct mctp_binding_i2c_gpio_controller *b =
		CONTAINER_OF(binding, struct mctp_binding_i2c_gpio_controller, binding);

	i2c_recover_bus(b->i2c);

	mpsc_init(&b->rx_q);

	for (int i = 0; i < b->num_endpoints; i++) {
		gpio_init_callback(&b->endpoint_gpio_cbs[i].callback, &mctp_tx_requested_isr,
				   BIT(b->endpoint_gpios[i].pin));
		b->endpoint_gpio_cbs[i].binding = b;
		b->endpoint_gpio_cbs[i].index = i;
		gpio_add_callback_dt(&b->endpoint_gpios[i], &b->endpoint_gpio_cbs[i].callback);
		gpio_pin_configure_dt(&b->endpoint_gpios[i], GPIO_INPUT);
	}

	mctp_binding_set_tx_enabled(binding, true);

	for (int i = 0; i < b->num_endpoints; i++) {
		gpio_pin_interrupt_configure_dt(&b->endpoint_gpios[i], GPIO_INT_LEVEL_HIGH);
	}

	LOG_DBG("started");

	return 0;
}
