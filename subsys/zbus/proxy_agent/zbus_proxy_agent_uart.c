/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_uart.h>

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_proxy_agent_uart, CONFIG_ZBUS_PROXY_AGENT_UART_LOG_LEVEL);

static inline uint8_t *get_rx_buffer(struct zbus_proxy_agent_uart_config *config, int idx)
{
	return (uint8_t *)config->rx.async_rx_buf + (idx * CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE);
}

static void reset_rx_state_machine(struct zbus_proxy_agent_uart_config *uart_config)
{
	uart_config->rx.fsm.state = UART_RX_SYNC_SEARCH;
	uart_config->rx.fsm.sync_bytes_found = 0;
	uart_config->rx.fsm.expected_length = 0;
	uart_config->rx.fsm.bytes_received = 0;
	uart_config->rx.fsm.assembly_buffer_pos = 0;
}

/* Helper function to store byte and increment counters */
static inline void store_rx_byte(struct zbus_proxy_agent_uart_config *uart_config, uint8_t byte)
{
	uart_config->rx.fsm.assembly_buffer[uart_config->rx.fsm.assembly_buffer_pos++] = byte;
	uart_config->rx.fsm.bytes_received++;
}

static void handle_sync_search(struct zbus_proxy_agent_uart_config *uart_config, uint8_t byte)
{
	if (uart_config->rx.fsm.sync_bytes_found == 0) {
		if (byte == UART_SYNC_PATTERN_BYTE1) {
			uart_config->rx.fsm.sync_bytes_found = 1;
			uart_config->rx.fsm.assembly_buffer[0] = byte;
			uart_config->rx.fsm.assembly_buffer_pos = 1;
		}
	} else {
		if (byte == UART_SYNC_PATTERN_BYTE2) {
			uart_config->rx.fsm.assembly_buffer[1] = byte;
			uart_config->rx.fsm.assembly_buffer_pos = 2;
			uart_config->rx.fsm.state = UART_RX_LENGTH_READ;
			uart_config->rx.fsm.bytes_received = 0;
		} else {
			/* Sync lost, restart */
			uart_config->rx.fsm.sync_bytes_found = 0;
			if (byte == UART_SYNC_PATTERN_BYTE1) {
				uart_config->rx.fsm.sync_bytes_found = 1;
				uart_config->rx.fsm.assembly_buffer[0] = byte;
				uart_config->rx.fsm.assembly_buffer_pos = 1;
			}
		}
	}
}

static int handle_length_read(struct zbus_proxy_agent_uart_config *uart_config, uint8_t byte)
{
	store_rx_byte(uart_config, byte);

	if (uart_config->rx.fsm.bytes_received == sizeof(uint32_t)) {
		uart_config->rx.fsm.expected_length =
			sys_get_le32(&uart_config->rx.fsm.assembly_buffer[2]);

		if (uart_config->rx.fsm.expected_length >
		    (uart_config->rx.fsm.assembly_buffer_size - UART_FRAME_OVERHEAD)) {
			LOG_ERR("Message too large: %d bytes", uart_config->rx.fsm.expected_length);
			reset_rx_state_machine(uart_config);
			return -EMSGSIZE;
		}

		uart_config->rx.fsm.state = UART_RX_PAYLOAD_READ;
		uart_config->rx.fsm.bytes_received = 0;
	}
	return 0;
}

static void handle_payload_read(struct zbus_proxy_agent_uart_config *uart_config, uint8_t byte)
{
	store_rx_byte(uart_config, byte);

	if (uart_config->rx.fsm.bytes_received == uart_config->rx.fsm.expected_length) {
		uart_config->rx.fsm.state = UART_RX_CRC_READ;
		uart_config->rx.fsm.bytes_received = 0;
	}
}

static int handle_crc_read(struct zbus_proxy_agent_uart_config *uart_config, uint8_t byte)
{
	store_rx_byte(uart_config, byte);

	if (uart_config->rx.fsm.bytes_received == sizeof(uint32_t)) {
		uint32_t received_crc = sys_get_le32(
			&uart_config->rx.fsm
				 .assembly_buffer[uart_config->rx.fsm.assembly_buffer_pos -
						  sizeof(uint32_t)]);
		uint32_t calculated_crc =
			crc32_ieee(uart_config->rx.fsm.assembly_buffer,
				   UART_FRAME_HEADER_SIZE + uart_config->rx.fsm.expected_length);

		if (received_crc != calculated_crc) {
			LOG_ERR("CRC mismatch: received 0x%08X, calculated 0x%08X", received_crc,
				calculated_crc);
			reset_rx_state_machine(uart_config);
			return -EBADMSG;
		}

		if (uart_config->callbacks.recv_cb) {
			int ret = uart_config->callbacks.recv_cb(
				&uart_config->rx.fsm.assembly_buffer[UART_FRAME_HEADER_SIZE],
				uart_config->rx.fsm.expected_length,
				uart_config->callbacks.recv_cb_user_data);
			if (ret < 0) {
				LOG_ERR("Protocol layer callback failed: %d", ret);
			}
		}

		reset_rx_state_machine(uart_config);
		return 0;
	}
	return 0;
}

static int process_rx_chunk(struct zbus_proxy_agent_uart_config *uart_config, const uint8_t *data,
			    size_t len)
{
	LOG_HEXDUMP_DBG(data, len, "Received UART data chunk");

	for (size_t i = 0; i < len; i++) {
		uint8_t byte = data[i];
		int ret = 0;

		switch (uart_config->rx.fsm.state) {
		case UART_RX_SYNC_SEARCH:
			handle_sync_search(uart_config, byte);
			break;

		case UART_RX_LENGTH_READ:
			ret = handle_length_read(uart_config, byte);
			if (ret < 0) {
				return ret;
			}
			break;

		case UART_RX_PAYLOAD_READ:
			handle_payload_read(uart_config, byte);
			break;

		case UART_RX_CRC_READ:
			ret = handle_crc_read(uart_config, byte);
			if (ret < 0) {
				return ret;
			}
			break;
		default:
			LOG_ERR("Invalid RX state: %d", uart_config->rx.fsm.state);
			reset_rx_state_machine(uart_config);
			return -EINVAL;
		}
	}
	return 0;
}

static void zbus_proxy_agent_uart_recv_callback(const struct device *dev, struct uart_event *evt,
						void *config)
{
	struct zbus_proxy_agent_uart_config *uart_config =
		(struct zbus_proxy_agent_uart_config *)config;
	int ret;

	CHECKIF(uart_config == NULL) {
		LOG_ERR("Invalid UART config in callback");
		return;
	}

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&uart_config->tx.busy_sem);
		break;

	case UART_TX_ABORTED:
		k_sem_give(&uart_config->tx.busy_sem);
		break;

	case UART_RX_RDY:
		CHECKIF(evt->data.rx.buf == NULL || evt->data.rx.len == 0) {
			LOG_ERR("Received invalid UART data");
			break;
		}

		ret = process_rx_chunk(uart_config, &evt->data.rx.buf[evt->data.rx.offset],
				       evt->data.rx.len);
		if (ret < 0) {
			LOG_ERR("Failed to process RX chunk: %d", ret);
		}
		break;

	case UART_RX_BUF_REQUEST:
		ret = uart_rx_buf_rsp(dev, get_rx_buffer(uart_config, uart_config->rx.buf_idx),
				      CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE);
		if (ret < 0) {
			LOG_ERR("Failed to provide RX buffer: %d", ret);
		} else {
			uart_config->rx.buf_idx = (uart_config->rx.buf_idx + 1) %
						  CONFIG_ZBUS_PROXY_AGENT_RX_BUF_COUNT;
		}
		break;

	case UART_RX_BUF_RELEASED:
		break;

	case UART_RX_DISABLED:
		LOG_WRN("UART RX disabled, re-enabling");

		ret = uart_rx_enable(dev, get_rx_buffer(uart_config, uart_config->rx.buf_idx),
				     CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE,
				     uart_config->rx.rx_buf_timeout_us);
		if (ret < 0) {
			LOG_ERR("Failed to re-enable UART RX: %d", ret);
		}
		break;

	default:
		LOG_DBG("Unhandled UART event: %d", evt->type);
		break;
	}
}

static int zbus_proxy_agent_uart_backend_init(void *config)
{
	struct zbus_proxy_agent_uart_config *uart_config =
		(struct zbus_proxy_agent_uart_config *)config;
	int ret;

	_ZBUS_ASSERT(uart_config != NULL, "Invalid UART backend configuration");
	_ZBUS_ASSERT(uart_config->dev != NULL, "UART device is NULL");

	CHECKIF(!device_is_ready(uart_config->dev)) {
		LOG_ERR("UART device %s is not ready", uart_config->dev->name);
		return -ENODEV;
	}

	ret = k_sem_init(&uart_config->tx.busy_sem, 1, 1);
	if (ret < 0) {
		LOG_ERR("Failed to initialize UART TX semaphore: %d", ret);
		return ret;
	}

	reset_rx_state_machine(uart_config);

	ret = uart_callback_set(uart_config->dev, zbus_proxy_agent_uart_recv_callback, uart_config);
	if (ret < 0) {
		LOG_ERR("Failed to set UART callback: %d", ret);
		return ret;
	}

	uart_config->rx.buf_idx = 1;

	ret = uart_rx_enable(uart_config->dev, get_rx_buffer(uart_config, 0),
			     CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE,
			     uart_config->rx.rx_buf_timeout_us);
	if (ret < 0 && ret != -EBUSY) {
		LOG_ERR("Failed to enable UART RX: %d", ret);
		return ret;
	}

	LOG_DBG("ZBUS Proxy agent UART initialized for device %s",
		uart_config->dev->name);
	return 0;
}

static size_t calculate_padded_frame_size(size_t payload_len, size_t chunk_size)
{
	size_t frame_size = UART_FRAME_OVERHEAD + payload_len;
	size_t padded_size = ((frame_size + chunk_size - 1) / chunk_size) * chunk_size;
	return padded_size;
}

static int zbus_proxy_agent_uart_backend_send(void *config, uint8_t *data, size_t length)
{
	struct zbus_proxy_agent_uart_config *uart_config =
		(struct zbus_proxy_agent_uart_config *)config;
	size_t padded_frame_size;
	uint32_t crc;
	int ret;

	_ZBUS_ASSERT(uart_config != NULL, "Invalid UART backend configuration");
	_ZBUS_ASSERT(data != NULL, "Data pointer is NULL");
	_ZBUS_ASSERT(length > 0, "Data length is zero");

	padded_frame_size =
		calculate_padded_frame_size(length, CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE);
	if (padded_frame_size > uart_config->tx.frame_buffer_size) {
		LOG_ERR("Frame too large: %zu bytes, max %zu bytes", padded_frame_size,
			uart_config->tx.frame_buffer_size);
		return -EMSGSIZE;
	}

	ret = k_sem_take(&uart_config->tx.busy_sem, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("Failed to take UART TX semaphore: %d", ret);
		return ret;
	}

	memset(uart_config->tx.frame_buffer, 0, padded_frame_size);
	size_t pos = 0;

	uart_config->tx.frame_buffer[pos++] = UART_SYNC_PATTERN_BYTE1;
	uart_config->tx.frame_buffer[pos++] = UART_SYNC_PATTERN_BYTE2;

	sys_put_le32(length, &uart_config->tx.frame_buffer[pos]);
	pos += sizeof(uint32_t);

	memcpy(&uart_config->tx.frame_buffer[pos], data, length);
	pos += length;

	crc = crc32_ieee(uart_config->tx.frame_buffer, UART_FRAME_HEADER_SIZE + length);
	sys_put_le32(crc, &uart_config->tx.frame_buffer[pos]);

	ret = uart_tx(uart_config->dev, uart_config->tx.frame_buffer, padded_frame_size,
		      SYS_FOREVER_US);
	if (ret < 0) {
		LOG_ERR("Failed to send message via UART: %d", ret);
		k_sem_give(&uart_config->tx.busy_sem);
		return ret;
	}

	LOG_DBG("Sent framed message: %zu payload + %u overhead + %zu padding = %zu total bytes",
		length, UART_FRAME_OVERHEAD, (padded_frame_size - (UART_FRAME_OVERHEAD + length)),
		padded_frame_size);

	/* Note: TX semaphore will be released in UART_TX_DONE callback */
	return 0;
}

static int zbus_proxy_agent_uart_backend_set_recv_cb(void *config,
						     int (*recv_cb)(const uint8_t *data,
								    size_t length, void *user_data),
						     void *user_data)
{
	struct zbus_proxy_agent_uart_config *uart_config =
		(struct zbus_proxy_agent_uart_config *)config;

	_ZBUS_ASSERT(config != NULL, "Invalid UART backend configuration");
	_ZBUS_ASSERT(recv_cb != NULL, "Receive callback is NULL");

	uart_config->callbacks.recv_cb = recv_cb;
	uart_config->callbacks.recv_cb_user_data = user_data;
	LOG_DBG("Set receive callback for UART device %s", uart_config->dev->name);
	return 0;
}

/* Define the UART backend API */
const struct zbus_proxy_agent_backend_api zbus_proxy_agent_uart_backend_api = {
	.backend_init = zbus_proxy_agent_uart_backend_init,
	.backend_send = zbus_proxy_agent_uart_backend_send,
	.backend_set_recv_cb = zbus_proxy_agent_uart_backend_set_recv_cb,
};
