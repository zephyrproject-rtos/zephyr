/** @file
 * @brief Modem interface for UART header file.
 *
 * Modem interface UART handling for modem context driver.
 */

/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_IFACE_UART_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_IFACE_UART_H_

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

struct modem_iface_uart_data {
	/* ISR char buffer */
	char *isr_buf;
	size_t isr_buf_len;

	/* ring buffer char buffer */
	char *rx_rb_buf;
	size_t rx_rb_buf_len;

	/* ring buffer */
	struct ring_buf rx_rb;

	/* rx semaphore */
	struct k_sem rx_sem;
};

/**
 * @brief  Init modem interface device for UART
 *
 * @details This can be called after the init if the UART is changed.
 *
 * @param  *iface: modem interface to initialize.
 * @param  *dev_name: name of the UART device to use
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_iface_uart_init_dev(struct modem_iface *iface,
			      const char *dev_name);

/**
 * @brief  Init modem interface for UART
 *
 * @param  *iface: modem interface to initialize.
 * @param  *data: modem interface data to use
 * @param  *dev_name: name of the UART device to use
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_iface_uart_init(struct modem_iface *iface,
			  struct modem_iface_uart_data *data,
			  const char *dev_name);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_IFACE_UART_H_ */
