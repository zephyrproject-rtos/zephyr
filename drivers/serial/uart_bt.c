/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/services/nus.h>

#define DT_DRV_COMPAT zephyr_nus_uart

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_nus, CONFIG_UART_LOG_LEVEL);

K_THREAD_STACK_DEFINE(nus_work_queue_stack, CONFIG_UART_BT_WORKQUEUE_STACK_SIZE);
static struct k_work_q nus_work_queue;

#define UART_BT_MTU_INVALID 0xFFFF

struct uart_bt_data {
	struct {
		struct bt_nus_inst *inst;
		struct bt_nus_cb cb;
		atomic_t enabled;
	} bt;
	struct {
		struct ring_buf *rx_ringbuf;
		struct ring_buf *tx_ringbuf;
		struct k_work cb_work;
		struct k_work_delayable tx_work;
		bool rx_irq_ena;
		bool tx_irq_ena;
		struct {
			const struct device *dev;
			uart_irq_callback_user_data_t cb;
			void *cb_data;
		} callback;
	} uart;
};

static void bt_notif_enabled(bool enabled, void *ctx)
{
	__ASSERT_NO_MSG(ctx);

	const struct device *dev = (const struct device *)ctx;
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;

	(void)atomic_set(&dev_data->bt.enabled, enabled ? 1 : 0);

	LOG_DBG("%s() - %s", __func__, enabled ? "enabled" : "disabled");

	if (!ring_buf_is_empty(dev_data->uart.tx_ringbuf)) {
		k_work_reschedule_for_queue(&nus_work_queue, &dev_data->uart.tx_work, K_NO_WAIT);
	}
}

static void bt_received(struct bt_conn *conn, const void *data, uint16_t len, void *ctx)
{
	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(ctx);
	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(len > 0);

	const struct device *dev = (const struct device *)ctx;
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;
	struct ring_buf *ringbuf = dev_data->uart.rx_ringbuf;
	uint32_t put_len;

	LOG_DBG("%s() - len: %d, rx_ringbuf space %d", __func__, len, ring_buf_space_get(ringbuf));
	LOG_HEXDUMP_DBG(data, len, "data");

	put_len = ring_buf_put(ringbuf, (const uint8_t *)data, len);
	if (put_len < len) {
		LOG_ERR("RX Ring buffer full. received: %d, added to queue: %d", len, put_len);
	}

	k_work_submit_to_queue(&nus_work_queue, &dev_data->uart.cb_work);
}

static void foreach_conn_handler_get_att_mtu(struct bt_conn *conn, void *data)
{
	uint16_t *min_att_mtu = (uint16_t *)data;
	uint16_t conn_att_mtu = 0;
	struct bt_conn_info conn_info;
	int err;

	err = bt_conn_get_info(conn, &conn_info);
	if (!err && conn_info.state == BT_CONN_STATE_CONNECTED) {
		conn_att_mtu = bt_gatt_get_uatt_mtu(conn);

		if (conn_att_mtu > 0) {
			*min_att_mtu = MIN(*min_att_mtu, conn_att_mtu);
		}
	}
}

static inline uint16_t get_max_chunk_size(void)
{
	uint16_t min_att_mtu = UART_BT_MTU_INVALID;

	bt_conn_foreach(BT_CONN_TYPE_LE, foreach_conn_handler_get_att_mtu, &min_att_mtu);

	if (min_att_mtu == UART_BT_MTU_INVALID) {
		/** Default ATT MTU */
		min_att_mtu = 23;
	}

	/** ATT NTF Payload overhead: opcode (1 octet) + attribute (2 octets) */
	return (min_att_mtu - 1 - 2);
}

static void cb_work_handler(struct k_work *work)
{
	struct uart_bt_data *dev_data = CONTAINER_OF(work, struct uart_bt_data, uart.cb_work);

	if (dev_data->uart.callback.cb) {
		dev_data->uart.callback.cb(
			dev_data->uart.callback.dev,
			dev_data->uart.callback.cb_data);
	}
}

static void tx_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_bt_data *dev_data = CONTAINER_OF(dwork, struct uart_bt_data, uart.tx_work);
	uint8_t *data = NULL;
	size_t len;
	int err;

	__ASSERT_NO_MSG(dev_data);

	uint16_t chunk_size = get_max_chunk_size();
	do {
		/** The chunk size is based on the smallest MTU among all
		 * peers, and the same chunk is sent to everyone. This avoids
		 * managing separate read pointers: one per connection.
		 */
		len = ring_buf_get_claim(dev_data->uart.tx_ringbuf, &data, chunk_size);
		if (len > 0) {
			err = bt_nus_inst_send(NULL, dev_data->bt.inst, data, len);
			if (err) {
				LOG_ERR("Failed to send data over BT: %d", err);
			}
		}

		ring_buf_get_finish(dev_data->uart.tx_ringbuf, len);
	} while (len > 0 && !err);

	if ((ring_buf_space_get(dev_data->uart.tx_ringbuf) > 0) && dev_data->uart.tx_irq_ena) {
		k_work_submit_to_queue(&nus_work_queue, &dev_data->uart.cb_work);
	}
}

static int uart_bt_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;
	size_t wrote;

	wrote = ring_buf_put(dev_data->uart.tx_ringbuf, tx_data, len);
	if (wrote < len) {
		LOG_WRN("Ring buffer full, drop %zd bytes", len - wrote);
	}

	if (atomic_get(&dev_data->bt.enabled)) {
		k_work_reschedule_for_queue(&nus_work_queue, &dev_data->uart.tx_work, K_NO_WAIT);
	}

	return wrote;
}

static int uart_bt_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;

	return ring_buf_get(dev_data->uart.rx_ringbuf, rx_data, size);
}

static int uart_bt_poll_in(const struct device *dev, unsigned char *c)
{
	int err = uart_bt_fifo_read(dev, c, 1);

	return err == 1 ? 0 : -1;
}

static void uart_bt_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;
	struct ring_buf *ringbuf = dev_data->uart.tx_ringbuf;

	/** Right now we're discarding data if ring-buf is full. */
	while (!ring_buf_put(ringbuf, &c, 1)) {
		if (k_is_in_isr() || !atomic_get(&dev_data->bt.enabled)) {
			LOG_WRN_ONCE("Ring buffer full, discard %c", c);
			break;
		}

		k_sleep(K_MSEC(1));
	}

	/** Don't flush the data until notifications are enabled. */
	if (atomic_get(&dev_data->bt.enabled)) {
		/** Delay will allow buffering some characters before transmitting
		 * data, so more than one byte is transmitted (e.g: when poll_out is
		 * called inside a for-loop).
		 */
		k_work_schedule_for_queue(&nus_work_queue, &dev_data->uart.tx_work, K_MSEC(1));
	}
}

static int uart_bt_irq_tx_ready(const struct device *dev)
{
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;

	if ((ring_buf_space_get(dev_data->uart.tx_ringbuf) > 0) && dev_data->uart.tx_irq_ena) {
		return 1;
	}

	return 0;
}

static void uart_bt_irq_tx_enable(const struct device *dev)
{
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;

	dev_data->uart.tx_irq_ena = true;

	if (uart_bt_irq_tx_ready(dev)) {
		k_work_submit_to_queue(&nus_work_queue, &dev_data->uart.cb_work);
	}
}

static void uart_bt_irq_tx_disable(const struct device *dev)
{
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;

	dev_data->uart.tx_irq_ena = false;
}

static int uart_bt_irq_rx_ready(const struct device *dev)
{
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;

	if (!ring_buf_is_empty(dev_data->uart.rx_ringbuf) && dev_data->uart.rx_irq_ena) {
		return 1;
	}

	return 0;
}

static void uart_bt_irq_rx_enable(const struct device *dev)
{
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;

	dev_data->uart.rx_irq_ena = true;

	k_work_submit_to_queue(&nus_work_queue, &dev_data->uart.cb_work);
}

static void uart_bt_irq_rx_disable(const struct device *dev)
{
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;

	dev_data->uart.rx_irq_ena = false;
}

static int uart_bt_irq_is_pending(const struct device *dev)
{
	return uart_bt_irq_rx_ready(dev);
}

static int uart_bt_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static void uart_bt_irq_callback_set(const struct device *dev,
				     uart_irq_callback_user_data_t cb,
				     void *cb_data)
{
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;

	dev_data->uart.callback.cb = cb;
	dev_data->uart.callback.cb_data = cb_data;
}

static const struct uart_driver_api uart_bt_driver_api = {
	.poll_in = uart_bt_poll_in,
	.poll_out = uart_bt_poll_out,
	.fifo_fill = uart_bt_fifo_fill,
	.fifo_read = uart_bt_fifo_read,
	.irq_tx_enable = uart_bt_irq_tx_enable,
	.irq_tx_disable = uart_bt_irq_tx_disable,
	.irq_tx_ready = uart_bt_irq_tx_ready,
	.irq_rx_enable = uart_bt_irq_rx_enable,
	.irq_rx_disable = uart_bt_irq_rx_disable,
	.irq_rx_ready = uart_bt_irq_rx_ready,
	.irq_is_pending = uart_bt_irq_is_pending,
	.irq_update = uart_bt_irq_update,
	.irq_callback_set = uart_bt_irq_callback_set,
};

static int uart_bt_workqueue_init(void)
{
	k_work_queue_init(&nus_work_queue);
	k_work_queue_start(&nus_work_queue, nus_work_queue_stack,
			   K_THREAD_STACK_SIZEOF(nus_work_queue_stack),
			   CONFIG_UART_BT_WORKQUEUE_PRIORITY, NULL);

	return 0;
}

/** The work-queue is shared across all instances, hence we initialize it separatedly */
SYS_INIT(uart_bt_workqueue_init, POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY);

static int uart_bt_init(const struct device *dev)
{
	int err;
	struct uart_bt_data *dev_data = (struct uart_bt_data *)dev->data;

	/** As a way to backtrace the device handle from uart_bt_data.
	 * Used in cb_work_handler.
	 */
	dev_data->uart.callback.dev = dev;

	k_work_init_delayable(&dev_data->uart.tx_work, tx_work_handler);
	k_work_init(&dev_data->uart.cb_work, cb_work_handler);

	err = bt_nus_inst_cb_register(dev_data->bt.inst, &dev_data->bt.cb, (void *)dev);
	if (err) {
		return err;
	}

	return 0;
}

#define UART_BT_RX_FIFO_SIZE(inst) (DT_INST_PROP(inst, rx_fifo_size))
#define UART_BT_TX_FIFO_SIZE(inst) (DT_INST_PROP(inst, tx_fifo_size))

#define UART_BT_INIT(n)										   \
												   \
	BT_NUS_INST_DEFINE(bt_nus_inst_##n);							   \
												   \
	RING_BUF_DECLARE(bt_nus_rx_rb_##n, UART_BT_RX_FIFO_SIZE(n));				   \
	RING_BUF_DECLARE(bt_nus_tx_rb_##n, UART_BT_TX_FIFO_SIZE(n));				   \
												   \
	static struct uart_bt_data uart_bt_data_##n = {						   \
		.bt = {										   \
			.inst = &bt_nus_inst_##n,						   \
			.enabled = ATOMIC_INIT(0),						   \
			.cb = {									   \
				.notif_enabled = bt_notif_enabled,				   \
				.received = bt_received,					   \
			},									   \
		},										   \
		.uart = {									   \
			.rx_ringbuf = &bt_nus_rx_rb_##n,					   \
			.tx_ringbuf = &bt_nus_tx_rb_##n,					   \
		},										   \
	};											   \
												   \
	DEVICE_DT_INST_DEFINE(n, uart_bt_init, NULL, &uart_bt_data_##n,				   \
			      NULL, PRE_KERNEL_1,						   \
			      CONFIG_SERIAL_INIT_PRIORITY,					   \
			      &uart_bt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_BT_INIT)
