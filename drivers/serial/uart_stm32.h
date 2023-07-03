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

#include <zephyr/drivers/pinctrl.h>

#include <stm32_ll_usart.h>

/* device config */
struct uart_stm32_config {
	/* USART instance */
	USART_TypeDef *usart;
	/* clock subsystem driving this peripheral */
	const struct stm32_pclken *pclken;
	/* number of clock subsystems */
	size_t pclk_len;
	/* initial hardware flow control, 1 for RTS/CTS */
	bool hw_flow_control;
	/* initial parity, 0 for none, 1 for odd, 2 for even */
	int  parity;
	/* switch to enable single wire / half duplex feature */
	bool single_wire;
	/* enable tx/rx pin swap */
	bool tx_rx_swap;
	/* enable rx pin inversion */
	bool rx_invert;
	/* enable tx pin inversion */
	bool tx_invert;
	/* enable de signal */
	bool de_enable;
	/* de signal assertion time in 1/16 of a bit */
	uint8_t de_assert_time;
	/* de signal deassertion time in 1/16 of a bit */
	uint8_t de_deassert_time;
	/* enable de pin inversion */
	bool de_invert;
	const struct pinctrl_dev_config *pcfg;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API) || \
	defined(CONFIG_PM)
	uart_irq_config_func_t irq_config_func;
#endif
#if defined(CONFIG_PM)
	/* Device defined as wake-up source */
	bool wakeup_source;
	uint32_t wakeup_line;
#endif /* CONFIG_PM */
};

#ifdef CONFIG_UART_ASYNC_API
struct uart_dma_stream {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	uint8_t priority;
	bool src_addr_increment;
	bool dst_addr_increment;
	int fifo_threshold;
	struct dma_block_config blk_cfg;
	uint8_t *buffer;
	size_t buffer_length;
	size_t offset;
	volatile size_t counter;
	int32_t timeout;
	struct k_work_delayable timeout_work;
	bool enabled;
};
#endif

/* driver data */
struct uart_stm32_data {
	/* Baud rate */
	uint32_t baud_rate;
	/* clock device */
	const struct device *clock;
	/* Reset controller device configuration */
	const struct reset_dt_spec reset;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif

#ifdef CONFIG_UART_ASYNC_API
	const struct device *uart_dev;
	uart_callback_t async_cb;
	void *async_user_data;
	struct uart_dma_stream dma_rx;
	struct uart_dma_stream dma_tx;
	uint8_t *rx_next_buffer;
	size_t rx_next_buffer_len;
#endif
#ifdef CONFIG_PM
	bool tx_poll_stream_on;
	bool tx_int_stream_on;
	bool pm_policy_state_on;
#endif
};

#endif	/* ZEPHYR_DRIVERS_SERIAL_UART_STM32_H_ */
