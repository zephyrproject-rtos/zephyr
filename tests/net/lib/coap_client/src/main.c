/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/logging/log.h>
#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/ztest.h>
#if defined(CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME)
#include "nsi_timer_model.h"
#endif
#include "stubs.h"

LOG_MODULE_REGISTER(coap_client_test, LOG_LEVEL_DBG);

DEFINE_FFF_GLOBALS;
#define FFF_FAKES_LIST(FAKE)

#define LONG_ACK_TIMEOUT_MS                 (2 * CONFIG_COAP_INIT_ACK_TIMEOUT_MS)
#define MORE_THAN_EXCHANGE_LIFETIME_MS      4 * CONFIG_COAP_INIT_ACK_TIMEOUT_MS
#define MORE_THAN_LONG_EXCHANGE_LIFETIME_MS 4 * LONG_ACK_TIMEOUT_MS
#define MORE_THAN_ACK_TIMEOUT_MS                                                                   \
	(CONFIG_COAP_INIT_ACK_TIMEOUT_MS + CONFIG_COAP_INIT_ACK_TIMEOUT_MS / 2)
#define COAP_SEPARATE_TIMEOUT (6000 * 2) /* Needs a safety marging, tests run faster than -rt */
#define VALID_MESSAGE_ID BIT(31)
#define TOKEN_OFFSET          4

void coap_callback(int16_t code, size_t offset, const uint8_t *payload, size_t len, bool last_block,
		   void *user_data);

static int16_t last_response_code;
static const char test_path[] = "test";

static uint32_t messages_needing_response[2];
static uint8_t last_token[2][COAP_TOKEN_MAX_LEN];
static const uint8_t empty_token[COAP_TOKEN_MAX_LEN] = {0};
K_SEM_DEFINE(sem1, 0, 1);
K_SEM_DEFINE(sem2, 0, 1);

static struct coap_client client;
static struct coap_client client2 = {
	.fd = 1,
};

static const char short_payload[] = "testing";
static const char long_payload[] = LOREM_IPSUM_SHORT;
static struct coap_client_request short_request = {
	.method = COAP_METHOD_GET,
	.confirmable = true,
	.path = test_path,
	.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
	.cb = coap_callback,
	.payload = short_payload,
	.len = sizeof(short_payload) - 1,
	.user_data = &sem1,
};
static struct coap_client_request long_request = {
	.method = COAP_METHOD_GET,
	.confirmable = true,
	.path = test_path,
	.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
	.cb = coap_callback,
	.payload = long_payload,
	.len = sizeof(long_payload) - 1,
	.user_data = &sem2,
};
static struct sockaddr dst_address;



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

static ssize_t z_impl_zsock_recvfrom_custom_fake_rst(int sock, void *buf, size_t max_len, int flags,
						     struct sockaddr *src_addr, socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	static uint8_t rst_data[] = {0x70, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	last_message_id = get_next_pending_message_id();

	rst_data[2] = (uint8_t)(last_message_id >> 8);
	rst_data[3] = (uint8_t)last_message_id;

	memcpy(buf, rst_data, sizeof(rst_data));
	clear_socket_events(sock, ZSOCK_POLLIN);

	return sizeof(rst_data);
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

static ssize_t z_impl_zsock_recvfrom_custom_fake_observe(int sock, void *buf, size_t max_len,
							 int flags, struct sockaddr *src_addr,
							 socklen_t *addrlen)
{
	int ret = z_impl_zsock_recvfrom_custom_fake_duplicate_response(sock, buf, max_len, flags,
								       src_addr, addrlen);

	set_next_pending_message_id(get_next_pending_message_id() + 1);
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_observe;
	return ret;
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

extern void net_coap_init(void);

static void *suite_setup(void)
{
#if defined(CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME)
	/* It is enough that some slow-down is happening on sleeps, it does not have to be
	 * real time
	 */
	hwtimer_set_rt_ratio(100.0);
	k_sleep(K_MSEC(1));
#endif
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
	k_sem_reset(&sem1);
	k_sem_reset(&sem2);

	k_mutex_unlock(&client.lock);
}

static void test_after(void *data)
{
	coap_client_cancel_requests(&client);
	coap_client_cancel_requests(&client2);
}

ZTEST_SUITE(coap_client, NULL, suite_setup, test_setup, test_after, NULL);

ZTEST(coap_client, test_get_request)
{
	zassert_ok(coap_client_req(&client, 0, &dst_address, &short_request, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}

ZTEST(coap_client, test_request_block)
{
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_block;

	zassert_equal(coap_client_req(&client, 0, &dst_address, &short_request, NULL), -EAGAIN, "");
}

ZTEST(coap_client, test_resend_request)
{
	int (*sendto_fakes[])(int, void *, size_t, int, const struct sockaddr *, socklen_t) = {
		z_impl_zsock_sendto_custom_fake_no_reply,
		z_impl_zsock_sendto_custom_fake_block,
		z_impl_zsock_sendto_custom_fake,
	};

	SET_CUSTOM_FAKE_SEQ(z_impl_zsock_sendto, sendto_fakes, ARRAY_SIZE(sendto_fakes));
	set_socket_events(client.fd, ZSOCK_POLLOUT);

	zassert_ok(coap_client_req(&client, 0, &dst_address, &short_request, NULL));
	k_sleep(K_MSEC(MORE_THAN_ACK_TIMEOUT_MS));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
	zassert_equal(z_impl_zsock_sendto_fake.call_count, 3);
}

ZTEST(coap_client, test_echo_option)
{
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_echo;

	zassert_ok(coap_client_req(&client, 0, &dst_address, &short_request, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}

ZTEST(coap_client, test_echo_option_next_req)
{
	struct coap_client_request req = short_request;

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_echo_next_req;

	zassert_ok(coap_client_req(&client, 0, &dst_address, &short_request, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");

	char *payload = "echo testing";

	req.method = COAP_METHOD_POST;
	req.payload = payload;
	req.len = strlen(payload);

	LOG_INF("Send next request");
	zassert_ok(coap_client_req(&client, 0, &dst_address, &req, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}

ZTEST(coap_client, test_get_no_path)
{
	struct coap_client_request req = short_request;

	req.path = NULL;
	zassert_equal(coap_client_req(&client, 0, &dst_address, &req, NULL), -EINVAL, "");
}

ZTEST(coap_client, test_send_large_data)
{
	zassert_ok(coap_client_req(&client, 0, &dst_address, &long_request, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}

ZTEST(coap_client, test_no_response)
{
	struct coap_transmission_parameters params = {
		.ack_timeout = LONG_ACK_TIMEOUT_MS,
		.coap_backoff_percent = 200,
		.max_retransmission = 0
	};

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;
	set_socket_events(client.fd, ZSOCK_POLLOUT);

	zassert_ok(coap_client_req(&client, 0, &dst_address, &short_request, &params));

	k_sleep(K_MSEC(MORE_THAN_LONG_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, -ETIMEDOUT, "Unexpected response");
}

ZTEST(coap_client, test_separate_response)
{
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_empty_ack;

	zassert_ok(coap_client_req(&client, 0, &dst_address, &short_request, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}

ZTEST(coap_client, test_separate_response_lost)
{
	struct coap_client_request req = short_request;

	req.user_data = &sem1;

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_only_ack;
	set_socket_events(client.fd, ZSOCK_POLLOUT);

	zassert_ok(coap_client_req(&client, 0, &dst_address, &req, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(COAP_SEPARATE_TIMEOUT)));
	zassert_equal(last_response_code, -ETIMEDOUT, "");
}

ZTEST(coap_client, test_separate_response_ack_fail)
{
	struct coap_client_request req = short_request;

	req.user_data = &sem1;

	int (*sendto_fakes[])(int, void *, size_t, int, const struct sockaddr *, socklen_t) = {
		z_impl_zsock_sendto_custom_fake,
		z_impl_zsock_sendto_custom_fake_err,
	};

	SET_CUSTOM_FAKE_SEQ(z_impl_zsock_sendto, sendto_fakes, ARRAY_SIZE(sendto_fakes));
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_empty_ack;

	zassert_ok(coap_client_req(&client, 0, &dst_address, &req, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(COAP_SEPARATE_TIMEOUT)));
	zassert_equal(last_response_code, -ENETDOWN, "");
}

ZTEST(coap_client, test_multiple_requests)
{
	struct coap_client_request req1 = short_request;
	struct coap_client_request req2 = short_request;

	req1.user_data = &sem1;
	req2.user_data = &sem2;

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;

	zassert_ok(coap_client_req(&client, 0, &dst_address, &req1, NULL));
	zassert_ok(coap_client_req(&client, 0, &dst_address, &req2, NULL));

	set_socket_events(client.fd, ZSOCK_POLLIN);
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");

	last_response_code = 0;
	set_socket_events(client.fd, ZSOCK_POLLIN);
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}

ZTEST(coap_client, test_unmatching_tokens)
{
	struct coap_transmission_parameters params = {
		.ack_timeout = LONG_ACK_TIMEOUT_MS,
		.coap_backoff_percent = 200,
		.max_retransmission = 0
	};

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_unmatching;
	set_socket_events(client.fd, ZSOCK_POLLIN | ZSOCK_POLLOUT);

	zassert_ok(coap_client_req(&client, 0, &dst_address, &short_request, &params));

	k_sleep(K_MSEC(MORE_THAN_LONG_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, -ETIMEDOUT, "Unexpected response");
}

ZTEST(coap_client, test_multiple_clients)
{
	struct coap_client_request req1 = short_request;
	struct coap_client_request req2 = long_request;

	req1.user_data = &sem1;
	req2.user_data = &sem2;

	zassert_ok(coap_client_req(&client, client.fd, &dst_address, &req1, NULL));
	zassert_ok(coap_client_req(&client2, client2.fd, &dst_address, &req2, NULL));

	/* ensure we got both responses */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");
}


ZTEST(coap_client, test_poll_err)
{
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;
	set_socket_events(client.fd, ZSOCK_POLLERR);

	zassert_ok(coap_client_req(&client, 0, &dst_address, &short_request, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, -EIO, "Unexpected response");
}

ZTEST(coap_client, test_poll_err_after_response)
{
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;
	set_socket_events(client.fd, ZSOCK_POLLIN);

	zassert_ok(coap_client_req(&client, 0, &dst_address, &short_request, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");

	set_socket_events(client.fd, ZSOCK_POLLERR);
	zassert_not_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
}

ZTEST(coap_client, test_poll_err_on_another_sock)
{
	struct coap_client_request req1 = short_request;
	struct coap_client_request req2 = short_request;

	req1.user_data = &sem1;
	req2.user_data = &sem2;

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;
	set_socket_events(client.fd, ZSOCK_POLLERR);

	zassert_ok(coap_client_req(&client2, client2.fd, &dst_address, &req2, NULL));
	zassert_ok(coap_client_req(&client, client.fd, &dst_address, &req1, NULL));

	set_socket_events(client2.fd, ZSOCK_POLLIN);

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -EIO, "");
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "");
}

ZTEST(coap_client, test_duplicate_response)
{
	z_impl_zsock_recvfrom_fake.custom_fake =
		z_impl_zsock_recvfrom_custom_fake_duplicate_response;

	zassert_ok(coap_client_req(&client, 0, &dst_address, &short_request, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "Unexpected response");

	zassert_equal(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)), -EAGAIN, "");
}

ZTEST(coap_client, test_observe)
{
	struct coap_client_option options = {
		.code = COAP_OPTION_OBSERVE,
		.value[0] = 0,
		.len = 1,
	};
	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
		.options = &options,
		.num_options = 1,
		.user_data = &sem1,
	};

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_observe;


	zassert_ok(coap_client_req(&client, 0, &dst_address, &req, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));

	coap_client_cancel_requests(&client);
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -ECANCELED, "");

	zassert_not_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
}

ZTEST(coap_client, test_request_rst)
{
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_rst;

	zassert_ok(coap_client_req(&client, 0, &dst_address, &short_request, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -ECONNRESET, "");
}

ZTEST(coap_client, test_cancel)
{
	struct coap_client_request req1 = short_request;
	struct coap_client_request req2 = short_request;

	req1.user_data = &sem1;
	req2.user_data = &sem2;

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;

	zassert_ok(coap_client_req(&client, 0, &dst_address, &req1, NULL));
	zassert_ok(coap_client_req(&client, 0, &dst_address, &req2, NULL));

	k_sleep(K_SECONDS(1));

	coap_client_cancel_request(&client, &req1);
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_not_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -ECANCELED, "");

	set_socket_events(client.fd, ZSOCK_POLLIN); /* First response is the cancelled one */
	zassert_not_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	set_socket_events(client.fd, ZSOCK_POLLIN);
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_OK, "");
}

ZTEST(coap_client, test_cancel_match)
{
	struct coap_client_request req1 = short_request;
	struct coap_client_request req2 = short_request;

	req1.user_data = &sem1;
	req2.user_data = &sem2;
	req2.path = "another";

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;

	zassert_ok(coap_client_req(&client, 0, &dst_address, &req1, NULL));
	zassert_ok(coap_client_req(&client, 0, &dst_address, &req2, NULL));

	k_sleep(K_SECONDS(1));

	/* match only one */
	coap_client_cancel_request(&client, &(struct coap_client_request) {
		.path = test_path
	});
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_not_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -ECANCELED, "");

	zassert_ok(coap_client_req(&client, 0, &dst_address, &req1, NULL));

	/* should not match */
	coap_client_cancel_request(&client, &(struct coap_client_request) {
		.path = test_path,
		.user_data = &sem2,
	});
	zassert_not_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_not_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));

	/* match both (all GET queries) */
	coap_client_cancel_request(&client, &(struct coap_client_request) {
		.method = COAP_METHOD_GET,
	});
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));

	zassert_ok(coap_client_req(&client, 0, &dst_address, &req1, NULL));
	zassert_ok(coap_client_req(&client, 0, &dst_address, &req2, NULL));

	/* match both (wildcard)*/
	coap_client_cancel_request(&client, &(struct coap_client_request) {0});
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));

}

ZTEST(coap_client, test_non_confirmable)
{
	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = false,
		.path = test_path,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
		.user_data = &sem1
	};

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;
	set_socket_events(client.fd, ZSOCK_POLLOUT);

	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		zassert_ok(coap_client_req(&client, 0, &dst_address, &req, NULL));
	}
	zassert_equal(coap_client_req(&client, 0, &dst_address, &req, NULL), -EAGAIN, "");

	k_sleep(K_MSEC(MORE_THAN_LONG_EXCHANGE_LIFETIME_MS));

	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		zassert_ok(coap_client_req(&client, 0, &dst_address, &req, NULL));
	}
	zassert_equal(coap_client_req(&client, 0, &dst_address, &req, NULL), -EAGAIN, "");

	/* No callbacks from non-confirmable */
	zassert_not_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
}
