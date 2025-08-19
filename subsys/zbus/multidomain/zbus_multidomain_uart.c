/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/multidomain/zbus_multidomain_uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_multidomain_uart, CONFIG_ZBUS_MULTIDOMAIN_LOG_LEVEL);

int zbus_multidomain_uart_backend_ack(void *config, uint32_t msg_id)
{
	int ret;

	struct zbus_multidomain_uart_config *uart_config =
		(struct zbus_multidomain_uart_config *)config;

	if (!uart_config) {
		LOG_ERR("Invalid parameters to send ACK");
		return -EINVAL;
	}

	struct zbus_proxy_agent_msg ack_msg;

	ret = zbus_create_proxy_agent_ack_msg(&ack_msg, msg_id);
	if (ret < 0) {
		LOG_ERR("Failed to create ACK message: %d", ret);
		return ret;
	}

	ret = k_sem_take(&uart_config->tx_busy_sem, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("Failed to take TX busy semaphore: %d", ret);
		return ret;
	}

	/* Create a copy of the ACK message to avoid issues with the caller
	 * modifying the message before it is sent.
	 */
	memcpy(&uart_config->tx_msg_copy, &ack_msg, sizeof(ack_msg));

	ret = uart_tx(uart_config->dev, (uint8_t *)&uart_config->tx_msg_copy,
		      sizeof(uart_config->tx_msg_copy), SYS_FOREVER_US);
	if (ret < 0) {
		LOG_ERR("Failed to send ACK message: %d", ret);
		k_sem_give(&uart_config->tx_busy_sem);
		return ret;
	}
	LOG_DBG("Sent ACK for message %d via UART device %s", msg_id, uart_config->dev->name);

	return 0;
}

static void uart_ack_work_handler(struct k_work *work)
{
	struct zbus_multidomain_uart_config *uart_config =
		CONTAINER_OF(work, struct zbus_multidomain_uart_config, ack_work);

	if (!uart_config) {
		LOG_ERR("Invalid UART config in ACK work handler");
		return;
	}
	int ret = zbus_multidomain_uart_backend_ack(uart_config, uart_config->ack_msg_id);

	if (ret < 0) {
		LOG_ERR("Failed to send ACK for message %d: %d", uart_config->ack_msg_id, ret);
	}
}

static void handle_uart_recv_ack(struct zbus_multidomain_uart_config *uart_config,
				 const struct zbus_proxy_agent_msg *msg)
{
	int ret;

	if (!uart_config->ack_cb) {
		LOG_ERR("ACK callback not set, dropping ACK");
		return;
	}

	ret = uart_config->ack_cb(msg->id, uart_config->ack_cb_user_data);
	if (ret < 0) {
		LOG_ERR("Failed to process received ACK: %d", ret);
	}
}

static void handle_uart_recv_msg(const struct device *dev,
				 struct zbus_multidomain_uart_config *uart_config,
				 const struct zbus_proxy_agent_msg *msg)
{
	int ret;
	/* Synchronize uart message with rx buffer */
	if (msg->message_size == 0 || msg->message_size > CONFIG_ZBUS_MULTIDOMAIN_MESSAGE_SIZE) {
		LOG_ERR("Invalid message size: %u", msg->message_size);
		return;
	}

	if (!uart_config->recv_cb) {
		LOG_ERR("Receive callback not set, dropping message");
		return;
	}

	ret = uart_config->recv_cb(msg);
	if (ret < 0) {
		LOG_ERR("Failed to process received message: %d", ret);
		return;
	}

	/* Schedule work to send ACK to avoid blocking the receive callback */
	uart_config->ack_msg_id = msg->id;
	ret = k_work_submit(&uart_config->ack_work);
	if (ret < 0) {
		LOG_ERR("Failed to submit ACK work: %d", ret);
		return;
	}
}

void zbus_multidomain_uart_backend_cb(const struct device *dev, struct uart_event *evt,
				      void *config)
{
	int ret;
	struct zbus_multidomain_uart_config *uart_config =
		(struct zbus_multidomain_uart_config *)config;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&uart_config->tx_busy_sem);
		break;

	case UART_TX_ABORTED:
		LOG_ERR("UART TX aborted");
		k_sem_give(&uart_config->tx_busy_sem);
		break;

	case UART_RX_RDY:

		const struct zbus_proxy_agent_msg *msg =
			(struct zbus_proxy_agent_msg *)evt->data.rx.buf;

		if (verify_proxy_agent_msg_crc(msg) != 0) {
			LOG_ERR("Received message with invalid CRC, dropping");
			LOG_HEXDUMP_DBG((const uint8_t *)msg, sizeof(*msg), "Invalid message:");
			LOG_DBG("Received CRC32: 0x%08X, Expected CRC32: 0x%08X", msg->crc32,
				crc32_ieee((const uint8_t *)msg,
					   sizeof(*msg) - sizeof(msg->crc32)));

			/* Force uart reset to dump buffer with possible overflow */
			ret = uart_rx_disable(dev);
			if (ret < 0) {
				LOG_ERR("Failed to disable RX for reset: %d", ret);
			}

			return;
		}

		if (msg->type == ZBUS_PROXY_AGENT_MSG_TYPE_ACK) {
			handle_uart_recv_ack(uart_config, msg);
		} else if (msg->type == ZBUS_PROXY_AGENT_MSG_TYPE_MSG) {
			handle_uart_recv_msg(dev, uart_config, msg);
		} else {
			LOG_WRN("Unknown message type: %d", msg->type);

			/* Force uart reset to dump buffer with possible overflow */
			ret = uart_rx_disable(dev);
			if (ret < 0) {
				LOG_ERR("Failed to disable RX for reset: %d", ret);
			}
			return;
		}

		break;

	case UART_RX_BUF_REQUEST:
		ret = uart_rx_buf_rsp(dev, uart_config->async_rx_buf[uart_config->async_rx_buf_idx],
				      sizeof(uart_config->async_rx_buf[0]));
		if (ret < 0) {
			LOG_ERR("Failed to provide RX buffer: %d", ret);
		} else {
			uart_config->async_rx_buf_idx = (uart_config->async_rx_buf_idx + 1) %
							CONFIG_ZBUS_MULTIDOMAIN_UART_BUF_COUNT;
			LOG_DBG("Provided RX buffer %d", uart_config->async_rx_buf_idx);
		}
		break;

	case UART_RX_BUF_RELEASED:
		break;

	case UART_RX_DISABLED:
		LOG_WRN("UART RX disabled, re-enabling");
		ret = uart_rx_enable(dev, uart_config->async_rx_buf[uart_config->async_rx_buf_idx],
				     sizeof(uart_config->async_rx_buf[0]), SYS_FOREVER_US);
		if (ret < 0) {
			LOG_ERR("Failed to re-enable UART RX: %d", ret);
		}
		break;

	default:
		LOG_DBG("Unhandled UART event: %d", evt->type);
		break;
	}
}

int zbus_multidomain_uart_backend_set_recv_cb(
	void *config, int (*recv_cb)(const struct zbus_proxy_agent_msg *msg))
{
	struct zbus_multidomain_uart_config *uart_config =
		(struct zbus_multidomain_uart_config *)config;

	if (!uart_config || !recv_cb) {
		LOG_ERR("Invalid parameters to set receive callback");
		return -EINVAL;
	}

	uart_config->recv_cb = recv_cb;
	LOG_DBG("Set receive callback for UART device %s", uart_config->dev->name);
	return 0;
}

int zbus_multidomain_uart_backend_set_ack_cb(void *config,
					     int (*ack_cb)(uint32_t msg_id, void *user_data),
					     void *user_data)
{
	struct zbus_multidomain_uart_config *uart_config =
		(struct zbus_multidomain_uart_config *)config;

	if (!uart_config || !ack_cb) {
		LOG_ERR("Invalid parameters to set ACK callback");
		return -EINVAL;
	}

	uart_config->ack_cb = ack_cb;
	uart_config->ack_cb_user_data = user_data;
	LOG_DBG("Set ACK callback for UART device %s", uart_config->dev->name);
	return 0;
}

int zbus_multidomain_uart_backend_init(void *config)
{
	int ret;

	if (!config) {
		LOG_ERR("Invalid UART backend configuration");
		return -EINVAL;
	}

	struct zbus_multidomain_uart_config *uart_config =
		(struct zbus_multidomain_uart_config *)config;

	k_work_init(&uart_config->ack_work, uart_ack_work_handler);

	if (!device_is_ready(uart_config->dev)) {
		LOG_ERR("Device %s is not ready", uart_config->dev->name);
		return -ENODEV;
	}

	ret = uart_callback_set(uart_config->dev, zbus_multidomain_uart_backend_cb, uart_config);
	if (ret < 0) {
		LOG_ERR("Failed to set UART callback: %d", ret);
		return ret;
	}

	ret = uart_rx_enable(uart_config->dev, uart_config->async_rx_buf[0],
			     sizeof(uart_config->async_rx_buf[0]), SYS_FOREVER_US);
	if (ret < 0 && ret != -EBUSY) { /* -EBUSY if allready enabled */
		LOG_ERR("Failed to enable UART RX: %d", ret);
		return ret;
	}

	ret = k_sem_init(&uart_config->tx_busy_sem, 1, 1);
	if (ret < 0) {
		LOG_ERR("Failed to initialize TX busy semaphore: %d", ret);
		return ret;
	}

	LOG_DBG("ZBUS Multidomain UART initialized for device %s", uart_config->dev->name);

	return 0;
}

int zbus_multidomain_uart_backend_send(void *config, struct zbus_proxy_agent_msg *msg)
{
	int ret;

	struct zbus_multidomain_uart_config *uart_config =
		(struct zbus_multidomain_uart_config *)config;

	if (!config || !msg || msg->message_size == 0 ||
	    msg->message_size > CONFIG_ZBUS_MULTIDOMAIN_MESSAGE_SIZE) {
		LOG_ERR("Invalid parameters to send message");
		return -EINVAL;
	}

	ret = k_sem_take(&uart_config->tx_busy_sem, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("Failed to take TX busy semaphore: %d", ret);
		return ret;
	}

	/* Create a copy of the message to hand to uart tx, as the original message may be modified
	 */
	memcpy(&uart_config->tx_msg_copy, msg, sizeof(uart_config->tx_msg_copy));

	ret = uart_tx(uart_config->dev, (uint8_t *)&uart_config->tx_msg_copy,
		      sizeof(uart_config->tx_msg_copy), SYS_FOREVER_US);
	if (ret < 0) {
		LOG_ERR("Failed to send message via UART: %d", ret);
		k_sem_give(&uart_config->tx_busy_sem);
		return ret;
	}

	LOG_DBG("Sent message of size %d via UART", uart_config->tx_msg_copy.message_size);

	return 0;
}

/* Define the UART backend API */
const struct zbus_proxy_agent_api zbus_multidomain_uart_api = {
	.backend_init = zbus_multidomain_uart_backend_init,
	.backend_send = zbus_multidomain_uart_backend_send,
	.backend_set_recv_cb = zbus_multidomain_uart_backend_set_recv_cb,
	.backend_set_ack_cb = zbus_multidomain_uart_backend_set_ack_cb,
};
