/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/net/socket.h>

#include "lwm2m_engine.h"
#include "lwm2m_pull_context.h"
#include "lwm2m_registry.h"

#define PUSH_TIMEOUT_MS (CONFIG_LWM2M_FIRMWARE_UPDATE_PUSH_TIMEOUT * MSEC_PER_SEC)

/* A CoAP server on the loopback interface stands in for the pull's file server */
#define PULL_SERVER_PORT 5699
#define PULL_URI "coap://127.0.0.1:5699/fw.bin"

struct pull_request {
	struct coap_packet pkt;
	uint8_t buf[128];
	struct net_sockaddr from;
	net_socklen_t from_len;
};

static uint8_t block[16];
static uint8_t pull_buf[CONFIG_LWM2M_COAP_BLOCK_SIZE];
static int server_fd = -1;
static int write_calls;
static int cancel_calls;

static int write_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
		    uint16_t data_len, bool last_block, size_t total_size, size_t offset)
{
	write_calls++;
	return 0;
}

static int cancel_cb(uint16_t obj_inst_id)
{
	cancel_calls++;
	return 0;
}

static uint8_t get_state(void)
{
	uint8_t value;

	zassert_ok(lwm2m_get_u8(&LWM2M_OBJ(5, 0, 3), &value));
	return value;
}

static uint8_t get_result(void)
{
	uint8_t value;

	zassert_ok(lwm2m_get_u8(&LWM2M_OBJ(5, 0, 5), &value));
	return value;
}

/* Drive the resource callbacks the way the engine does for a write request,
 * under the registry lock.
 */
static int package_write(uint8_t *data, uint16_t len, bool last_block, size_t offset)
{
	struct lwm2m_engine_res *res = lwm2m_engine_get_res(&LWM2M_OBJ(5, 0, 0));
	int ret;

	zassert_not_null(res);
	zassert_not_null(res->post_write_cb);

	lwm2m_registry_lock();
	ret = res->post_write_cb(0, 0, 0, data, len, last_block, 0, offset);
	lwm2m_registry_unlock();

	return ret;
}

/* The engine lets the pre-write callback pick the buffer, then stores the
 * string with its terminator and calls the post-write callback with that length.
 * No assertion runs here: a failed one aborts the test thread with the
 * registry lock held, also when the caller holds it already. Returns false
 * when the resource or its buffer is not fit for the write.
 */
static bool package_uri_write_locked(const char *uri, int *ret)
{
	struct lwm2m_engine_res *res = lwm2m_engine_get_res(&LWM2M_OBJ(5, 0, 1));
	size_t buf_len = 0;
	char *buf;

	if (res == NULL || res->pre_write_cb == NULL || res->post_write_cb == NULL) {
		return false;
	}

	lwm2m_registry_lock();
	buf = res->pre_write_cb(0, 1, 0, &buf_len);
	if (buf != NULL && strlen(uri) < buf_len) {
		strcpy(buf, uri);
		*ret = res->post_write_cb(0, 1, 0, (uint8_t *)buf, strlen(uri) + 1, true, 0, 0);
	}
	lwm2m_registry_unlock();

	return buf != NULL && strlen(uri) < buf_len;
}

/* For callers that do not hold the registry lock */
static int package_uri_write(const char *uri)
{
	int ret = -EINVAL;

	zassert_true(package_uri_write_locked(uri, &ret));

	return ret;
}

/* What a reader of the resource gets, through the object's read callback */
static const char *stored_uri(void)
{
	static char uri[CONFIG_LWM2M_SWMGMT_PACKAGE_URI_LEN];

	zassert_ok(lwm2m_get_string(&LWM2M_OBJ(5, 0, 1), uri, sizeof(uri)));
	return uri;
}

static void push_first_block(void)
{
	int writes = write_calls;

	zassert_ok(package_write(block, sizeof(block), false, 0));
	zassert_equal(get_state(), STATE_DOWNLOADING);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(write_calls, writes + 1);
}

/* The pull stores each received block here before the write callback sees it */
static void *package_buf_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			    size_t *data_len)
{
	*data_len = sizeof(pull_buf);
	return pull_buf;
}

static void pull_server_open(void)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(PULL_SERVER_PORT),
	};

	zassert_equal(zsock_inet_pton(NET_AF_INET, "127.0.0.1", &addr.sin_addr), 1);
	server_fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(server_fd >= 0);
	zassert_ok(zsock_bind(server_fd, (struct net_sockaddr *)&addr, sizeof(addr)));
}

/* Wait for the client to ask for a block; -EAGAIN when it does not */
static int pull_server_recv(struct pull_request *req, int timeout_ms)
{
	struct zsock_pollfd pfd = { .fd = server_fd, .events = ZSOCK_POLLIN };
	int len;

	if (zsock_poll(&pfd, 1, timeout_ms) <= 0) {
		return -EAGAIN;
	}

	req->from_len = sizeof(req->from);
	len = zsock_recvfrom(server_fd, req->buf, sizeof(req->buf), 0, &req->from, &req->from_len);
	zassert_true(len > 0);
	zassert_ok(coap_packet_parse(&req->pkt, req->buf, len, NULL, 0));
	zassert_equal(coap_header_get_code(&req->pkt), COAP_METHOD_GET);

	return 0;
}

/* Answer a block request with a full block of an image that is not over yet */
static void pull_server_send_block(const struct pull_request *req)
{
	static const uint8_t payload[CONFIG_LWM2M_COAP_BLOCK_SIZE];
	uint8_t buf[CONFIG_LWM2M_COAP_BLOCK_SIZE + 32];
	uint8_t token[COAP_TOKEN_MAX_LEN];
	struct coap_packet rsp;
	struct coap_block_context ctx;
	uint32_t num;
	bool more;
	uint8_t tkl;

	zassert_true(coap_get_block2_option(&req->pkt, &more, &num) > 0);
	tkl = coap_header_get_token(&req->pkt, token);

	zassert_ok(coap_packet_init(&rsp, buf, sizeof(buf), COAP_VERSION_1, COAP_TYPE_ACK, tkl,
				    token, COAP_RESPONSE_CODE_CONTENT,
				    coap_header_get_id(&req->pkt)));
	/* Two blocks past the requested one keeps the M bit set */
	coap_block_transfer_init(&ctx, coap_bytes_to_block_size(sizeof(payload)),
				 (num + 2) * sizeof(payload));
	ctx.current = num * sizeof(payload);
	zassert_ok(coap_append_block2_option(&rsp, &ctx));
	zassert_ok(coap_packet_append_payload_marker(&rsp));
	zassert_ok(coap_packet_append_payload(&rsp, payload, sizeof(payload)));

	zassert_equal(zsock_sendto(server_fd, rsp.data, rsp.offset, 0, &req->from, req->from_len),
		      rsp.offset);
}

/* Start a pull and serve it up to the client's request for the second block */
static void pull_to_second_block(struct pull_request *req)
{
	int writes = write_calls;

	zassert_ok(package_uri_write(PULL_URI));
	zassert_equal(get_state(), STATE_DOWNLOADING);

	zassert_ok(pull_server_recv(req, 1000));
	pull_server_send_block(req);
	zassert_ok(pull_server_recv(req, 1000));
	zassert_equal(write_calls, writes + 1);
}

/* The state machine was reset mid-pull: the late block is dropped and the pull is over */
static void pull_check_stopped(struct pull_request *req)
{
	int writes = write_calls;

	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);

	pull_server_send_block(req);
	zassert_equal(pull_server_recv(req, 500), -EAGAIN);
	zassert_equal(write_calls, writes);
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
}

static void *suite_setup(void)
{
	lwm2m_register_pre_write_callback(&LWM2M_OBJ(5, 0, 0), package_buf_cb);
	lwm2m_firmware_set_write_cb(write_cb);
	lwm2m_firmware_set_cancel_cb(cancel_cb);
	pull_server_open();

	return NULL;
}

static void test_before(void *fixture)
{
	/* Back to Idle whatever the previous case left behind, without callbacks */
	lwm2m_firmware_set_update_result(RESULT_DEFAULT);

	zassert_equal(get_state(), STATE_IDLE);
	write_calls = 0;
	cancel_calls = 0;
}

/* A case that failed mid-pull leaves neither a transfer nor an unread request behind */
static void test_after(void *fixture)
{
	uint8_t drop[64];

	lwm2m_registry_lock();
	(void)lwm2m_pull_context_cancel(true);
	lwm2m_registry_unlock();
	while (zsock_recv(server_fd, drop, sizeof(drop), ZSOCK_MSG_DONTWAIT) > 0) {
	}
}

ZTEST_SUITE(lwm2m_firmware_update, NULL, suite_setup, test_before, test_after, NULL);

ZTEST(lwm2m_firmware_update, test_push_timeout)
{
	push_first_block();

	k_sleep(K_MSEC(PUSH_TIMEOUT_MS / 2));
	zassert_ok(package_write(block, sizeof(block), false, sizeof(block)));
	zassert_equal(write_calls, 2);

	/* The timeout counts from the last block */
	k_sleep(K_MSEC(PUSH_TIMEOUT_MS * 3 / 4));
	zassert_equal(get_state(), STATE_DOWNLOADING);
	zassert_equal(cancel_calls, 0);

	k_sleep(K_MSEC(PUSH_TIMEOUT_MS / 2));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_CONNECTION_LOST);
	zassert_equal(cancel_calls, 1);

	/* A new push is accepted after the failure */
	push_first_block();
}

ZTEST(lwm2m_firmware_update, test_push_late_block_after_timeout)
{
	push_first_block();

	k_sleep(K_MSEC(PUSH_TIMEOUT_MS * 3 / 2));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_CONNECTION_LOST);

	/* The server was only slow: its next block must not start a download mid-image */
	zassert_equal(package_write(block, sizeof(block), false, sizeof(block)), -EFAULT);
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(write_calls, 1);
}

ZTEST(lwm2m_firmware_update, test_push_completes)
{
	push_first_block();

	zassert_ok(package_write(block, sizeof(block), true, sizeof(block)));
	zassert_equal(get_state(), STATE_DOWNLOADED);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(write_calls, 2);

	k_sleep(K_MSEC(PUSH_TIMEOUT_MS * 2));
	zassert_equal(get_state(), STATE_DOWNLOADED);
	zassert_equal(cancel_calls, 0);
}

ZTEST(lwm2m_firmware_update, test_push_reset_by_null_package)
{
	uint8_t nul = '\0';

	push_first_block();

	zassert_ok(package_write(&nul, 1, true, 0));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(cancel_calls, 1);
	zassert_equal(write_calls, 1);

	k_sleep(K_MSEC(PUSH_TIMEOUT_MS * 2));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(cancel_calls, 1);
}

ZTEST(lwm2m_firmware_update, test_push_reset_by_empty_package)
{
	push_first_block();

	zassert_ok(package_write(block, 0, true, 0));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(cancel_calls, 1);
	zassert_equal(write_calls, 1);
}

ZTEST(lwm2m_firmware_update, test_push_last_block_null_byte_is_data)
{
	uint8_t nul = '\0';

	push_first_block();

	zassert_ok(package_write(&nul, 1, true, sizeof(block)));
	zassert_equal(get_state(), STATE_DOWNLOADED);
	zassert_equal(write_calls, 2);
	zassert_equal(cancel_calls, 0);
}

ZTEST(lwm2m_firmware_update, test_push_reset_by_empty_uri)
{
	push_first_block();

	zassert_ok(package_uri_write(""));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(cancel_calls, 1);
	zassert_equal(strlen(stored_uri()), 0);

	k_sleep(K_MSEC(PUSH_TIMEOUT_MS * 2));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(cancel_calls, 1);
}

ZTEST(lwm2m_firmware_update, test_uri_rejected_while_downloading)
{
	push_first_block();

	zassert_equal(package_uri_write("coap://[::1]/fw.bin"), -EPERM);
	zassert_equal(get_state(), STATE_DOWNLOADING);
	zassert_equal(cancel_calls, 0);
	/* The rejected URI never reached the resource */
	zassert_equal(strlen(stored_uri()), 0);
}

ZTEST(lwm2m_firmware_update, test_reset_rejected_while_updating)
{
	uint8_t nul = '\0';

	push_first_block();
	zassert_ok(package_write(block, sizeof(block), true, sizeof(block)));
	zassert_equal(get_state(), STATE_DOWNLOADED);
	lwm2m_firmware_set_update_state(STATE_UPDATING);

	zassert_equal(package_write(&nul, 1, true, 0), -EPERM);
	zassert_equal(package_uri_write(""), -EPERM);
	zassert_equal(package_uri_write("coap://[::1]/fw.bin"), -EPERM);

	zassert_equal(get_state(), STATE_UPDATING);
	zassert_equal(write_calls, 2);
	zassert_equal(cancel_calls, 0);
}

ZTEST(lwm2m_firmware_update, test_writes_rejected_during_pull)
{
	/* A pull is Downloading without a push in progress */
	lwm2m_firmware_set_update_state(STATE_DOWNLOADING);

	zassert_equal(package_write(block, sizeof(block), false, 0), -EPERM);
	zassert_equal(package_uri_write("coap://[::1]/fw.bin"), -EPERM);

	zassert_equal(get_state(), STATE_DOWNLOADING);
	zassert_equal(write_calls, 0);
	zassert_equal(cancel_calls, 0);
}

ZTEST(lwm2m_firmware_update, test_pull_reset_by_empty_uri)
{
	struct pull_request req;

	pull_to_second_block(&req);

	zassert_ok(package_uri_write(""));
	zassert_equal(cancel_calls, 1);
	zassert_equal(strlen(stored_uri()), 0);
	pull_check_stopped(&req);
}

ZTEST(lwm2m_firmware_update, test_pull_reset_by_null_package)
{
	struct pull_request req;
	uint8_t nul = '\0';

	pull_to_second_block(&req);

	zassert_ok(package_write(&nul, 1, true, 0));
	zassert_equal(cancel_calls, 1);
	pull_check_stopped(&req);
}

ZTEST(lwm2m_firmware_update, test_pull_restarts_after_reset)
{
	struct pull_request req;
	int reset_ret = -EINVAL;
	int start_ret = -EINVAL;
	bool reset_ok;
	bool start_ok;
	int calls;

	pull_to_second_block(&req);

	/* The pull service needs the registry lock to close the stopped transfer,
	 * so a URI written while this holds it is queued behind that transfer.
	 * Nothing asserts until the lock is released.
	 */
	lwm2m_registry_lock();
	reset_ok = package_uri_write_locked("", &reset_ret);
	calls = cancel_calls;
	start_ok = package_uri_write_locked(PULL_URI, &start_ret);
	lwm2m_registry_unlock();

	zassert_true(reset_ok);
	zassert_ok(reset_ret);
	zassert_equal(calls, 1);
	zassert_true(start_ok);
	zassert_ok(start_ret);
	zassert_equal(get_state(), STATE_DOWNLOADING);

	zassert_ok(pull_server_recv(&req, 1000));
	pull_server_send_block(&req);
	zassert_ok(pull_server_recv(&req, 1000));
	zassert_equal(write_calls, 2);

	zassert_ok(package_uri_write(""));
	zassert_equal(cancel_calls, 2);
	pull_check_stopped(&req);
}

ZTEST(lwm2m_firmware_update, test_pull_reset_without_transfer)
{
	uint8_t nul = '\0';

	/* Downloading without a push or a pull behind it still resets */
	lwm2m_firmware_set_update_state(STATE_DOWNLOADING);
	zassert_ok(package_write(&nul, 1, true, 0));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(cancel_calls, 1);

	lwm2m_firmware_set_update_state(STATE_DOWNLOADING);
	zassert_ok(package_uri_write(""));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(cancel_calls, 2);
	zassert_equal(write_calls, 0);
}

ZTEST(lwm2m_firmware_update, test_pull_refused_without_write_cb)
{
	/* The pull cannot start without a write callback; the object reports it and stays usable */
	lwm2m_firmware_set_write_cb(NULL);

	zassert_ok(package_uri_write("coap://[::1]/fw.bin"));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_UPDATE_FAILED);

	zassert_ok(package_uri_write("coap://[::1]/fw.bin"));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_UPDATE_FAILED);

	lwm2m_firmware_set_write_cb(write_cb);
	push_first_block();
}

ZTEST(lwm2m_firmware_update, test_uri_rejected_when_downloaded)
{
	push_first_block();
	zassert_ok(package_write(block, sizeof(block), true, sizeof(block)));
	zassert_equal(get_state(), STATE_DOWNLOADED);

	zassert_equal(package_uri_write("coap://[::1]/fw.bin"), -EPERM);
	zassert_equal(get_state(), STATE_DOWNLOADED);
}

ZTEST(lwm2m_firmware_update, test_reset_when_idle)
{
	uint8_t nul = '\0';

	lwm2m_set_u8(&LWM2M_OBJ(5, 0, 5), RESULT_CONNECTION_LOST);

	zassert_ok(package_write(&nul, 1, true, 0));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(write_calls, 0);
	zassert_equal(cancel_calls, 0);

	lwm2m_set_u8(&LWM2M_OBJ(5, 0, 5), RESULT_CONNECTION_LOST);

	zassert_ok(package_uri_write(""));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(cancel_calls, 0);
}

ZTEST(lwm2m_firmware_update, test_downloaded_reset)
{
	uint8_t nul = '\0';

	push_first_block();
	zassert_ok(package_write(block, sizeof(block), true, sizeof(block)));
	zassert_equal(get_state(), STATE_DOWNLOADED);

	zassert_ok(package_write(&nul, 1, true, 0));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(cancel_calls, 1);
	zassert_equal(write_calls, 2);
}
