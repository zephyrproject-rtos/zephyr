/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UART_ERRORS_COMMON_H_
#define UART_ERRORS_COMMON_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

extern const struct device *const uart_dev;
extern const struct device *const uart_dev_aux;

extern uint8_t rx_buffer[256];
extern uint32_t rx_buffer_cnt;
extern volatile uint32_t rx_stopped_cnt;
extern volatile uint32_t rx_parity_err_cnt;
extern volatile uint32_t rx_framing_err_cnt;
extern volatile bool rx_active;

void reset_rx_state(void);
void receiver_shutdown(void);
void start_receiver(bool hwfc, enum uart_config_parity parity, bool two_stop_bits);

void uart_pair_configure(bool hwfc, enum uart_config_parity parity, bool two_stop_bits);

void reconfigure(const struct device *dev, enum uart_config_parity parity, bool set_parity,
		 bool two_stop_bits, bool set_stop_bits, bool *hwfc);
void aux_tx(const struct device *dev, const uint8_t *buf, size_t len);

void *test_setup(void);
void test_before(void *fixture);
void test_after(void *fixture);

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
struct parity_error_tx_data {
	const uint8_t *buf;
	size_t len;
	size_t curr;
	int err_byte;
	struct k_sem *sem;
	bool cfg_ok;
};

void parity_error_tx_int_callback(const struct device *dev, void *user_data);
#endif

#if IS_ENABLED(CONFIG_UART_ASYNC_API)
void aux_async_callback(const struct device *dev, struct uart_event *evt, void *user_data);
void async_rx_reclaim_chunks(void);
void async_rx_start(bool hwfc);
#endif

#endif /* UART_ERRORS_COMMON_H_ */
