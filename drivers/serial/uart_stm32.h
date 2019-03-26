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

#ifdef CONFIG_UART_ASYNC_API
	u8_t dma_slot;
	u8_t channel_tx;
	u8_t channel_rx;
#endif
};

/* driver data */
struct uart_stm32_data {
	/* Baud rate */
	u32_t baud_rate;
	/* clock device */
	struct device *clock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif

#ifdef CONFIG_UART_ASYNC_API
	struct device *dma_dev;
	uart_callback_t async_cb;
	void *async_user_data;

	struct dma_block_config rx_blk_cfg;
	u8_t *rx_buffer;
	size_t rx_buffer_length;
	size_t rx_offset;
	volatile size_t rx_counter;
	u32_t rx_timeout;
	struct k_delayed_work rx_timeout_work;
	bool rx_enabled;

	struct dma_block_config tx_blk_cfg;
	const u8_t *tx_buffer;
	size_t tx_buffer_length;
	volatile size_t tx_counter;
	u32_t tx_timeout;
	struct k_delayed_work tx_timeout_work;
#endif
};

#endif	/* ZEPHYR_DRIVERS_SERIAL_UART_STM32_H_ */
