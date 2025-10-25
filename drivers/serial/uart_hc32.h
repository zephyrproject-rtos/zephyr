/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for interrupt/event controller in HC32 MCUs
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_HC32_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_HC32_H_

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/uart.h>

#include <hc32_ll_usart.h>

#define HC32_UART_DEFAULT_BAUDRATE  115200
#define HC32_UART_DEFAULT_PARITY    UART_CFG_PARITY_NONE
#define HC32_UART_DEFAULT_STOP_BITS UART_CFG_STOP_BITS_1
#define HC32_UART_DEFAULT_DATA_BITS UART_CFG_DATA_BITS_8

#define HC32_UART_FUNC (USART_TX | USART_RX | USART_INT_RX | USART_INT_TX_CPLT | USART_INT_TX_EMPTY)

#if CONFIG_UART_INTERRUPT_DRIVEN
enum UART_TYPE {
	HC32_UART_IDX_EI,
	HC32_UART_IDX_RI,
	HC32_UART_IDX_TI,
	HC32_UART_IDX_TCI,
	HC32_UART_INT_NUM
};
#endif /* UART_INTERRUPT_DRIVEN */

/* device config */
struct uart_hc32_config {
	/* USART instance */
	CM_USART_TypeDef *usart;
	/* clock subsystem driving this peripheral */
	struct hc32_modules_clock_sys *clk_cfg;
	/* pin muxing */
	const struct pinctrl_dev_config *pin_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

struct hc32_usart_cb_data {
	/** Callback function */
	uart_irq_callback_user_data_t user_cb;
	/** User data. */
	void *user_data;
};

/* driver data */
struct uart_hc32_data {
	/* clock device */
	const struct device *clock;
	/* uart config */
	struct uart_config *uart_cfg;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	struct hc32_usart_cb_data cb[HC32_UART_INT_NUM];
#endif

#ifdef CONFIG_UART_ASYNC_API
	const struct device *uart_dev;
	uart_callback_t async_cb;
	void *async_user_data;
	uint8_t *rx_next_buffer;
	size_t rx_next_buffer_len;
#endif
};

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_HC32_H_ */
