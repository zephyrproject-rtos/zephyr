/*
 * Copyright (c) 2019 omSquare s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <SEGGER_RTT.h>

#define DT_DRV_COMPAT segger_rtt_uart

extern struct k_mutex rtt_term_mutex;

struct uart_rtt_config {
	void *up_buffer;
	size_t up_size;
	void *down_buffer;
	size_t down_size;
	uint8_t channel;
};

struct uart_rtt_data {
#ifdef CONFIG_UART_ASYNC_API
	uart_callback_t callback;
	void *user_data;
#endif /* CONFIG_UART_ASYNC_API */
};

static int uart_rtt_init(const struct device *dev)
{
	/*
	 * Channel 0 is initialized at compile-time, Kconfig ensures that
	 * it is configured in correct, non-blocking mode. Other channels
	 * need to be configured at run-time.
	 */
	if (dev->config) {
		const struct uart_rtt_config *cfg = dev->config;

		SEGGER_RTT_ConfigUpBuffer(cfg->channel, dev->name,
					  cfg->up_buffer, cfg->up_size,
					  SEGGER_RTT_MODE_NO_BLOCK_SKIP);
		SEGGER_RTT_ConfigDownBuffer(cfg->channel, dev->name,
					    cfg->down_buffer, cfg->down_size,
					    SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	}
	return 0;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */

static int uart_rtt_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_rtt_config *config = dev->config;
	unsigned int ch = config ? config->channel : 0;
	unsigned int ret = SEGGER_RTT_Read(ch, c, 1);

	return ret ? 0 : -1;
}

/**
 * @brief Output a character in polled mode.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_rtt_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_rtt_config *config = dev->config;
	unsigned int ch = config ? config->channel : 0;

	SEGGER_RTT_Write(ch, &c, 1);
}

#ifdef CONFIG_UART_ASYNC_API

static int uart_rtt_callback_set(const struct device *dev,
				 uart_callback_t callback, void *user_data)
{
	struct uart_rtt_data *data = dev->data;

	data->callback = callback;
	data->user_data = user_data;
	return 0;
}

static int uart_rtt_tx(const struct device *dev,
		       const uint8_t *buf, size_t len, int32_t timeout)
{
	const struct uart_rtt_config *cfg = dev->config;
	struct uart_rtt_data *data = dev->data;
	unsigned int ch = cfg ? cfg->channel : 0;

	ARG_UNUSED(timeout);

	/* RTT mutex cannot be claimed in ISRs */
	if (k_is_in_isr()) {
		return -ENOTSUP;
	}

	/* Claim the RTT lock */
	if (k_mutex_lock(&rtt_term_mutex, K_NO_WAIT) != 0) {
		return -EBUSY;
	}

	/* Output the buffer */
	SEGGER_RTT_WriteNoLock(ch, buf, len);

	/* Return RTT lock */
	SEGGER_RTT_UNLOCK();

	/* Send the TX complete callback */
	if (data->callback) {
		struct uart_event evt = {
			.type = UART_TX_DONE,
			.data.tx.buf = buf,
			.data.tx.len = len
		};
		data->callback(dev, &evt, data->user_data);
	}

	return 0;
}

static int uart_rtt_tx_abort(const struct device *dev)
{
	/* RTT TX is a memcpy, there is never a transmission to abort */
	ARG_UNUSED(dev);

	return -EFAULT;
}

static int uart_rtt_rx_enable(const struct device *dev,
			      uint8_t *buf, size_t len, int32_t timeout)
{
	/* SEGGER RTT reception is implemented as a direct memory write to RAM
	 * by a connected debugger. As such there is no hardware interrupt
	 * or other mechanism to know when the debugger has added data to be
	 * read. Asynchronous RX does not make sense in such a context, and is
	 * therefore not supported.
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(timeout);

	return -ENOTSUP;
}

static int uart_rtt_rx_disable(const struct device *dev)
{
	/* Asynchronous RX not supported, see uart_rtt_rx_enable */
	ARG_UNUSED(dev);

	return -EFAULT;
}

static int uart_rtt_rx_buf_rsp(const struct device *dev,
			       uint8_t *buf, size_t len)
{
	/* Asynchronous RX not supported, see uart_rtt_rx_enable */
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	return -ENOTSUP;
}

#endif /* CONFIG_UART_ASYNC_API */

static const struct uart_driver_api uart_rtt_driver_api = {
	.poll_in = uart_rtt_poll_in,
	.poll_out = uart_rtt_poll_out,
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_rtt_callback_set,
	.tx = uart_rtt_tx,
	.tx_abort = uart_rtt_tx_abort,
	.rx_enable = uart_rtt_rx_enable,
	.rx_buf_rsp = uart_rtt_rx_buf_rsp,
	.rx_disable = uart_rtt_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
};

#define UART_RTT(idx)                   DT_NODELABEL(rtt##idx)
#define UART_RTT_PROP(idx, prop)        DT_PROP(UART_RTT(idx), prop)
#define UART_RTT_CONFIG_NAME(idx)       uart_rtt##idx##_config

#define UART_RTT_CONFIG(idx)						    \
	static								    \
	uint8_t uart_rtt##idx##_tx_buf[UART_RTT_PROP(idx, tx_buffer_size)]; \
	static								    \
	uint8_t uart_rtt##idx##_rx_buf[UART_RTT_PROP(idx, rx_buffer_size)]; \
									    \
	static const struct uart_rtt_config UART_RTT_CONFIG_NAME(idx) = {   \
		.up_buffer = uart_rtt##idx##_tx_buf,			    \
		.up_size = sizeof(uart_rtt##idx##_tx_buf),		    \
		.down_buffer = uart_rtt##idx##_rx_buf,			    \
		.down_size = sizeof(uart_rtt##idx##_rx_buf),		    \
	}

#define UART_RTT_INIT(idx, config)					      \
	struct uart_rtt_data uart_rtt##idx##_data;			      \
									      \
	DEVICE_DT_DEFINE(UART_RTT(idx), uart_rtt_init, NULL, \
			    &uart_rtt##idx##_data, config,		      \
			    PRE_KERNEL_2, CONFIG_SERIAL_INIT_PRIORITY,	      \
			    &uart_rtt_driver_api)

#ifdef CONFIG_UART_RTT_0
UART_RTT_INIT(0, NULL);
#endif

#ifdef CONFIG_UART_RTT_1
UART_RTT_CONFIG(1);
UART_RTT_INIT(1, &UART_RTT_CONFIG_NAME(1));
#endif

#ifdef CONFIG_UART_RTT_2
UART_RTT_CONFIG(2);
UART_RTT_INIT(2, &UART_RTT_CONFIG_NAME(2));
#endif

#ifdef CONFIG_UART_RTT_3
UART_RTT_CONFIG(3);
UART_RTT_INIT(3, &UART_RTT_CONFIG_NAME(3));
#endif
