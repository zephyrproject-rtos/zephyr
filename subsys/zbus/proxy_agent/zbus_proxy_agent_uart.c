/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/data/cobs.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_uart.h>

#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(zbus_proxy_agent_uart, CONFIG_ZBUS_PROXY_AGENT_UART_LOG_LEVEL);

#define ZBUS_PROXY_AGENT_UART_FRAME_DELIMITER 0x00

struct zbus_proxy_agent_uart_mem_sink {
	uint8_t *buf;
	size_t len;
	size_t capacity;
};

static int zbus_proxy_agent_uart_process_frame(const struct zbus_proxy_agent *agent,
					       struct zbus_proxy_agent_uart_data *uart_data);

static struct zbus_proxy_agent_work_ctx *uart_work_ctx_from_work(struct k_work *work)
{
	return (struct zbus_proxy_agent_work_ctx *)
		((char *)work - offsetof(struct zbus_proxy_agent_work_ctx, work));
}

static void zbus_proxy_agent_uart_consume_byte(const struct zbus_proxy_agent *agent,
					       struct zbus_proxy_agent_uart_data *uart_data,
					       uint8_t byte)
{
	int ret;

	if (byte == ZBUS_PROXY_AGENT_UART_FRAME_DELIMITER) {
		if (!uart_data->rx_drop_until_delimiter && uart_data->rx_frame_len > 0U) {
			ret = zbus_proxy_agent_uart_process_frame(agent, uart_data);
			if (ret < 0) {
				LOG_WRN("Dropping UART proxy frame for agent '%s': %d",
					agent->name, ret);
			}
		}

		uart_data->rx_frame_len = 0U;
		uart_data->rx_drop_until_delimiter = false;
		return;
	}

	if (uart_data->rx_drop_until_delimiter) {
		return;
	}

	if (uart_data->rx_frame_len >= sizeof(uart_data->rx_frame)) {
		LOG_WRN("UART proxy frame too large for agent '%s'; discarding until delimiter",
			agent->name);
		uart_data->rx_frame_len = 0U;
		uart_data->rx_drop_until_delimiter = true;
		return;
	}

	uart_data->rx_frame[uart_data->rx_frame_len++] = byte;
}

static int zbus_proxy_agent_uart_mem_write(const uint8_t *buf, size_t len, void *user_data)
{
	struct zbus_proxy_agent_uart_mem_sink *sink = user_data;

	if (sink->len + len > sink->capacity) {
		return -ENOMEM;
	}

	memcpy(&sink->buf[sink->len], buf, len);
	sink->len += len;

	return (int)len;
}

static int zbus_proxy_agent_uart_process_frame(const struct zbus_proxy_agent *agent,
					       struct zbus_proxy_agent_uart_data *uart_data)
{
	struct cobs_decoder dec;
	struct zbus_proxy_agent_uart_mem_sink sink = {
		.buf = uart_data->decoded_frame,
		.capacity = sizeof(uart_data->decoded_frame),
	};
	struct zbus_proxy_msg msg = {0};
	uint32_t message_size;
	uint32_t channel_name_len;
	size_t expected_len;
	int ret;

	ret = cobs_decoder_init(&dec, zbus_proxy_agent_uart_mem_write, &sink, 0);
	if (ret < 0) {
		return ret;
	}

	ret = cobs_decoder_write(&dec, uart_data->rx_frame, uart_data->rx_frame_len);
	if (ret < 0) {
		return ret;
	}

	ret = cobs_decoder_close(&dec);
	if (ret < 0) {
		return ret;
	}

	if (sink.len < ZBUS_PROXY_AGENT_UART_WIRE_HEADER_SIZE) {
		LOG_WRN("Received UART proxy frame shorter than header: %zu", sink.len);
		return -EMSGSIZE;
	}

	message_size = sys_get_le32(sink.buf);
	channel_name_len = sys_get_le32(sink.buf + sizeof(uint32_t));

	if (message_size == 0 || message_size > CONFIG_ZBUS_PROXY_AGENT_MAX_MESSAGE_SIZE) {
		LOG_WRN("Invalid UART proxy payload size: %u", message_size);
		return -EMSGSIZE;
	}

	if (channel_name_len == 0 ||
	    channel_name_len > CONFIG_ZBUS_PROXY_AGENT_MAX_CHANNEL_NAME_SIZE) {
		LOG_WRN("Invalid UART proxy channel name size: %u", channel_name_len);
		return -EMSGSIZE;
	}

	expected_len = ZBUS_PROXY_AGENT_UART_WIRE_HEADER_SIZE + message_size + channel_name_len;
	if (sink.len != expected_len) {
		LOG_WRN("UART proxy frame size mismatch: got %zu expected %zu", sink.len,
			expected_len);
		return -EINVAL;
	}

	if (sink.buf[expected_len - 1] != '\0') {
		LOG_WRN("UART proxy channel name is not NUL terminated");
		return -EINVAL;
	}

	msg.message_size = message_size;
	memcpy(msg.message, sink.buf + ZBUS_PROXY_AGENT_UART_WIRE_HEADER_SIZE, message_size);
	msg.channel_name_len = channel_name_len;
	memcpy(msg.channel_name,
	       sink.buf + ZBUS_PROXY_AGENT_UART_WIRE_HEADER_SIZE + message_size,
	       channel_name_len);

	LOG_DBG("Decoded UART proxy frame for agent '%s': channel='%s' payload=%u",
		agent->name, msg.channel_name, msg.message_size);

	if (uart_data->recv_cb == NULL) {
		return -ENOSYS;
	}

	return uart_data->recv_cb(uart_data->recv_cb_config_ptr, &msg);
}

static void zbus_proxy_agent_uart_rx_work_handler(struct k_work *work)
{
	struct zbus_proxy_agent_work_ctx *work_ctx =
		uart_work_ctx_from_work(work);
	const struct zbus_proxy_agent *agent = work_ctx->agent;
	const struct zbus_proxy_agent_uart_config *uart_config = agent->backend_config;
	struct zbus_proxy_agent_uart_data *uart_data = uart_config->data;
	uint8_t byte;

	while (true) {
		if (atomic_set(&uart_data->rx_overrun, 0) != 0) {
			LOG_WRN("UART RX ring buffer overrun for agent '%s'; dropping until "
				"delimiter", agent->name);
			uart_data->rx_frame_len = 0U;
			uart_data->rx_drop_until_delimiter = true;
		}

		while (ring_buf_get(&uart_data->rx_ringbuf, &byte, 1U) == 1U) {
			zbus_proxy_agent_uart_consume_byte(agent, uart_data, byte);
		}

		if (atomic_get(&uart_data->rx_overrun) == 0 &&
		    ring_buf_is_empty(&uart_data->rx_ringbuf)) {
			break;
		}
	}

	if (atomic_get(&uart_data->rx_overrun) != 0 ||
	    !ring_buf_is_empty(&uart_data->rx_ringbuf)) {
		(void)k_work_submit_to_queue(agent->work_q, &uart_data->rx_work.work);
	}
}

static void zbus_proxy_agent_uart_isr(const struct device *dev, void *user_data)
{
	struct zbus_proxy_agent_uart_data *uart_data = user_data;
	const struct zbus_proxy_agent *agent = uart_data->agent;
	uint8_t *dst;
	uint8_t dummy;
	uint32_t claim_len;
	bool submit_work = false;
	int rd_len;
	int ret;

	if (!uart_irq_update(dev)) {
		return;
	}

	while (uart_irq_rx_ready(dev)) {
		claim_len = ring_buf_put_claim(&uart_data->rx_ringbuf, &dst,
					       ring_buf_space_get(&uart_data->rx_ringbuf));
		if (claim_len == 0U) {
			rd_len = uart_fifo_read(dev, &dummy, 1);
			if (rd_len <= 0) {
				break;
			}

			atomic_set(&uart_data->rx_overrun, 1);
			submit_work = true;
			continue;
		}

			rd_len = uart_fifo_read(dev, dst, claim_len);
			if (rd_len <= 0) {
				ret = ring_buf_put_finish(&uart_data->rx_ringbuf, 0U);
				if (ret != 0) {
					__ASSERT_NO_MSG(false);
					return;
				}
				break;
			}

			ret = ring_buf_put_finish(&uart_data->rx_ringbuf, rd_len);
			if (ret != 0) {
				__ASSERT_NO_MSG(false);
				return;
			}
			submit_work = true;

		if ((uint32_t)rd_len < claim_len) {
			break;
		}
	}

	if (submit_work) {
		(void)k_work_submit_to_queue(agent->work_q, &uart_data->rx_work.work);
	}
}

static int zbus_proxy_agent_uart_backend_init(const struct zbus_proxy_agent *agent)
{
	const struct zbus_proxy_agent_uart_config *uart_config;
	struct zbus_proxy_agent_uart_data *uart_data;

	_ZBUS_ASSERT(agent != NULL, "Proxy agent config is NULL in UART backend init");
	_ZBUS_ASSERT(agent->backend_config != NULL,
		     "Backend configuration is NULL in UART backend init");

	uart_config = agent->backend_config;
	uart_data = uart_config->data;

	if (uart_config->dev == NULL || !device_is_ready(uart_config->dev)) {
		LOG_ERR("UART device is not ready");
		return -ENODEV;
	}

	if (uart_data == NULL) {
		LOG_ERR("UART runtime data is NULL");
		return -EINVAL;
	}

	if (agent->work_q == NULL) {
		LOG_ERR("Proxy agent workqueue is NULL");
		return -EINVAL;
	}

	if (uart_data->initialized != 0U) {
		LOG_ERR("UART backend for proxy agent '%s' already initialized", agent->name);
		return -EALREADY;
	}

	k_work_init(&uart_data->rx_work.work, zbus_proxy_agent_uart_rx_work_handler);
	uart_data->rx_work.agent = agent;
	uart_data->agent = agent;
	k_mutex_init(&uart_data->tx_lock);
	ring_buf_init(&uart_data->rx_ringbuf, sizeof(uart_data->rx_ringbuf_storage),
		      uart_data->rx_ringbuf_storage);
	uart_data->rx_frame_len = 0U;
	uart_data->rx_drop_until_delimiter = false;
	atomic_clear(&uart_data->rx_overrun);

	uart_irq_rx_disable(uart_config->dev);
	if (uart_irq_callback_user_data_set(uart_config->dev, zbus_proxy_agent_uart_isr,
					    uart_data) < 0) {
		LOG_ERR("Failed to set UART IRQ callback");
		return -ENOTSUP;
	}

	uart_irq_rx_enable(uart_config->dev);
	uart_data->initialized = 1U;

	LOG_DBG("Initialized UART proxy backend for agent '%s' on device '%s'", agent->name,
		uart_config->dev->name);

	return 0;
}

static int zbus_proxy_agent_uart_backend_send(const struct zbus_proxy_agent *agent,
					      struct zbus_proxy_msg *msg)
{
	const struct zbus_proxy_agent_uart_config *uart_config;
	struct zbus_proxy_agent_uart_data *uart_data;
	struct zbus_proxy_agent_uart_mem_sink sink = {0};
	struct cobs_encoder enc;
	uint8_t header[ZBUS_PROXY_AGENT_UART_WIRE_HEADER_SIZE];
	int ret;

	_ZBUS_ASSERT(agent != NULL, "Proxy agent config is NULL in UART backend send");
	_ZBUS_ASSERT(msg != NULL, "Message is NULL in UART backend send");
	_ZBUS_ASSERT(agent->backend_config != NULL,
		     "Backend configuration is NULL in UART backend send");

	uart_config = agent->backend_config;
	uart_data = uart_config->data;

	if (uart_config->dev == NULL || !device_is_ready(uart_config->dev)) {
		return -ENODEV;
	}

	if (uart_data == NULL) {
		return -EINVAL;
	}

	sink.buf = uart_data->tx_frame;
	sink.capacity = sizeof(uart_data->tx_frame);

	if (msg->message_size == 0 ||
	    msg->message_size > CONFIG_ZBUS_PROXY_AGENT_MAX_MESSAGE_SIZE) {
		return -EMSGSIZE;
	}

	if (msg->channel_name_len == 0 ||
	    msg->channel_name_len > CONFIG_ZBUS_PROXY_AGENT_MAX_CHANNEL_NAME_SIZE) {
		return -EMSGSIZE;
	}

	if (msg->channel_name[msg->channel_name_len - 1] != '\0') {
		return -EINVAL;
	}

	sys_put_le32(msg->message_size, header);
	sys_put_le32(msg->channel_name_len, header + sizeof(uint32_t));

	LOG_DBG("Sending UART proxy frame for agent '%s': channel='%s' payload=%u", agent->name,
		msg->channel_name, msg->message_size);

	k_mutex_lock(&uart_data->tx_lock, K_FOREVER);

	ret = cobs_encoder_init(&enc, zbus_proxy_agent_uart_mem_write, &sink,
				COBS_FLAG_TRAILING_DELIMITER);
	if (ret >= 0) {
		ret = cobs_encoder_write(&enc, header, sizeof(header));
	}
	if (ret >= 0) {
		ret = cobs_encoder_write(&enc, msg->message, msg->message_size);
	}
	if (ret >= 0) {
		ret = cobs_encoder_write(&enc, (const uint8_t *)msg->channel_name,
					 msg->channel_name_len);
	}
	if (ret >= 0) {
		ret = cobs_encoder_close(&enc);
	}
	if (ret >= 0) {
		for (size_t i = 0; i < sink.len; ++i) {
			uart_poll_out(uart_config->dev, uart_data->tx_frame[i]);
		}
	}

	k_mutex_unlock(&uart_data->tx_lock);

	return ret < 0 ? ret : 0;
}

static int zbus_proxy_agent_uart_backend_set_recv_cb(const struct zbus_proxy_agent *agent,
						     zbus_proxy_agent_recv_cb_t recv_cb)
{
	const struct zbus_proxy_agent_uart_config *uart_config;
	struct zbus_proxy_agent_uart_data *uart_data;

	_ZBUS_ASSERT(agent != NULL, "Proxy agent config is NULL in set_recv_cb for UART backend");
	_ZBUS_ASSERT(recv_cb != NULL, "Receive callback is NULL in set_recv_cb for UART backend");
	_ZBUS_ASSERT(agent->backend_config != NULL,
		     "Backend configuration is NULL in set_recv_cb for UART backend");

	uart_config = agent->backend_config;
	uart_data = uart_config->data;

	if (uart_data == NULL) {
		return -EINVAL;
	}

	uart_data->recv_cb = recv_cb;
	uart_data->recv_cb_config_ptr = agent;

	return 0;
}

const struct zbus_proxy_agent_backend_api zbus_proxy_agent_uart_backend_api = {
	.backend_init = zbus_proxy_agent_uart_backend_init,
	.backend_send = zbus_proxy_agent_uart_backend_send,
	.backend_set_recv_cb = zbus_proxy_agent_uart_backend_set_recv_cb,
};
