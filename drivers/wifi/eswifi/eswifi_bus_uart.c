/**
 * Copyright (c) 2018 Linaro
 * Copyright (c) 2020 ATL Electronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT inventek_eswifi_uart

#include "eswifi_log.h"
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <sys/ring_buffer.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>

#include "eswifi.h"

#define ESWIFI_RING_BUF_SIZE 2048

enum eswifi_uart_fsm {
	ESWIFI_UART_FSM_WAIT_CR,
	ESWIFI_UART_FSM_WAIT_LF,
	ESWIFI_UART_FSM_WAIT_MARK,
	ESWIFI_UART_FSM_WAIT_SPACE,
	ESWIFI_UART_FSM_END,
};

struct eswifi_uart_data {
	const struct device *dev;
	enum eswifi_uart_fsm fsm;
	size_t rx_count;
	size_t rx_buf_size;
	char *rx_buf;

	/* RX Ring Buf */
	uint8_t iface_rb_buf[ESWIFI_RING_BUF_SIZE];
	struct ring_buf rx_rb;
};

static struct eswifi_uart_data eswifi_uart0; /* Static instance */

static void eswifi_iface_uart_flush(struct eswifi_uart_data *uart)
{
	uint8_t c;

	while (uart_fifo_read(uart->dev, &c, 1) > 0) {
		continue;
	}
}

static void eswifi_iface_uart_isr(const struct device *uart_dev,
				  void *user_data)
{
	struct eswifi_uart_data *uart = &eswifi_uart0; /* Static instance */
	int rx = 0;
	uint8_t *dst;
	uint32_t partial_size = 0;
	uint32_t total_size = 0;

	ARG_UNUSED(user_data);

	while (uart_irq_update(uart->dev) &&
	       uart_irq_rx_ready(uart->dev)) {
		if (!partial_size) {
			partial_size = ring_buf_put_claim(&uart->rx_rb, &dst,
							  UINT32_MAX);
		}
		if (!partial_size) {
			LOG_ERR("Rx buffer doesn't have enough space");
			eswifi_iface_uart_flush(uart);
			break;
		}

		rx = uart_fifo_read(uart->dev, dst, partial_size);
		if (rx <= 0) {
			continue;
		}

		dst += rx;
		total_size += rx;
		partial_size -= rx;
	}

	ring_buf_put_finish(&uart->rx_rb, total_size);
}

static char get_fsm_char(int fsm)
{
	switch (fsm) {
	case ESWIFI_UART_FSM_WAIT_CR:
		return('C');
	case ESWIFI_UART_FSM_WAIT_LF:
		return('L');
	case ESWIFI_UART_FSM_WAIT_MARK:
		return('M');
	case ESWIFI_UART_FSM_WAIT_SPACE:
		return('S');
	case ESWIFI_UART_FSM_END:
		return('E');
	}

	return('?');
}

static int eswifi_uart_get_resp(struct eswifi_uart_data *uart)
{
	uint8_t c;

	while (ring_buf_get(&uart->rx_rb, &c, 1) > 0) {
		LOG_DBG("FSM: %c, RX: 0x%02x : %c",
			get_fsm_char(uart->fsm), c, c);

		if (uart->rx_buf_size > 0) {
			uart->rx_buf[uart->rx_count++] = c;

			if (uart->rx_count == uart->rx_buf_size) {
				return -ENOMEM;
			}
		}

		switch (uart->fsm) {
		case ESWIFI_UART_FSM_WAIT_CR:
			if (c == '\r') {
				uart->fsm = ESWIFI_UART_FSM_WAIT_LF;
			}
			break;
		case ESWIFI_UART_FSM_WAIT_LF:
			if (c == '\n') {
				uart->fsm = ESWIFI_UART_FSM_WAIT_MARK;
			} else if (c != '\r') {
				uart->fsm = ESWIFI_UART_FSM_WAIT_CR;
			}
			break;
		case ESWIFI_UART_FSM_WAIT_MARK:
			if (c == '>') {
				uart->fsm = ESWIFI_UART_FSM_WAIT_SPACE;
			} else if (c == '\r') {
				uart->fsm = ESWIFI_UART_FSM_WAIT_LF;
			} else {
				uart->fsm = ESWIFI_UART_FSM_WAIT_CR;
			}
			break;
		case ESWIFI_UART_FSM_WAIT_SPACE:
			if (c == ' ') {
				uart->fsm = ESWIFI_UART_FSM_END;
			} else if (c == '\r') {
				uart->fsm = ESWIFI_UART_FSM_WAIT_LF;
			} else {
				uart->fsm = ESWIFI_UART_FSM_WAIT_CR;
			}
			break;
		default:
			break;
		}
	}

	return 0;
}

static int eswifi_uart_wait_prompt(struct eswifi_uart_data *uart)
{
	unsigned int max_retries = 60 * 1000; /* 1 minute */
	int err;

	while (--max_retries) {
		err = eswifi_uart_get_resp(uart);
		if (err) {
			LOG_DBG("Err: 0x%08x - %d", err, err);
			return err;
		}

		if (uart->fsm == ESWIFI_UART_FSM_END) {
			LOG_DBG("Success!");
			return uart->rx_count;
		}

		/* allow other threads to be scheduled */
		k_sleep(K_MSEC(1));
	}

	LOG_DBG("Timeout");
	return -ETIMEDOUT;
}

static int eswifi_uart_request(struct eswifi_dev *eswifi, char *cmd,
			       size_t clen, char *rsp, size_t rlen)
{
	struct eswifi_uart_data *uart = eswifi->bus_data;
	int count;
	int err;

	LOG_DBG("cmd=%p (%u byte), rsp=%p (%u byte)", cmd, clen, rsp, rlen);

	/* Send CMD */
	for (count = 0; count < clen; count++) {
		uart_poll_out(uart->dev, cmd[count]);
	}

	uart->fsm = ESWIFI_UART_FSM_WAIT_CR;
	uart->rx_count = 0;
	uart->rx_buf = rsp;
	uart->rx_buf_size = rlen;

	err = eswifi_uart_wait_prompt(uart);

	if (err > 0) {
		LOG_HEXDUMP_DBG(uart->rx_buf, uart->rx_count, "Stream");
	}

	return err;
}

int eswifi_uart_init(struct eswifi_dev *eswifi)
{
	struct eswifi_uart_data *uart = &eswifi_uart0; /* Static instance */

	uart->dev = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!uart->dev) {
		LOG_ERR("Failed to initialize uart driver");
		return -ENODEV;
	}

	eswifi->bus_data = uart;

	uart_irq_rx_disable(uart->dev);
	uart_irq_tx_disable(uart->dev);
	eswifi_iface_uart_flush(uart);
	uart_irq_callback_set(uart->dev, eswifi_iface_uart_isr);
	uart_irq_rx_enable(uart->dev);

	ring_buf_init(&uart->rx_rb, sizeof(uart->iface_rb_buf),
		      uart->iface_rb_buf);

	LOG_DBG("success");

	return 0;
}

static struct eswifi_bus_ops eswifi_bus_ops_uart = {
	.init = eswifi_uart_init,
	.request = eswifi_uart_request,
};

struct eswifi_bus_ops *eswifi_get_bus(void)
{
	return &eswifi_bus_ops_uart;
}
