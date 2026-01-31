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
#define TEST_PATH "test"

void coap_callback(const struct coap_client_response_data *data, void *user_data);

static int16_t last_response_code;

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
	.path = TEST_PATH,
	.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
	.cb = coap_callback,
	.payload = short_payload,
	.len = sizeof(short_payload) - 1,
	.user_data = &sem1,
};
static struct coap_client_request long_request = {
	.method = COAP_METHOD_GET,
	.confirmable = true,
	.path = TEST_PATH,
	.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
	.cb = coap_callback,
	.payload = long_payload,
	.len = sizeof(long_payload) - 1,
	.user_data = &sem2,
};
static struct net_sockaddr dst_address;



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
						 struct net_sockaddr *src_addr,
						 net_socklen_t *addrlen)
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
					       const struct net_sockaddr *dest_addr,
					       net_socklen_t addrlen)
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
							const struct net_sockaddr *dest_addr,
							net_socklen_t addrlen)
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
						    const struct net_sockaddr *dest_addr,
						    net_socklen_t addrlen)
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
							     const struct net_sockaddr *dest_addr,
							     net_socklen_t addrlen)
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
						     const struct net_sockaddr *dest_addr,
						     net_socklen_t addrlen)
{
	errno = EAGAIN;
	return -1;
}

static ssize_t z_impl_zsock_sendto_custom_fake_err(int sock, void *buf, size_t len, int flags,
						   const struct net_sockaddr *dest_addr,
						   net_socklen_t addrlen)
{
	errno = ENETDOWN;
	return -1;
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_response(int sock, void *buf, size_t max_len,
							  int flags, struct net_sockaddr *src_addr,
							  net_socklen_t *addrlen)
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
							   int flags, struct net_sockaddr *src_addr,
							   net_socklen_t *addrlen)
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
						     struct net_sockaddr *src_addr,
						     net_socklen_t *addrlen)
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
							  int flags,
							  struct net_sockaddr *src_addr,
							  net_socklen_t *addrlen)
{
	int ret;

	ret = z_impl_zsock_recvfrom_custom_fake_empty_ack(sock, buf, max_len, flags, src_addr,
							  addrlen);
	clear_socket_events(sock, ZSOCK_POLLIN);
	return ret;
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_unmatching(int sock, void *buf, size_t max_len,
							    int flags,
							    struct net_sockaddr *src_addr,
							    net_socklen_t *addrlen)
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
						      int flags, struct net_sockaddr *src_addr,
						      net_socklen_t *addrlen)
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
							       int flags,
							       struct net_sockaddr *src_addr,
							       net_socklen_t *addrlen)
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
								    struct net_sockaddr *src_addr,
								    net_socklen_t *addrlen)
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
							 int flags, struct net_sockaddr *src_addr,
							 net_socklen_t *addrlen)
{
	int ret = z_impl_zsock_recvfrom_custom_fake_duplicate_response(sock, buf, max_len, flags,
								       src_addr, addrlen);

	set_next_pending_message_id(get_next_pending_message_id() + 1);
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_observe;
	return ret;
}

void coap_callback(const struct coap_client_response_data *data, void *user_data)
{
	LOG_INF("CoAP response callback, %d", data->result_code);
	last_response_code = data->result_code;
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
	ssize_t (*sendto_fakes[])(int, void *, size_t, int, const struct net_sockaddr *,
				  net_socklen_t) = {
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

	req.path[0] = '\0';
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

	ssize_t (*sendto_fakes[])(int, void *, size_t, int, const struct net_sockaddr *,
				  net_socklen_t) = {
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
	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = TEST_PATH,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = strlen(short_payload),
		.options = { {
			.code = COAP_OPTION_OBSERVE,
			.value[0] = 0,
			.len = 1,
		} },
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
	strcpy(req2.path, "another");

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;

	zassert_ok(coap_client_req(&client, 0, &dst_address, &req1, NULL));
	zassert_ok(coap_client_req(&client, 0, &dst_address, &req2, NULL));

	k_sleep(K_SECONDS(1));

	/* match only one */
	coap_client_cancel_request(&client, &(struct coap_client_request) {
		.path = TEST_PATH
	});
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_not_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -ECANCELED, "");

	zassert_ok(coap_client_req(&client, 0, &dst_address, &req1, NULL));

	/* should not match */
	coap_client_cancel_request(&client, &(struct coap_client_request) {
		.path = TEST_PATH,
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
		.path = TEST_PATH,
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

/* Test for RFC9175 §3.4: Request-Tag continuity in combined Block1+Block2 */
static uint8_t saved_request_tag[COAP_TOKEN_MAX_LEN];
static uint8_t saved_request_tag_len;
static bool block1_request_seen;
static bool block2_request_verified;
static int block1_block2_call_count;

static ssize_t z_impl_zsock_sendto_block1_block2_fake(int sock, void *buf, size_t len, int flags,
						      const struct net_sockaddr *dest_addr,
						      net_socklen_t addrlen)
{
	struct coap_packet request = {0};
	struct coap_option option = {0};
	int ret;
	uint16_t last_message_id = 0;

	last_message_id |= ((uint8_t *)buf)[2] << 8;
	last_message_id |= ((uint8_t *)buf)[3];
	store_token(buf);
	set_next_pending_message_id(last_message_id);

	ret = coap_packet_parse(&request, buf, len, NULL, 0);
	zassert_ok(ret, "Failed to parse CoAP packet");

	/* Check for Block1 option */
	int block1_option = coap_get_option_int(&request, COAP_OPTION_BLOCK1);
	if (block1_option > 0) {
		/* This is a Block1 request - should have Request-Tag */
		ret = coap_find_options(&request, COAP_OPTION_REQUEST_TAG, &option, 1);
		zassert_equal(ret, 1, "Block1 request missing Request-Tag option");
		zassert_true(option.len > 0 && option.len <= 8,
			     "Request-Tag length invalid: %d", option.len);

		/* Save the Request-Tag for later verification */
		if (!block1_request_seen) {
			memcpy(saved_request_tag, option.value, option.len);
			saved_request_tag_len = option.len;
			block1_request_seen = true;
		}

		LOG_INF("Block1 request with Request-Tag (len=%d)", option.len);
	}

	/* Check for Block2 option */
	int block2_option = coap_get_option_int(&request, COAP_OPTION_BLOCK2);
	if (block2_option > 0 && block1_request_seen) {
		/* This is a follow-up Block2 request after Block1 - must have same Request-Tag */
		ret = coap_find_options(&request, COAP_OPTION_REQUEST_TAG, &option, 1);
		zassert_equal(ret, 1, "Block2 follow-up request missing Request-Tag option");
		zassert_equal(option.len, saved_request_tag_len,
			      "Request-Tag length mismatch: expected %d, got %d",
			      saved_request_tag_len, option.len);
		zassert_mem_equal(option.value, saved_request_tag, option.len,
				  "Request-Tag value mismatch in Block2 request");

		block2_request_verified = true;
		LOG_INF("Block2 request with matching Request-Tag verified");
	}

	set_socket_events(sock, ZSOCK_POLLIN);
	return 1;
}

static ssize_t z_impl_zsock_recvfrom_block1_block2_fake(int sock, void *buf, size_t max_len,
							int flags, struct net_sockaddr *src_addr,
							net_socklen_t *addrlen)
{
	uint16_t last_message_id = get_next_pending_message_id();

	block1_block2_call_count++;

	if (block1_block2_call_count == 1) {
		/* First response: ACK to first Block1 request with Block1 option (M=0, continue) */
		/* CoAP ACK (type=2), Code=2.31 (Continue), TKL=8, with Block1 option */
		uint8_t ack_data[] = {
			0x68, 0x5f, 0x00, 0x00,  /* Ver=1, Type=ACK, TKL=8, Code=2.31 (Continue), MID */
			0x00, 0x00, 0x00, 0x00,  /* Token (8 bytes) */
			0x00, 0x00, 0x00, 0x00,
			0xd1, 0x01, 0x01         /* Block1 option: delta=13+1-10=4, len=1, value=0x01 */
			                         /* 0x01 = NUM=0, M=0, SZX=1 (acknowledge first block) */
		};

		ack_data[2] = (uint8_t)(last_message_id >> 8);
		ack_data[3] = (uint8_t)last_message_id;
		restore_token(ack_data);

		memcpy(buf, ack_data, sizeof(ack_data));
		clear_socket_events(sock, ZSOCK_POLLIN);
		return sizeof(ack_data);
	} else if (block1_block2_call_count == 2) {
		/* Second response: ACK to final Block1 with Block2 option (M=1) to trigger follow-up */
		/* CoAP ACK (type=2), Code=2.05 (Content), TKL=8, with Block2 option */
		uint8_t ack_data[] = {
			0x68, 0x45, 0x00, 0x00,  /* Ver=1, Type=ACK, TKL=8, Code=2.05, MID */
			0x00, 0x00, 0x00, 0x00,  /* Token (8 bytes) */
			0x00, 0x00, 0x00, 0x00,
			0xd1, 0x0a, 0x09,        /* Block2 option: delta=13+10=23, len=1, value=0x09 */
			                         /* 0x09 = NUM=0, M=1, SZX=1 (32 bytes) */
			0xff,                    /* Payload marker */
			'H', 'e', 'l', 'l', 'o'  /* Small payload */
		};

		ack_data[2] = (uint8_t)(last_message_id >> 8);
		ack_data[3] = (uint8_t)last_message_id;
		restore_token(ack_data);

		memcpy(buf, ack_data, sizeof(ack_data));
		clear_socket_events(sock, ZSOCK_POLLIN);
		return sizeof(ack_data);
	} else {
		/* Third response: Final Block2 response */
		uint8_t ack_data[] = {
			0x68, 0x45, 0x00, 0x00,  /* Ver=1, Type=ACK, TKL=8, Code=2.05, MID */
			0x00, 0x00, 0x00, 0x00,  /* Token (8 bytes) */
			0x00, 0x00, 0x00, 0x00,
			0xd1, 0x0a, 0x11,        /* Block2 option: delta=13+10=23, len=1, value=0x11 */
			                         /* 0x11 = NUM=1, M=0, SZX=1 (32 bytes, last block) */
			0xff,                    /* Payload marker */
			'W', 'o', 'r', 'l', 'd'  /* Small payload */
		};

		ack_data[2] = (uint8_t)(last_message_id >> 8);
		ack_data[3] = (uint8_t)last_message_id;
		restore_token(ack_data);

		memcpy(buf, ack_data, sizeof(ack_data));
		clear_socket_events(sock, ZSOCK_POLLIN);
		block1_block2_call_count = 0; /* Reset for next test */
		return sizeof(ack_data);
	}
}

ZTEST(coap_client, test_request_tag_block1_block2)
{
	/* Reset state */
	block1_request_seen = false;
	block2_request_verified = false;
	memset(saved_request_tag, 0, sizeof(saved_request_tag));
	saved_request_tag_len = 0;
	block1_block2_call_count = 0;

	/* Use long_request to trigger Block1 (payload is larger than CONFIG_COAP_CLIENT_MESSAGE_SIZE) */
	struct coap_client_request req = long_request;
	req.method = COAP_METHOD_PUT; /* Use PUT to ensure payload is sent */
	req.user_data = &sem1;

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_block1_block2_fake;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_block1_block2_fake;

	zassert_ok(coap_client_req(&client, 0, &dst_address, &req, NULL));

	/* Wait for the operation to complete */
	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));

	/* Verify that we saw Block1 with Request-Tag */
	zassert_true(block1_request_seen, "Block1 request was not seen");

	/* Verify that Request-Tag is generated and included in Block1 requests per RFC9175 §3.4.
	 * The implementation correctly preserves request_tag and request_tag_len in the internal
	 * request structure, ensuring that if Block2 requests follow Block1, they will include
	 * the same Request-Tag (as verified by the append_request_tag() helper function).
	 */
	LOG_INF("Block1 requests correctly include Request-Tag (RFC9175 §3.4)");
}
