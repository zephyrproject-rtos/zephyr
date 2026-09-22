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
static uint8_t saved_observe_token[COAP_TOKEN_MAX_LEN];
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

/* Dummy destination addresses */
static struct net_sockaddr_storage dst_address = {
	.ss_family = NET_AF_INET,
};
static struct net_sockaddr_in mcast_address = {
	.sin_family = NET_AF_INET,
	.sin_addr = {{{224, 0, 1, 187}}},
};

static const struct net_sockaddr_in recv_src_address = {
	.sin_family = NET_AF_INET,
	.sin_port = 0x1600,
	.sin_addr = {{{192, 0, 2, 1}}},
};

static void fill_recv_src_addr(struct net_sockaddr *src_addr, net_socklen_t *addrlen)
{
	zassert_not_null(src_addr, "Unexpected NULL source address");
	zassert_not_null(addrlen, "Unexpected NULL source address length");
	zassert_true(*addrlen >= sizeof(recv_src_address), "Source address buffer too small");

	memcpy(src_addr, &recv_src_address, sizeof(recv_src_address));
	*addrlen = sizeof(recv_src_address);
}

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
	uint8_t ack_data[] = {0x68, 0x45, 0x00, 0x00, 0x00, 0x00,
			      0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	last_message_id = get_next_pending_message_id();

	ack_data[2] = (uint8_t)(last_message_id >> 8);
	ack_data[3] = (uint8_t)last_message_id;
	restore_token(ack_data);

	memcpy(buf, ack_data, sizeof(ack_data));

	fill_recv_src_addr(src_addr, addrlen);

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

static ssize_t z_impl_zsock_sendto_custom_fake_connected(int sock, void *buf, size_t len,
							 int flags,
							 const struct net_sockaddr *dest_addr,
							 net_socklen_t addrlen)
{
	/* The request was issued without a destination address (connected
	 * transport). Every send, including ACK/RST replies, must reuse the
	 * connected send path rather than a source address that recvfrom() did
	 * not populate.
	 */
	zassert_equal(addrlen, 0, "Connected socket send must not carry an address");
	zassert_is_null(dest_addr, "Connected socket send must not carry an address");

	return z_impl_zsock_sendto_custom_fake(sock, buf, len, flags, dest_addr, addrlen);
}

static ssize_t z_impl_zsock_sendto_custom_fake_check_reply_addr(
	int sock, void *buf, size_t len, int flags,
	const struct net_sockaddr *dest_addr, net_socklen_t addrlen)
{
	uint8_t type = (((uint8_t *)buf)[0] & 0x30) >> 4;

	/* ACK/RST replies must be addressed to the source of the received
	 * packet when the transport reported one.
	 */
	if (type == COAP_TYPE_ACK || type == COAP_TYPE_RESET) {
		zassert_equal(addrlen, sizeof(recv_src_address),
			      "Reply not addressed to received source");
		zassert_mem_equal(dest_addr, &recv_src_address, sizeof(recv_src_address),
				  "Reply not addressed to received source");
	}

	return z_impl_zsock_sendto_custom_fake(sock, buf, len, flags, dest_addr, addrlen);
}

static ssize_t z_impl_zsock_sendto_custom_fake_check_req_addr(
	int sock, void *buf, size_t len, int flags,
	const struct net_sockaddr *dest_addr, net_socklen_t addrlen)
{
	uint8_t type = (((uint8_t *)buf)[0] & 0x30) >> 4;

	/* recvfrom() reported no source, so an ACK/RST reply must fall back to
	 * the address the request was sent to, not a zeroed one.
	 */
	if (type == COAP_TYPE_ACK || type == COAP_TYPE_RESET) {
		zassert_equal(addrlen, sizeof(struct net_sockaddr_in),
			      "Reply not addressed to request address");
		zassert_mem_equal(dest_addr, &dst_address, sizeof(struct net_sockaddr_in),
				  "Reply not addressed to request address");
	}

	return z_impl_zsock_sendto_custom_fake(sock, buf, len, flags, dest_addr, addrlen);
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_response(int sock, void *buf, size_t max_len,
							  int flags, struct net_sockaddr *src_addr,
							  net_socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	static uint8_t ack_data[] = {0x48, 0x45, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	last_message_id = get_next_pending_message_id();

	ack_data[2] = (uint8_t)(last_message_id >> 8);
	ack_data[3] = (uint8_t)last_message_id;
	restore_token(ack_data);

	memcpy(buf, ack_data, sizeof(ack_data));

	fill_recv_src_addr(src_addr, addrlen);

	clear_socket_events(sock, ZSOCK_POLLIN);

	return sizeof(ack_data);
}

/* CON response like z_impl_zsock_recvfrom_custom_fake_response, but modelling a
 * connected transport that does not report a per-packet source address.
 * src_addr/addrlen are deliberately left untouched.
 */
static ssize_t z_impl_zsock_recvfrom_custom_fake_connected(int sock, void *buf, size_t max_len,
							   int flags, struct net_sockaddr *src_addr,
							   net_socklen_t *addrlen)
{
	uint16_t last_message_id = 0;

	static uint8_t ack_data[] = {0x48, 0x45, 0x00, 0x00, 0x00, 0x00,
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

	fill_recv_src_addr(src_addr, addrlen);

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

	fill_recv_src_addr(src_addr, addrlen);

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

	static uint8_t ack_data[] = {0x68, 0x45, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

	last_message_id = get_next_pending_message_id();

	ack_data[2] = (uint8_t)(last_message_id >> 8);
	ack_data[3] = (uint8_t)last_message_id;

	memcpy(buf, ack_data, sizeof(ack_data));

	fill_recv_src_addr(src_addr, addrlen);

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

	fill_recv_src_addr(src_addr, addrlen);

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
	uint8_t ack_data[] = {0x68, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			      0x00, 0x00, 0x00, 0x00, 0xda, 0xef, 'e',  'c',
			      'h',  'o',  '_',  'v',  'a',  'l',  'u',  'e'};

	last_message_id = get_next_pending_message_id();

	ack_data[2] = (uint8_t)(last_message_id >> 8);
	ack_data[3] = (uint8_t)last_message_id;
	restore_token(ack_data);

	memcpy(buf, ack_data, sizeof(ack_data));

	fill_recv_src_addr(src_addr, addrlen);

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

/* Token of the observe registration, captured from the first block-0 notification and reused
 * when the server pushes the next notification.
 */
static uint8_t observe_reg_token[COAP_TOKEN_MAX_LEN];
static int observe_block_step;

/* Shared scratch for the interleaved-notification and mid-transfer deregister tests.
 * ox_reg_token is the observe registration token; ox_last_token/ox_last_id track the most
 * recently sent GET so the next block can echo it.
 */
static uint8_t ox_reg_token[COAP_TOKEN_MAX_LEN];
static uint8_t ox_last_token[COAP_TOKEN_MAX_LEN];
static uint16_t ox_last_id;
static int ox_step;
static int ox_dereg_count;
static int ox_interleave_cb_idx;
static int ox_blockwise_cb_count;
static struct k_work ox_dereg_work;

static void ox_dereg_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	(void)coap_client_deregister_observe(&client, &(struct coap_client_request){0});
}

static size_t build_block2_response(uint8_t *buf, size_t buf_len, uint8_t type,
				    const uint8_t *token, uint16_t id, int observe_seq,
				    int block_num, bool more)
{
	static uint8_t block_payload[CONFIG_COAP_CLIENT_BLOCK_SIZE];
	struct coap_packet pkt;
	uint16_t plen = more ? CONFIG_COAP_CLIENT_BLOCK_SIZE : 100;
	int block2 = (block_num << 4) | (more ? 0x08 : 0) | COAP_BLOCK_256;

	zassert_ok(coap_packet_init(&pkt, buf, buf_len, 1, type, COAP_TOKEN_MAX_LEN, token,
				    COAP_RESPONSE_CODE_CONTENT, id));
	if (observe_seq >= 0) {
		zassert_ok(coap_append_option_int(&pkt, COAP_OPTION_OBSERVE, observe_seq));
	}
	zassert_ok(coap_append_option_int(&pkt, COAP_OPTION_BLOCK2, block2));
	zassert_ok(coap_packet_append_payload_marker(&pkt));
	zassert_true(coap_packet_append_payload(&pkt, block_payload, plen) >= 0);

	return pkt.offset;
}

/* Minimal piggybacked 2.05 ACK (no payload), used for CON deregister responses. */
static size_t build_content_ack(uint8_t *buf, size_t buf_len, const uint8_t *token, uint16_t id)
{
	struct coap_packet pkt;

	zassert_ok(coap_packet_init(&pkt, buf, buf_len, 1, COAP_TYPE_ACK, COAP_TOKEN_MAX_LEN, token,
				    COAP_RESPONSE_CODE_CONTENT, id));

	return pkt.offset;
}

/* Deliver a resource that is transferred blockwise (two blocks) both for the initial
 * notification and for a subsequent server-pushed notification.
 */
static ssize_t z_impl_zsock_recvfrom_custom_fake_observe_block(int sock, void *buf, size_t max_len,
							       int flags,
							       struct net_sockaddr *src_addr,
							       net_socklen_t *addrlen)
{
	uint8_t tokbuf[TOKEN_OFFSET + COAP_TOKEN_MAX_LEN] = {0};
	size_t resp_len;
	uint16_t id;

	switch (observe_block_step) {
	case 0: /* notification #1, block 0: carries Observe and the registration token */
		id = get_next_pending_message_id();
		restore_token(tokbuf);
		memcpy(observe_reg_token, tokbuf + TOKEN_OFFSET, COAP_TOKEN_MAX_LEN);
		resp_len = build_block2_response(buf, max_len, COAP_TYPE_ACK, observe_reg_token,
						 id, 1, 0, true);
		clear_socket_events(sock, ZSOCK_POLLIN);
		break;
	case 1: /* notification #1, block 1: answers the continuation GET (token T1) */
		id = get_next_pending_message_id();
		restore_token(tokbuf);
		resp_len = build_block2_response(buf, max_len, COAP_TYPE_ACK,
						 tokbuf + TOKEN_OFFSET, id, -1, 1, false);
		set_socket_events(sock, ZSOCK_POLLIN); /* push the next notification */
		break;
	case 2: /* notification #2, block 0: server push reusing the registration token */
		resp_len = build_block2_response(buf, max_len, COAP_TYPE_NON_CON,
						 observe_reg_token, 0xA000, 2, 0, true);
		clear_socket_events(sock, ZSOCK_POLLIN);
		break;
	case 3: /* notification #2, block 1: answers the continuation GET (token T2) */
		id = get_next_pending_message_id();
		restore_token(tokbuf);
		resp_len = build_block2_response(buf, max_len, COAP_TYPE_ACK,
						 tokbuf + TOKEN_OFFSET, id, -1, 1, false);
		clear_socket_events(sock, ZSOCK_POLLIN);
		break;
	default:
		clear_socket_events(sock, ZSOCK_POLLIN);
		return 0;
	}

	observe_block_step++;
	return resp_len;
}

/* Same as observe_block, but only the first two blocks (one notification). */
static ssize_t z_impl_zsock_recvfrom_custom_fake_observe_block_one(
	int sock, void *buf, size_t max_len, int flags,
	struct net_sockaddr *src_addr, net_socklen_t *addrlen)
{
	ssize_t ret;

	if (observe_block_step >= 2) {
		clear_socket_events(sock, ZSOCK_POLLIN);
		return 0;
	}

	ret = z_impl_zsock_recvfrom_custom_fake_observe_block(sock, buf, max_len, flags,
							       src_addr, addrlen);
	if (observe_block_step >= 2) {
		clear_socket_events(sock, ZSOCK_POLLIN);
	}

	return ret;
}

static ssize_t z_impl_zsock_sendto_custom_fake_observe_block(int sock, void *buf, size_t len,
							     int flags,
							     const struct net_sockaddr *dest_addr,
							     net_socklen_t addrlen)
{
	struct coap_packet req = {0};
	struct coap_option opt = {0};
	uint16_t id = (((uint8_t *)buf)[2] << 8) | ((uint8_t *)buf)[3];
	uint8_t type = (((uint8_t *)buf)[0] & 0x30) >> 4;

	store_token(buf);
	set_next_pending_message_id(id);

	zassert_ok(coap_packet_parse(&req, buf, len, NULL, 0));

	if (coap_get_option_int(&req, COAP_OPTION_OBSERVE) == 1) {
		uint8_t token[COAP_TOKEN_MAX_LEN];

		coap_header_get_token(&req, token);
		zassert_mem_equal(token, observe_reg_token, COAP_TOKEN_MAX_LEN,
				  "Deregister must use the observe registration token");
		ox_dereg_count++;
		return 1;
	}

	/* RFC 7959 Section 3.4: Block2 continuation GETs must not carry the Observe option. */
	if (coap_get_option_int(&req, COAP_OPTION_BLOCK2) > 0) {
		zassert_equal(coap_find_options(&req, COAP_OPTION_OBSERVE, &opt, 1), 0,
			      "Observe must be absent on block retrievals");
	}

	if (type == 0) {
		set_socket_events(sock, ZSOCK_POLLIN);
	}

	return 1;
}

/* Serve notification #1 block 0, then push notification #2 (block 0) WHILE the block-1
 * retrieval of notification #1 is still outstanding. The pushed notification reuses the
 * registration token, which must match via observe_token while request_token holds the
 * block-retrieval token.
 */
static ssize_t z_impl_zsock_recvfrom_custom_fake_observe_interleave(
	int sock, void *buf, size_t max_len, int flags,
	struct net_sockaddr *src_addr, net_socklen_t *addrlen)
{
	size_t resp_len;

	switch (ox_step) {
	case 0: /* notification #1, block 0: Observe + registration token, more to come */
		memcpy(ox_reg_token, ox_last_token, COAP_TOKEN_MAX_LEN);
		resp_len = build_block2_response(buf, max_len, COAP_TYPE_ACK, ox_reg_token,
						 ox_last_id, 1, 0, true);
		clear_socket_events(sock, ZSOCK_POLLIN);
		break;
	case 1: /* notification #2, block 0: async push reusing the registration token,
		 * arriving before the block-1 continuation GET is answered
		 */
		resp_len = build_block2_response(buf, max_len, COAP_TYPE_NON_CON, ox_reg_token,
						 0xB000, 2, 0, true);
		clear_socket_events(sock, ZSOCK_POLLIN);
		break;
	case 2: /* notification #2, block 1: answer the latest continuation GET, last block */
		resp_len = build_block2_response(buf, max_len, COAP_TYPE_ACK, ox_last_token,
						 ox_last_id, -1, 1, false);
		clear_socket_events(sock, ZSOCK_POLLIN);
		break;
	default:
		clear_socket_events(sock, ZSOCK_POLLIN);
		return 0;
	}

	ox_step++;
	return resp_len;
}

static ssize_t z_impl_zsock_sendto_custom_fake_observe_interleave(
	int sock, void *buf, size_t len, int flags,
	const struct net_sockaddr *dest_addr, net_socklen_t addrlen)
{
	struct coap_packet req = {0};
	struct coap_option opt = {0};
	uint8_t type = (((uint8_t *)buf)[0] & 0x30) >> 4;

	zassert_ok(coap_packet_parse(&req, buf, len, NULL, 0));

	/* The async notification must stay matchable: the client must never RST it. */
	zassert_not_equal(type, COAP_TYPE_RESET,
			  "Registration token unmatchable: client sent an RST");

	if (coap_get_option_int(&req, COAP_OPTION_BLOCK2) > 0) {
		zassert_equal(coap_find_options(&req, COAP_OPTION_OBSERVE, &opt, 1), 0,
			      "Observe must be absent on block retrievals");
	}

	ox_last_id = (((uint8_t *)buf)[2] << 8) | ((uint8_t *)buf)[3];
	memcpy(ox_last_token, (uint8_t *)buf + TOKEN_OFFSET, COAP_TOKEN_MAX_LEN);

	if (type == COAP_TYPE_CON || type == COAP_TYPE_NON_CON) {
		set_socket_events(sock, ZSOCK_POLLIN);
	}

	return 1;
}

/* Serve block 0 of a notification (more to come) so the client issues a block-1
 * continuation GET, then deliver the CON deregister 2.05. Block 1 is never sent.
 */
static ssize_t z_impl_zsock_recvfrom_custom_fake_observe_dereg(
	int sock, void *buf, size_t max_len, int flags,
	struct net_sockaddr *src_addr, net_socklen_t *addrlen)
{
	size_t resp_len;

	switch (ox_step) {
	case 0: /* notification block 0: Observe + registration token, more to come */
		memcpy(ox_reg_token, ox_last_token, COAP_TOKEN_MAX_LEN);
		resp_len = build_block2_response(buf, max_len, COAP_TYPE_ACK, ox_reg_token,
						 ox_last_id, 1, 0, true);
		clear_socket_events(sock, ZSOCK_POLLIN);
		break;
	case 1: /* CON deregister response: 2.05 with the registration token */
		resp_len = build_content_ack(buf, max_len, ox_reg_token, ox_last_id);
		clear_socket_events(sock, ZSOCK_POLLIN);
		break;
	default:
		clear_socket_events(sock, ZSOCK_POLLIN);
		return 0;
	}

	ox_step++;
	fill_recv_src_addr(src_addr, addrlen);
	return resp_len;
}

static ssize_t z_impl_zsock_sendto_custom_fake_observe_dereg(
	int sock, void *buf, size_t len, int flags,
	const struct net_sockaddr *dest_addr, net_socklen_t addrlen)
{
	struct coap_packet req = {0};
	struct coap_option opt = {0};
	uint8_t type = (((uint8_t *)buf)[0] & 0x30) >> 4;
	uint8_t tkl = ((uint8_t *)buf)[0] & 0x0f;

	zassert_ok(coap_packet_parse(&req, buf, len, NULL, 0));

	ox_last_id = (((uint8_t *)buf)[2] << 8) | ((uint8_t *)buf)[3];
	memcpy(ox_last_token, (uint8_t *)buf + TOKEN_OFFSET, COAP_TOKEN_MAX_LEN);

	if (coap_get_option_int(&req, COAP_OPTION_OBSERVE) == 1) {
		/* Deregister GET: must reuse the registration token, even though request_token
		 * currently holds the outstanding block-retrieval (continuation) token.
		 */
		zassert_equal(tkl, COAP_TOKEN_MAX_LEN, "Unexpected deregister token length");
		zassert_mem_equal((uint8_t *)buf + TOKEN_OFFSET, ox_reg_token, COAP_TOKEN_MAX_LEN,
				  "Deregister must use the observe registration token");
		ox_dereg_count++;
		if (type == COAP_TYPE_CON) {
			set_socket_events(sock, ZSOCK_POLLIN);
		}
		return 1;
	}

	if (coap_get_option_int(&req, COAP_OPTION_BLOCK2) > 0) {
		zassert_equal(coap_find_options(&req, COAP_OPTION_OBSERVE, &opt, 1), 0,
			      "Observe must be absent on block retrievals");
		/* Continuation GET is out, so request_token now holds the continuation token. */
		k_sem_give(&sem2);
		return 1;
	}

	/* Registration GET. */
	set_socket_events(sock, ZSOCK_POLLIN);
	return 1;
}

void coap_callback(const struct coap_client_response_data *data, void *user_data)
{
	LOG_INF("CoAP response callback, %d", data->result_code);
	last_response_code = data->result_code;
	if (user_data) {
		k_sem_give((struct k_sem *) user_data);
	}
}

static void coap_callback_interleave_observe(const struct coap_client_response_data *data,
					     void *user_data)
{
	static const int expected_observe_seq[] = {1, 2};
	int observe;

	coap_callback(data, user_data);

	if (data->packet == NULL || data->result_code != COAP_RESPONSE_CODE_CONTENT) {
		return;
	}

	observe = coap_get_option_int(data->packet, COAP_OPTION_OBSERVE);
	if (data->offset == 0 && !data->last_block) {
		zassert_true(observe >= 0, "Observe missing on block-0 notification");
		zassert_equal(observe, expected_observe_seq[ox_interleave_cb_idx],
			      "Unexpected Observe sequence on interleaved notification");
		ox_interleave_cb_idx++;
	} else if (data->last_block) {
		zassert_equal(observe, -ENOENT, "Observe must be absent on final block");
	}
}

static void coap_callback_deregister_on_last_block(const struct coap_client_response_data *data,
						 void *user_data)
{
	coap_callback(data, user_data);

	if (data->result_code != COAP_RESPONSE_CODE_CONTENT) {
		return;
	}

	ox_blockwise_cb_count++;
	if (ox_blockwise_cb_count == 2) {
		/* coap_client_deregister_observe() must not run synchronously from the
		 * response callback: the recv path still holds the slot in_callback.
		 * Post to the workqueue and run once the callback has returned.
		 */
		k_work_submit(&ox_dereg_work);
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
	k_work_init(&ox_dereg_work, ox_dereg_work_handler);

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

	observe_block_step = 0;
	memset(observe_reg_token, 0, sizeof(observe_reg_token));
	ox_step = 0;
	ox_dereg_count = 0;
	ox_interleave_cb_idx = 0;
	ox_blockwise_cb_count = 0;
	memset(ox_reg_token, 0, sizeof(ox_reg_token));
	memset(ox_last_token, 0, sizeof(ox_last_token));
	ox_last_id = 0;

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
	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &short_request, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");
}

ZTEST(coap_client, test_request_block)
{
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_block;

	zassert_equal(coap_client_req(&client, 0, net_sad(&dst_address), &short_request, NULL),
		      -EAGAIN, "");
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

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &short_request, NULL));
	k_sleep(K_MSEC(MORE_THAN_ACK_TIMEOUT_MS));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");
	zassert_equal(z_impl_zsock_sendto_fake.call_count, 3);
}

ZTEST(coap_client, test_echo_option)
{
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_echo;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &short_request, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");
}

ZTEST(coap_client, test_echo_option_next_req)
{
	struct coap_client_request req = short_request;

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_echo_next_req;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &short_request, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");

	char *payload = "echo testing";

	req.method = COAP_METHOD_POST;
	req.payload = payload;
	req.len = strlen(payload);

	LOG_INF("Send next request");
	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");
}

ZTEST(coap_client, test_get_no_path)
{
	struct coap_client_request req = short_request;

	req.path[0] = '\0';
	zassert_equal(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL), -EINVAL, "");
}

ZTEST(coap_client, test_send_large_data)
{
	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &long_request, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");
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

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &short_request, &params));

	k_sleep(K_MSEC(MORE_THAN_LONG_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, -ETIMEDOUT, "Unexpected response");
}

ZTEST(coap_client, test_separate_response)
{
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_empty_ack;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &short_request, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");
}

ZTEST(coap_client, test_separate_response_lost)
{
	struct coap_client_request req = short_request;

	req.user_data = &sem1;

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_only_ack;
	set_socket_events(client.fd, ZSOCK_POLLOUT);

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));

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

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(COAP_SEPARATE_TIMEOUT)));
	zassert_equal(last_response_code, -ENETDOWN, "");
}

ZTEST(coap_client, test_separate_response_connected)
{
	struct coap_client_request req = short_request;

	req.user_data = &sem1;

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_connected;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_connected;

	/* addr == NULL models a connected transport that does not report a
	 * per-packet source address on recvfrom(). The empty ACK sent for the
	 * CON response must therefore be sent on the connected socket, which
	 * z_impl_zsock_sendto_custom_fake_connected asserts.
	 */
	zassert_ok(coap_client_req(&client, 0, NULL, &req, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");
}

ZTEST(coap_client, test_separate_response_reply_to_source)
{
	struct coap_client_request req = short_request;

	req.user_data = &sem1;

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_check_reply_addr;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_response;

	/* Connectionless transport: recvfrom() reports the source, so the empty
	 * ACK for the CON response must be addressed to that source.
	 */
	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");
}

ZTEST(coap_client, test_separate_response_req_addr_no_src)
{
	struct coap_client_request req = short_request;

	req.user_data = &sem1;

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_check_req_addr;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_connected;

	/* The request provides a destination address, but recvfrom() reports no
	 * source (connected socket used with an explicit peer). The ACK must
	 * fall back to the request's address rather than a bogus one.
	 */
	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");
}

ZTEST(coap_client, test_multiple_requests)
{
	struct coap_client_request req1 = short_request;
	struct coap_client_request req2 = short_request;

	req1.user_data = &sem1;
	req2.user_data = &sem2;

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req1, NULL));
	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req2, NULL));

	set_socket_events(client.fd, ZSOCK_POLLIN);
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");

	last_response_code = 0;
	set_socket_events(client.fd, ZSOCK_POLLIN);
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");
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

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &short_request, &params));

	k_sleep(K_MSEC(MORE_THAN_LONG_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, -ETIMEDOUT, "Unexpected response");
}

ZTEST(coap_client, test_multiple_clients)
{
	struct coap_client_request req1 = short_request;
	struct coap_client_request req2 = long_request;

	req1.user_data = &sem1;
	req2.user_data = &sem2;

	zassert_ok(coap_client_req(&client, client.fd, net_sad(&dst_address), &req1, NULL));
	zassert_ok(coap_client_req(&client2, client2.fd, net_sad(&dst_address), &req2, NULL));

	/* ensure we got both responses */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");
}


ZTEST(coap_client, test_poll_err)
{
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;
	set_socket_events(client.fd, ZSOCK_POLLERR);

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &short_request, NULL));

	k_sleep(K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS));
	zassert_equal(last_response_code, -EIO, "Unexpected response");
}

ZTEST(coap_client, test_poll_err_after_response)
{
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;
	set_socket_events(client.fd, ZSOCK_POLLIN);

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &short_request, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");

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

	zassert_ok(coap_client_req(&client2, client2.fd, net_sad(&dst_address), &req2, NULL));
	zassert_ok(coap_client_req(&client, client.fd, net_sad(&dst_address), &req1, NULL));

	set_socket_events(client2.fd, ZSOCK_POLLIN);

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -EIO, "");
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "");
}

ZTEST(coap_client, test_duplicate_response)
{
	z_impl_zsock_recvfrom_fake.custom_fake =
		z_impl_zsock_recvfrom_custom_fake_duplicate_response;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &short_request, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "Unexpected response");

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
		.len = sizeof(short_payload) - 1,
		.options = { {
			.code = COAP_OPTION_OBSERVE,
			.value[0] = 0,
			.len = 1,
		} },
		.num_options = 1,
		.user_data = &sem1,
	};

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_observe;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));

	coap_client_cancel_requests(&client);
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -ECANCELED, "");

	zassert_not_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
}

/* Captures the token from the first outgoing packet (subscribe), then falls through to the
 * normal sendto fake.
 */
static ssize_t
z_impl_zsock_sendto_custom_fake_observe_subscribe(int sock, void *buf, size_t len, int flags,
						  const struct net_sockaddr *dest_addr,
						  net_socklen_t addrlen)
{
	memcpy(saved_observe_token, (uint8_t *)buf + TOKEN_OFFSET, COAP_TOKEN_MAX_LEN);
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake;
	return z_impl_zsock_sendto_custom_fake(sock, buf, len, flags, dest_addr, addrlen);
}

static void verify_deregister_packet(void *buf, size_t len, uint8_t expected_type)
{
	struct coap_packet pkt = {0};
	struct coap_option obs_opt = {0};
	uint8_t token[COAP_TOKEN_MAX_LEN];
	int ret;

	ret = coap_packet_parse(&pkt, buf, len, NULL, 0);
	zassert_ok(ret, "Failed to parse deregister packet");

	zassert_equal(coap_header_get_type(&pkt), expected_type, "Unexpected CON/NON type");

	ret = coap_find_options(&pkt, COAP_OPTION_OBSERVE, &obs_opt, 1);
	zassert_equal(ret, 1, "Observe option missing in deregister");
	zassert_equal(coap_option_value_to_int(&obs_opt), 1,
		      "Observe option must be 1 (deregister)");

	coap_header_get_token(&pkt, token);
	zassert_mem_equal(token, saved_observe_token, COAP_TOKEN_MAX_LEN,
			  "Deregister token must match original observe token");
}

static ssize_t z_impl_zsock_sendto_custom_fake_deregister_con(int sock, void *buf, size_t len,
							      int flags,
							      const struct net_sockaddr *dest_addr,
							      net_socklen_t addrlen)
{
	verify_deregister_packet(buf, len, COAP_TYPE_CON);
	return z_impl_zsock_sendto_custom_fake(sock, buf, len, flags, dest_addr, addrlen);
}

static ssize_t z_impl_zsock_sendto_custom_fake_deregister_non(int sock, void *buf, size_t len,
							      int flags,
							      const struct net_sockaddr *dest_addr,
							      net_socklen_t addrlen)
{
	verify_deregister_packet(buf, len, COAP_TYPE_NON_CON);
	return z_impl_zsock_sendto_custom_fake(sock, buf, len, flags, dest_addr, addrlen);
}

ZTEST(coap_client, test_observe_deregister_con)
{
	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = TEST_PATH,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.options = {{
			.code = COAP_OPTION_OBSERVE,
			.value[0] = 0,
			.len = 1,
		}},
		.num_options = 1,
		.user_data = &sem1,
	};

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_observe_subscribe;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));

	/* Wait for subscription confirmation */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT);

	/* Deregister: CON packet with Observe=1 and same token; server ACKs with 2.05 */
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_deregister_con;
	zassert_ok(coap_client_deregister_observe(&client, &req));

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT);

	zassert_not_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
}

ZTEST(coap_client, test_observe_deregister_non)
{
	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = false,
		.path = TEST_PATH,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.options = {{
			.code = COAP_OPTION_OBSERVE,
			.value[0] = 0,
			.len = 1,
		}},
		.num_options = 1,
		.user_data = &sem1,
	};

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_observe_subscribe;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));

	/* NON: sendto does not trigger POLLIN; manually deliver a subscription notification */
	set_socket_events(client.fd, ZSOCK_POLLIN);
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT);

	/* Deregister: NON packet with Observe=1 and same token; released immediately */
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_deregister_non;
	zassert_ok(coap_client_deregister_observe(&client, &req));

	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -ECANCELED);

	zassert_not_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
}

ZTEST(coap_client, test_observe_blockwise)
{
	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = TEST_PATH,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = sizeof(short_payload) - 1,
		.options = { {
			.code = COAP_OPTION_OBSERVE,
			.value[0] = 0,
			.len = 1,
		} },
		.num_options = 1,
		.user_data = &sem1,
	};

	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_observe_block;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_observe_block;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));

	/* Notification #1, transferred as two blocks. */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));

	/* Notification #2 must still be delivered: observe_token keeps the registration
	 * token matchable while block retrievals use fresh tokens in request_token.
	 */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));

	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "");

	coap_client_cancel_requests(&client);
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -ECANCELED, "");
}

ZTEST(coap_client, test_observe_blockwise_async_notification)
{
	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = TEST_PATH,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback_interleave_observe,
		.payload = short_payload,
		.len = sizeof(short_payload) - 1,
		.options = { {
			.code = COAP_OPTION_OBSERVE,
			.value[0] = 0,
			.len = 1,
		} },
		.num_options = 1,
		.user_data = &sem1,
	};

	z_impl_zsock_recvfrom_fake.custom_fake =
		z_impl_zsock_recvfrom_custom_fake_observe_interleave;
	z_impl_zsock_sendto_fake.custom_fake =
		z_impl_zsock_sendto_custom_fake_observe_interleave;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));

	/* Block 0 of notification #1, then notification #2 pushed mid-retrieval, then its
	 * final block. All three deliveries must reach the callback and none must be RST'd
	 * (asserted in the sendto fake).
	 */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(ox_interleave_cb_idx, 2, "Expected two block-0 Observe notifications");
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "");

	coap_client_cancel_requests(&client);
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -ECANCELED, "");
}

ZTEST(coap_client, test_observe_deregister_blockwise)
{
	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = TEST_PATH,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.payload = short_payload,
		.len = sizeof(short_payload) - 1,
		.options = { {
			.code = COAP_OPTION_OBSERVE,
			.value[0] = 0,
			.len = 1,
		} },
		.num_options = 1,
		.user_data = &sem1,
	};

	z_impl_zsock_recvfrom_fake.custom_fake =
		z_impl_zsock_recvfrom_custom_fake_observe_dereg;
	z_impl_zsock_sendto_fake.custom_fake =
		z_impl_zsock_sendto_custom_fake_observe_dereg;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));

	/* Block 0 is delivered and the client issues the block-1 continuation GET, so
	 * request_token now holds the continuation token, not the registration token.
	 */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));

	/* Deregister mid-transfer: outgoing GET must use the registration token (sendto
	 * fake) and the CON 2.05 response must match after request_token is synced.
	 */
	zassert_ok(coap_client_deregister_observe(&client, &req));
	zassert_equal(ox_dereg_count, 1, "Deregister GET was not sent");

	/* Server 2.05 for the CON deregister. */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT,
		      "Deregister CON response was not handled");

	zassert_not_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
}

ZTEST(coap_client, test_observe_deregister_blockwise_in_callback)
{
	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = TEST_PATH,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback_deregister_on_last_block,
		.payload = short_payload,
		.len = sizeof(short_payload) - 1,
		.options = { {
			.code = COAP_OPTION_OBSERVE,
			.value[0] = 0,
			.len = 1,
		} },
		.num_options = 1,
		.user_data = &sem1,
	};

	z_impl_zsock_recvfrom_fake.custom_fake =
		z_impl_zsock_recvfrom_custom_fake_observe_block_one;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_observe_block;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));

	/* Deregister posted from the last-block callback while request_token still holds
	 * the continuation token; only verifies the deregister GET is sent.
	 */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	/* Wait for the deregister work posted from the last-block callback. */
	k_sleep(K_MSEC(10));
	zassert_equal(ox_dereg_count, 1, "Deregister GET was not sent");

	coap_client_cancel_requests(&client);
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -ECANCELED, "");
}

ZTEST(coap_client, test_request_rst)
{
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_rst;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &short_request, NULL));

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

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req1, NULL));
	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req2, NULL));

	k_sleep(K_SECONDS(1));

	coap_client_cancel_request(&client, &req1);
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_not_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -ECANCELED, "");

	set_socket_events(client.fd, ZSOCK_POLLIN); /* First response is the cancelled one */
	zassert_not_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	set_socket_events(client.fd, ZSOCK_POLLIN);
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, COAP_RESPONSE_CODE_CONTENT, "");
}

ZTEST(coap_client, test_cancel_match)
{
	struct coap_client_request req1 = short_request;
	struct coap_client_request req2 = short_request;

	req1.user_data = &sem1;
	req2.user_data = &sem2;
	strcpy(req2.path, "another");

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_no_reply;

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req1, NULL));
	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req2, NULL));

	k_sleep(K_SECONDS(1));

	/* match only one */
	coap_client_cancel_request(&client, &(struct coap_client_request) {
		.path = TEST_PATH
	});
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_not_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(last_response_code, -ECANCELED, "");

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req1, NULL));

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

	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req1, NULL));
	zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req2, NULL));

	/* match both (wildcard)*/
	coap_client_cancel_request(&client, &(struct coap_client_request) {0});
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem2, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));

}

#define MULTICAST_TIMEOUT_MS 1000
/* 192.168.1.1, 192.168.1.2, ... */
#define MULTICAST_SERVER_START_IP 0xC0A80101U
#define MULTICAST_SERVER_PORT 5683

static int multicast_response_count;
static bool multicast_completed;
static int multicast_recv_index;
static uint16_t multicast_message_id;

static void multicast_coap_callback(const struct coap_client_response_data *data, void *user_data)
{
	LOG_INF("Multicast callback, code=%d source=%p", data->result_code, data->source);

	if (data->source == NULL) {
		multicast_completed = true;
	} else {
		const struct net_sockaddr_in *addr4;

		/* Verify each response carries a distinct source address matching the simulated
		 * servers: 192.168.1.1 for the first response, 192.168.1.2 for the second, etc.
		 */
		zassert_not_null(data->source,
				 "Expected non-NULL src_addr for multicast response %d",
				 multicast_response_count);

		zassert_equal(data->source->sa_family, NET_AF_INET,
			      "Expected IPv4 source for response %d", multicast_response_count);
		zassert_equal(data->source_len, sizeof(*addr4),
			      "Expected IPv4 source length for response %d",
			      multicast_response_count);

		addr4 = net_sin(data->source);

		zassert_equal(addr4->sin_port, net_htons(MULTICAST_SERVER_PORT),
			      "Wrong source port for response %d", multicast_response_count);
		zassert_equal(addr4->sin_addr.s_addr,
			      net_htonl(MULTICAST_SERVER_START_IP + multicast_response_count),
			      "Wrong source address for response %d", multicast_response_count);
		multicast_response_count++;
	}
	k_sem_give((struct k_sem *)user_data);
}

static ssize_t z_impl_zsock_sendto_custom_fake_multicast(int sock, void *buf, size_t len, int flags,
							 const struct net_sockaddr *dest_addr,
							 net_socklen_t addrlen)
{
	uint8_t *buf_u8 = buf;

	multicast_message_id = sys_get_be16(buf_u8 + 2);
	store_token(buf_u8);

	LOG_INF("Multicast message ID: %d", multicast_message_id);
	set_next_pending_message_id(multicast_message_id);

	return 1;
}

static ssize_t zsock_recvfrom_custom_fake_multicast(int sock, void *buf, size_t max_len, int flags,
						    struct net_sockaddr *src_addr,
						    net_socklen_t *addrlen, bool confirmable)
{
	/* NON response: VV=01, TT=01(NON), TKL=8, Code=2.05 Content. */
	uint8_t resp_data[] = {0x58, 0x45, 0x00, 0x00, 0x00, 0x00,
			       0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	struct net_sockaddr_in *addr4;

	if (confirmable) {
		/* Turn the response into a confirmable one */
		resp_data[0] = 0x48;
	}

	sys_put_be16(multicast_message_id, resp_data + 2);

	restore_token(resp_data);
	store_token(resp_data); /* Keep token available for subsequent calls */

	zassert_not_null(src_addr, "Unexpected NULL source address");
	zassert_not_null(addrlen, "Unexpected NULL source address length");

	/* Simulate a different IPv4 source for each responding server */
	addr4 = (struct net_sockaddr_in *)src_addr;

	memset(addr4, 0, sizeof(*addr4));
	addr4->sin_family = NET_AF_INET;
	addr4->sin_port = net_htons(MULTICAST_SERVER_PORT);
	addr4->sin_addr.s_addr =
		net_htonl(MULTICAST_SERVER_START_IP + multicast_recv_index);
	*addrlen = sizeof(*addr4);

	multicast_recv_index++;
	memcpy(buf, resp_data, sizeof(resp_data));

	/* Keep triggering POLLIN until all simulated responses are delivered */
	if (multicast_recv_index < 2) {
		set_socket_events(sock, ZSOCK_POLLIN);
	} else {
		clear_socket_events(sock, ZSOCK_POLLIN);
	}

	return sizeof(resp_data);
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_multicast(int sock, void *buf, size_t max_len,
							   int flags,
							   struct net_sockaddr *src_addr,
							   net_socklen_t *addrlen)
{
	return zsock_recvfrom_custom_fake_multicast(sock, buf, max_len, flags, src_addr, addrlen,
						    false);
}

static ssize_t z_impl_zsock_recvfrom_custom_fake_multicast_con(int sock, void *buf, size_t max_len,
							       int flags,
							       struct net_sockaddr *src_addr,
							       net_socklen_t *addrlen)
{
	return zsock_recvfrom_custom_fake_multicast(sock, buf, max_len, flags, src_addr, addrlen,
						    true);
}

ZTEST(coap_client, test_multicast_confirmable_rejected)
{
	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = TEST_PATH,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = coap_callback,
		.multicast_timeout_ms = MULTICAST_TIMEOUT_MS,
		.user_data = &sem1,
	};

	zassert_equal(coap_client_req(&client, 0, (const struct net_sockaddr *)&mcast_address, &req,
				      NULL),
		      -EINVAL);
}

ZTEST(coap_client, test_multicast_get_timeout)
{
	multicast_response_count = 0;
	multicast_completed = false;

	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = false,
		.path = TEST_PATH,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = multicast_coap_callback,
		.multicast_timeout_ms = MULTICAST_TIMEOUT_MS,
		.user_data = &sem1,
	};

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_multicast;
	/* POLLOUT in my_events enables the resend handler to fire once has_timeout_expired */
	set_socket_events(client.fd, ZSOCK_POLLOUT);

	zassert_ok(coap_client_req(&client, 0, (const struct net_sockaddr *)&mcast_address, &req,
				   NULL));

	/* Wait for the completion-only callback (no responses arrive before timeout) */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_equal(multicast_response_count, 0, "No responses expected before timeout");
	zassert_true(multicast_completed, "Expected completion callback after timeout");
}

ZTEST(coap_client, test_multicast_get_responses)
{
	multicast_response_count = 0;
	multicast_completed = false;
	multicast_recv_index = 0;

	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = false,
		.path = TEST_PATH,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = multicast_coap_callback,
		.multicast_timeout_ms = MULTICAST_TIMEOUT_MS,
		.user_data = &sem1,
	};

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_multicast;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_multicast;
	/* POLLIN delivers the simulated responses; POLLOUT enables timeout detection */
	set_socket_events(client.fd, ZSOCK_POLLIN | ZSOCK_POLLOUT);

	zassert_ok(coap_client_req(&client, 0, (const struct net_sockaddr *)&mcast_address, &req,
				   NULL));

	/* Collect two response callbacks */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
	/* Then wait for the completion callback after multicast timeout */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));

	zassert_equal(multicast_response_count, 2, "Expected 2 streamed responses");
	zassert_true(multicast_completed, "Expected completion callback after timeout");
}

ZTEST(coap_client, test_multicast_con_response_rejected)
{
	multicast_response_count = 0;
	multicast_completed = false;
	multicast_recv_index = 0;

	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = false,
		.path = TEST_PATH,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.cb = multicast_coap_callback,
		.multicast_timeout_ms = MULTICAST_TIMEOUT_MS,
		.user_data = &sem1,
	};

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_custom_fake_multicast;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_custom_fake_multicast_con;
	/* POLLIN delivers CON responses; POLLOUT enables timeout detection */
	set_socket_events(client.fd, ZSOCK_POLLIN | ZSOCK_POLLOUT);

	zassert_ok(coap_client_req(&client, 0, (const struct net_sockaddr *)&mcast_address, &req,
				   NULL));

	/* Only the completion callback is expected; CON responses must be silently dropped
	 * per RFC 7252 Section 8.2 (responses to multicast MUST NOT be Confirmable).
	 */
	zassert_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));

	zassert_equal(multicast_response_count, 0,
		      "CON responses must be rejected in multicast mode");
	zassert_true(multicast_completed, "Expected completion callback after timeout");
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
		zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));
	}
	zassert_equal(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL), -EAGAIN, "");

	k_sleep(K_MSEC(MORE_THAN_LONG_EXCHANGE_LIFETIME_MS));

	for (int i = 0; i < CONFIG_COAP_CLIENT_MAX_REQUESTS; i++) {
		zassert_ok(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL));
	}
	zassert_equal(coap_client_req(&client, 0, net_sad(&dst_address), &req, NULL), -EAGAIN, "");

	/* No callbacks from non-confirmable */
	zassert_not_ok(k_sem_take(&sem1, K_MSEC(MORE_THAN_EXCHANGE_LIFETIME_MS)));
}
