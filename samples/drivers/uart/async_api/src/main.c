/*
 * Copyright (c) 2025 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net_buf.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/random/random.h>

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

/* Maximum number of packets to generate per iteration */
#define LOOP_ITER_MAX_TX 4
/* Maximum size of our TX packets */
#define MAX_TX_LEN 32
#define RX_CHUNK_LEN 32

/* Buffer pool for our TX payloads */
NET_BUF_POOL_DEFINE(tx_pool, LOOP_ITER_MAX_TX, MAX_TX_LEN, 0, NULL);

struct k_fifo tx_queue;
struct net_buf *tx_pending_buffer;
uint8_t async_rx_buffer[2][RX_CHUNK_LEN];
volatile uint8_t async_rx_buffer_idx;

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct net_buf *buf;
	int rc;

	LOG_DBG("EVENT: %d", evt->type);

	switch (evt->type) {
	case UART_TX_DONE:
		LOG_DBG("TX complete %p", tx_pending_buffer);

		/* Free TX buffer */
		net_buf_unref(tx_pending_buffer);
		tx_pending_buffer = NULL;

		/* Handle any queued buffers */
		buf = k_fifo_get(&tx_queue, K_NO_WAIT);
		if (buf != NULL) {
			rc = uart_tx(dev, buf->data, buf->len, 0);
			if (rc != 0) {
				LOG_ERR("TX from ISR failed (%d)", rc);
				net_buf_unref(buf);
			} else {
				tx_pending_buffer = buf;
			}
		}
		break;
	case UART_RX_BUF_REQUEST:
		/* Return the next buffer index */
		LOG_DBG("Providing buffer index %d", async_rx_buffer_idx);
		rc = uart_rx_buf_rsp(dev, async_rx_buffer[async_rx_buffer_idx],
				     sizeof(async_rx_buffer[0]));
		__ASSERT_NO_MSG(rc == 0);
		async_rx_buffer_idx = async_rx_buffer_idx ? 0 : 1;
		break;
	case UART_RX_BUF_RELEASED:
	case UART_RX_DISABLED:
		break;
	case UART_RX_RDY:
		LOG_HEXDUMP_INF(evt->data.rx.buf + evt->data.rx.offset,
				evt->data.rx.len, "RX_RDY");
		break;
	default:
		LOG_WRN("Unhandled event %d", evt->type);
	}
}

int main(void)
{
	bool rx_enabled = false;
	struct net_buf *tx_buf;
	int loop_counter = 0;
	uint8_t num_tx;
	int tx_len;
	int rc;

	/* Register the async interrupt handler */
	uart_callback_set(uart_dev, uart_callback, (void *)uart_dev);

	while (1) {
		/* Wait a while until the next burst transmission */
		k_sleep(K_SECONDS(5));

		/* Each loop, try to send a random number of packets */
		num_tx = (sys_rand32_get() % LOOP_ITER_MAX_TX) + 1;
		LOG_INF("Loop %d: Sending %d packets", loop_counter, num_tx);
		for (int i = 0; i < num_tx; i++) {
			/* Allocate the data packet */
			tx_buf = net_buf_alloc(&tx_pool, K_FOREVER);
			/* Populate it with data */
			tx_len = snprintk(tx_buf->data, net_buf_tailroom(tx_buf),
					  "Loop %d: Packet: %d\r\n", loop_counter, i);
			net_buf_add(tx_buf, tx_len);

			/* Queue packet for transmission */
			rc = uart_tx(uart_dev, tx_buf->data, tx_buf->len, SYS_FOREVER_US);
			if (rc == 0) {
				/* Store the pending buffer */
				tx_pending_buffer = tx_buf;
			} else if (rc == -EBUSY) {
				/* Transmission is already in progress */
				LOG_DBG("Queuing buffer %p", tx_buf);
				k_fifo_put(&tx_queue, tx_buf);
			} else {
				LOG_ERR("Unknown error (%d)", rc);
			}
		}

		/* Toggle the RX state */
		if (rx_enabled) {
			uart_rx_disable(uart_dev);
		} else {
			async_rx_buffer_idx = 1;
			uart_rx_enable(uart_dev, async_rx_buffer[0], RX_CHUNK_LEN, 100);
		}
		rx_enabled = !rx_enabled;
		LOG_INF("RX is now %s", rx_enabled ? "enabled" : "disabled");

		loop_counter += 1;
	}
}
