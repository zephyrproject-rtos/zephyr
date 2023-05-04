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

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

struct modem_iface_uart_data {
	/* HW flow control */
	bool hw_flow_control;

	/* ring buffer */
	struct ring_buf rx_rb;

	/* rx semaphore */
	struct k_sem rx_sem;

#ifdef CONFIG_MODEM_IFACE_UART_ASYNC

	/* tx semaphore */
	struct k_sem tx_sem;

#endif /* CONFIG_MODEM_IFACE_UART_ASYNC */
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
			      const struct device *dev);

/**
 * @brief Modem uart interface configuration
 *
 * @param rx_rb_buf Buffer used for internal ring buffer
 * @param rx_rb_buf_len Size of buffer used for internal ring buffer
 * @param dev UART device used for interface
 * @param hw_flow_control Set if hardware flow control is used
 */
struct modem_iface_uart_config {
	char *rx_rb_buf;
	size_t rx_rb_buf_len;
	const struct device *dev;
	bool hw_flow_control;
};

/**
 * @brief Initialize modem interface for UART
 *
 * @param iface Interface structure to initialize
 * @param data UART data structure used by the modem interface
 * @param config UART configuration structure used to configure UART data structure
 *
 * @return -EINVAL if any argument is invalid
 * @return 0 if successful
 */
int modem_iface_uart_init(struct modem_iface *iface, struct modem_iface_uart_data *data,
			  const struct modem_iface_uart_config *config);

/**
 * @brief Wait for rx data ready from uart interface
 *
 * @param iface Interface to wait on
 *
 * @return 0 if data is ready
 * @return -EBUSY if returned without waiting
 * @return -EAGAIN if timeout occurred
 */
static inline int modem_iface_uart_rx_wait(struct modem_iface *iface, k_timeout_t timeout)
{
	struct modem_iface_uart_data *data = (struct modem_iface_uart_data *)iface->iface_data;

	return k_sem_take(&data->rx_sem, timeout);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_IFACE_UART_H_ */
