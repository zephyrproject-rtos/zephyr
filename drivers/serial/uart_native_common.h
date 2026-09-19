/*
 * Copyright (c) 2018, Oticon A/S
 * Copyright (c) 2025, Nordic Semiconductor ASA
 * Copyright (c) 2026, Canonical
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_NATIVE_COMMON_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_NATIVE_COMMON_H_

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

struct native_common_data {
	int in_fd;
	int out_fd;

	bool stdin_disconnected;
	bool wait_pts;

	void (*uart_poll_out_pre)(struct native_common_data *data,
				  const unsigned char *buf, size_t len);
	int (*uart_read_n)(struct native_common_data *data, unsigned char *p_char, int len);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct {
		bool tx_enabled;
		bool rx_enabled;
		uart_irq_callback_user_data_t callback;
		void *cb_data;

		char char_store;
		bool char_ready;

		atomic_t thread_started;

		/* Instance-specific IRQ emulation thread. */
		struct k_thread poll_thread;
		/* Stack for IRQ emulation thread */
		K_KERNEL_STACK_MEMBER(poll_stack, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
	} irq;
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
void nc_uart_irq_tx_enable(const struct device *dev);
void nc_uart_irq_tx_disable(const struct device *dev);
void nc_uart_irq_rx_enable(const struct device *dev);
void nc_uart_irq_rx_disable(const struct device *dev);
int nc_uart_irq_tx_complete(const struct device *dev);
int nc_uart_irq_rx_ready(const struct device *dev);
int nc_uart_irq_tx_ready(const struct device *dev);
int nc_uart_irq_rx_ready(const struct device *dev);
int nc_uart_irq_is_pending(const struct device *dev);
void nc_uart_irq_callback_set(const struct device *dev,
			      uart_irq_callback_user_data_t cb,
			      void *cb_data);
void nc_uart_irq_handler(const struct device *dev);
int nc_uart_fifo_read(const struct device *dev, uint8_t *rx_data, int size);
int nc_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size);
void nc_uart_irq_read_1_ahead(struct native_common_data *data);
void nc_uart_irq_thread_start(const struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

int nc_uart_poll_out_n(struct native_common_data *data, const unsigned char *buf, size_t len);
void nc_uart_poll_out(const struct device *dev, unsigned char out_char);

int nc_uart_read_n(struct native_common_data *data, unsigned char *p_char, int len);
int nc_uart_poll_in(const struct device *dev, unsigned char *p_char);

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_NATIVE_COMMON_H_ */
