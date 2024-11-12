/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/logging/log.h>
#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/ztest.h>

#include "stubs.h"

LOG_MODULE_REGISTER(coap_client_test, LOG_LEVEL_DBG);

DEFINE_FFF_GLOBALS;
#define FFF_FAKES_LIST(FAKE)

#define LONG_ACK_TIMEOUT_MS                 200
#define MORE_THAN_EXCHANGE_LIFETIME_MS      4 * CONFIG_COAP_INIT_ACK_TIMEOUT_MS
#define MORE_THAN_LONG_EXCHANGE_LIFETIME_MS 4 * LONG_ACK_TIMEOUT_MS
#define MORE_THAN_ACK_TIMEOUT_MS                                                                   \
	(CONFIG_COAP_INIT_ACK_TIMEOUT_MS + CONFIG_COAP_INIT_ACK_TIMEOUT_MS / 2)
#define COAP_SEPARATE_TIMEOUT (6000 * 3) /* Needs a safety marging, tests run faster than -rt */
#define VALID_MESSAGE_ID BIT(31)
#define TOKEN_OFFSET          4

static int16_t last_response_code;
static const char *test_path = "test";

static uint32_t messages_needing_response[2];
static uint8_t last_token[2][COAP_TOKEN_MAX_LEN];
static const uint8_t empty_token[COAP_TOKEN_MAX_LEN] = {0};

static struct coap_client client;
static struct coap_client client2 = {
	.fd = 1,
};

static char *short_payload = "testing";
static char *long_payload = LOREM_IPSUM_SHORT;

static uint16_t get_next_pending_message_id(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(messages_needing_response); i++) {
		if (messages_needing_response[i] & VALID_MESSAGE_ID) {
			messages_needing_response[i] &= ~VALID_MESSAGE_ID;
			return messages_needing_response[i];
		}
	}

	return UINT16_MAX;
}

static void set_next_pending_message_id(uint16_t id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(messages_needing_response); i++) {
		if (!(messages_needing_response[i] & VALID_MESSAGE_ID)) {
			messages_needing_response[i] = id;
			messages_needing_response[i] |= VALID_MESSAGE_ID;
			return;
		}
	}
}

static void store_token(uint8_t *buf)
{
	for (int i = 0; i < ARRAY_SIZE(last_token); i++) {
		if (memcmp(last_token[i], empty_token, 8) == 0) {
			memcpy(last_token[i], buf + TOKEN_OFFSET, COAP_TOKEN_MAX_LEN);
			return;
		}
	}
}

static void restore_token(uint8_t *buf)
{
	for (int i = 0; i < ARRAY_SIZE(last_token); i++) {
		if (memcmp(last_token[i], empty_token, 8) != 0) {
			memcpy(buf + TOKEN_OFFSET, last_token[i], COAP_TOKEN_MAX_LEN);
			memset(last_token[i], 0, COAP_TOKEN_MAX_LEN);
			return;
		}
	}
}

static ssize_t z_impl_zsock_recvfrom_custom_fake(int sock, void *buf, size_t max_len, int flags,
						 struct sockaddr *src_addr, socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	LOG_INF("Recvfrom");
	uint8_t ack_data[] = {0x68, 0x40, 0x00, 0x00, 0x00, 0x00,
			      0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	last_message_id = get_next_pending_message_id();

	ack_data[2] = (uint8_t)(last_message_id >> 8);
	ack_data[3] = (uint8_t)last_message_id;
	restore_token(ack_data);

	memcpy(buf, ack_data, sizeof(ack_data));

	clear_socket_events(sock, ZSOCK_POLLIN);

	return sizeof(ack_data);
}

static ssize_t z_impl_zsock_sendto_custom_fake(int sock, void *buf, size_t len, int flags,
					       const struct sockaddr *dest_addr, socklen_t addrlen)
{
	uint16_t last_message_id = 0;
	uint8_t type;

	last_message_id |= ((uint8_t *)buf)[2] << 8;
	last_message_id |= ((uint8_t *)buf)[3];
	type = (((uint8_t *)buf)[0] & 0x30) >> 4;
	store_token(buf);

	set_next_pending_message_id(last_message_id);
	LOG_INF("Latest message ID: %d", last_message_id);

	if (type == 0) {
		set_socket_events(sock, ZSOCK_POLLIN);
	}

	return 1;
}

static ssize_t z_impl_zsock_sendto_custom_fake_no_reply(int sock, void *buf, size_t len, int flags,
							const struct sockaddr *dest_addr,
							socklen_t addrlen)
{
	uint16_t last_message_id = 0;

	last_message_id |= ((uint8_t *)buf)[2] << 8;
	last_message_id |= ((uint8_t *)buf)[3];
	store_token(buf);

	set_next_pending_message_id(last_message_id);
	LOG_INF("Latest message ID: %d", last_message_id);

	return 1;
}

static ssize_t z_impl_zsock_sendto_custom_fake_echo(int sock, void *buf, size_t len, int flags,
						    const struct sockaddr *dest_addr,
						    socklen_t addrlen)
{
	uint16_t last_message_id = 0;
	struct coap_packet response = {0};
	struct coap_option option = {0};

	last_message_id |= ((uint8_t *)buf)[2] << 8;
	last_message_id |= ((uint8_t *)buf)[3];
	store_token(buf);

	set_next_pending_message_id(last_message_id);
	LOG_INF("Latest message ID: %d", last_message_id);

	int ret = coap_packet_parse(&response, buf, len, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
	}

	ret = coap_find_options(&response, COAP_OPTION_ECHO, &option, 1);

	zassert_equal(ret, 1, "Coap echo option not found, %d", ret);
	zassert_mem_equal(option.value, "echo_value", option.len, "Incorrect echo data");

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake;

	set_socket_events(sock, ZSOCK_POLLIN);

	return 1;
}

static ssize_t z_impl_zsock_sendto_custom_fake_echo_next_req(int sock, void *buf, size_t len,
							     int flags,
							     const struct sockaddr *dest_addr,
							     socklen_t addrlen)
{
	uint16_t last_message_id = 0;
	struct coap_packet response = {0};
	struct coap_option option = {0};

	last_message_id |= ((uint8_t *)buf)[2] << 8;
	last_message_id |= ((uint8_t *)buf)[3];
	store_token(buf);

	set_next_pending_message_id(last_message_id);
	LOG_INF("Latest message ID: %d", last_message_id);

	int ret = coap_packet_parse(&response, buf, len, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
	}

	ret = coap_header_get_code(&response);
	zassert_equal(ret, COAP_METHOD_POST, "Incorrect method, %d", ret);

	uint16_t payload_len;

	const uint8_t *payload = coap_packet_get_payload(&response, &payload_len);

	zassert_mem_equal(payload, "echo testing", payload_len, "Incorrect payload");

	ret = coap_find_options(&response, COAP_OPTION_ECHO, &option, 1);
	zassert_equal(ret, 1, "Coap echo option not found, %d", ret);
	zassert_mem_equal(option.value, "echo_value", option.len, "Incorrect echo data");

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake;

	set_socket_events(sock, ZSOCK_POLLIN);

	return 1;
}

static ssize_t z_impl_zsock_sendto_custom_fake_block(int sock, void *buf, size_t len, int flags,
						     const struct sockaddr *dest_addr,
						     socklen_t addrlen)
{
	errno = EAGAIN;
	return -1;
}

static ssize_t z_impl_zsock_sendto_custom_fake_err(int sock, void *buf, size_t len, int flags,
						   const struct sockaddr *dest_addr,
						   socklen_t addrlen)
{
	errno = ENETDOWN;
	return -1;
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_response(int sock, void *buf, size_t max_len,
							  int flags, struct sockaddr *src_addr,
							  socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	static uint8_t ack_data[] = {0x48, 0x40, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	last_message_id = get_next_pending_message_id();

	ack_data[2] = (uint8_t)(last_message_id >> 8);
	ack_data[3] = (uint8_t)last_message_id;
	restore_token(ack_data);

	memcpy(buf, ack_data, sizeof(ack_data));

	clear_socket_events(sock, ZSOCK_POLLIN);

	return sizeof(ack_data);
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_empty_ack(int sock, void *buf, size_t max_len,
							   int flags, struct sockaddr *src_addr,
							   socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	static uint8_t ack_data[] = {0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	last_message_id = get_next_pending_message_id();

	ack_data[2] = (uint8_t)(last_message_id >> 8);
	ack_data[3] = (uint8_t)last_message_id;

	memcpy(buf, ack_data, sizeof(ack_data));

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_response;

	return sizeof(ack_data);
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_only_ack(int sock, void *buf, size_t max_len,
							  int flags, struct sockaddr *src_addr,
							  socklen_t *addrlen)
{
	int ret;

	ret = z_impl_zsock_recvfrom_custom_fake_empty_ack(sock, buf, max_len, flags, src_addr,
							  addrlen);
	clear_socket_events(sock, ZSOCK_POLLIN);
	return ret;
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_unmatching(int sock, void *buf, size_t max_len,
							    int flags, struct sockaddr *src_addr,
							    socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	static uint8_t ack_data[] = {0x68, 0x40, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

	last_message_id = get_next_pending_message_id();

	ack_data[2] = (uint8_t)(last_message_id >> 8);
	ack_data[3] = (uint8_t)last_message_id;

	memcpy(buf, ack_data, sizeof(ack_data));

	clear_socket_events(sock, ZSOCK_POLLIN);

	return sizeof(ack_data);
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_echo(int sock, void *buf, size_t max_len,
						      int flags, struct sockaddr *src_addr,
						      socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	LOG_INF("Recvfrom");
	uint8_t ack_data[] = {0x68, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			      0x00, 0x00, 0x00, 0x00, 0xda, 0xef, 'e',  'c',
			      'h',  'o',  '_',  'v',  'a',  'l',  'u',  'e'};

	last_message_id = get_next_pending_message_id();

	ack_data[2] = (uint8_t)(last_message_id >> 8);
	ack_data[3] = (uint8_t)last_message_id;
	restore_token(ack_data);

	memcpy(buf, ack_data, sizeof(ack_data));

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_response;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_echo;

	clear_socket_events(sock, ZSOCK_POLLIN);

	return sizeof(ack_data);
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_echo_next_req(int sock, void *buf, size_t max_len,
							       int flags, struct sockaddr *src_addr,
							       socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	LOG_INF("Recvfrom");
	uint8_t ack_data[] = {0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			      0x00, 0x00, 0x00, 0x00, 0xda, 0xef, 'e',  'c',
			      'h',  'o',  '_',  'v',  'a',  'l',  'u',  'e'};

	last_message_id = get_next_pending_message_id();

	ack_data[2] = (uint8_t)(last_message_id >> 8);
	ack_data[3] = (uint8_t)last_message_id;
	restore_token(ack_data);

	memcpy(buf, ack_data, sizeof(ack_data));

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_response;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_echo_next_req;

	clear_socket_events(sock, ZSOCK_POLLIN);

	return sizeof(ack_data);
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_duplicate_response(int sock, void *buf,
								    size_t max_len, int flags,
								    struct sockaddr *src_addr,
								    socklen_t *addrlen)
{
	uint8_t token[TOKEN_OFFSET + COAP_TOKEN_MAX_LEN];

	uint16_t last_message_id = get_next_pending_message_id();

	restore_token(token);

	set_next_pending_message_id(last_message_id);
	set_next_pending_message_id(last_message_id);
	store_token(token);
	store_token(token);

	int ret = z_impl_zsock_recvfrom_custom_fake(sock, buf, max_len, flags, src_addr, addrlen);

	set_socket_events(sock, ZSOCK_POLLIN);
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake;

	return ret;
}

extern void net_coap_init(void);

static void *suite_setup(void)
{
	net_coap_init();
	zassert_ok(coap_client_init(&client, NULL));
	zassert_ok(coap_client_init(&client2, NULL));

	return NULL;
}

static void test_setup(void *data)
{
	int i;

	k_mutex_lock(&client.lock, K_FOREVER);

	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);
	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake;
	clear_socket_events(0, ZSOCK_POLLIN | ZSOCK_POLLOUT | ZSOCK_POLLERR);
	clear_socket_events(1, ZSOCK_POLLIN | ZSOCK_POLLOUT | ZSOCK_POLLERR);

	for (i = 0; i < ARRAY_SIZE(messages_needing_response); i++) {
		messages_needing_response[i] = 0;
	}

	memset(&client.requests, 0, sizeof(client.requests));
	memset(last_token, 0, sizeof(last_token));

	last_response_code = 0;

	k_mutex_unlock(&client.lock);
}

void coap_callback(int16_t code, size_t offset, const uint8_t *payload, size_t len, bool last_block,
		   void *user_data)
{
	LOG_INF("CoAP response callback, %d", code);
	last_response_code = code;
	if (user_data) {
		k_sem_give((struct k_sem *) user_data);
	}
}

ZTEST_SUITE(coap_client, NULL, suite_setup, test_setup, NULL, NULL);

ZTEST(coap_client, test_get_request)
{
	int ret = 0;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = NULL,
		.len = 0
	};

	client_request.payload = short_payload;
	client_request.len = strlen(short_payload);

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
	LOG_INF("Test done");
}

ZTEST(coap_client, test_request_block)
{
	int ret = 0;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
	};

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_block;

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_equal(ret, -EAGAIN, "");
}


ZTEST(coap_client, test_resend_request)
{
	int ret = 0;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
	};

	int (*sendto_fakes[])(int, void *, size_t, int, const struct sockaddr *, socklen_t) = {
		z_impl_zsock_sendto_custom_fake_no_reply,
		z_impl_zsock_sendto_custom_fake_block,
		z_impl_zsock_sendto_custom_fake,
	};

	SET_CUSTOM_FAKE_SEQ(z_impl_zsock_sendto, sendto_fakes, ARRAY_SIZE(sendto_fakes));
	set_socket_events(client.fd, ZSOCK_POLLOUT);

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);
	k_sleep(K_MSEC(MORE_THAN_ACK_TIMEOUT_MS));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
	zassert_equal(z_impl_zsock_sendto_fake.call_count, 3);
	LOG_INF("Test done");
}

ZTEST(coap_client, test_echo_option)
{
	int ret = 0;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = NULL,
		.len = 0
	};

	client_request.payload = short_payload;
	client_request.len = strlen(short_payload);

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_echo;

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
	LOG_INF("Test done");
}

ZTEST(coap_client, test_echo_option_next_req)
{
	int ret = 0;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = NULL,
		.len = 0
	};

	client_request.payload = short_payload;
	client_request.len = strlen(short_payload);

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_echo_next_req;

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");

	char *payload = "echo testing";

	client_request.method = COAP_METHOD_POST;
	client_request.payload = payload;
	client_request.len = strlen(payload);

	LOG_INF("Send next request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}

ZTEST(coap_client, test_get_no_path)
{
	int ret = 0;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = NULL,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = NULL,
		.len = 0
	};

	client_request.payload = short_payload;
	client_request.len = strlen(short_payload);

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);

	zassert_equal(ret, -EINVAL, "Get request without path");
}

ZTEST(coap_client, test_send_large_data)
{
	int ret = 0;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = NULL,
		.len = 0
	};

	client_request.payload = long_payload;
	client_request.len = strlen(long_payload);

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}

ZTEST(coap_client, test_no_response)
{
	int ret = 0;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = NULL,
		.len = 0
	};
	struct coap_transmission_parameters params = {
		.ack_timeout = LONG_ACK_TIMEOUT_MS,
		.coap_backoff_percent = 200,
		.max_retransmission = 0
	};

	client_request.payload = short_payload;
	client_request.len = strlen(short_payload);

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;
	set_socket_events(client.fd, ZSOCK_POLLOUT);

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, &params);

	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	k_sleep(K_MSEC(MORE_THAN_LONG_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, -ETIMEDOUT, "Unexpected response");
}

ZTEST(coap_client, test_separate_response)
{
	int ret = 0;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = NULL,
		.len = 0
	};

	client_request.payload = short_payload;
	client_request.len = strlen(short_payload);

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_empty_ack;

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}

ZTEST(coap_client, test_separate_response_lost)
{
	int ret = 0;
	struct k_sem sem;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
		.user_data = &sem,
	};

	zassert_ok(k_sem_init(&sem, 0, 1));
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_only_ack;
	set_socket_events(client.fd, ZSOCK_POLLOUT);

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	zassert_ok(k_sem_take(&sem, K_MSEC(COAP_SEPARATE_TIMEOUT)));
	zassert_equal(last_response_code, -ETIMEDOUT, "");
}

ZTEST(coap_client, test_separate_response_ack_fail)
{
	int ret = 0;
	struct k_sem sem;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
		.user_data = &sem,
	};
	zassert_ok(k_sem_init(&sem, 0, 1));

	int (*sendto_fakes[])(int, void *, size_t, int, const struct sockaddr *, socklen_t) = {
		z_impl_zsock_sendto_custom_fake,
		z_impl_zsock_sendto_custom_fake_err,
	};

	SET_CUSTOM_FAKE_SEQ(z_impl_zsock_sendto, sendto_fakes, ARRAY_SIZE(sendto_fakes));
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_empty_ack;

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	zassert_ok(k_sem_take(&sem, K_MSEC(COAP_SEPARATE_TIMEOUT)));
	zassert_equal(last_response_code, -ENETDOWN, "");
}

ZTEST(coap_client, test_multiple_requests)
{
	int ret = 0;
	int retry = MORE_THAN_EXCHANGE_LIFETIME_MS;
	struct k_sem sem1, sem2;

	struct sockaddr address = {0};
	struct coap_client_request req1 = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
		.user_data = &sem1
	};
	struct coap_client_request req2 = req1;

	req2.user_data = &sem2;

	k_sem_init(&sem1, 0, 1);
	k_sem_init(&sem2, 0, 1);

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &req1, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	ret = coap_client_req(&client, 0, &address, &req2, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	set_socket_events(client.fd, ZSOCK_POLLIN);
	while (last_response_code == 0 && retry > 0) {
		retry--;
		k_sleep(K_MSEC(1));
	}
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");

	last_response_code = 0;
	set_socket_events(client.fd, ZSOCK_POLLIN);
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}

ZTEST(coap_client, test_unmatching_tokens)
{
	int ret = 0;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = NULL,
		.len = 0
	};
	struct coap_transmission_parameters params = {
		.ack_timeout = LONG_ACK_TIMEOUT_MS,
		.coap_backoff_percent = 200,
		.max_retransmission = 0
	};

	client_request.payload = short_payload;
	client_request.len = strlen(short_payload);

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_unmatching;
	set_socket_events(client.fd, ZSOCK_POLLIN | ZSOCK_POLLOUT);

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, &params);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	k_sleep(K_MSEC(MORE_THAN_LONG_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, -ETIMEDOUT, "Unexpected response");
}

ZTEST(coap_client, test_multiple_clients)
{
	int ret;
	int retry = MORE_THAN_EXCHANGE_LIFETIME_MS;
	struct k_sem sem1, sem2;
	struct sockaddr address = {0};
	struct coap_client_request req1 = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
		.user_data = &sem1
	};
	struct coap_client_request req2 = req1;

	req2.user_data = &sem2;
	req2.payload = long_payload;
	req2.len = strlen(long_payload);

	zassert_ok(k_sem_init(&sem1, 0, 1));
	zassert_ok(k_sem_init(&sem2, 0, 1));

	k_sleep(K_MSEC(1));

	LOG_INF("Sending requests");
	ret = coap_client_req(&client, client.fd, &address, &req1, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	ret = coap_client_req(&client2, client2.fd, &address, &req2, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	while (last_response_code == 0 && retry > 0) {
		retry--;
		k_sleep(K_MSEC(1));
	}
	set_socket_events(client2.fd, ZSOCK_POLLIN);

	k_sleep(K_SECONDS(1));

	/* ensure we got both responses */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}


ZTEST(coap_client, test_poll_err)
{
	int ret = 0;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
	};

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;
	set_socket_events(client.fd, ZSOCK_POLLERR);

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, -EIO, "Unexpected response");
}

ZTEST(coap_client, test_poll_err_after_response)
{
	int ret = 0;
	struct k_sem sem1;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
		.user_data = &sem1
	};

	zassert_ok(k_sem_init(&sem1, 0, 1));
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;
	set_socket_events(client.fd, ZSOCK_POLLIN);

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");

	set_socket_events(client.fd, ZSOCK_POLLERR);
	zassert_not_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
}

ZTEST(coap_client, test_poll_err_on_another_sock)
{
	int ret = 0;
	struct k_sem sem1, sem2;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
		.user_data = &sem1
	};
	struct coap_client_request request2 = client_request;

	request2.user_data = &sem2;

	zassert_ok(k_sem_init(&sem1, 0, 1));
	zassert_ok(k_sem_init(&sem2, 0, 1));

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;
	set_socket_events(client.fd, ZSOCK_POLLERR);

	k_sleep(K_MSEC(1));

	ret = coap_client_req(&client2, client2.fd, &address, &request2, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);
	ret = coap_client_req(&client, client.fd, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	set_socket_events(client2.fd, ZSOCK_POLLIN);

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -EIO, "");
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "");
}

ZTEST(coap_client, test_duplicate_response)
{
	int ret = 0;
	struct k_sem sem;
	struct sockaddr address = {0};
	struct coap_client_request client_request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
		.user_data = &sem,
	};

	zassert_ok(k_sem_init(&sem, 0, 2));
	z_impl_zsock_recvfrom_fake.custom_fake =
		z_impl_zsock_recvfrom_custom_fake_duplicate_response;

	k_sleep(K_MSEC(1));
	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	zassert_ok(k_sem_take(&sem, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");

	zassert_equal(k_sem_take(&sem, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)), -EAGAIN, "");
}
