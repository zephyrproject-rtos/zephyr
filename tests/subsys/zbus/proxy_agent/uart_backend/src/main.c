/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/data/cobs.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_uart.h>

#include <string.h>

#define TEST_UART_FRAME_DELIMITER 0x00U
#define TEST_PROXY_WORK_Q_STACK_SIZE 2048

struct observed_proxy_msg {
	struct zbus_proxy_msg msg;
	const struct zbus_proxy_agent *agent;
};

struct uart_link {
	const struct device *peer;
	uint8_t captured[ZBUS_PROXY_AGENT_UART_MAX_ENCODED_FRAME_SIZE];
	size_t captured_len;
};

struct cobs_test_sink {
	uint8_t *buf;
	size_t len;
	size_t cap;
};

static const struct device *const uart_a = DEVICE_DT_GET(DT_NODELABEL(euart0));
static const struct device *const uart_b = DEVICE_DT_GET(DT_NODELABEL(euart1));
static struct uart_link link_a_to_b = {
	.peer = DEVICE_DT_GET(DT_NODELABEL(euart1)),
};

static struct uart_link link_b_to_a = {
	.peer = DEVICE_DT_GET(DT_NODELABEL(euart0)),
};

static struct observed_proxy_msg last_rx_on_a;
static struct observed_proxy_msg last_rx_on_b;
K_KERNEL_STACK_DEFINE(test_proxy_work_q_stack, TEST_PROXY_WORK_Q_STACK_SIZE);
static struct k_work_q test_proxy_work_q;

K_SEM_DEFINE(a_rx_sem, 0, 1);
K_SEM_DEFINE(b_rx_sem, 0, 1);

_ZBUS_PROXY_AGENT_UART_CONFIG_DEFINE(test_agent_a, DEVICE_DT_GET(DT_NODELABEL(euart0)));
_ZBUS_PROXY_AGENT_UART_CONFIG_DEFINE(test_agent_b, DEVICE_DT_GET(DT_NODELABEL(euart1)));

static struct zbus_proxy_agent agent_a = {
	.name = "agent_a",
	.backend_config = _ZBUS_PROXY_AGENT_UART_CONFIG(test_agent_a),
	.backend_api = _ZBUS_PROXY_AGENT_UART_API,
	.work_q = &test_proxy_work_q,
};

static struct zbus_proxy_agent agent_b = {
	.name = "agent_b",
	.backend_config = _ZBUS_PROXY_AGENT_UART_CONFIG(test_agent_b),
	.backend_api = _ZBUS_PROXY_AGENT_UART_API,
	.work_q = &test_proxy_work_q,
};

static int test_cobs_sink_cb(const uint8_t *buf, size_t len, void *user_data)
{
	struct cobs_test_sink *sink = user_data;

	if (sink->len + len > sink->cap) {
		return -ENOMEM;
	}

	memcpy(&sink->buf[sink->len], buf, len);
	sink->len += len;

	return (int)len;
}

static int test_recv_capture_cb(const struct zbus_proxy_agent *agent,
				const struct zbus_proxy_msg *msg)
{
	struct observed_proxy_msg *captured;
	struct k_sem *sem;

	if (agent == &agent_a) {
		captured = &last_rx_on_a;
		sem = &a_rx_sem;
	} else if (agent == &agent_b) {
		captured = &last_rx_on_b;
		sem = &b_rx_sem;
	} else {
		return -EINVAL;
	}

	memset(captured, 0, sizeof(*captured));
	captured->agent = agent;
	captured->msg = *msg;
	k_sem_give(sem);

	return 0;
}

static int encode_raw_frame(const uint8_t *decoded, size_t decoded_len, uint8_t *encoded,
			    size_t encoded_capacity)
{
	struct cobs_encoder enc;
	struct cobs_test_sink sink = {
		.buf = encoded,
		.cap = encoded_capacity,
	};
	int ret;

	ret = cobs_encoder_init(&enc, test_cobs_sink_cb, &sink, COBS_FLAG_TRAILING_DELIMITER);
	if (ret < 0) {
		return ret;
	}

	ret = cobs_encoder_write(&enc, decoded, decoded_len);
	if (ret < 0) {
		return ret;
	}

	ret = cobs_encoder_close(&enc);
	if (ret < 0) {
		return ret;
	}

	return (int)sink.len;
}

static void uart_forward_callback(const struct device *dev, size_t size, void *user_data)
{
	struct uart_link *link = user_data;
	uint8_t buf[32];
	uint32_t len;

	ARG_UNUSED(size);

	do {
		len = uart_emul_get_tx_data(dev, buf, sizeof(buf));
		if (len == 0U) {
			break;
		}

		zassert_true(link->captured_len + len <= sizeof(link->captured),
			     "Captured UART frame exceeded test buffer");
		memcpy(&link->captured[link->captured_len], buf, len);
		link->captured_len += len;
		zassert_equal(uart_emul_put_rx_data(link->peer, buf, len), len,
			      "Failed to forward UART bytes to peer");
	} while (len > 0U);
}

static void reset_backend_state(const struct zbus_proxy_agent *agent)
{
	const struct zbus_proxy_agent_uart_config *config = agent->backend_config;

	ring_buf_reset(&config->data->rx_ringbuf);
	config->data->rx_frame_len = 0U;
	config->data->rx_drop_until_delimiter = false;
	atomic_clear(&config->data->rx_overrun);
}

static void init_test_proxy_msg(struct zbus_proxy_msg *msg, const uint8_t *payload,
				uint32_t payload_len, const char *channel_name)
{
	memset(msg, 0, sizeof(*msg));
	msg->message_size = payload_len;
	memcpy(msg->message, payload, payload_len);
	msg->channel_name_len = strlen(channel_name) + 1U;
	memcpy(msg->channel_name, channel_name, msg->channel_name_len);
}

static void *test_setup(void)
{
	static bool initialized;
	struct k_work_queue_config cfg = {
		.name = "zbus_uart_test",
	};
	int ret;

	zassert_true(device_is_ready(uart_a), "UART A not ready");
	zassert_true(device_is_ready(uart_b), "UART B not ready");

	if (initialized) {
		return NULL;
	}

	k_work_queue_init(&test_proxy_work_q);
	k_work_queue_start(&test_proxy_work_q, test_proxy_work_q_stack,
			   K_KERNEL_STACK_SIZEOF(test_proxy_work_q_stack), 0, &cfg);
	k_thread_name_set(&test_proxy_work_q.thread, "zbus_uart_test");

	uart_emul_callback_tx_data_ready_set(uart_a, uart_forward_callback, &link_a_to_b);
	uart_emul_callback_tx_data_ready_set(uart_b, uart_forward_callback, &link_b_to_a);

	ret = agent_a.backend_api->backend_set_recv_cb(&agent_a, test_recv_capture_cb);
	zassert_ok(ret, "Failed to install receive callback for agent A");
	ret = agent_b.backend_api->backend_set_recv_cb(&agent_b, test_recv_capture_cb);
	zassert_ok(ret, "Failed to install receive callback for agent B");

	ret = agent_a.backend_api->backend_init(&agent_a);
	zassert_ok(ret, "Failed to initialize UART backend for agent A");
	ret = agent_b.backend_api->backend_init(&agent_b);
	zassert_ok(ret, "Failed to initialize UART backend for agent B");

	initialized = true;
	return NULL;
}

static void test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_true(k_work_queue_drain(&test_proxy_work_q, false) >= 0,
		     "Failed to drain UART test workqueue before test");
	uart_emul_flush_rx_data(uart_a);
	uart_emul_flush_tx_data(uart_a);
	uart_emul_flush_rx_data(uart_b);
	uart_emul_flush_tx_data(uart_b);

	memset(&last_rx_on_a, 0, sizeof(last_rx_on_a));
	memset(&last_rx_on_b, 0, sizeof(last_rx_on_b));
	memset(link_a_to_b.captured, 0, sizeof(link_a_to_b.captured));
	memset(link_b_to_a.captured, 0, sizeof(link_b_to_a.captured));
	link_a_to_b.captured_len = 0U;
	link_b_to_a.captured_len = 0U;

	reset_backend_state(&agent_a);
	reset_backend_state(&agent_b);

	k_sem_reset(&a_rx_sem);
	k_sem_reset(&b_rx_sem);
}

ZTEST(uart_backend, test_macro_config_generation)
{
	const struct zbus_proxy_agent_uart_config *config_a =
		_ZBUS_PROXY_AGENT_UART_CONFIG(test_agent_a);
	const struct zbus_proxy_agent_uart_config *config_b =
		_ZBUS_PROXY_AGENT_UART_CONFIG(test_agent_b);

	zassert_not_null(config_a, "Agent A backend config should exist");
	zassert_not_null(config_b, "Agent B backend config should exist");
	zassert_equal(config_a->dev, uart_a, "Agent A UART device mismatch");
	zassert_equal(config_b->dev, uart_b, "Agent B UART device mismatch");
	zassert_not_null(config_a->data, "Agent A runtime data should exist");
	zassert_not_null(config_b->data, "Agent B runtime data should exist");
}

ZTEST(uart_backend, test_send_receive_with_cobs_framing)
{
	struct zbus_proxy_msg outbound;
	uint8_t payload[4];
	int ret;

	sys_put_le32(0x12345678U, payload);
	init_test_proxy_msg(&outbound, payload, sizeof(payload), "test_channel");

	ret = agent_a.backend_api->backend_send(&agent_a, &outbound);
	zassert_ok(ret, "Sending from agent A should succeed");

	ret = k_sem_take(&b_rx_sem, K_SECONDS(1));
	zassert_ok(ret, "Timed out waiting for UART frame on agent B");
	zassert_equal(last_rx_on_b.agent, &agent_b, "Unexpected receive target");
	zassert_equal(last_rx_on_b.msg.message_size, outbound.message_size,
		      "Received payload size mismatch");
	zassert_mem_equal(last_rx_on_b.msg.message, outbound.message, outbound.message_size,
			  "Received payload mismatch");
	zassert_equal(last_rx_on_b.msg.channel_name_len, outbound.channel_name_len,
		      "Received channel name length mismatch");
	zassert_mem_equal(last_rx_on_b.msg.channel_name, outbound.channel_name,
			  outbound.channel_name_len, "Received channel name mismatch");

	zassert_true(link_a_to_b.captured_len > 1U, "A->B should capture a framed UART payload");
	zassert_equal(link_a_to_b.captured[link_a_to_b.captured_len - 1], 0x00U,
		      "COBS frame should end with a zero delimiter");
	for (size_t i = 0; i < link_a_to_b.captured_len - 1U; ++i) {
		zassert_not_equal(link_a_to_b.captured[i], 0x00U,
				  "COBS payload must not contain zero bytes before delimiter");
	}

	sys_put_le32(0xCAFEBABEU, outbound.message);
	ret = agent_b.backend_api->backend_send(&agent_b, &outbound);
	zassert_ok(ret, "Sending from agent B should succeed");

	ret = k_sem_take(&a_rx_sem, K_SECONDS(1));
	zassert_ok(ret, "Timed out waiting for UART frame on agent A");
	zassert_equal(last_rx_on_a.agent, &agent_a, "Unexpected reverse receive target");
	zassert_mem_equal(last_rx_on_a.msg.message, outbound.message, outbound.message_size,
			  "Reverse payload mismatch");
	zassert_mem_equal(last_rx_on_a.msg.channel_name, outbound.channel_name,
			  outbound.channel_name_len, "Reverse channel name mismatch");
}

ZTEST(uart_backend, test_invalid_frame_is_dropped)
{
	uint8_t decoded[ZBUS_PROXY_AGENT_UART_WIRE_HEADER_SIZE] = {0};
	uint8_t encoded[ZBUS_PROXY_AGENT_UART_MAX_ENCODED_FRAME_SIZE];
	int encoded_len;
	int ret;

	sys_put_le32(4U, decoded);
	sys_put_le32(4U, decoded + sizeof(uint32_t));

	encoded_len = encode_raw_frame(decoded, sizeof(decoded), encoded, sizeof(encoded));
	zassert_true(encoded_len > 0, "Failed to encode invalid test frame");

	zassert_equal(uart_emul_put_rx_data(uart_b, encoded, (size_t)encoded_len),
		      (uint32_t)encoded_len, "Failed to inject invalid UART frame");

	ret = k_sem_take(&b_rx_sem, K_MSEC(100));
	zassert_equal(ret, -EAGAIN, "Invalid frame should not reach the receive callback");
}

ZTEST(uart_backend, test_corrupted_length_header_is_dropped_and_recovery_works)
{
	uint8_t decoded[ZBUS_PROXY_AGENT_UART_WIRE_HEADER_SIZE] = {0};
	uint8_t encoded[ZBUS_PROXY_AGENT_UART_MAX_ENCODED_FRAME_SIZE];
	struct zbus_proxy_msg outbound;
	uint8_t payload[4];
	int encoded_len;
	int ret;

	sys_put_le32(UINT32_MAX, decoded);
	sys_put_le32(4U, decoded + sizeof(uint32_t));

	encoded_len = encode_raw_frame(decoded, sizeof(decoded), encoded, sizeof(encoded));
	zassert_true(encoded_len > 0, "Failed to encode corrupted-length test frame");

	zassert_equal(uart_emul_put_rx_data(uart_b, encoded, (size_t)encoded_len),
		      (uint32_t)encoded_len, "Failed to inject corrupted-length UART frame");
	zassert_equal(k_sem_take(&b_rx_sem, K_MSEC(100)), -EAGAIN,
		      "Corrupted header must be rejected before callback dispatch");

	sys_put_le32(0xA5A55A5AU, payload);
	init_test_proxy_msg(&outbound, payload, sizeof(payload), "post_corruption");

	ret = agent_a.backend_api->backend_send(&agent_a, &outbound);
	zassert_ok(ret, "Valid frame after corrupted header should still send");
	zassert_ok(k_sem_take(&b_rx_sem, K_SECONDS(1)),
		   "Receiver did not recover after corrupted-length header");
	zassert_mem_equal(last_rx_on_b.msg.message, outbound.message, outbound.message_size,
			  "Recovered payload mismatch after corrupted header");
	zassert_mem_equal(last_rx_on_b.msg.channel_name, outbound.channel_name,
			  outbound.channel_name_len,
			  "Recovered channel name mismatch after corrupted header");
}

ZTEST(uart_backend, test_send_rejects_invalid_message_metadata)
{
	struct zbus_proxy_msg outbound = {0};
	int ret;

	memcpy(outbound.channel_name, "test", sizeof("test"));

	outbound.message_size = 0U;
	outbound.channel_name_len = sizeof("test");
	ret = agent_a.backend_api->backend_send(&agent_a, &outbound);
	zassert_equal(ret, -EMSGSIZE, "Zero-sized messages must be rejected");

	outbound.message_size = CONFIG_ZBUS_PROXY_AGENT_MAX_MESSAGE_SIZE + 1U;
	ret = agent_a.backend_api->backend_send(&agent_a, &outbound);
	zassert_equal(ret, -EMSGSIZE, "Oversized messages must be rejected");

	outbound.message_size = 1U;
	outbound.channel_name_len = 0U;
	ret = agent_a.backend_api->backend_send(&agent_a, &outbound);
	zassert_equal(ret, -EMSGSIZE, "Zero-length channel names must be rejected");

	outbound.channel_name_len = CONFIG_ZBUS_PROXY_AGENT_MAX_CHANNEL_NAME_SIZE + 1U;
	ret = agent_a.backend_api->backend_send(&agent_a, &outbound);
	zassert_equal(ret, -EMSGSIZE, "Oversized channel names must be rejected");

	outbound.channel_name_len = 4U;
	memcpy(outbound.channel_name, "ABCD", outbound.channel_name_len);
	ret = agent_a.backend_api->backend_send(&agent_a, &outbound);
	zassert_equal(ret, -EINVAL, "Channel names without a NUL terminator must be rejected");

	zassert_equal(link_a_to_b.captured_len, 0U, "Rejected sends must not emit UART bytes");
	zassert_equal(k_sem_take(&b_rx_sem, K_MSEC(100)), -EAGAIN,
		      "Rejected sends must not reach the peer");
}

ZTEST(uart_backend, test_send_accepts_maximum_message_and_name_lengths)
{
	struct zbus_proxy_msg outbound = {0};
	char channel_name[CONFIG_ZBUS_PROXY_AGENT_MAX_CHANNEL_NAME_SIZE];
	int ret;

	for (size_t i = 0; i < CONFIG_ZBUS_PROXY_AGENT_MAX_MESSAGE_SIZE; ++i) {
		outbound.message[i] = (uint8_t)(0x80U + i);
	}

	memset(channel_name, 'x', sizeof(channel_name));
	channel_name[sizeof(channel_name) - 1U] = '\0';

	outbound.message_size = CONFIG_ZBUS_PROXY_AGENT_MAX_MESSAGE_SIZE;
	outbound.channel_name_len = sizeof(channel_name);
	memcpy(outbound.channel_name, channel_name, sizeof(channel_name));

	ret = agent_a.backend_api->backend_send(&agent_a, &outbound);
	zassert_ok(ret, "Maximum-sized UART proxy frame should be accepted");
	zassert_ok(k_sem_take(&b_rx_sem, K_SECONDS(1)),
		   "Timed out waiting for the maximum-sized frame");
	zassert_equal(last_rx_on_b.msg.message_size, outbound.message_size,
		      "Maximum-sized frame payload length mismatch");
	zassert_mem_equal(last_rx_on_b.msg.message, outbound.message, outbound.message_size,
			  "Maximum-sized frame payload mismatch");
	zassert_equal(last_rx_on_b.msg.channel_name_len, outbound.channel_name_len,
		      "Maximum-sized frame channel name length mismatch");
	zassert_mem_equal(last_rx_on_b.msg.channel_name, outbound.channel_name,
			  outbound.channel_name_len, "Maximum-sized frame channel name mismatch");
}

ZTEST(uart_backend, test_overlong_frame_is_dropped_until_delimiter_then_recovers)
{
	uint8_t oversized[ZBUS_PROXY_AGENT_UART_MAX_ENCODED_FRAME_SIZE + 1U];
	const uint8_t delimiter[] = {TEST_UART_FRAME_DELIMITER};
	struct zbus_proxy_msg outbound;
	uint8_t payload[4];
	int ret;

	memset(oversized, 0x55U, sizeof(oversized));

	zassert_equal(uart_emul_put_rx_data(uart_b, oversized, sizeof(oversized)),
		      sizeof(oversized), "Failed to inject oversized UART payload");
	zassert_equal(uart_emul_put_rx_data(uart_b, delimiter, sizeof(delimiter)),
		      sizeof(delimiter),
		      "Failed to inject delimiter for oversized UART payload");
	zassert_equal(k_sem_take(&b_rx_sem, K_MSEC(100)), -EAGAIN,
		      "Oversized frames must be discarded before callback dispatch");

	sys_put_le32(0x0BADB002U, payload);
	init_test_proxy_msg(&outbound, payload, sizeof(payload), "recovered_channel");

	ret = agent_a.backend_api->backend_send(&agent_a, &outbound);
	zassert_ok(ret, "Valid frame after oversized payload should still send");
	zassert_ok(k_sem_take(&b_rx_sem, K_SECONDS(1)),
		   "Receiver did not recover after oversized frame");
	zassert_mem_equal(last_rx_on_b.msg.message, outbound.message, outbound.message_size,
			  "Recovered payload mismatch");
	zassert_mem_equal(last_rx_on_b.msg.channel_name, outbound.channel_name,
			  outbound.channel_name_len, "Recovered channel name mismatch");
}

ZTEST(uart_backend, test_backend_init_rejects_double_init)
{
	int ret = agent_a.backend_api->backend_init(&agent_a);

	zassert_equal(ret, -EALREADY, "Reinitializing a UART backend must fail");
}

ZTEST_SUITE(uart_backend, NULL, test_setup, test_before, NULL, NULL);
