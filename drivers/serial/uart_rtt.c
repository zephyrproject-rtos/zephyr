/*
 * Copyright (c) 2019 omSquare s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/uart.h>
#include <SEGGER_RTT.h>

#define DT_DRV_COMPAT segger_rtt_uart

struct uart_rtt_config {
	void *up_buffer;
	size_t up_size;
	void *down_buffer;
	size_t down_size;
	uint8_t channel;
};

static inline const struct uart_rtt_config *get_dev_config(const struct device *dev)
{
	return dev->config;
}

static int uart_rtt_init(const struct device *dev)
{
	/*
	 * Channel 0 is initialized at compile-time, Kconfig ensures that
	 * it is configured in correct, non-blocking mode. Other channels
	 * need to be configured at run-time.
	 */
	if (get_dev_config(dev)) {
		const struct uart_rtt_config *cfg = get_dev_config(dev);

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
	unsigned int ch =
		get_dev_config(dev) ? get_dev_config(dev)->channel : 0;
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
	unsigned int ch =
		get_dev_config(dev) ? get_dev_config(dev)->channel : 0;

	SEGGER_RTT_Write(ch, &c, 1);
}

static const struct uart_driver_api uart_rtt_driver_api = {
	.poll_in = uart_rtt_poll_in,
	.poll_out = uart_rtt_poll_out,
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
	DEVICE_AND_API_INIT(uart_rtt##idx, DT_LABEL(UART_RTT(idx)),	      \
			    uart_rtt_init, NULL, config,		      \
			    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
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
