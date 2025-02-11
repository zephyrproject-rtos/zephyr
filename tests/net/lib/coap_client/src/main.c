/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#include "stubs.h"

LOG_MODULE_REGISTER(coap_client_test);

DEFINE_FFF_GLOBALS;
#define FFF_FAKES_LIST(FAKE)

static int16_t last_response_code;
static const char *test_path = "test";

static uint16_t messages_needing_response[2];

static struct coap_client client;

static char *short_payload = "testing";
static char *long_payload = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
				  "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
				  "enim ad minim veniam, quis nostrud exercitation ullamco laboris "
				  "nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor "
				  "in reprehenderit in voluptate velit esse cillum dolore eu fugiat"
				  " nulla pariatur. Excepteur sint occaecat cupidatat non proident,"
				  " sunt in culpa qui officia deserunt mollit anim id est laborum.";

static ssize_t z_impl_zsock_recvfrom_custom_fake(int sock, void *buf, size_t max_len, int flags,
					  struct sockaddr *src_addr, socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	LOG_INF("Recvfrom");
	uint8_t ack_data[] = {
		0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if (messages_needing_response[0] != 0) {
		last_message_id = messages_needing_response[0];
		messages_needing_response[0] = 0;
	} else {
		last_message_id = messages_needing_response[1];
		messages_needing_response[1] = 0;
	}

	ack_data[2] = (uint8_t) (last_message_id >> 8);
	ack_data[3] = (uint8_t) last_message_id;

	memcpy(buf, ack_data, sizeof(ack_data));

	return sizeof(ack_data);
}

static ssize_t z_impl_zsock_sendto_custom_fake(int sock, void *buf, size_t len,
			       int flags, const struct sockaddr *dest_addr,
			       socklen_t addrlen)
{
	uint16_t last_message_id = 0;

	last_message_id |= ((uint8_t *) buf)[2] << 8;
	last_message_id |= ((uint8_t *) buf)[3];

	if (messages_needing_response[0] == 0) {
		messages_needing_response[0] = last_message_id;
	} else {
		messages_needing_response[1] = last_message_id;
	}

	last_response_code = ((uint8_t *) buf)[1];

	LOG_INF("Latest message ID: %d", last_message_id);

	return 1;
}

static ssize_t z_impl_zsock_sendto_custom_fake_echo(int sock, void *buf, size_t len,
			       int flags, const struct sockaddr *dest_addr,
			       socklen_t addrlen)
{
	uint16_t last_message_id = 0;
	struct coap_packet response = {0};
	struct coap_option option = {0};

	last_message_id |= ((uint8_t *) buf)[2] << 8;
	last_message_id |= ((uint8_t *) buf)[3];

	if (messages_needing_response[0] == 0) {
		messages_needing_response[0] = last_message_id;
	} else {
		messages_needing_response[1] = last_message_id;
	}

	last_response_code = ((uint8_t *) buf)[1];

	LOG_INF("Latest message ID: %d", last_message_id);

	int ret = coap_packet_parse(&response, buf, len, NULL, 0);

	if (ret < 0) {
		LOG_ERR("Invalid data received");
	}

	ret = coap_find_options(&response, COAP_OPTION_ECHO, &option, 1);

	zassert_equal(ret, 1, "Coap echo option not found, %d", ret);
	zassert_mem_equal(option.value, "echo_value", option.len, "Incorrect echo data");

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake;

	return 1;
}

static ssize_t z_impl_zsock_sendto_custom_fake_echo_next_req(int sock, void *buf, size_t len,
			       int flags, const struct sockaddr *dest_addr,
			       socklen_t addrlen)
{
	uint16_t last_message_id = 0;
	struct coap_packet response = {0};
	struct coap_option option = {0};

	last_message_id |= ((uint8_t *) buf)[2] << 8;
	last_message_id |= ((uint8_t *) buf)[3];

	if (messages_needing_response[0] == 0) {
		messages_needing_response[0] = last_message_id;
	} else {
		messages_needing_response[1] = last_message_id;
	}

	last_response_code = ((uint8_t *) buf)[1];

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

	return 1;
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_response(int sock, void *buf, size_t max_len,
							  int flags, struct sockaddr *src_addr,
							  socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	static uint8_t ack_data[] = {
		0x48, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if (messages_needing_response[0] != 0) {
		last_message_id = messages_needing_response[0];
		messages_needing_response[0] = 0;
	} else {
		last_message_id = messages_needing_response[1];
		messages_needing_response[1] = 0;
	}

	ack_data[2] = (uint8_t) (last_message_id >> 8);
	ack_data[3] = (uint8_t) last_message_id;

	memcpy(buf, ack_data, sizeof(ack_data));

	return sizeof(ack_data);
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_delayed_response(int sock, void *buf,
								  size_t max_len, int flags,
								  struct sockaddr *src_addr,
								  socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	static uint8_t ack_data[] = {
		0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if (messages_needing_response[0] != 0) {
		last_message_id = messages_needing_response[0];
		messages_needing_response[0] = 0;
	} else {
		last_message_id = messages_needing_response[1];
		messages_needing_response[1] = 0;
	}

	ack_data[2] = (uint8_t) (last_message_id >> 8);
	ack_data[3] = (uint8_t) last_message_id;

	memcpy(buf, ack_data, sizeof(ack_data));
	k_sleep(K_MSEC(10));

	return sizeof(ack_data);
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_empty_ack(int sock, void *buf, size_t max_len,
							   int flags, struct sockaddr *src_addr,
							   socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	static uint8_t ack_data[] = {
		0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if (messages_needing_response[0] != 0) {
		last_message_id = messages_needing_response[0];
		messages_needing_response[0] = 0;
	} else {
		last_message_id = messages_needing_response[1];
		messages_needing_response[1] = 0;
	}

	ack_data[2] = (uint8_t) (last_message_id >> 8);
	ack_data[3] = (uint8_t) last_message_id;

	memcpy(buf, ack_data, sizeof(ack_data));

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_response;

	return sizeof(ack_data);
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_unmatching(int sock, void *buf, size_t max_len,
							    int flags, struct sockaddr *src_addr,
							    socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	static uint8_t ack_data[] = {
		0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
	};

	if (messages_needing_response[0] != 0) {
		last_message_id = messages_needing_response[0];
		messages_needing_response[0] = 0;
	} else {
		last_message_id = messages_needing_response[1];
		messages_needing_response[1] = 0;
	}

	ack_data[2] = (uint8_t) (last_message_id >> 8);
	ack_data[3] = (uint8_t) last_message_id;

	memcpy(buf, ack_data, sizeof(ack_data));

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

	if (messages_needing_response[0] != 0) {
		last_message_id = messages_needing_response[0];
		messages_needing_response[0] = 0;
	} else {
		last_message_id = messages_needing_response[1];
		messages_needing_response[1] = 0;
	}

	ack_data[2] = (uint8_t) (last_message_id >> 8);
	ack_data[3] = (uint8_t) last_message_id;

	memcpy(buf, ack_data, sizeof(ack_data));

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_response;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_echo;

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

	if (messages_needing_response[0] != 0) {
		last_message_id = messages_needing_response[0];
		messages_needing_response[0] = 0;
	} else {
		last_message_id = messages_needing_response[1];
		messages_needing_response[1] = 0;
	}

	ack_data[2] = (uint8_t) (last_message_id >> 8);
	ack_data[3] = (uint8_t) last_message_id;

	memcpy(buf, ack_data, sizeof(ack_data));

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_response;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_echo_next_req;

	return sizeof(ack_data);
}

static void *suite_setup(void)
{
	coap_client_init(&client, NULL);

	return NULL;
}

static void test_setup(void *data)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);
	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake;
	messages_needing_response[0] = 0;
	messages_needing_response[1] = 0;
}

void coap_callback(int16_t code, size_t offset, const uint8_t *payload, size_t len, bool last_block,
		   void *user_data)
{
	LOG_INF("CoAP response callback, %d", code);
	last_response_code = code;
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
	set_socket_events(ZSOCK_POLLIN);

	k_sleep(K_MSEC(5));
	k_sleep(K_MSEC(100));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
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
		.payload = NULL,
		.len = 0
	};

	client_request.payload = short_payload;
	client_request.len = strlen(short_payload);

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_delayed_response;

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);
	k_sleep(K_MSEC(300));
	set_socket_events(ZSOCK_POLLIN);

	k_sleep(K_MSEC(100));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
	zassert_equal(z_impl_zsock_sendto_fake.call_count, 2);
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
	set_socket_events(ZSOCK_POLLIN);

	k_sleep(K_MSEC(5));
	k_sleep(K_MSEC(100));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
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
	set_socket_events(ZSOCK_POLLIN);

	k_sleep(K_MSEC(5));
	k_sleep(K_MSEC(100));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");

	char *payload = "echo testing";

	client_request.method = COAP_METHOD_POST;
	client_request.payload = payload;
	client_request.len = strlen(payload);

	LOG_INF("Send next request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);
	set_socket_events(ZSOCK_POLLIN);

	k_sleep(K_MSEC(5));
	k_sleep(K_MSEC(100));
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
	set_socket_events(ZSOCK_POLLIN);

	k_sleep(K_MSEC(5));
	k_sleep(K_MSEC(100));
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
		.ack_timeout = 200,
		.coap_backoff_percent = 200,
		.max_retransmission = 0
	};

	client_request.payload = short_payload;
	client_request.len = strlen(short_payload);

	k_sleep(K_MSEC(1));

	LOG_INF("Send request");
	clear_socket_events();
	ret = coap_client_req(&client, 0, &address, &client_request, &params);

	zassert_true(ret >= 0, "Sending request failed, %d", ret);
	k_sleep(K_MSEC(300));

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
	set_socket_events(ZSOCK_POLLIN);

	k_sleep(K_MSEC(5));
	k_sleep(K_MSEC(100));

	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}

ZTEST(coap_client, test_multiple_requests)
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
	set_socket_events(ZSOCK_POLLIN);

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	ret = coap_client_req(&client, 0, &address, &client_request, NULL);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);

	k_sleep(K_MSEC(5));
	k_sleep(K_MSEC(100));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");

	k_sleep(K_MSEC(5));
	k_sleep(K_MSEC(100));
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
		.ack_timeout = 200,
		.coap_backoff_percent = 200,
		.max_retransmission = 0
	};

	client_request.payload = short_payload;
	client_request.len = strlen(short_payload);

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_unmatching;

	LOG_INF("Send request");
	ret = coap_client_req(&client, 0, &address, &client_request, &params);
	zassert_true(ret >= 0, "Sending request failed, %d", ret);
	set_socket_events(ZSOCK_POLLIN);

	k_sleep(K_MSEC(1));
	k_sleep(K_MSEC(1));
	clear_socket_events();
	k_sleep(K_MSEC(500));
	zassert_equal(last_response_code, -ETIMEDOUT, "Unexpected response");
}
