/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include "zbus_multidomain_mock_backend.h"
#include <zephyr/zbus/multidomain/zbus_multidomain.h>

/* Define test channels */
ZBUS_CHAN_DEFINE(test_channel_1, uint32_t, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(test_channel_2, uint32_t, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

/* Define shadow channels for receiving tests */
ZBUS_SHADOW_CHAN_DEFINE(test_shadow_channel_1, uint32_t, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
			ZBUS_MSG_INIT(0));
ZBUS_SHADOW_CHAN_DEFINE(test_shadow_channel_2, uint32_t, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
			ZBUS_MSG_INIT(0));

/* Channel for max size testing */
typedef struct {
	uint8_t data[CONFIG_ZBUS_MULTIDOMAIN_MESSAGE_SIZE];
} max_size_msg_t;
ZBUS_CHAN_DEFINE(test_max_size_channel, max_size_msg_t, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT({}));

/* Define the proxy agent using the mock backend */
ZBUS_PROXY_AGENT_DEFINE(test_proxy_agent, ZBUS_MULTIDOMAIN_TYPE_MOCK, "test_mock_backend");

/* Add channels to the proxy agent */
ZBUS_PROXY_ADD_CHANNEL(test_proxy_agent, test_channel_1);
ZBUS_PROXY_ADD_CHANNEL(test_proxy_agent, test_channel_2);
ZBUS_PROXY_ADD_CHANNEL(test_proxy_agent, test_max_size_channel);

/* Global variables for tracking received messages in tests */
static bool message_received;
static const struct zbus_channel *last_published_channel;

/* Observer callback to track shadow channel publications */
static void test_shadow_channel_observer_cb(const struct zbus_channel *chan)
{
	last_published_channel = chan;
	message_received = true;
}

/* Define observer for shadow channels */
ZBUS_LISTENER_DEFINE(test_shadow_observer, test_shadow_channel_observer_cb);

/* Add observer to shadow channels */
ZBUS_CHAN_ADD_OBS(test_shadow_channel_1, test_shadow_observer, 3);
ZBUS_CHAN_ADD_OBS(test_shadow_channel_2, test_shadow_observer, 3);

int get_total_timeout(int attempts)
{
	int total = 0;

	for (int i = 0; i < attempts; i++) {
		int timeout = CONFIG_ZBUS_MULTIDOMAIN_SENT_MSG_ACK_TIMEOUT << i;

		if (timeout > CONFIG_ZBUS_MULTIDOMAIN_SENT_MSG_ACK_TIMEOUT_MAX) {
			timeout = CONFIG_ZBUS_MULTIDOMAIN_SENT_MSG_ACK_TIMEOUT_MAX;
		}
		total += timeout;
	}
	return total;
}

ZTEST(proxy_agent_test, test_proxy_agent_creation)
{
	extern struct zbus_proxy_agent_config test_proxy_agent_config;

	zassert_not_null(&test_proxy_agent_config, "Proxy agent config should exist");
	zassert_str_equal(test_proxy_agent_config.name, "test_proxy_agent", "Name should match");
	zassert_equal((int)test_proxy_agent_config.type, (int)ZBUS_MULTIDOMAIN_TYPE_MOCK,
		      "Type should be MOCK");
	zassert_not_null(test_proxy_agent_config.api, "API should not be NULL");
	zassert_not_null(test_proxy_agent_config.backend_config,
			 "Backend config should not be NULL");
	zassert_not_null(test_proxy_agent_config.sent_msg_pool, "Sent msg pool should not be NULL");
}

ZTEST(proxy_agent_test, test_proxy_agent_initialization)
{
	extern struct zbus_proxy_agent_config test_proxy_agent_config;
	extern const struct zbus_observer test_proxy_agent_subscriber;

	/* Test that the proxy agent was created with correct configuration */
	zassert_not_null(&test_proxy_agent_config, "Config should exist");
	zassert_not_null(&test_proxy_agent_subscriber, "Subscriber should exist");

	/* Verify that the API structure is properly set up */
	zassert_not_null(test_proxy_agent_config.api, "API should not be NULL");
	zassert_not_null(test_proxy_agent_config.api->backend_init,
			 "Backend init should not be NULL");
	zassert_not_null(test_proxy_agent_config.api->backend_send,
			 "Backend send should not be NULL");
	zassert_not_null(test_proxy_agent_config.api->backend_set_recv_cb,
			 "Set recv CB should not be NULL");
	zassert_not_null(test_proxy_agent_config.api->backend_set_ack_cb,
			 "Set ack CB should not be NULL");

	/* Test that the API functions can be called */
	int ret = test_proxy_agent_config.api->backend_init(test_proxy_agent_config.backend_config);

	zassert_equal(ret, 0, "Mock backend init should return 0");
}

ZTEST(proxy_agent_test, test_message_forwarding)
{
	uint32_t test_data = 0x12345678U;

	zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	k_sleep(K_MSEC(1));

	/* Verify the backend send was called */
	zassert_true(mock_backend_send_fake.call_count == 1,
		     "Backend send should be called at least once");

	/* Verify the sent message content */
	if (mock_backend_send_fake.call_count > 0) {
		struct zbus_proxy_agent_msg *sent_msg = mock_backend_get_last_sent_message();

		zassert_not_null(sent_msg, "Sent message should not be NULL");
		zassert_equal(sent_msg->type, ZBUS_PROXY_AGENT_MSG_TYPE_MSG,
			      "Message type should be MSG");
		zassert_str_equal(sent_msg->channel_name, "test_channel_1",
				  "Channel name should match");
	}
}

ZTEST(proxy_agent_test, test_multiple_channels)
{
	uint32_t test_data1 = 0xAABBCCDDU;
	uint32_t test_data2 = 0x11223344U;

	zbus_chan_pub(&test_channel_1, &test_data1, K_MSEC(100));
	zbus_chan_pub(&test_channel_2, &test_data2, K_MSEC(100));
	k_sleep(K_MSEC(1));

	zassert_true(mock_backend_send_fake.call_count == 2,
		     "Should send messages for both channels");
}

ZTEST(proxy_agent_test, test_retransmission_timeout)
{
	/* Disable auto-ACK for this test to observe retransmissions */
	mock_backend_set_auto_ack(false);

	size_t initial_send_count = mock_backend_send_fake.call_count;

	uint32_t test_data = 0xDEADBEEFU;

	zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	k_sleep(K_MSEC(1));

	zassert_true(mock_backend_send_fake.call_count > initial_send_count,
		     "Message should be sent initially");

	size_t first_send_count = mock_backend_send_fake.call_count;

	/* Wait for retransmission timeout */
	k_sleep(K_MSEC(get_total_timeout(2) - 1));

	zassert_true(mock_backend_send_fake.call_count > first_send_count,
		     "Message should be retransmitted after timeout");
}

ZTEST(proxy_agent_test, test_ack_stops_retransmission)
{
	/* Disable auto-ACK initially to test manual ACK behavior */
	mock_backend_set_auto_ack(false);

	size_t initial_send_count = mock_backend_send_fake.call_count;
	uint32_t test_data = 0xACEACE00U;

	zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	k_sleep(K_MSEC(1));

	zassert_true(mock_backend_send_fake.call_count > initial_send_count,
		     "Message should be sent initially");

	/* Get the message ID from the copied message to avoid use-after-scope */
	struct zbus_proxy_agent_msg *sent_msg = mock_backend_get_last_sent_message();

	zassert_not_null(sent_msg, "Sent message should not be NULL");

	uint32_t msg_id = sent_msg->id;
	size_t send_count_after_first = mock_backend_send_fake.call_count;

	/* Simulate receiving an ACK for the message using the stored callback from backend */
	if (mock_backend_stored_ack_cb) {
		mock_backend_stored_ack_cb(msg_id, mock_backend_stored_ack_user_data);
	}

	printk("Sleeping to check no retransmission after ACK\n");
	/* Wait longer than retransmission timeout */
	k_sleep(K_MSEC(get_total_timeout(2) - 1));

	zassert_equal(mock_backend_send_fake.call_count, send_count_after_first,
		      "No retransmissions should occur after ACK received");
}

ZTEST(proxy_agent_test, test_max_retransmission_attempts)
{
	/* Disable auto-ACK for this test to observe max retransmission behavior */
	mock_backend_set_auto_ack(false);

	size_t initial_send_count = mock_backend_send_fake.call_count;
	uint32_t test_data = 0xDEADBEADU;

	zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	k_sleep(K_MSEC(1));

	zassert_true(mock_backend_send_fake.call_count > initial_send_count,
		     "Message should be sent initially");

	k_sleep(K_MSEC(get_total_timeout(CONFIG_ZBUS_MULTIDOMAIN_MAX_TRANSMIT_ATTEMPTS) + 10));

	/* Verify that the number of sends matches max attempts (5 total sends) */
	size_t final_send_count = mock_backend_send_fake.call_count;

	zassert_equal(final_send_count,
		      initial_send_count + CONFIG_ZBUS_MULTIDOMAIN_MAX_TRANSMIT_ATTEMPTS,
		      "Should have exactly max retransmission attempts (5 total sends)");

	k_sleep(K_MSEC(get_total_timeout(CONFIG_ZBUS_MULTIDOMAIN_MAX_TRANSMIT_ATTEMPTS)));

	zassert_equal(mock_backend_send_fake.call_count, final_send_count,
		      "Retransmissions should eventually stop after max attempts");
}

ZTEST(proxy_agent_test, test_message_content_in_retransmissions)
{
	/* Disable auto-ACK for this test to observe retransmissions */
	mock_backend_set_auto_ack(false);

	size_t initial_send_count = mock_backend_send_fake.call_count;
	uint32_t test_data = 0x12345678U;

	printk("Send count: %u\n", mock_backend_send_fake.call_count);

	zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));

	k_sleep(K_MSEC(get_total_timeout(2) - 1));

	/* Verify multiple sends occurred */
	printk("Send count: %u\n", mock_backend_send_fake.call_count);
	zassert_true(mock_backend_send_fake.call_count >= initial_send_count + 1,
		     "At least initial send + 1 retransmission should occur, "
		     "initial_send_count=%u, send_count=%u",
		     initial_send_count, mock_backend_send_fake.call_count);

	/* Verify the content */
	struct zbus_proxy_agent_msg *last_sent_msg = mock_backend_get_last_sent_message();

	zassert_not_null(last_sent_msg, "Last sent message should not be NULL");
	zassert_equal(last_sent_msg->type, ZBUS_PROXY_AGENT_MSG_TYPE_MSG,
		      "Should be a data message");
	zassert_str_equal(last_sent_msg->channel_name, "test_channel_1",
			  "Channel name should match");

	/* Verify message data */
	uint32_t received_data;

	memcpy(&received_data, last_sent_msg->message_data, sizeof(received_data));

	zassert_equal(received_data, test_data, "Message data should match original");
}

ZTEST(proxy_agent_test, test_backend_send_failure_cleanup)
{
	/* Configure backend to fail on send */
	mock_backend_send_fake.return_val = -EIO;

	size_t initial_count = mock_backend_send_fake.call_count;
	uint32_t test_data = 0xFADEU;

	zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));

	k_sleep(K_MSEC(1));

	/* Verify backend was called but failed */
	zassert_true(mock_backend_send_fake.call_count > initial_count,
		     "Backend send should be attempted");

	/* Restore normal behavior */
	mock_backend_send_fake.return_val = 0;

	/* Verify system continues working after failure */
	size_t count_before_recovery = mock_backend_send_fake.call_count;
	uint32_t recovery_data = 0xC0FFEEU;

	zbus_chan_pub(&test_channel_1, &recovery_data, K_MSEC(100));
	k_sleep(K_MSEC(1));

	zassert_true(mock_backend_send_fake.call_count > count_before_recovery,
		     "System should recover and send new messages after backend failure");
}

ZTEST(proxy_agent_test, test_concurrent_messages)
{
	/* Send multiple messages rapidly */
	for (int i = 0; i < 3; i++) {
		uint32_t test_data = 0x1000 + i;

		zbus_chan_pub(&test_channel_1, &test_data, K_NO_WAIT);
	}

	k_sleep(K_MSEC(20));

	zassert_true(mock_backend_send_fake.call_count == 3,
		     "All concurrent messages should be sent, count=%u",
		     mock_backend_send_fake.call_count);
}

ZTEST(proxy_agent_test, test_pool_exhaustion_recovery)
{
	mock_backend_set_auto_ack(false);

	/* Fill the message pool */
	for (int i = 0; i < CONFIG_ZBUS_MULTIDOMAIN_SENT_MSG_POOL_SIZE + 2; i++) {
		uint32_t test_data = 0x2000 + i;

		zbus_chan_pub(&test_channel_1, &test_data, K_NO_WAIT);
		k_sleep(K_MSEC(1)); /* Small delay between messages */
	}
	k_sleep(K_MSEC(get_total_timeout(2)));

	printk("Send count after pool exhaustion: %u\n", mock_backend_send_fake.call_count);
	zassert_true(mock_backend_send_fake.call_count >= 10,
		     "Multiple messages should be sent even with pool pressure");

	/* Re-enable auto-ACK to clear pool */
	mock_backend_set_auto_ack(true);
	/* Wait enough time for all messages to be ACKed and pool to recover */
	k_sleep(K_MSEC(get_total_timeout(CONFIG_ZBUS_MULTIDOMAIN_MAX_TRANSMIT_ATTEMPTS) + 20));

	/* Reset counter to test recovery */
	RESET_FAKE(mock_backend_send);
	mock_backend_send_fake.return_val = 0;

	/* Verify normal operation resumes */
	uint32_t recovery_data = 0xEC08E7U;

	zbus_chan_pub(&test_channel_1, &recovery_data, K_MSEC(100));
	k_sleep(K_MSEC(1));

	printk("Send count after pool recovery: %u\n", mock_backend_send_fake.call_count);
	zassert_true(mock_backend_send_fake.call_count >= 1,
		     "Normal operation should resume after pool recovery");
}

ZTEST(proxy_agent_test, test_invalid_ack_message_id)
{
	/* Send ACK for non-existent message ID */
	if (mock_backend_stored_ack_cb) {
		uint32_t invalid_id = 0xDEADBEEFU;
		int ret = mock_backend_stored_ack_cb(invalid_id, mock_backend_stored_ack_user_data);

		zassert_equal(ret, -ENOENT, "ACK for invalid ID should return -ENOENT");
	}

	/* System should continue working normally after invalid ACK */
	uint32_t test_data = 0x87654321U;

	zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	k_sleep(K_MSEC(1));

	zassert_true(mock_backend_send_fake.call_count >= 1,
		     "System should continue working after invalid ACK");
}

ZTEST(proxy_agent_test, test_duplicate_ack_handling)
{
	/* Disable auto-ACK for manual control */
	mock_backend_set_auto_ack(false);

	uint32_t test_data = 0xDCDE1234U;

	zbus_chan_pub(&test_channel_1, &test_data, K_MSEC(100));
	k_sleep(K_MSEC(1));

	zassert_true(mock_backend_send_fake.call_count >= 1, "Message should be sent");

	/* Get the message ID from the copied message to avoid use-after-scope */
	struct zbus_proxy_agent_msg *sent_msg = mock_backend_get_last_sent_message();

	zassert_not_null(sent_msg, "Sent message should not be NULL");
	uint32_t msg_id = sent_msg->id;

	/* Send duplicate ACKs manually */
	mock_backend_send_duplicate_ack(msg_id);

	/* Verify no retransmissions occur after first ACK */
	size_t count_after_acks = mock_backend_send_fake.call_count;

	k_sleep(K_MSEC(get_total_timeout(2) - 1));

	zassert_equal(mock_backend_send_fake.call_count, count_after_acks,
		      "No retransmissions should occur after duplicate ACKs");
}

ZTEST(proxy_agent_test, test_message_size_edge_cases)
{
	max_size_msg_t max_data;

	memset(&max_data, 0xAB, sizeof(max_data));

	/* Publish maximum size message */
	zbus_chan_pub(&test_max_size_channel, &max_data, K_MSEC(100));
	k_sleep(K_MSEC(1));

	zassert_true(mock_backend_send_fake.call_count >= 1, "Should send maximum size message");

	if (mock_backend_send_fake.call_count > 0) {
		struct zbus_proxy_agent_msg *sent_msg = mock_backend_get_last_sent_message();

		zassert_not_null(sent_msg, "Sent message should not be NULL");
		zassert_equal(sent_msg->message_size, sizeof(max_size_msg_t),
			      "Message size should be maximum");
		zassert_mem_equal(sent_msg->message_data, &max_data, sent_msg->message_size,
				  "Message data should match");
	}
}

ZTEST(proxy_agent_test, test_backend_initialization_failure)
{
	/* Configure backend init to fail */
	mock_backend_init_fake.return_val = -ENODEV;

	extern struct zbus_proxy_agent_config test_proxy_agent_config;

	/* Call init directly to test failure handling */
	int ret = test_proxy_agent_config.api->backend_init(test_proxy_agent_config.backend_config);

	zassert_equal(ret, -ENODEV, "Backend init should fail with -ENODEV");

	/* Restore normal behavior */
	mock_backend_init_fake.return_val = 0;
}

ZTEST(proxy_agent_test, test_thread_safety_concurrent_publishing)
{
	/* Send messages from "different threads" rapidly with different data */
	for (int i = 0; i < 8; i++) {
		uint32_t data1 = 0x1000 + i;
		uint32_t data2 = 0x2000 + i;

		/* Simulate concurrent publishing */
		zbus_chan_pub(&test_channel_1, &data1, K_NO_WAIT);
		zbus_chan_pub(&test_channel_2, &data2, K_NO_WAIT);
		k_sleep(K_MSEC(1));
	}

	/* Give time for all messages to be processed */
	k_sleep(K_MSEC(10));

	zassert_true(mock_backend_send_fake.call_count >= 10,
		     "Should handle concurrent messages from multiple channels");
}

ZTEST(proxy_agent_test, test_message_receiving_basic)
{
	zassert_true(mock_backend_has_recv_callback(), "Receive callback should be stored");

	/* Create a message to simulate receiving from backend */
	struct zbus_proxy_agent_msg recv_msg;
	uint32_t test_data = 0xABCDEF00U;

	mock_backend_create_test_message(&recv_msg, "test_shadow_channel_1", &test_data,
					 sizeof(test_data));

	/* Simulate receiving the message via callback */
	int ret = mock_backend_get_stored_recv_cb()(&recv_msg);

	zassert_equal(ret, 0, "Receive callback should succeed");

	zassert_true(message_received, "Message should be received on shadow channel");
	zassert_not_null(last_published_channel, "Published channel should be tracked");
	zassert_str_equal(last_published_channel->name, "test_shadow_channel_1",
			  "Should publish to correct shadow channel");
}

ZTEST(proxy_agent_test, test_message_receiving_unknown_channel)
{
	zassert_true(mock_backend_has_recv_callback(), "Receive callback should be stored");

	/* Create a message for unknown channel */
	struct zbus_proxy_agent_msg recv_msg;
	uint32_t test_data = 0xDEADBEEFU;

	mock_backend_create_test_message(&recv_msg, "unknown_channel", &test_data,
					 sizeof(test_data));

	/* Simulate receiving the message via callback */
	int ret = mock_backend_get_stored_recv_cb()(&recv_msg);

	zassert_equal(ret, -ENOENT, "Should fail for unknown channel");

	zassert_false(message_received, "No message should be received for unknown channel");
}

ZTEST(proxy_agent_test, test_message_receiving_non_shadow_channel)
{
	zassert_true(mock_backend_has_recv_callback(), "Receive callback should be stored");

	/* Create a message for regular (non-shadow) channel */
	struct zbus_proxy_agent_msg recv_msg;
	uint32_t test_data = 0xCAFEBABEU;

	mock_backend_create_test_message(&recv_msg, "test_channel_1", &test_data,
					 sizeof(test_data));

	/* Simulate receiving the message via callback */
	int ret = mock_backend_get_stored_recv_cb()(&recv_msg);

	zassert_equal(ret, -EPERM, "Should fail for non-shadow channel");

	zassert_false(message_received, "No message should be received for non-shadow channel");
}

ZTEST(proxy_agent_test, test_message_receiving_null_message)
{
	zassert_true(mock_backend_has_recv_callback(), "Receive callback should be stored");

	int ret = mock_backend_get_stored_recv_cb()(NULL);

	zassert_equal(ret, -EINVAL, "Should fail for NULL message");
}

ZTEST(proxy_agent_test, test_message_receiving_max_size)
{
	zassert_true(mock_backend_has_recv_callback(), "Receive callback should be stored");

	struct zbus_proxy_agent_msg recv_msg;
	uint8_t pattern_data[CONFIG_ZBUS_MULTIDOMAIN_MESSAGE_SIZE];

	for (size_t i = 0; i < sizeof(pattern_data); i++) {
		pattern_data[i] = (uint8_t)(i & 0xFF);
	}

	mock_backend_create_test_message(&recv_msg, "test_shadow_channel_2", pattern_data,
					 sizeof(pattern_data));

	/* Simulate receiving the message via callback */
	int ret = mock_backend_get_stored_recv_cb()(&recv_msg);

	zassert_equal(ret, 0, "Should handle maximum size message");

	zassert_true(message_received, "Max size message should be received on shadow channel");
	zassert_not_null(last_published_channel, "Published channel should be tracked");
	zassert_str_equal(last_published_channel->name, "test_shadow_channel_2",
			  "Should publish to correct shadow channel");
}

static void test_setup(void *fixture)
{
	RESET_FAKE(mock_backend_init);
	RESET_FAKE(mock_backend_send);
	RESET_FAKE(mock_backend_set_recv_cb);
	RESET_FAKE(mock_backend_set_ack_cb);

	mock_backend_init_fake.return_val = 0;
	mock_backend_send_fake.return_val = 0;
	mock_backend_set_recv_cb_fake.return_val = 0;
	mock_backend_set_ack_cb_fake.return_val = 0;
	message_received = false;
	last_published_channel = NULL;

	mock_backend_set_auto_ack(true);
}

static void test_teardown(void *fixture)
{
	/* Re-enable auto-ACK to clear any pending messages */
	mock_backend_set_auto_ack(true);

	/* Wait long enough for all messages to either be ACK'd or reach max attempts */
	k_sleep(K_MSEC(get_total_timeout(CONFIG_ZBUS_MULTIDOMAIN_MAX_TRANSMIT_ATTEMPTS + 1)));

	RESET_FAKE(mock_backend_init);
	RESET_FAKE(mock_backend_send);
	RESET_FAKE(mock_backend_set_recv_cb);
	RESET_FAKE(mock_backend_set_ack_cb);
}

ZTEST_SUITE(proxy_agent_test, NULL, NULL, test_setup, test_teardown, NULL);
