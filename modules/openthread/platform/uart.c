/*
 * Copyright (c) 2020 Tridonic GmbH & Co KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_OPENTHREAD_LOG_LEVEL
#define LOG_MODULE_NAME net_otPlat_uart

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/drivers/uart.h>

#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/usb/usb_device.h>

#include <openthread/ncp.h>
#include <openthread-system.h>
#include <utils/uart.h>

#include "platform-zephyr.h"

struct openthread_uart {
	struct ring_buf *rx_ringbuf;
	const struct device *dev;
	atomic_t tx_busy;
	atomic_t tx_finished;
};

#define OT_UART_DEFINE(_name, _ringbuf_size) \
	RING_BUF_DECLARE(_name##_rx_ringbuf, _ringbuf_size); \
	static struct openthread_uart _name = { \
		.rx_ringbuf = &_name##_rx_ringbuf, \
	}

OT_UART_DEFINE(ot_uart, CONFIG_OPENTHREAD_COPROCESSOR_UART_RING_BUFFER_SIZE);

#define RX_FIFO_SIZE 128

static bool is_panic_mode;
static const uint8_t *write_buffer;
static uint16_t write_length;

static void uart_rx_handle(const struct device *dev)
{
	uint8_t *data;
	uint32_t len;
	uint32_t rd_len;
	bool new_data = false;

	do {
		len = ring_buf_put_claim(
			ot_uart.rx_ringbuf, &data,
			ot_uart.rx_ringbuf->size);
		if (len > 0) {
			rd_len = uart_fifo_read(dev, data, len);
			if (rd_len > 0) {
				new_data = true;
			}

			int err = ring_buf_put_finish(
				ot_uart.rx_ringbuf, rd_len);
			(void)err;
			__ASSERT_NO_MSG(err == 0);
		} else {
			uint8_t dummy;

			/* No space in the ring buffer - consume byte. */
			LOG_WRN("RX ring buffer full.");

			rd_len = uart_fifo_read(dev, &dummy, 1);
		}
	} while (rd_len && (rd_len == len));

	if (new_data) {
		otSysEventSignalPending();
	}
}

static void uart_tx_handle(const struct device *dev)
{
	uint32_t len;

	if (write_length) {
		len = uart_fifo_fill(dev, write_buffer, write_length);
		write_buffer += len;
		write_length -= len;
	} else {
		uart_irq_tx_disable(dev);
		ot_uart.tx_busy = 0;
		atomic_set(&(ot_uart.tx_finished), 1);
		otSysEventSignalPending();
	}
}

static void uart_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {

		if (uart_irq_rx_ready(dev)) {
			uart_rx_handle(dev);
		}

		if (uart_irq_tx_ready(dev) &&
		    atomic_get(&ot_uart.tx_busy) == 1) {
			uart_tx_handle(dev);
		}
	}
}

void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
	otNcpHdlcReceive(aBuf, aBufLength);
}

void otPlatUartSendDone(void)
{
	otNcpHdlcSendDone();
}

void platformUartProcess(otInstance *aInstance)
{
	uint32_t len = 0;
	const uint8_t *data;

	/* Process UART RX */
	while ((len = ring_buf_get_claim(
			ot_uart.rx_ringbuf,
			(uint8_t **)&data,
			ot_uart.rx_ringbuf->size)) > 0) {
		int err;

		otPlatUartReceived(data, len);
		err = ring_buf_get_finish(
				ot_uart.rx_ringbuf,
				len);
		(void)err;
		__ASSERT_NO_MSG(err == 0);
	}

	/* Process UART TX */
	if (ot_uart.tx_finished) {
		LOG_DBG("UART TX done");
		otPlatUartSendDone();
		ot_uart.tx_finished = 0;
	}
}

otError otPlatUartEnable(void)
{
	ot_uart.dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_ot_uart));

	if (!device_is_ready(ot_uart.dev)) {
		LOG_ERR("UART device not ready");
		return OT_ERROR_FAILED;
	}

	uart_irq_callback_user_data_set(ot_uart.dev,
					uart_callback,
					(void *)&ot_uart);

	if (DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_ot_uart), zephyr_cdc_acm_uart)) {
		int ret;
		uint32_t dtr = 0U;

		ret = usb_enable(NULL);
		if (ret != 0) {
			LOG_ERR("Failed to enable USB");
			return OT_ERROR_FAILED;
		}

		LOG_INF("Waiting for host to be ready to communicate");

		/* Data Terminal Ready - check if host is ready to communicate */
		while (!dtr) {
			ret = uart_line_ctrl_get(ot_uart.dev,
						 UART_LINE_CTRL_DTR, &dtr);
			if (ret) {
				LOG_ERR("Failed to get Data Terminal Ready line state: %d",
					ret);
				continue;
			}
			k_msleep(100);
		}

		/* Data Carrier Detect Modem - mark connection as established */
		(void)uart_line_ctrl_set(ot_uart.dev, UART_LINE_CTRL_DCD, 1);
		/* Data Set Ready - the NCP SoC is ready to communicate */
		(void)uart_line_ctrl_set(ot_uart.dev, UART_LINE_CTRL_DSR, 1);
	}

	uart_irq_rx_enable(ot_uart.dev);

	return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
	if (DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_ot_uart), zephyr_cdc_acm_uart)) {
		int ret = usb_disable();

		if (ret) {
			LOG_WRN("Failed to disable USB (%d)", ret);
		}
	}

	uart_irq_tx_disable(ot_uart.dev);
	uart_irq_rx_disable(ot_uart.dev);
	return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
	if (aBuf == NULL) {
		return OT_ERROR_FAILED;
	}

	if (atomic_cas(&(ot_uart.tx_busy), 0, 1)) {
		write_buffer = aBuf;
		write_length = aBufLength;

		if (is_panic_mode) {
			/* In panic mode all data have to be send immediately
			 * without using interrupts
			 */
			otPlatUartFlush();
		} else {
			uart_irq_tx_enable(ot_uart.dev);
		}
		return OT_ERROR_NONE;
	}

	return OT_ERROR_BUSY;
}

otError otPlatUartFlush(void)
{
	otError result = OT_ERROR_NONE;

	if (write_length) {
		for (size_t i = 0; i < write_length; i++) {
			uart_poll_out(ot_uart.dev, *(write_buffer+i));
		}
	}

	ot_uart.tx_busy = 0;
	atomic_set(&(ot_uart.tx_finished), 1);
	otSysEventSignalPending();
	return result;
}

void platformUartPanic(void)
{
	is_panic_mode = true;
	/* In panic mode data are send without using interrupts.
	 * Reception in this mode is not supported.
	 */
	uart_irq_tx_disable(ot_uart.dev);
	uart_irq_rx_disable(ot_uart.dev);
}
