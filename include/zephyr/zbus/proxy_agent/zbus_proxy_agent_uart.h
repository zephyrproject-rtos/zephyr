/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_UART_H_
#define ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_UART_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zbus Multi-domain API
 * @defgroup zbus_proxy_agent_uart_backend Zbus Proxy Agent UART Backend
 * @ingroup zbus_proxy_agent
 * @{
 */

/**
 * @brief UART Message Framing Protocol
 *
 * The UART backend uses a structured framing protocol for message transmission
 * over the UART interface with variable-length messages.
 *
 * @section protocol_format Frame Format
 *
 * ```
 * |       0       |       1       |       2       |       3       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  SYNC (0xAA)  |  SYNC (0x55)  |          LENGTH (LSB)         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |         LENGTH (MSB)          |                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
 * |                                                               |
 * +                         PAYLOAD (variable)                    +
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                          CRC32 (LSB)                          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                          CRC32 (MSB)                          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                       PADDING (zeros)                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * ```
 *
 * @section frame_processing Frame Processing
 *
 * **Transmission**:
 * 1. Serialize message to payload
 * 2. Construct header (sync + length)
 * 3. Calculate CRC32 over header + payload
 * 4. Add zero padding to chunk boundary
 * 5. Transmit complete frame
 *
 * **Reception**:
 * 1. Search for sync pattern (0xAA 0x55)
 * 2. Read length field (4 bytes LE)
 * 3. Read payload (length bytes)
 * 4. Read CRC32 (4 bytes LE)
 * 5. Verify CRC32 matches calculated value
 * 6. Deliver payload to upper layer
 *
 */
#define UART_SYNC_PATTERN
#define UART_SYNC_PATTERN_BYTE1 0xAA
#define UART_SYNC_PATTERN_BYTE2 0x55
#define UART_SYNC_PATTERN_SIZE  2
#define UART_LENGTH_FIELD_SIZE  4
#define UART_FRAME_HEADER_SIZE  (UART_SYNC_PATTERN_SIZE + UART_LENGTH_FIELD_SIZE)
#define UART_FRAME_CRC_SIZE     4
#define UART_FRAME_OVERHEAD     (UART_FRAME_HEADER_SIZE + UART_FRAME_CRC_SIZE)

/**
 * @brief Calculate UART idle timeout based on character transmission time
 *
 * Calculates the time required to transmit a specified number of UART characters,
 * useful for setting idle timeouts in async RX operations. Assumes standard UART
 * framing of 10 bits per character (1 start + 8 data + 1 stop).
 *
 * @param _baud UART baud rate in bits per second
 * @param _chars Number of character times for the timeout
 * @return Timeout in microseconds
 */
#define ZBUS_PROXY_UART_CHAR_TIMEOUT_US(_baud, _chars) \
	(((_chars) * 10 * 1000000) / (_baud))

/**
 * @brief UART RX state enumeration for message assembly
 */
enum uart_rx_state {
	UART_RX_SYNC_SEARCH,
	UART_RX_LENGTH_READ,
	UART_RX_PAYLOAD_READ,
	UART_RX_CRC_READ,
};

/**
 * @brief RX configuration and state for UART backend
 */
struct zbus_proxy_agent_uart_rx_config {
	/** Asynchronous RX buffer */
	void *async_rx_buf;

	/** Index of the current async RX buffer fragment */
	volatile uint8_t buf_idx;

	/** Timeout for processing async RX buffer fragments (in microseconds) */
	uint32_t rx_buf_timeout_us;

	/** RX state machine for message assembly */
	struct {
		enum uart_rx_state state;
		uint8_t sync_bytes_found;
		uint32_t expected_length;
		uint32_t bytes_received;
		uint8_t *assembly_buffer;
		size_t assembly_buffer_size;
		size_t assembly_buffer_pos;
	} fsm;
};

/**
 * @brief TX configuration for UART backend
 */
struct zbus_proxy_agent_uart_tx_config {
	/** Semaphore to signal when TX is done */
	struct k_sem busy_sem;

	/** TX frame buffer for construction and transmission */
	uint8_t *frame_buffer;

	/** Size of the TX frame buffer */
	size_t frame_buffer_size;
};

/**
 * @brief Callback configuration for UART backend
 */
struct zbus_proxy_agent_uart_callback_config {
	/** Callback function for received data */
	int (*recv_cb)(const uint8_t *data, size_t length, void *user_data);

	/** User data for the receive callback */
	void *recv_cb_user_data;
};

/**
 * @brief configuration for UART backend.
 */
struct zbus_proxy_agent_uart_config {
	/** UART device reference */
	const struct device *dev;

	/** RX configuration and state */
	struct zbus_proxy_agent_uart_rx_config rx;

	/** TX configuration */
	struct zbus_proxy_agent_uart_tx_config tx;

	/** Callback configuration */
	struct zbus_proxy_agent_uart_callback_config callbacks;
};

/**
 * @brief Instantiate a UART proxy agent from device tree node.
 *
 * @param node_id Device tree node identifier passed by DT_FOREACH_STATUS_OKAY
 */
#define ZBUS_PROXY_AGENT_INSTANTIATE_UART(node_id)                                                 \
	ZBUS_PROXY_AGENT_DEFINE(node_id, zbus_##node_id, ZBUS_PROXY_AGENT_TYPE_UART)

/** @cond INTERNAL_HIDDEN */

/* UART backend API structure */
extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_uart_backend_api;

/**
 * @brief Backend dispatch macros for the UART backend.
 *
 * These macros follow the standard backend implementation pattern defined in
 * zbus_proxy_agent.h. They provide API and configuration access for the UART
 * communication backend.
 */
#define _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART() &zbus_proxy_agent_uart_backend_api
#define _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(_name) (void *)&_name##_uart_config

/**
 * @brief Macro to generate device specific backend configurations for the UART type.
 *
 * @param _name The name of the proxy agent (used as identifier prefix).
 * @param _node_id The device tree node ID for the proxy agent.
 */
#define _ZBUS_GENERATE_BACKEND_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(_name, _node_id)                  \
	static uint8_t _name##_uart_rx_bufs[CONFIG_ZBUS_PROXY_AGENT_RX_BUF_COUNT]                  \
					   [CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE];                \
	static uint8_t _name##_uart_tx_buf[CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE +                  \
					   CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE + 128];       \
	static uint8_t _name##_uart_rx_assembly_buf[CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE +         \
						    CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE +    \
						    64];                                           \
	static struct zbus_proxy_agent_uart_config _name##_uart_config = {                         \
		.dev = DEVICE_DT_GET(_node_id),                                                    \
		.rx =                                                                              \
			{                                                                          \
				.async_rx_buf = _name##_uart_rx_bufs,                              \
				.buf_idx = 0,                                                      \
				.rx_buf_timeout_us =                                               \
					ZBUS_PROXY_UART_CHAR_TIMEOUT_US(                           \
						DT_PROP(_node_id, current_speed), 4),              \
				.fsm =                                                             \
					{                                                          \
						.state = UART_RX_SYNC_SEARCH,                      \
						.sync_bytes_found = 0,                             \
						.expected_length = 0,                              \
						.bytes_received = 0,                               \
						.assembly_buffer = _name##_uart_rx_assembly_buf,   \
						.assembly_buffer_size =                            \
							sizeof(_name##_uart_rx_assembly_buf),      \
						.assembly_buffer_pos = 0,                          \
					},                                                         \
			},                                                                         \
		.tx =                                                                              \
			{                                                                          \
				.frame_buffer = _name##_uart_tx_buf,                               \
				.frame_buffer_size = sizeof(_name##_uart_tx_buf),                  \
			},                                                                         \
		.callbacks =                                                                       \
			{                                                                          \
				.recv_cb = NULL,                                                   \
				.recv_cb_user_data = NULL,                                         \
			},                                                                         \
	}

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_UART_H_ */
