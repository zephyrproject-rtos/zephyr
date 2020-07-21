/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell_uart.h>
#include <drivers/uart.h>
#include <init.h>
#include <logging/log.h>

#define LOG_MODULE_NAME shell_uart
LOG_MODULE_REGISTER(shell_uart);

#ifdef CONFIG_SHELL_BACKEND_SERIAL_RX_POLL_PERIOD
#define RX_POLL_PERIOD K_MSEC(CONFIG_SHELL_BACKEND_SERIAL_RX_POLL_PERIOD)
#else
#define RX_POLL_PERIOD K_NO_WAIT
#endif

SHELL_UART_DEFINE(shell_transport_uart,
		  CONFIG_SHELL_BACKEND_SERIAL_TX_RING_BUFFER_SIZE,
		  CONFIG_SHELL_BACKEND_SERIAL_RX_RING_BUFFER_SIZE);
SHELL_DEFINE(shell_uart, CONFIG_SHELL_PROMPT_UART, &shell_transport_uart,
	     CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_SIZE,
	     CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_TIMEOUT,
	     SHELL_FLAG_OLF_CRLF);

#ifdef CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN
static void uart_rx_handle(struct device *dev,
			   const struct shell_uart *sh_uart)
{
	uint8_t *data;
	uint32_t len;
	uint32_t rd_len;
	bool new_data = false;

	do {
		len = ring_buf_put_claim(sh_uart->rx_ringbuf, &data,
					 sh_uart->rx_ringbuf->size);

		if (len > 0) {
			rd_len = uart_fifo_read(dev, data, len);
#ifdef CONFIG_MCUMGR_SMP_SHELL
			/* Divert bytes from shell handling if it is
			 * part of an mcumgr frame.
			 */
			size_t i;

			for (i = 0; i < rd_len; i++) {
				if (!smp_shell_rx_byte(&sh_uart->ctrl_blk->smp,
						       data[i])) {
					break;
				}
			}

			rd_len -= i;
			new_data = true;
			if (rd_len) {
				for (uint32_t j = 0; j < rd_len; j++) {
					data[j] = data[i + j];
				}
			}
#else
			if (rd_len > 0) {
				new_data = true;
			}
#endif /* CONFIG_MCUMGR_SMP_SHELL */
			int err = ring_buf_put_finish(sh_uart->rx_ringbuf,
						      rd_len);
			(void)err;
			__ASSERT_NO_MSG(err == 0);
		} else {
			uint8_t dummy;

			/* No space in the ring buffer - consume byte. */
			LOG_WRN("RX ring buffer full.");

			rd_len = uart_fifo_read(dev, &dummy, 1);
#ifdef CONFIG_MCUMGR_SMP_SHELL
			/* Divert this byte from shell handling if it
			 * is part of an mcumgr frame.
			 */
			smp_shell_rx_byte(&sh_uart->ctrl_blk->smp, dummy);
#endif /* CONFIG_MCUMGR_SMP_SHELL */
		}
	} while (rd_len && (rd_len == len));

	if (new_data) {
		sh_uart->ctrl_blk->handler(SHELL_TRANSPORT_EVT_RX_RDY,
					   sh_uart->ctrl_blk->context);
	}
}

static void uart_tx_handle(struct device *dev, const struct shell_uart *sh_uart)
{
	uint32_t len;
	int err;
	const uint8_t *data;

	len = ring_buf_get_claim(sh_uart->tx_ringbuf, (uint8_t **)&data,
				 sh_uart->tx_ringbuf->size);
	if (len) {
		len = uart_fifo_fill(dev, data, len);
		err = ring_buf_get_finish(sh_uart->tx_ringbuf, len);
		__ASSERT_NO_MSG(err == 0);
	} else {
		uart_irq_tx_disable(dev);
		sh_uart->ctrl_blk->tx_busy = 0;
	}

	sh_uart->ctrl_blk->handler(SHELL_TRANSPORT_EVT_TX_RDY,
				   sh_uart->ctrl_blk->context);
}

static void uart_callback(struct device *dev, void *user_data)
{
	const struct shell_uart *sh_uart = (struct shell_uart *)user_data;

	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		uart_rx_handle(dev, sh_uart);
	}

	if (uart_irq_tx_ready(dev)) {
		uart_tx_handle(dev, sh_uart);
	}
}
#endif /* CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN */

static void uart_irq_init(const struct shell_uart *sh_uart)
{
#ifdef CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN
	struct device *dev = sh_uart->ctrl_blk->dev;

	uart_irq_callback_user_data_set(dev, uart_callback, (void *)sh_uart);
	uart_irq_rx_enable(dev);
#endif
}

static void timer_handler(struct k_timer *timer)
{
	uint8_t c;
	const struct shell_uart *sh_uart = k_timer_user_data_get(timer);

	while (uart_poll_in(sh_uart->ctrl_blk->dev, &c) == 0) {
		if (ring_buf_put(sh_uart->rx_ringbuf, &c, 1) == 0U) {
			/* ring buffer full. */
			LOG_WRN("RX ring buffer full.");
		}
		sh_uart->ctrl_blk->handler(SHELL_TRANSPORT_EVT_RX_RDY,
					   sh_uart->ctrl_blk->context);
	}
}

static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	const struct shell_uart *sh_uart = (struct shell_uart *)transport->ctx;

	sh_uart->ctrl_blk->dev = (struct device *)config;
	sh_uart->ctrl_blk->handler = evt_handler;
	sh_uart->ctrl_blk->context = context;

	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN)) {
		uart_irq_init(sh_uart);
	} else {
		k_timer_init(sh_uart->timer, timer_handler, NULL);
		k_timer_user_data_set(sh_uart->timer, (void *)sh_uart);
		k_timer_start(sh_uart->timer, RX_POLL_PERIOD, RX_POLL_PERIOD);
	}

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	const struct shell_uart *sh_uart = (struct shell_uart *)transport->ctx;

	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN)) {
		struct device *dev = sh_uart->ctrl_blk->dev;

		uart_irq_rx_disable(dev);
	} else {
		k_timer_stop(sh_uart->timer);
	}

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking_tx)
{
	const struct shell_uart *sh_uart = (struct shell_uart *)transport->ctx;

	sh_uart->ctrl_blk->blocking_tx = blocking_tx;

	if (blocking_tx) {
#ifdef CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN
		uart_irq_tx_disable(sh_uart->ctrl_blk->dev);
#endif
	}

	return 0;
}

static void irq_write(const struct shell_uart *sh_uart, const void *data,
		     size_t length, size_t *cnt)
{
	*cnt = ring_buf_put(sh_uart->tx_ringbuf, data, length);

	if (atomic_set(&sh_uart->ctrl_blk->tx_busy, 1) == 0) {
#ifdef CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN
		uart_irq_tx_enable(sh_uart->ctrl_blk->dev);
#endif
	}
}

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{
	const struct shell_uart *sh_uart = (struct shell_uart *)transport->ctx;
	const uint8_t *data8 = (const uint8_t *)data;

	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN) &&
		!sh_uart->ctrl_blk->blocking_tx) {
		irq_write(sh_uart, data, length, cnt);
	} else {
		for (size_t i = 0; i < length; i++) {
			uart_poll_out(sh_uart->ctrl_blk->dev, data8[i]);
		}

		*cnt = length;

		sh_uart->ctrl_blk->handler(SHELL_TRANSPORT_EVT_TX_RDY,
					   sh_uart->ctrl_blk->context);
	}

	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct shell_uart *sh_uart = (struct shell_uart *)transport->ctx;

	*cnt = ring_buf_get(sh_uart->rx_ringbuf, data, length);

	return 0;
}

#ifdef CONFIG_MCUMGR_SMP_SHELL
static void update(const struct shell_transport *transport)
{
	struct shell_uart *sh_uart = (struct shell_uart *)transport->ctx;

	smp_shell_process(&sh_uart->ctrl_blk->smp);
}
#endif /* CONFIG_MCUMGR_SMP_SHELL */

const struct shell_transport_api shell_uart_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read,
#ifdef CONFIG_MCUMGR_SMP_SHELL
	.update = update,
#endif /* CONFIG_MCUMGR_SMP_SHELL */
};

static int enable_shell_uart(struct device *arg)
{
	ARG_UNUSED(arg);
	struct device *dev =
			device_get_binding(CONFIG_UART_SHELL_ON_DEV_NAME);
	bool log_backend = CONFIG_SHELL_BACKEND_SERIAL_LOG_LEVEL > 0;
	uint32_t level =
		(CONFIG_SHELL_BACKEND_SERIAL_LOG_LEVEL > LOG_LEVEL_DBG) ?
		CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_BACKEND_SERIAL_LOG_LEVEL;

	if (dev == NULL) {
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_MCUMGR_SMP_SHELL)) {
		smp_shell_init();
	}

	shell_init(&shell_uart, dev, true, log_backend, level);

	return 0;
}
SYS_INIT(enable_shell_uart, POST_KERNEL, 0);

const struct shell *shell_backend_uart_get_ptr(void)
{
	return &shell_uart;
}
