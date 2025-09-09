/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(proxy_agent_test, LOG_LEVEL_DBG);

#include "zbus_proxy_agent_mock.h"

ZBUS_PROXY_AGENT_DEFINE(proxy_agent,                /* Proxy agent name */
			ZBUS_PROXY_AGENT_TYPE_MOCK, /* Proxy agent type */
			no_node, 10,                /* Initial ack timeout ms */
			5,                          /* Maximum transmission attempts */
			4                           /* Maximum concurrent tracked messages */
);

ZBUS_CHAN_DEFINE(test_channel_1, uint32_t, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(test_channel_2, uint32_t, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_PROXY_ADD_CHAN(proxy_agent, test_channel_1);
ZBUS_PROXY_ADD_CHAN(proxy_agent, test_channel_2);
ZBUS_SHADOW_CHAN_DEFINE(test_shadow_channel_1, uint32_t, proxy_agent, NULL, ZBUS_OBSERVERS_EMPTY,
			ZBUS_MSG_INIT(0));
ZBUS_SHADOW_CHAN_DEFINE(test_shadow_channel_2, uint32_t, proxy_agent, NULL, ZBUS_OBSERVERS_EMPTY,
			ZBUS_MSG_INIT(0));

/* Global variables for tracking received messages in tests */
static bool message_received;
static const struct zbus_channel *last_published_channel;

static void test_shadow_channel_observer_cb(const struct zbus_channel *chan)
{
	last_published_channel = chan;
	message_received = true;
}

ZBUS_LISTENER_DEFINE(test_shadow_observer, test_shadow_channel_observer_cb);
ZBUS_CHAN_ADD_OBS(test_shadow_channel_1, test_shadow_observer, 3);
ZBUS_CHAN_ADD_OBS(test_shadow_channel_2, test_shadow_observer, 3);

/* Helper function to calculate total timeout for given number of attempts using global config */
static uint32_t get_total_timeout(uint8_t attempts)
{
	extern struct zbus_proxy_agent_config proxy_agent_config;
	struct zbus_proxy_agent_config *config = &proxy_agent_config;
	uint32_t total = 0;
	uint32_t max_timeout = CONFIG_ZBUS_PROXY_AGENT_ACK_TIMEOUT_MAX_MS;

	for (uint8_t i = 0; i < attempts; i++) {
		uint32_t multiplier = 1U << i;
		uint32_t timeout;

		if (i >= 32 || config->tracking.ack_timeout_initial_ms > (UINT32_MAX >> i)) {
			timeout = max_timeout;
		} else {
			timeout = config->tracking.ack_timeout_initial_ms * multiplier;
			if (max_timeout > 0 && timeout > max_timeout) {
				timeout = max_timeout;
			}
		}
		total += timeout;
	}
	return total;
}

ZTEST(proxy_agent_test, test_proxy_agent_creation)
{
	/* Verify the proxy agent was instantiated */
	extern struct zbus_proxy_agent_config proxy_agent_config;

	zassert_not_null(&proxy_agent_config, "Proxy agent config should exist");
	zassert_not_null(proxy_agent_config.backend.backend_api, "Backend API should exist");
	zassert_not_null(proxy_agent_config.backend.backend_config, "Backend config should exist");
	zassert_equal((int)proxy_agent_config.backend.type, (int)ZBUS_PROXY_AGENT_TYPE_MOCK,
		      "Type should be MOCK");
}

ZTEST(proxy_agent_test, test_proxy_agent_backend)
{
	int ret;

	extern struct zbus_proxy_agent_config proxy_agent_config;

	zassert_not_null(proxy_agent_config.backend.backend_api, "API should not be NULL");
	zassert_not_null(proxy_agent_config.backend.backend_api->backend_init,
			 "Backend init should not be NULL");
	zassert_not_null(proxy_agent_config.backend.backend_api->backend_send,
			 "Backend send should not be NULL");
	zassert_not_null(proxy_agent_config.backend.backend_api->backend_set_recv_cb,
			 "Set recv CB should not be NULL");

	ret = proxy_agent_config.backend.backend_api->backend_init(
		proxy_agent_config.backend.backend_config);
	zassert_equal(ret, 0, "Mock backend init should return 0");
}

ZTEST(proxy_agent_test, test_serialization)
{
	int ret;
	size_t result;
	uint32_t test_data = 0x12345678U;
	uint8_t buffer[256];
	uint8_t small_buffer[10]; /* Too small for any real message */
	struct zbus_proxy_agent_msg test_msg = {0};
	struct zbus_proxy_agent_msg deserialized_msg = {0};

	/* Create valid message */
	ret = zbus_create_proxy_agent_msg(&test_msg, &test_data, sizeof(test_data), "test_channel",
					  strlen("test_channel"));
	zassert_equal(ret, 0, "Message creation should succeed");

	result = serialize_proxy_agent_msg(NULL, buffer, sizeof(buffer));
	zassert_equal(result, -EFAULT, "Serialization should fail with NULL message");
	result = serialize_proxy_agent_msg(&test_msg, NULL, sizeof(buffer));
	zassert_equal(result, -EFAULT, "Serialization should fail with NULL buffer");

	result = serialize_proxy_agent_msg(&test_msg, small_buffer, sizeof(small_buffer));
	zassert_equal(result, -EFAULT, "Serialization should fail with buffer too small");

	result = serialize_proxy_agent_msg(&test_msg, buffer, sizeof(buffer));
	zassert_true(result > 0, "Valid serialization should succeed");

	ret = deserialize_proxy_agent_msg(NULL, result, &deserialized_msg);
	zassert_equal(ret, -EFAULT, "Deserialization should fail with NULL buffer");
	ret = deserialize_proxy_agent_msg(buffer, result, NULL);
	zassert_equal(ret, -EFAULT, "Deserialization should fail with NULL message");

	ret = deserialize_proxy_agent_msg(buffer, 5, &deserialized_msg);
	zassert_equal(ret, -ENOMEM, "Deserialization should fail with insufficient buffer size");

	ret = deserialize_proxy_agent_msg(buffer, result, &deserialized_msg);
	zassert_equal(ret, 0, "Valid deserialization should succeed");

	/* Verify deserialized content matches original */
	zassert_equal(deserialized_msg.type, test_msg.type, "Message type should match");
	zassert_equal(deserialized_msg.id, test_msg.id, "Message ID should match");
	zassert_equal(deserialized_msg.message_size, test_msg.message_size,
		      "Message size should match");
	zassert_str_equal(deserialized_msg.channel_name, test_msg.channel_name,
			  "Channel name should match");
	zassert_mem_equal(deserialized_msg.message_data, test_msg.message_data,
			  test_msg.message_size, "Message data should match");
}

ZTEST(proxy_agent_test, test_send_basic)
{
	int ret;
	uint32_t initial_send_count;
	uint32_t final_send_count;
	uint32_t test_data = 0xDEADBEEFU;
	uint32_t sent_data;

	uint8_t sent_buffer[512];
	size_t sent_size;
	struct zbus_proxy_agent_msg sent_msg;

	initial_send_count = get_mock_backend_send_count();

	ret = zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	zassert_equal(ret, 0, "Channel publish should succeed");

	k_sleep(K_MSEC(get_total_timeout(1)));

	final_send_count = get_mock_backend_send_count();
	zassert_true(final_send_count == initial_send_count + 1, "Should send one message");

	/* Verify data sent matches */
	get_last_sent_message(sent_buffer, &sent_size);
	zassert_true(sent_size > 0, "Sent size should be greater than 0");

	ret = deserialize_proxy_agent_msg(sent_buffer, sent_size, &sent_msg);
	zassert_equal(ret, 0, "Deserialization of sent message should succeed");
	zassert_equal(sent_msg.type, ZBUS_PROXY_AGENT_MSG_TYPE_MSG,
		      "Sent message type should be DATA");
	zassert_equal(sent_msg.id != 0, true, "Sent message ID should be non-zero");
	zassert_str_equal(sent_msg.channel_name, test_channel_1.name,
			  "Sent channel name should match");
	zassert_equal(sent_msg.message_size, sizeof(test_data), "Sent message size should match");
	memcpy(&sent_data, sent_msg.message_data, sizeof(test_data));
	zassert_equal(sent_data, test_data, "Sent message data should match");
}

ZTEST(proxy_agent_test, test_ack_stops_retransmission)
{
	int ret;
	uint32_t initial_send_count, send_count;
	uint32_t test_data = 0xACEACE00U;

	/* Disable auto-ACK for manual control */
	set_ack_mode(MOCK_BACKEND_ACK_MODE_MANUAL);

	initial_send_count = get_mock_backend_send_count();

	ret = zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	zassert_equal(ret, 0, "Channel publish should succeed");

	k_sleep(K_MSEC(1));
	send_count = get_mock_backend_send_count();
	zassert_true(send_count == initial_send_count + 1, "Message should be sent initially");

	/* Re-enable auto-ACK */
	set_ack_mode(MOCK_BACKEND_ACK_MODE_AUTO);
	/* Wait for first timeout and retransmission */
	k_sleep(K_MSEC(get_total_timeout(1)));
	send_count = get_mock_backend_send_count();
	zassert_true(send_count == initial_send_count + 2, "Message should be retransmitted");

	k_sleep(K_MSEC(get_total_timeout(2)));
	send_count = get_mock_backend_send_count();
	zassert_true(send_count == initial_send_count + 2, "No further retransmissions after ACK");
}

ZTEST(proxy_agent_test, test_retransmission_timeout)
{
	int ret;
	uint32_t initial_send_count, retransmit_count;
	uint32_t test_data = 0xDEADBEEFU;
	extern struct zbus_proxy_agent_config proxy_agent_config;
	uint32_t expected_attempts = proxy_agent_config.tracking.ack_attempt_limit;

	set_ack_mode(MOCK_BACKEND_ACK_MODE_MANUAL);

	initial_send_count = get_mock_backend_send_count();
	test_data = 0xDEADBEEFU;

	ret = zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	zassert_equal(ret, 0, "Channel publish should succeed");

	/* Wait for retransmission timeout */
	k_sleep(K_MSEC(get_total_timeout(expected_attempts + 1) + 10));

	retransmit_count = get_mock_backend_send_count();
	zassert_true(retransmit_count == initial_send_count + expected_attempts,
		     "Should have exactly max retransmission attempts (%d total sends)",
		     expected_attempts);
}

ZTEST(proxy_agent_test, test_backend_send_failure_recovery)
{
	int ret;
	uint32_t initial_count, count_after_failure, final_count;
	uint32_t test_data = 0xFADEU;

	set_ack_mode(MOCK_BACKEND_ACK_MODE_MANUAL);
	set_mock_backend_send_failure(true);

	initial_count = get_mock_backend_send_count();

	ret = zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	zassert_equal(ret, 0, "Channel publish should succeed even if backend fails");

	k_sleep(K_MSEC(get_total_timeout(1)));

	count_after_failure = get_mock_backend_send_count();
	zassert_true(count_after_failure == initial_count + 1, "Backend send should be attempted");
	/* Error logs are expected here due to send failure, checked by regex in testcase.yaml */

	set_ack_mode(MOCK_BACKEND_ACK_MODE_AUTO);
	set_mock_backend_send_failure(false);

	/* Verify system continues working after failure */
	ret = zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	zassert_equal(ret, 0, "Channel publish should succeed after recovery");

	k_sleep(K_MSEC(get_total_timeout(1)));

	final_count = get_mock_backend_send_count();
	zassert_true(final_count == count_after_failure + 1,
		     "System should recover and send new messages after backend failure");
}

ZTEST(proxy_agent_test, test_concurrent_publishing)
{
	uint32_t final_send_count;
	uint32_t initial_send_count = get_mock_backend_send_count();

	for (int i = 0; i < 2; i++) {
		uint32_t data1 = 0x1000 + i;
		uint32_t data2 = 0x2000 + i;

		int ret1 = zbus_chan_pub(&test_channel_1, &data1, K_NO_WAIT);
		int ret2 = zbus_chan_pub(&test_channel_2, &data2, K_NO_WAIT);

		zassert_equal(ret1, 0, "Channel 1 publish should succeed");
		zassert_equal(ret2, 0, "Channel 2 publish should succeed");
	}

	k_sleep(K_MSEC(get_total_timeout(1)));

	final_send_count = get_mock_backend_send_count();
	zassert_true(final_send_count == initial_send_count + 4,
		     "Should handle concurrent messages from multiple channels");
}

ZTEST(proxy_agent_test, test_pool_exhaustion_recovery)
{
	int ret;
	uint32_t initial_count, count_after_flood, final_count;
	uint32_t recovery_data = 0xEC08E7U;
	extern struct zbus_proxy_agent_config proxy_agent_config;
	uint32_t max_attempts = proxy_agent_config.tracking.ack_attempt_limit;

	/* Prevent ACKs to fill pool */
	set_ack_mode(MOCK_BACKEND_ACK_MODE_MANUAL);

	initial_count = get_mock_backend_send_count();

	for (int i = 0; i < 5; i++) {
		uint32_t test_data = 0x2000 + i;
		/* Some publishes may fail due to pool exhaustion, which is expected */
		zbus_chan_pub(&test_channel_1, &test_data, K_NO_WAIT);
		/* Some simulation platform uses a poor resolution uptime as sys_clock_cycle_get()
		 * which may cause multiple sends to get the same timestamp.
		 * Adding a small sleep to mitigate this.
		 */
		k_sleep(K_MSEC(1));
		/* The warning about pool exhaustion is validated via regex in testcase.yaml to
		 * ensure the pool exhaustion condition is met.
		 */
	}
	/* Re-enable auto-ACK to clear pool */
	set_ack_mode(MOCK_BACKEND_ACK_MODE_AUTO);
	k_sleep(K_MSEC(get_total_timeout(max_attempts + 1)));

	count_after_flood = get_mock_backend_send_count();
	zassert_true(count_after_flood > initial_count, "At least some messages should be sent");

	/* Verify normal operation resumes */
	ret = zbus_chan_pub(&test_channel_1, &recovery_data, K_MSEC(100));
	zassert_equal(ret, 0, "Channel publish should succeed after pool recovery");

	k_sleep(K_MSEC(get_total_timeout(1)));

	final_count = get_mock_backend_send_count();
	zassert_true(final_count == count_after_flood + 1,
		     "Normal operation should resume after pool recovery");
}

ZTEST(proxy_agent_test, test_publishing_shadow_channel)
{
	int ret;
	uint32_t initial_send_count;
	uint32_t final_send_count;
	uint32_t test_data = 0xBEEFCA;

	initial_send_count = get_mock_backend_send_count();

	ret = zbus_chan_pub(&test_shadow_channel_1, &test_data, K_MSEC(100));
	zassert_equal(ret, -ENOMSG, "Shadow channel publish should fail with -ENOMSG");

	k_sleep(K_MSEC(get_total_timeout(1)));

	final_send_count = get_mock_backend_send_count();
	zassert_true(final_send_count == initial_send_count, "Should not send any message");
}

ZTEST(proxy_agent_test, test_receiving_basic)
{
	int ret;
	struct zbus_proxy_agent_msg recv_msg = {0};
	uint32_t test_data = 0xABCDEF00U;

	ret = zbus_create_proxy_agent_msg(&recv_msg, &test_data, sizeof(test_data),
					  "test_shadow_channel_1", strlen("test_shadow_channel_1"));
	zassert_equal(ret, 0, "Message creation should succeed");

	trigger_receive_message(&recv_msg);
	k_sleep(K_MSEC(1));

	zassert_true(message_received, "Message should be received on shadow channel");
	zassert_not_null(last_published_channel, "Published channel should be tracked");
	zassert_str_equal(last_published_channel->name, "test_shadow_channel_1",
			  "Should publish to correct shadow channel");
}

ZTEST(proxy_agent_test, test_message_receiving_unknown_channel)
{
	int ret;
	struct zbus_proxy_agent_msg recv_msg = {0};
	uint32_t test_data = 0xDEADBEEFU;

	ret = zbus_create_proxy_agent_msg(&recv_msg, &test_data, sizeof(test_data),
					  "unknown_channel", strlen("unknown_channel"));
	zassert_equal(ret, 0, "Message creation should succeed");

	trigger_receive_message(&recv_msg);
	k_sleep(K_MSEC(1));

	zassert_false(message_received, "No message should be received for unknown channel");
	zassert_is_null(last_published_channel, "No channel should be published to");
	/* The warning about unknown channel is validated via regex in testcase.yaml */
}

ZTEST(proxy_agent_test, test_message_receiving_non_shadow_channel)
{
	int ret;
	struct zbus_proxy_agent_msg recv_msg = {0};
	uint32_t test_data = 0xCAFEBABEU;

	ret = zbus_create_proxy_agent_msg(&recv_msg, &test_data, sizeof(test_data),
					  "test_channel_1", strlen("test_channel_1"));
	zassert_equal(ret, 0, "Message creation should succeed");

	trigger_receive_message(&recv_msg);
	k_sleep(K_MSEC(1));

	zassert_false(message_received, "No message should be received for non-shadow channel");
	zassert_is_null(last_published_channel, "No channel should be published to");
	/* The warning about non-shadow channel is validated via regex in testcase.yaml */
}

ZTEST(proxy_agent_test, test_duplicate_message_detection)
{
	int ret;
	struct zbus_proxy_agent_msg recv_msg = {0};
	uint32_t test_data = 0xDEFCAE;

	ret = zbus_create_proxy_agent_msg(&recv_msg, &test_data, sizeof(test_data),
					  "test_shadow_channel_1", strlen("test_shadow_channel_1"));
	zassert_equal(ret, 0, "Message creation should succeed");

	/* Send the message first time */
	trigger_receive_message(&recv_msg);
	k_sleep(K_MSEC(get_total_timeout(1)));

	zassert_true(message_received, "First message should be received");
	zassert_not_null(last_published_channel, "Channel should be published to");

	/* Reset flags for duplicate test */
	message_received = false;
	last_published_channel = NULL;

	/* Send the same message again (duplicate) */
	trigger_receive_message(&recv_msg);
	k_sleep(K_MSEC(get_total_timeout(1)));

	zassert_false(message_received, "Duplicate message should not be published");
	zassert_is_null(last_published_channel, "No channel should be published to for duplicate");
}

ZTEST(proxy_agent_test, test_message_receiving_max_size)
{
	int ret;
	struct zbus_proxy_agent_msg recv_msg = {0};
	uint8_t pattern_data[CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE];

	for (size_t i = 0; i < sizeof(pattern_data); i++) {
		pattern_data[i] = (uint8_t)(i & 0xFF);
	}

	ret = zbus_create_proxy_agent_msg(&recv_msg, pattern_data, sizeof(pattern_data),
					  "test_shadow_channel_2", strlen("test_shadow_channel_2"));
	zassert_equal(ret, 0, "Max size message creation should succeed");

	trigger_receive_message(&recv_msg);
	k_sleep(K_MSEC(get_total_timeout(1)));

	zassert_true(message_received, "Max size message should be received on shadow channel");
	zassert_not_null(last_published_channel, "Published channel should be tracked");
	zassert_str_equal(last_published_channel->name, "test_shadow_channel_2",
			  "Should publish to correct shadow channel");
}

ZTEST(proxy_agent_test, test_receive_oversized_message_rejection)
{
	uint8_t buffer[512];
	size_t offset = 0;

	/* Manually craft a serialized message with oversized message data */
	uint8_t type = ZBUS_PROXY_AGENT_MSG_TYPE_MSG;
	uint32_t id = 12345;
	uint32_t oversized_message_size = CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE + 1; /* Too large */
	uint8_t dummy_data[32] = {0};
	uint32_t channel_name_len = strlen("test_shadow_channel_1");

	/* Build serialized buffer manually */
	memcpy(buffer + offset, &type, sizeof(type));
	offset += sizeof(type);
	memcpy(buffer + offset, &id, sizeof(id));
	offset += sizeof(id);
	memcpy(buffer + offset, &oversized_message_size, sizeof(oversized_message_size));
	offset += sizeof(oversized_message_size);
	memcpy(buffer + offset, dummy_data, sizeof(dummy_data));
	offset += sizeof(dummy_data);
	memcpy(buffer + offset, &channel_name_len, sizeof(channel_name_len));
	offset += sizeof(channel_name_len);
	memcpy(buffer + offset, "test_shadow_channel_1", channel_name_len);
	offset += channel_name_len;

	trigger_receive(buffer, offset);
	k_sleep(K_MSEC(get_total_timeout(1)));

	zassert_false(message_received, "Oversized message should be rejected");
	zassert_is_null(last_published_channel,
			"No channel should be published to for oversized message");
}

ZTEST(proxy_agent_test, test_receive_oversized_channel_name_rejection)
{
	uint8_t buffer[512];
	size_t offset = 0;

	/* Manually craft a serialized message with oversized channel name */
	uint8_t type = ZBUS_PROXY_AGENT_MSG_TYPE_MSG;
	uint32_t id = 54321;
	uint32_t message_size = 4;
	uint32_t dummy_data = 0x12345678;
	uint32_t channel_name_len = CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE + 1; /* Too large */
	char dummy_name[64] = "very_long_channel_name_that_exceeds_limits";

	memcpy(buffer + offset, &type, sizeof(type));
	offset += sizeof(type);
	memcpy(buffer + offset, &id, sizeof(id));
	offset += sizeof(id);
	memcpy(buffer + offset, &message_size, sizeof(message_size));
	offset += sizeof(message_size);
	memcpy(buffer + offset, &dummy_data, message_size);
	offset += message_size;
	memcpy(buffer + offset, &channel_name_len, sizeof(channel_name_len));
	offset += sizeof(channel_name_len);
	memcpy(buffer + offset, dummy_name, sizeof(dummy_name));
	offset += sizeof(dummy_name);

	trigger_receive(buffer, offset);
	k_sleep(K_MSEC(get_total_timeout(1)));

	zassert_false(message_received, "Message with oversized channel name should be rejected");
	zassert_is_null(last_published_channel,
			"No channel should be published to for oversized channel name");
}

ZTEST(proxy_agent_test, test_nack_handling)
{
	int ret;
	uint32_t initial_send_count, first_send_count, retransmit_count;
	uint32_t msg_id;
	uint32_t test_data = 0xBADC0FFEU;

	set_ack_mode(MOCK_BACKEND_ACK_MODE_MANUAL);

	initial_send_count = get_mock_backend_send_count();
	ret = zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	zassert_equal(ret, 0, "Channel publish should succeed");

	k_sleep(K_MSEC(get_total_timeout(1) - 1));
	first_send_count = get_mock_backend_send_count();
	zassert_true(first_send_count > initial_send_count, "Message should be sent initially");

	msg_id = get_last_sent_message_id();
	trigger_nack(msg_id);

	k_sleep(K_MSEC(get_total_timeout(2)));

	retransmit_count = get_mock_backend_send_count();
	zassert_true(retransmit_count == first_send_count, "NACK should stop retransmission");
	/* The warning about NACK is validated via regex in testcase.yaml */
}

ZTEST(proxy_agent_test, test_duplicate_ack_handling)
{
	int ret;
	uint32_t count_after_acks, final_count;
	uint32_t msg_id;
	uint32_t test_data = 0xDCDE1234U;

	set_ack_mode(MOCK_BACKEND_ACK_MODE_MANUAL);

	ret = zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	zassert_equal(ret, 0, "Channel publish should succeed");

	k_sleep(K_MSEC(1));

	msg_id = get_last_sent_message_id();
	trigger_ack(msg_id);
	trigger_ack(msg_id); /* Duplicate ACK */

	count_after_acks = get_mock_backend_send_count();

	k_sleep(K_MSEC(get_total_timeout(2)));

	final_count = get_mock_backend_send_count();
	zassert_equal(final_count, count_after_acks,
		      "No retransmissions should occur after duplicate ACKs");
}

ZTEST(proxy_agent_test, test_correct_tracking_cancellation)
{
	int ret;
	uint32_t initial_send_count, retransmit_count;
	uint32_t msg_id_1, msg_id_2;
	uint32_t last_retransmitted_id;
	uint32_t test_data_1 = 0x1111AAAA;
	uint32_t test_data_2 = 0x2222BBBB;

	set_ack_mode(MOCK_BACKEND_ACK_MODE_MANUAL);
	initial_send_count = get_mock_backend_send_count();

	ret = zbus_chan_pub(&test_channel_1, &test_data_1, K_MSEC(100));
	zassert_equal(ret, 0, "First message should send successfully");
	k_sleep(K_USEC(100)); /* Yeild to allow send to process */
	msg_id_1 = get_last_sent_message_id();
	printk("Sent message ID 1: %u\n", msg_id_1);

	ret = zbus_chan_pub(&test_channel_1, &test_data_2, K_MSEC(100));
	zassert_equal(ret, 0, "Second message should send successfully");
	k_sleep(K_USEC(100)); /* Yeild to allow send to process */
	msg_id_2 = get_last_sent_message_id();
	printk("Sent message ID 2: %u\n", msg_id_2);

	zassert_not_equal(msg_id_1, msg_id_2, "Message IDs should be unique");

	/* ACK the second (last) message */
	trigger_ack(msg_id_2);

	k_sleep(K_MSEC(get_total_timeout(2)));

	retransmit_count = get_mock_backend_send_count();
	zassert_true(retransmit_count > initial_send_count,
		     "Retransmission should occur for msg 1");

	last_retransmitted_id = get_last_sent_message_id();
	zassert_equal(last_retransmitted_id, msg_id_1, "Only message 1 should retransmit");

	trigger_ack(msg_id_1);
}

static void test_setup(void *fixture)
{
	message_received = false;
	last_published_channel = NULL;

	reset_mock_backend_counters();
	set_ack_mode(MOCK_BACKEND_ACK_MODE_AUTO);
}

static void test_teardown(void *fixture)
{
	/* Time to allow any pending retransmissions to time out */
	extern struct zbus_proxy_agent_config proxy_agent_config;
	uint32_t max_attempts = proxy_agent_config.tracking.ack_attempt_limit;

	k_sleep(K_MSEC(get_total_timeout(max_attempts + 1)));
}

ZTEST_SUITE(proxy_agent_test, NULL, NULL, test_setup, test_teardown, NULL);
