/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/bluetooth/h4_uart.h>
#include <sys/byteorder.h>
#include <drivers/uart.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(bt_h4_uart, CONFIG_BT_H4_UART_LOG_LEVEL);

static uint32_t rx_buf_size(struct h4_uart_rx *rx)
{
	return sizeof(rx->buf_space) / 2;
}

static size_t is_rx_data_pending(struct h4_uart_rx *rx)
{
	return !ring_buf_is_empty(&rx->buffer);
}

void uart_rx_feed(struct h4_uart *transport)
{
	if (IS_ENABLED(CONFIG_BT_H4_UART_INTERRUPT_DRIVEN)) {
		uart_irq_rx_enable(transport->dev);
		return;
	}

	uint32_t hlen = rx_buf_size(&transport->rx);
	uint8_t *buf;
	uint32_t len = ring_buf_put_claim(&transport->rx.buffer, &buf, hlen);
	int err;

	/* When async API is used then uart is feed with buffers allocated from
	 * ring buffer. Buffer must always be size of half of ring buffer space
	 * (2 buffers). Since only halves are allocated it is assured that
	 * buffers will be aligned pointing to the beginning or middle of the
	 * ring buffer.
	 */
	if (len != hlen) {
		ring_buf_put_unclaim(&transport->rx.buffer, len);
		return;
	}

	/* Receiver may be disabled if new buffer was not provided on time
	 * (uart_rx_buf_rsp() not called). In that case receiver is reenabled.
	 */
	if (!transport->rx.enabled && !transport->rx.stopped) {
		LOG_INF("Reenabling RX");
		err = uart_rx_enable(transport->dev, buf, hlen, 1);
		__ASSERT_NO_MSG(err >= 0);
		transport->rx.enabled = true;
		return;
	}

	/* There is a period it is too late to respond with new buffer but
	 * receiver is not yet fully closed (enabled flag is not cleared). In
	 * that case UART return error. If we hit that here then gracefully
	 * return allocated buffer. Receiver will be reenabled when
	 * UART_RX_DISABLED event comes or when this function is called again.
	 */
	err = uart_rx_buf_rsp(transport->dev, buf, hlen);
	LOG_DBG("Rx buffer resposne (err:%d)", err);
	if (err == -EAGAIN) {
		LOG_INF("Rx buffer provided too late, RX will be disabled.");
		ring_buf_put_unclaim(&transport->rx.buffer, hlen);
	}
}

size_t h4_uart_read(struct h4_uart *transport, uint8_t *dst, size_t req_len)
{
	size_t len;

	len = ring_buf_get(&transport->rx.buffer, dst, req_len);
	LOG_DBG("read %d, req: %d", len, req_len);
	if (len) {
		/* If any data was read from the buffer it means that has been
		 * freed for reception, attempt to resume receiving since there
		 * is new free space for reception.
		 */
		uart_rx_feed(transport);
	}

	return len;
}

static void on_tx_buf_complete(struct h4_uart_tx *tx)
{
	tx->type = H4_NONE;
	net_buf_unref(tx->curr);
}

static int send_type(struct h4_uart *transport)
{
	switch (bt_buf_get_type(transport->tx.curr)) {
	case BT_BUF_ACL_OUT:
		transport->tx.type = H4_ACL;
		break;
	case BT_BUF_CMD:
		transport->tx.type = H4_CMD;
		break;
	default:
		LOG_ERR("Unknown buffer type");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_H4_UART_INTERRUPT_DRIVEN)) {
		int bytes = uart_fifo_fill(transport->dev, &transport->tx.type, 1);
		return (bytes) ? 0 : -EIO;
	}

	return uart_tx(transport->dev, &transport->tx.type, 1,
		       transport->tx.timeout);
}

static int rx_enable(struct h4_uart *transport,
		     const struct h4_uart_config_rx *config)
{
	struct h4_uart_rx *rx = &transport->rx;
	uint8_t *buf;
	uint32_t len = rx_buf_size(rx);
	int err;

	transport->rx.enabled = true;
	ring_buf_reset(&rx->buffer);

	if (IS_ENABLED(CONFIG_BT_H4_UART_INTERRUPT_DRIVEN)) {
		uart_irq_rx_enable(transport->dev);
		return 0;
	}

	len = ring_buf_put_claim(&rx->buffer, &buf, len);
	__ASSERT_NO_MSG(len == rx_buf_size(rx));

	err = uart_rx_enable(transport->dev,  buf, len, 1);
	if (err < 0) {
		return -EIO;
	}

	len = ring_buf_put_claim(&rx->buffer, &buf, len);
	__ASSERT_NO_MSG(len == rx_buf_size(rx));

	return uart_rx_buf_rsp(transport->dev,  buf, len);
}

static void on_tx_isr(struct h4_uart *transport)
{
	if (!transport->tx.curr) {
		transport->tx.curr = net_buf_get(&transport->tx.fifo,
						 K_NO_WAIT);
		if (!transport->tx.curr) {
			LOG_ERR("TX interrupt but no pending buffer!");
			uart_irq_tx_disable(transport->dev);
			return;
		}
	}

	if ((transport->tx.flags & H4_UART_TX_ADD_TYPE) &&
		transport->tx.type == H4_NONE) {
		int err = send_type(transport);

		if (err < 0) {
			goto done;
		}
	}

	int bytes = uart_fifo_fill(transport->dev,
				   transport->tx.curr->data,
				   transport->tx.curr->len);
	net_buf_pull(transport->tx.curr, bytes);

	if (transport->tx.curr->len) {
		return;
	}

done:
	on_tx_buf_complete(&transport->tx);

	transport->tx.curr = net_buf_get(&transport->tx.fifo, K_NO_WAIT);
	if (!transport->tx.curr) {
		uart_irq_tx_disable(transport->dev);
	}
}

static void on_rx_isr(struct h4_uart *transport)
{
	struct h4_uart_rx *rx = &transport->rx;
	uint8_t *buf;
	uint32_t len;

	len = ring_buf_put_claim(&rx->buffer, &buf, UINT32_MAX);
	if (!len) {
		LOG_INF("disabling RX, no space in rbuffer");
		uart_irq_rx_disable(transport->dev);
		rx->enabled = false;
		return;
	}

	len = uart_fifo_read(transport->dev, buf, len);
	ring_buf_put_finish(&rx->buffer, len, false);
	if (len) {
		/* Wake up RX thread */
		k_sem_give(&rx->sem);
	}

}

static void uart_isr(struct device *dev, void *user_data)
{
	struct h4_uart *transport = user_data;

	while (uart_irq_update(dev) &&
		uart_irq_is_pending(dev)) {
		if (uart_irq_tx_ready(dev)) {
			on_tx_isr(transport);
		}

		if (uart_irq_rx_ready(dev)) {
			on_rx_isr(transport);
		}
	}
}

static int send(struct h4_uart *transport)
{
	if ((transport->tx.flags & H4_UART_TX_ADD_TYPE) &&
		(transport->tx.type == H4_NONE)) {
		return send_type(transport);
	}

	transport->tx.type = H4_NONE;
	return uart_tx(transport->dev, transport->tx.curr->data,
			transport->tx.curr->len, transport->tx.timeout);
}

static void on_tx_done(struct h4_uart_tx *tx)
{
	if ((tx->flags & H4_UART_TX_ADD_TYPE) && (tx->type != H4_NONE)) {
		/* completed sending of first byte (type). */
		tx->type = H4_INV;
		return;
	}

	/* Whole buffer completed. */
	on_tx_buf_complete(tx);

	tx->curr = net_buf_get(&tx->fifo, K_NO_WAIT);
}

static void on_rx_rdy(struct h4_uart_rx *rx,
		      struct uart_event_rx *evt)
{
	ring_buf_put_finish(&rx->buffer, evt->len, true);

	/* Wake up RX thread */
	k_sem_give(&rx->sem);
}

static void uart_callback(struct device *dev,
			  struct uart_event *evt,
			  void *user_data)
{
	ARG_UNUSED(dev);

	int err;
	struct h4_uart *transport = user_data;

	switch (evt->type) {
	case UART_TX_ABORTED:
		LOG_ERR("Timeout. Failed to send packet.");
		/* fall through */
	case UART_TX_DONE:
		on_tx_done(&transport->tx);
		if (transport->tx.curr) {
			err = send(transport);
			if (err < 0) {
				on_tx_buf_complete(&transport->tx);
			}
		}
		break;

	case UART_RX_RDY:
		LOG_DBG("UART_RX_RDY %d bytes", evt->data.rx.len);
		on_rx_rdy(&transport->rx, &evt->data.rx);
		break;
	case UART_RX_BUF_REQUEST:
	{
		/* Do nothing here. */
		break;
	}
	case UART_RX_BUF_RELEASED:
	{
		/* buffers are released by the RX thread. On special cases when
		 * RX is disabled, buffer pool is reinitialized when enabling.
		 */
		break;
	}
	case UART_RX_STOPPED:
		LOG_WRN("RX error occured, reason: %d",
			evt->data.rx_stop.reason);
		transport->rx.stopped = true;
		break;
	case UART_RX_DISABLED:
		LOG_INF("Receiver disabled.");
		if (transport->rx.stopped) {
			err = rx_enable(transport, NULL);
			__ASSERT(!err, "Failed to enable RX (err: %d)", err);
			transport->rx.stopped = false;
		} else {
			transport->rx.enabled = false;
			/* Resume only if there is no RX data to process. It
			 * means that rx thread won't do it.
			 */
			if (!is_rx_data_pending(&transport->rx)) {
				LOG_INF("Resuming from RX disable");
				uart_rx_feed(transport);
			}
		}
		break;
	default:
		LOG_ERR("Unexpected UART event: %d", evt->type);
		break;
	}
}

static void rx_thread(void *p1, void *p2, void *p3)
{
	struct h4_uart *transport = p1;

	while (1) {
		bool idle;

		while (is_rx_data_pending(&transport->rx) != 0) {
			transport->rx.process(transport);
		}

		k_spinlock_key_t key = k_spin_lock(&transport->rx.lock);

		if (is_rx_data_pending(&transport->rx) == 0) {
			/* new data */
			idle = true;
			k_sem_init(&transport->rx.sem, 0, 1);
		} else {
			idle = false;
		}
		k_spin_unlock(&transport->rx.lock, key);

		if (idle) {
			k_sem_take(&transport->rx.sem, K_FOREVER);
		}
	}
}

static void bt_wait_nop(struct h4_uart *transport)
{
	int i;

	/* Issue a Command Complete with NOP */
	const struct {
		const uint8_t h4;
		const struct bt_hci_evt_hdr hdr;
		const struct bt_hci_evt_cmd_complete cc;
	} __packed cc_evt = {
		.h4 = H4_EVT,
		.hdr = {
			.evt = BT_HCI_EVT_CMD_COMPLETE,
			.len = sizeof(struct bt_hci_evt_cmd_complete),
		},
		.cc = {
			.ncmd = 1,
			.opcode = sys_cpu_to_le16(BT_OP_NOP),
		},
	};

	for (i = 0; i < sizeof(cc_evt); i++) {
		uart_poll_out(transport->dev, *(((const uint8_t *)&cc_evt)+i));
	}

	LOG_DBG("NOP command complete sent.");
}

int h4_uart_write(struct h4_uart *transport, struct net_buf *buf)
{
	if (!atomic_ptr_cas((atomic_ptr_t *)&transport->tx.curr, NULL, buf)) {
		LOG_DBG("busy, enqueueing TX");
		net_buf_put(&transport->tx.fifo, buf);
		if (transport->tx.curr == NULL) {
			transport->tx.curr =
				net_buf_get(&transport->tx.fifo, K_NO_WAIT);
		} else {
			return 0;
		}
	}

	if (IS_ENABLED(CONFIG_BT_H4_UART_INTERRUPT_DRIVEN)) {
		uart_irq_tx_enable(transport->dev);
		return 0;
	}

	int err = send(transport);
	if (err < 0) {
		on_tx_buf_complete(&transport->tx);
		return err;
	}

	return 0;
}

/** Setup the HCI transport, which usually means to reset the Bluetooth IC
 *
 * @param dev The device structure for the bus connecting to the IC
 *
 * @return 0 on success, negative error value on failure
 */
int __weak bt_hci_transport_setup(struct h4_uart *transport)
{
	if (IS_ENABLED(CONFIG_BT_H4_UART_INTERRUPT_DRIVEN)) {
		h4_uart_read(transport, NULL, 32);
	}

	return 0;
}

int h4_uart_init(struct h4_uart *transport, struct device *dev,
		 const struct h4_uart_config *config)
{
	int err;

	transport->dev = dev;
	transport->rx.process = config->rx.process;

	ring_buf_init(&transport->rx.buffer,
		      sizeof(transport->rx.buf_space),
		      transport->rx.buf_space);

	k_fifo_init(&transport->tx.fifo);
	transport->tx.timeout = config->tx.timeout;
	if (config->tx.add_type) {
		transport->tx.flags = H4_UART_TX_ADD_TYPE;
	}

	if (IS_ENABLED(CONFIG_BT_H4_UART_INTERRUPT_DRIVEN)) {
		uart_irq_callback_user_data_set(dev, uart_isr, transport);
	} else {
		err = uart_callback_set(transport->dev, uart_callback,
					transport);
		if (err < 0) {
			return err;
		}
	}

	err = rx_enable(transport, &config->rx);
	if (err < 0) {
		return err;
	}

	k_thread_create(&transport->rx.thread, config->rx.stack,
			config->rx.rx_thread_stack_size,
			rx_thread, transport, NULL, NULL,
			config->rx.thread_prio,
			0, K_NO_WAIT);

	err = bt_hci_transport_setup(transport);
	if (err < 0) {
		return err;
	}

	if (!IS_ENABLED(CONFIG_BT_H4)) {

		if (IS_ENABLED(CONFIG_BT_WAIT_NOP)) {
			bt_wait_nop(transport);
		}
	}

	return 0;
}
