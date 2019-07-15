/*
 * Copyright (c) 2019 omSquare s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/uart.h>
#include <SEGGER_RTT.h>

struct uart_rtt_config {
	void *up_buffer;
	size_t up_size;
	void *down_buffer;
	size_t down_size;
	u8_t channel;
};

static inline const struct uart_rtt_config *get_dev_config(struct device *dev)
{
	return dev->config->config_info;
}

static int uart_rtt_init(struct device *dev)
{
	/*
	 * Channel 0 is initialized at compile-time, Kconfig ensures that
	 * it is configured in correct, non-blocking mode. Other channels
	 * need to be configured at run-time.
	 */
	if (get_dev_config(dev)) {
		const struct uart_rtt_config *cfg = get_dev_config(dev);

		SEGGER_RTT_ConfigUpBuffer(cfg->channel, dev->config->name,
					  cfg->up_buffer, cfg->up_size,
					  SEGGER_RTT_MODE_NO_BLOCK_SKIP);
		SEGGER_RTT_ConfigDownBuffer(cfg->channel, dev->config->name,
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

static int uart_rtt_poll_in(struct device *dev, unsigned char *c)
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
static void uart_rtt_poll_out(struct device *dev, unsigned char c)
{
	unsigned int ch =
		get_dev_config(dev) ? get_dev_config(dev)->channel : 0;

	SEGGER_RTT_Write(ch, &c, 1);
}

static const struct uart_driver_api uart_rtt_driver_api = {
	.poll_in = uart_rtt_poll_in,
	.poll_out = uart_rtt_poll_out,
};

#if CONFIG_UART_RTT_0

DEVICE_DEFINE(uart_rtt0, "RTT_0", uart_rtt_init, NULL, NULL, NULL,
	      /* Initialize UART device after RTT init. */
	      PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      &uart_rtt_driver_api);

#endif

#define UART_RTT_CHANNEL(n)                                                    \
	static u8_t                                                            \
		uart_rtt##n##_tx_buffer[CONFIG_UART_RTT_##n##_TX_BUFFER_SIZE]; \
	static u8_t                                                            \
		uart_rtt##n##_rx_buffer[CONFIG_UART_RTT_##n##_RX_BUFFER_SIZE]; \
									       \
	static const char uart_rtt##n##_name[] = "RTT_" #n "\0";               \
									       \
	static const struct uart_rtt_config uart_rtt##n##_config = {           \
		.channel = n,                                                  \
		.up_buffer = uart_rtt##n##_tx_buffer,                          \
		.up_size = sizeof(uart_rtt##n##_tx_buffer),                    \
		.down_buffer = uart_rtt##n##_rx_buffer,                        \
		.down_size = sizeof(uart_rtt##n##_rx_buffer),                  \
	};                                                                     \
									       \
	DEVICE_DEFINE(uart_rtt##n, uart_rtt##n##_name, uart_rtt_init, NULL,    \
		      NULL, &uart_rtt##n##_config, PRE_KERNEL_2,               \
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                      \
		      &uart_rtt_driver_api)

#if CONFIG_UART_RTT_1
UART_RTT_CHANNEL(1);
#endif

#if CONFIG_UART_RTT_2
UART_RTT_CHANNEL(2);
#endif

#if CONFIG_UART_RTT_3
UART_RTT_CHANNEL(3);
#endif
