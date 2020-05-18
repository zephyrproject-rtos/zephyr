/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for UART port on STM32 family processor.
 *
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_STM32_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_STM32_H_

/* device config */
struct uart_stm32_config {
	struct uart_device_config uconf;
	/* clock subsystem driving this peripheral */
	struct stm32_pclken pclken;
	/* initial hardware flow control, 1 for RTS/CTS */
	bool hw_flow_control;
	/* initial parity, 0 for none, 1 for odd, 2 for even */
	int  parity;
};

#ifdef CONFIG_UART_ASYNC_API
struct uart_dma_stream {
	const char *dma_name;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	uint8_t priority;
	uint8_t src_addr_increment;
	uint8_t dst_addr_increment;
	uint8_t fifo_threshold;

	struct dma_block_config blk_cfg;
	uint8_t *buffer;
	size_t buffer_length;
	size_t offset;
	volatile size_t counter;
	k_timeout_t timeout;
	struct k_delayed_work timeout_work;
	bool enabled;
};
#endif

/* driver data */
struct uart_stm32_data {
	/* Baud rate */
	uint32_t baud_rate;
	/* clock device */
	struct device *clock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif

#ifdef CONFIG_UART_ASYNC_API
	struct device *uart_dev;
	struct device *dev_dma_tx;
	struct device *dev_dma_rx;
	uart_callback_t async_cb;
	void *async_user_data;
	struct uart_dma_stream rx;
	struct uart_dma_stream tx;
	uint8_t *rx_next_buffer;
	size_t rx_next_buffer_len;
#endif
};

#endif	/* ZEPHYR_DRIVERS_SERIAL_UART_STM32_H_ */
