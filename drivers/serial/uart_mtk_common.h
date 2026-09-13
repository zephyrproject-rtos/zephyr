/*
 * Copyright (c) 2026 MediaTek Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_SERIAL_UART_MTK_COMMON_H
#define DRIVERS_SERIAL_UART_MTK_COMMON_H

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/spinlock.h>

typedef struct {
	DEVICE_MMIO_ROM; /* Must be first */

	uint32_t clock_freq;

#ifdef CONFIG_CLOCK_CONTROL
	const struct device *clock_dev;

	clock_control_subsys_t clock_subsys;
#endif

#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pinctrl_config;
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
} uart_mtk_config_t;

typedef struct {
	DEVICE_MMIO_RAM; /* Must be first */

	struct uart_config uart_cfg;

	struct k_spinlock lock;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;

	void *cb_data;
#endif
} uart_mtk_data_t;

void uart_mtk_poll_out(const struct device *dev, unsigned char c);
int uart_mtk_poll_in(const struct device *dev, unsigned char *c);
int uart_mtk_err_check(const struct device *dev);

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
int uart_mtk_configure(const struct device *dev, const struct uart_config *cfg);
int uart_mtk_config_get(const struct device *dev, struct uart_config *cfg);
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
int uart_mtk_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size);
int uart_mtk_fifo_read(const struct device *dev, uint8_t *rx_data, const int size);
void uart_mtk_irq_tx_enable(const struct device *dev);
void uart_mtk_irq_tx_disable(const struct device *dev);
int uart_mtk_irq_tx_ready(const struct device *dev);
void uart_mtk_irq_rx_enable(const struct device *dev);
void uart_mtk_irq_rx_disable(const struct device *dev);
int uart_mtk_irq_rx_ready(const struct device *dev);
int uart_mtk_irq_is_pending(const struct device *dev);
void uart_mtk_irq_update(const struct device *dev);
void uart_mtk_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
			       void *cb_data);
void uart_mtk_isr(const struct device *dev);
#endif

int uart_mtk_init(const struct device *dev);

#endif /* DRIVERS_SERIAL_UART_MTK_COMMON_H */
