/*
 *  Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_uart.h"

K_SEM_DEFINE(tx_done, 0, 1);
K_SEM_DEFINE(tx_aborted, 0, 1);
K_SEM_DEFINE(rx_rdy, 0, 1);
K_SEM_DEFINE(rx_buf_released, 0, 1);
K_SEM_DEFINE(rx_disabled, 0, 1);



void test_single_read_callback(struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_TX_ABORTED:
		(*(u32_t *)user_data)++;
		break;
	case UART_RX_RDY:
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_RELEASED:
		k_sem_give(&rx_buf_released);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}

}

void test_single_read(void)
{
	struct device *uart_dev = device_get_binding(UART_DEVICE_NAME);

	volatile u32_t tx_aborted_count = 0;
	u8_t rx_buf[10] = {0};
	u8_t tx_buf[5] = "test";

	zassert_not_equal(memcmp(tx_buf, rx_buf, 5), 0,
			  "Initial buffer check failed");

	uart_callback_set(uart_dev,
			  test_single_read_callback,
			  (void *) &tx_aborted_count);

	uart_rx_enable(uart_dev, rx_buf, 10, 50);
	uart_tx(uart_dev, tx_buf, sizeof(tx_buf), 100);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");

	zassert_equal(memcmp(tx_buf, rx_buf, 5), 0, "Buffers not equal");
	zassert_not_equal(memcmp(tx_buf, rx_buf+5, 5), 0, "Buffers not equal");

	uart_tx(uart_dev, tx_buf, sizeof(tx_buf), 100);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)),
		      0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(1000)), 0,
		      "RX_DISABLED timeout");
	zassert_equal(memcmp(tx_buf, rx_buf+5, 5), 0, "Buffers not equal");
	zassert_equal(tx_aborted_count, 0, "TX aborted triggered");
}

u8_t chained_read_buf0[10];
u8_t chained_read_buf1[20];
u8_t chained_read_buf2[30];
u8_t buf_num = 1U;
u8_t *read_ptr;
volatile size_t read_len;

void test_chained_read_callback(struct uart_event *evt, void *user_data)
{
	struct device *uart_dev = (struct device *) user_data;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_RX_RDY:
		read_ptr = evt->data.rx.buf + evt->data.rx.offset;
		read_len = evt->data.rx.len;
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_REQUEST:
		if (buf_num == 1U) {
			uart_rx_buf_rsp(uart_dev,
					chained_read_buf1,
					sizeof(chained_read_buf1));
			buf_num = 2U;
		} else if (buf_num == 2U) {
			uart_rx_buf_rsp(uart_dev,
					chained_read_buf2,
					sizeof(chained_read_buf2));
			buf_num = 0U;
		}
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}

}

void test_chained_read(void)
{
	struct device *uart_dev = device_get_binding(UART_DEVICE_NAME);

	u8_t tx_buf[10];

	uart_callback_set(uart_dev, test_chained_read_callback, uart_dev);

	uart_rx_enable(uart_dev, chained_read_buf0, 10, 50);

	for (int i = 0; i < 6; i++) {
		zassert_not_equal(k_sem_take(&rx_disabled, K_MSEC(10)),
				  0,
				  "RX_DISABLED occurred");
		snprintf(tx_buf, sizeof(tx_buf), "Message %d", i);
		uart_tx(uart_dev, tx_buf, sizeof(tx_buf), 100);
		zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0,
			      "TX_DONE timeout");
		zassert_equal(k_sem_take(&rx_rdy, K_MSEC(1000)), 0,
			      "RX_RDY timeout");
		size_t read_len_temp = read_len;

		zassert_equal(read_len_temp, sizeof(tx_buf),
			      "Incorrect read length");
		zassert_equal(memcmp(tx_buf, read_ptr, sizeof(tx_buf)),
			      0,
			      "Buffers not equal");
	}
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
}

u8_t double_buffer[2][12];
u8_t *next_buf = double_buffer[1];

void test_double_buffer_callback(struct uart_event *evt, void *user_data)
{
	struct device *uart_dev = (struct device *) user_data;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_RX_RDY:
		read_ptr = evt->data.rx.buf + evt->data.rx.offset;
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_REQUEST:
		uart_rx_buf_rsp(uart_dev, next_buf, sizeof(double_buffer[0]));
		break;
	case UART_RX_BUF_RELEASED:
		next_buf = evt->data.rx_buf.buf;
		k_sem_give(&rx_buf_released);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}

}

void test_double_buffer(void)
{
	struct device *uart_dev = device_get_binding(UART_DEVICE_NAME);

	u8_t tx_buf[4];

	uart_callback_set(uart_dev, test_double_buffer_callback, uart_dev);

	zassert_equal(uart_rx_enable(uart_dev,
				     double_buffer[0],
				     sizeof(double_buffer[0]),
				     50),
		      0,
		      "Failed to enable receiving");

	for (int i = 0; i < 100; i++) {
		snprintf(tx_buf, sizeof(tx_buf), "%03d", i);
		uart_tx(uart_dev, tx_buf, sizeof(tx_buf), 100);
		zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0,
			      "TX_DONE timeout");
		zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0,
			      "RX_RDY timeout");
		zassert_equal(memcmp(tx_buf, read_ptr, sizeof(tx_buf)),
			      0,
			      "Buffers not equal");
	}
	uart_rx_disable(uart_dev);
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
}

void test_read_abort_callback(struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_RX_RDY:
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_RELEASED:
		k_sem_give(&rx_buf_released);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}
}

void test_read_abort(void)
{
	struct device *uart_dev = device_get_binding(UART_DEVICE_NAME);

	u8_t rx_buf[100];
	u8_t tx_buf[100];

	memset(rx_buf, 0, sizeof(rx_buf));
	memset(tx_buf, 1, sizeof(tx_buf));

	uart_callback_set(uart_dev, test_read_abort_callback, NULL);

	uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), 50);

	uart_tx(uart_dev, tx_buf, 5, 100);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(memcmp(tx_buf, rx_buf, 5), 0, "Buffers not equal");


	uart_tx(uart_dev, tx_buf, 95, 100);
	uart_rx_disable(uart_dev);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)),
		      0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
	zassert_not_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0,
			  "RX_RDY occurred");
	zassert_not_equal(memcmp(tx_buf, rx_buf, 100), 0, "Buffers equal");
}

volatile size_t sent;
volatile size_t received;

void test_write_abort_callback(struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_TX_ABORTED:
		sent = evt->data.tx.len;
		k_sem_give(&tx_aborted);
		break;
	case UART_RX_RDY:
		received = evt->data.rx.len;
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_RELEASED:
		k_sem_give(&rx_buf_released);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}
}

void test_write_abort(void)
{
	struct device *uart_dev = device_get_binding(UART_DEVICE_NAME);

	u8_t rx_buf[100];
	u8_t tx_buf[100];

	memset(rx_buf, 0, sizeof(rx_buf));
	memset(tx_buf, 1, sizeof(tx_buf));

	uart_callback_set(uart_dev, test_write_abort_callback, NULL);

	uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), 50);

	uart_tx(uart_dev, tx_buf, 5, 100);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(memcmp(tx_buf, rx_buf, 5), 0, "Buffers not equal");

	uart_tx(uart_dev, tx_buf, 95, 100);
	uart_tx_abort(uart_dev);
	zassert_equal(k_sem_take(&tx_aborted, K_MSEC(100)), 0,
		      "TX_ABORTED timeout");
	if (sent != 0) {
		zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0,
			      "RX_RDY timeout");
		zassert_equal(sent, received, "Sent is not equal to received.");
	}
	uart_rx_disable(uart_dev);
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)),
		      0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
}

u8_t chained_write_tx_bufs[2][10] = {"Message 1", "Message 2"};
bool chained_write_next_buf = true;
volatile u8_t tx_sent;

void test_chained_write_callback(struct uart_event *evt, void *user_data)
{
	struct device *uart_dev = (struct device *) user_data;

	switch (evt->type) {
	case UART_TX_DONE:
		if (chained_write_next_buf) {
			uart_tx(uart_dev, chained_write_tx_bufs[1], 10, 100);
			chained_write_next_buf = false;
		}
		tx_sent = 1;
		k_sem_give(&tx_done);
		break;
	case UART_TX_ABORTED:
		sent = evt->data.tx.len;
		k_sem_give(&tx_aborted);
		break;
	case UART_RX_RDY:
		received = evt->data.rx.len;
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_RELEASED:
		k_sem_give(&rx_buf_released);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}
}

void test_chained_write(void)
{
	struct device *uart_dev = device_get_binding(UART_DEVICE_NAME);

	u8_t rx_buf[20];

	memset(rx_buf, 0, sizeof(rx_buf));

	uart_callback_set(uart_dev, test_chained_write_callback, uart_dev);

	uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), 50);

	uart_tx(uart_dev, chained_write_tx_bufs[0], 10, 100);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(chained_write_next_buf, false, "Sent no message");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(memcmp(chained_write_tx_bufs[0], rx_buf, 10),
		      0,
		      "Buffers not equal");
	zassert_equal(memcmp(chained_write_tx_bufs[1], rx_buf + 10, 10),
		      0,
		      "Buffers not equal");

	uart_rx_disable(uart_dev);
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)),
		      0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
}

u8_t long_rx_buf[1024];
u8_t long_rx_buf2[1024];
u8_t long_tx_buf[1000];
volatile u8_t evt_num;
size_t long_received[2];

void test_long_buffers_callback(struct uart_event *evt, void *user_data)
{
	struct device *uart_dev = (struct device *) user_data;
	static bool next_buf = true;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_TX_ABORTED:
		sent = evt->data.tx.len;
		k_sem_give(&tx_aborted);
		break;
	case UART_RX_RDY:
		long_received[evt_num] = evt->data.rx.len;
		evt_num++;
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_RELEASED:
		k_sem_give(&rx_buf_released);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	case UART_RX_BUF_REQUEST:
		if (next_buf) {
			uart_rx_buf_rsp(uart_dev, long_rx_buf2, 1024);
			next_buf = false;
		}
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}
}

void test_long_buffers(void)
{
	struct device *uart_dev = device_get_binding(UART_DEVICE_NAME);


	memset(long_rx_buf, 0, sizeof(long_rx_buf));
	memset(long_tx_buf, 1, sizeof(long_tx_buf));

	uart_callback_set(uart_dev, test_long_buffers_callback, uart_dev);

	uart_rx_enable(uart_dev, long_rx_buf, sizeof(long_rx_buf), 10);

	uart_tx(uart_dev, long_tx_buf, 500, 200);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(200)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(200)), 0, "RX_RDY timeout");
	zassert_equal(long_received[0], 500, "Wrong number of bytes received.");
	zassert_equal(memcmp(long_tx_buf, long_rx_buf, 500),
		      0,
		      "Buffers not equal");

	evt_num = 0;
	uart_tx(uart_dev, long_tx_buf, 1000, 200);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(200)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(200)), 0, "RX_RDY timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(200)), 0, "RX_RDY timeout");
	zassert_equal(long_received[0], 524, "Wrong number of bytes received.");
	zassert_equal(long_received[1], 476, "Wrong number of bytes received.");
	zassert_equal(memcmp(long_tx_buf, long_rx_buf + 500, long_received[0]),
		      0,
		      "Buffers not equal");
	zassert_equal(memcmp(long_tx_buf, long_rx_buf2, long_received[1]),
		      0,
		      "Buffers not equal");

	uart_rx_disable(uart_dev);
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)),
		      0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
}
