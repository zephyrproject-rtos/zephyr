/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_UART_H_
#define ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_UART_H_

#include <zephyr/zbus/zbus.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/zbus/multidomain/zbus_multidomain_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zbus Multi-domain API
 * @defgroup zbus_multidomain_apis Zbus Multi-domain APIs
 * @ingroup zbus_apis
 * @since 3.3.0
 * @version 1.0.0
 * @ingroup os_services
 * @{
 */

struct zbus_multidomain_uart_config {
	/** UART device */
	const struct device *dev;

	/** Asynchronous RX buffer */
	uint8_t async_rx_buf[CONFIG_ZBUS_MULTIDOMAIN_UART_BUF_COUNT]
			    [sizeof(struct zbus_proxy_agent_msg)];

	/** Index of the current async RX buffer */
	volatile uint8_t async_rx_buf_idx;

	/** Semaphore to signal when TX is done */
	struct k_sem tx_busy_sem;

	struct zbus_proxy_agent_msg tx_msg_copy;

	/** Callback function for received messages */
	int (*recv_cb)(const struct zbus_proxy_agent_msg *msg);

	/** Callback function for ACKs */
	int (*ack_cb)(uint32_t msg_id, void *user_data);

	/** User data for the ACK callback */
	void *ack_cb_user_data;

	/** Work item for sending ACKs */
	struct k_work ack_work;

	/** Message ID to ACK */
	uint32_t ack_msg_id;
};

/** @cond INTERNAL_HIDDEN */

/* UART backend API structure */
extern const struct zbus_proxy_agent_api zbus_multidomain_uart_api;

/**
 * @brief Macros to get the API and configuration for the UART backend.
 *
 * These macros are used to retrieve the API and configuration for the UART backend
 * of the proxy agent. The macros are used in "zbus_multidomain.h" to define the
 * backend specific configurations and API for the UART type of proxy agent.
 *
 * @param _name The name of the proxy agent.
 */
#define _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_UART()         &zbus_multidomain_uart_api
#define _ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_UART(_name) (void *)&_name##_uart_config

/**
 * @brief Macros to generate device specific backend configurations for the UART type.
 *
 * This macro generates the backend specific configurations for the UART type of
 * proxy agent. The macro is used in "zbus_multidomain.h" to create the
 * backend specific configurations for the UART type of proxy agent.
 */
#define _ZBUS_GENERATE_BACKEND_CONFIG_ZBUS_MULTIDOMAIN_TYPE_UART(_name, _nodeid)                   \
	struct zbus_multidomain_uart_config _name##_uart_config = {                                \
		.dev = DEVICE_DT_GET(_nodeid),                                                     \
		.async_rx_buf = {{0}},                                                             \
		.async_rx_buf_idx = 0,                                                             \
	}

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_UART_H_ */
