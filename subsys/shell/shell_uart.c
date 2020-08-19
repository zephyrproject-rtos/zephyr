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
static inline size_t smp_rx_bytes(const struct shell_uart *sh_uart,
				  uint8_t *data, size_t len)
{
#ifdef CONFIG_MCUMGR_SMP_SHELL
	return smp_shell_rx_bytes(&sh_uart->ctrl_blk->smp, data, len);
#else
	return 0;
#endif
}

static void uart_rx_handle(const struct device *dev,
			   const struct shell_uart *sh_uart)
{
	uint8_t discard_buffer[1];
	uint8_t *rd_buffer;
	uint32_t rd_space; /* space in rd_buffer */
	uint32_t rd_len; /* bytes read from uart_fifo */
	uint32_t rd_idx;
	uint32_t shell_rd_len; /* bytes to be consumed by shell (non-SMP) */
	bool new_data = false;

	do {
		rd_space = ring_buf_put_claim(sh_uart->rx_ringbuf, &rd_buffer,
					      sh_uart->rx_ringbuf->size);
		if (rd_space == 0) {
			LOG_WRN("RX ring buffer full.");

			rd_buffer = discard_buffer;
			rd_space = sizeof(discard_buffer);
		}

		rd_len = uart_fifo_read(dev, rd_buffer, rd_space);
		rd_idx = 0;
		shell_rd_len = 0;

		while (rd_idx < rd_len) {
			size_t consumed = smp_rx_bytes(sh_uart, &rd_buffer[rd_idx],
						       rd_len - rd_idx);

			rd_idx += consumed;

			if (rd_idx >= rd_len) {
				/* Whole rd_buffer was consumed at this point */
				break;
			}

			/* First byte that was not consumed is shell byte */
			rd_buffer[shell_rd_len++] = rd_buffer[rd_idx++];
		}

		if (rd_buffer != discard_buffer) {
			int err = ring_buf_put_finish(sh_uart->rx_ringbuf,
						      shell_rd_len);
			(void)err;
			__ASSERT_NO_MSG(err == 0);

			new_data = true;
		} else if (shell_rd_len < sizeof(discard_buffer)) {
			/*
			 * This is the case when ringbuf was full, but
			 * some SMP byte was consumed.
			 */
			new_data = true;
		}
	} while (rd_len && (rd_len == rd_space));

	if (new_data) {
		sh_uart->ctrl_blk->handler(SHELL_TRANSPORT_EVT_RX_RDY,
					   sh_uart->ctrl_blk->context);
	}
}

static void uart_tx_handle(const struct device *dev,
			   const struct shell_uart *sh_uart)
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

static void uart_callback(const struct device *dev, void *user_data)
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
	const struct device *dev = sh_uart->ctrl_blk->dev;

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

	sh_uart->ctrl_blk->dev = (const struct device *)config;
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
		const struct device *dev = sh_uart->ctrl_blk->dev;

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

static int enable_shell_uart(const struct device *arg)
{
	ARG_UNUSED(arg);
	const struct device *dev =
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
SYS_INIT(enable_shell_uart, POST_KERNEL,
	 CONFIG_SHELL_BACKEND_SERIAL_INIT_PRIORITY);

const struct shell *shell_backend_uart_get_ptr(void)
{
	return &shell_uart;
}
