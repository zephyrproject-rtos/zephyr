/*
 * Copyright (c) 2020 Tridonic GmbH & Co KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_OPENTHREAD_PLATFORM_LOG_LEVEL
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

#if defined(CONFIG_OPENTHREAD_COPROCESSOR_UART_ASYNC)
#define OT_UART_ASYNC
#endif

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

static bool is_panic_mode;
static bool uart_enabled;

#ifndef OT_UART_ASYNC
#define RX_FIFO_SIZE 128

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
		atomic_clear(&ot_uart.tx_busy);
		atomic_set(&ot_uart.tx_finished, 1);
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
#else
/* UART with async rx buffer */
static uint8_t rx_buf[CONFIG_OPENTHREAD_COPROCESSOR_UART_RX_BUFFER_SIZE];

/* Async API */
static void uart_async_cb(const struct device *dev,
			  struct uart_event *evt,
			  void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_RX_RDY: {
		size_t len = ring_buf_put(ot_uart.rx_ringbuf,
					  evt->data.rx.buf,
					  evt->data.rx.len);

		if (len > 0) {
			otSysEventSignalPending();
		}

		if (len < evt->data.rx.len) {
			LOG_WRN("RX ring buffer full.");
		}

		break;
	}
	case UART_RX_DISABLED:
		if (uart_enabled) {
			uart_rx_enable(dev, rx_buf, sizeof(rx_buf), SYS_FOREVER_MS);
		}
		break;

	case UART_TX_DONE:
	case UART_TX_ABORTED:
		atomic_clear(&ot_uart.tx_busy);
		atomic_set(&ot_uart.tx_finished, 1);
		otSysEventSignalPending();
		break;
	default:
		break;
	}
}
#endif

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
	if (atomic_get(&ot_uart.tx_finished)) {
		LOG_DBG("UART TX done");
		otPlatUartSendDone();
		atomic_clear(&ot_uart.tx_finished);
	}
}

otError otPlatUartEnable(void)
{
	ot_uart.dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_ot_uart));

	if (!device_is_ready(ot_uart.dev)) {
		LOG_ERR("UART device not ready");
		return OT_ERROR_FAILED;
	}

	if (DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_ot_uart), zephyr_cdc_acm_uart)) {
		/* Data Carrier Detect Modem - mark connection as established */
		(void)uart_line_ctrl_set(ot_uart.dev, UART_LINE_CTRL_DCD, 1);
		/* Data Set Ready - the NCP SoC is ready to communicate */
		(void)uart_line_ctrl_set(ot_uart.dev, UART_LINE_CTRL_DSR, 1);
	}

	uart_enabled = true;
#ifndef OT_UART_ASYNC
	uart_irq_callback_user_data_set(ot_uart.dev,
					uart_callback,
					(void *)&ot_uart);

	uart_irq_rx_enable(ot_uart.dev);
#else
	uart_callback_set(ot_uart.dev, uart_async_cb, NULL);
	uart_rx_enable(ot_uart.dev, rx_buf, sizeof(rx_buf), SYS_FOREVER_MS);
#endif

	return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
	uart_enabled = false;
#ifndef OT_UART_ASYNC
	uart_irq_tx_disable(ot_uart.dev);
	uart_irq_rx_disable(ot_uart.dev);
#else
	uart_rx_disable(ot_uart.dev);
	uart_tx_abort(ot_uart.dev);
#endif
	return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
	if (aBuf == NULL) {
		return OT_ERROR_FAILED;
	}

	if (atomic_cas(&ot_uart.tx_busy, 0, 1)) {
#ifndef OT_UART_ASYNC
		write_buffer = aBuf;
		write_length = aBufLength;

		if (is_panic_mode) {
			/* In panic mode all data have to be sent immediately
			 * without using interrupts
			 */
			otPlatUartFlush();
		} else {
			uart_irq_tx_enable(ot_uart.dev);
		}
		return OT_ERROR_NONE;
#else
		int ret = uart_tx(ot_uart.dev, aBuf, aBufLength, SYS_FOREVER_MS);

		if (ret == 0) {
			if (is_panic_mode) {
				/* In panic mode all data have to
				 *  be sent before returning
				 */
				otPlatUartFlush();
			}
			return OT_ERROR_NONE;
		}

		/* TX did not start; clear busy flag */
		atomic_clear(&ot_uart.tx_busy);
		if (ret == -EBUSY) {
			return OT_ERROR_BUSY;
		}

		return OT_ERROR_FAILED;
#endif
	}

	return OT_ERROR_BUSY;
}

otError otPlatUartFlush(void)
{
	otError result = OT_ERROR_NONE;

#ifndef OT_UART_ASYNC
	if (write_length) {
		for (size_t i = 0; i < write_length; i++) {
			uart_poll_out(ot_uart.dev, *(write_buffer+i));
		}
	}

	atomic_clear(&ot_uart.tx_busy);
	atomic_set(&ot_uart.tx_finished, 1);
	otSysEventSignalPending();
#else
	while (atomic_get(&ot_uart.tx_busy)) {
		k_sleep(K_MSEC(1));
	}
#endif
	return result;
}

void platformUartPanic(void)
{
	is_panic_mode = true;
	/* In panic mode reception is not supported.
	 * In non-async mode, transmission is done without using interrupts
	 * (polling API). In async mode, transmission remains async but
	 * otPlatUartSend() blocks until the transfer completes.
	 */
#ifndef OT_UART_ASYNC
	uart_irq_tx_disable(ot_uart.dev);
	uart_irq_rx_disable(ot_uart.dev);
#else
	uart_rx_disable(ot_uart.dev);
#endif
}
