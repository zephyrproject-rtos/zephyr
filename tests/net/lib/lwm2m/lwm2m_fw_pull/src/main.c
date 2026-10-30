/*
 * Copyright (c) 2026 Mustafa Abdullah Kus, Amazeng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/net/socket.h>

#include "lwm2m_engine.h"
#include "lwm2m_registry.h"

#define SERVER_PORT 5699
#define PACKAGE_URI "coap://127.0.0.1:5699/fw"
/* Pull service releases the context on its next run; without tickless the
 * engine may not see a change made from this thread before ENGINE_SLEEP_MS (500 ms)
 */
#define RELEASE_DELAY K_MSEC(600)

static int server_fd = -1;
static uint8_t fw_buf[64];
static atomic_t write_calls;
static int write_ret;

static void *fw_get_buf(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			size_t *data_len)
{
	*data_len = sizeof(fw_buf);
	return fw_buf;
}

static int fw_write_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
		       uint8_t *data, uint16_t data_len, bool last_block, size_t total_size,
		       size_t offset)
{
	atomic_inc(&write_calls);
	return write_ret;
}

/* Same call the engine makes for a non block-wise Write of Package URI */
static int write_package_uri(const char *uri)
{
	struct lwm2m_engine_res *res;
	char *buf;
	uint16_t len = strlen(uri);

	zassert_ok(lwm2m_get_resource(&LWM2M_OBJ(5, 0, 1), &res));
	buf = res->res_instances->data_ptr;
	zassert_true(len < res->res_instances->max_data_len);
	memcpy(buf, uri, len + 1);
	res->res_instances->data_len = len;

	return res->post_write_cb(0, 1, 0, buf, len, true, 0, 0);
}

/* Wait for the client's GET; fills the request so a reply can match it */
static int recv_request(struct coap_packet *req, uint8_t *buf, size_t len,
			struct net_sockaddr *from, net_socklen_t *from_len, int timeout_ms)
{
	struct zsock_pollfd pfd = {.fd = server_fd, .events = ZSOCK_POLLIN};
	int ret;

	ret = zsock_poll(&pfd, 1, timeout_ms);
	if (ret <= 0) {
		return -EAGAIN;
	}

	ret = zsock_recvfrom(server_fd, buf, len, 0, from, from_len);
	zassert_true(ret > 0);
	zassert_ok(coap_packet_parse(req, buf, ret, NULL, 0));
	zassert_equal(coap_header_get_code(req), COAP_METHOD_GET);
	return 0;
}

static void send_reply(const struct coap_packet *req, uint8_t code, const uint8_t *payload,
		       size_t payload_len, bool more, const struct net_sockaddr *to,
		       net_socklen_t to_len)
{
	uint8_t buf[256];
	uint8_t token[COAP_TOKEN_MAX_LEN];
	struct coap_packet rsp;
	struct coap_block_context block;
	uint8_t tkl = coap_header_get_token(req, token);

	zassert_ok(coap_packet_init(&rsp, buf, sizeof(buf), COAP_VERSION_1, COAP_TYPE_ACK, tkl,
				    token, code, coap_header_get_id(req)));
	if (payload_len > 0) {
		coap_block_transfer_init(&block, COAP_BLOCK_64, more ? 4 * 64 : payload_len);
		zassert_ok(coap_append_block2_option(&rsp, &block));
		zassert_ok(coap_packet_append_payload_marker(&rsp));
		zassert_ok(coap_packet_append_payload(&rsp, payload, payload_len));
	}

	zassert_equal(zsock_sendto(server_fd, rsp.data, rsp.offset, 0, to, to_len), rsp.offset);
}

static void start_pull(struct coap_packet *req, uint8_t *buf, size_t len,
		       struct net_sockaddr *from, net_socklen_t *from_len)
{
	zassert_ok(write_package_uri(PACKAGE_URI));
	zassert_equal(lwm2m_firmware_get_update_state(), STATE_DOWNLOADING);
	zassert_ok(recv_request(req, buf, len, from, from_len, 1000));
}

static void *suite_setup(void)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(SERVER_PORT),
	};

	zassert_equal(zsock_inet_pton(NET_AF_INET, "127.0.0.1", &addr.sin_addr), 1);
	server_fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(server_fd >= 0);
	zassert_ok(zsock_bind(server_fd, (struct net_sockaddr *)&addr, sizeof(addr)));

	lwm2m_register_pre_write_callback(&LWM2M_OBJ(5, 0, 0), fw_get_buf);
	lwm2m_firmware_set_write_cb(fw_write_cb);
	return NULL;
}

static void before(void *f)
{
	lwm2m_firmware_set_update_result(RESULT_DEFAULT);
	atomic_clear(&write_calls);
	write_ret = 0;
	k_sleep(RELEASE_DELAY);
}

ZTEST(lwm2m_fw_pull, test_refused_package_is_unsupported)
{
	static const uint8_t junk[] = "not a firmware image";
	uint8_t buf[256];
	struct coap_packet req;
	struct net_sockaddr from;
	net_socklen_t from_len = sizeof(from);

	write_ret = -ENOMSG;
	start_pull(&req, buf, sizeof(buf), &from, &from_len);
	send_reply(&req, COAP_RESPONSE_CODE_CONTENT, junk, sizeof(junk), false, &from, from_len);
	k_sleep(RELEASE_DELAY);

	zassert_equal(atomic_get(&write_calls), 1);
	zassert_equal(lwm2m_firmware_get_update_state(), STATE_IDLE);
	zassert_equal(lwm2m_firmware_get_update_result(), RESULT_UNSUP_FW);
}

ZTEST(lwm2m_fw_pull, test_error_response_is_connection_lost)
{
	uint8_t buf[256];
	struct coap_packet req;
	struct net_sockaddr from;
	net_socklen_t from_len = sizeof(from);

	start_pull(&req, buf, sizeof(buf), &from, &from_len);
	send_reply(&req, COAP_RESPONSE_CODE_NOT_FOUND, NULL, 0, false, &from, from_len);
	k_sleep(RELEASE_DELAY);

	zassert_equal(atomic_get(&write_calls), 0);
	zassert_equal(lwm2m_firmware_get_update_state(), STATE_IDLE);
	zassert_equal(lwm2m_firmware_get_update_result(), RESULT_CONNECTION_LOST);
}

ZTEST_SUITE(lwm2m_fw_pull, NULL, suite_setup, before, NULL, NULL);
