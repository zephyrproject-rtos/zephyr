/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The OBEX layer when an ABORT or a DISCONNECT arrives or is sent during an operation. A fake
 * transport stands in for the peer: the tests feed it raw packets and check what was sent.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/classic/obex.h>

#include "host/classic/obex_internal.h"

#define TEST_MTU       255U
#define TEST_BUF_COUNT 6U
#define TEST_BUF_SIZE  255U

#define TEST_SRM_ENABLE 0x01U

/* Packet header: code and length */
#define TEST_PKT_HDR_LEN 3U

/* Version, flags and MOPL fields of a CONNECT packet */
#define CONNECT_FIELDS BT_OBEX_VERSION, 0x00, 0x00, TEST_MTU

/* Headers, as they appear in a packet */
#define HDR_BODY     0x48, 0x00, 0x04, 'x'
#define HDR_END_BODY 0x49, 0x00, 0x04, 'x'
#define HDR_DESC     0x05, 0x00, 0x05, 0x00, 0x00
#define HDR_SRM      0x97, 0x01
#define HDR_SRMP     0x98, 0x01

NET_BUF_POOL_FIXED_DEFINE(test_pool, TEST_BUF_COUNT, TEST_BUF_SIZE, 0, NULL);

struct test_rsp {
	uint32_t count;
	uint8_t code;
};

struct obex_mid_operation_fixture {
	struct bt_obex obex;
	struct bt_obex_client client;
	struct bt_obex_server server;
	struct k_fifo sent;
	int send_err;
	void (*on_alloc)(struct obex_mid_operation_fixture *f);
	void (*on_send)(struct obex_mid_operation_fixture *f);
	uint32_t transport_disconnects;
	struct test_rsp put_rsp;
	struct test_rsp get_rsp;
	struct test_rsp action_rsp;
	struct test_rsp abort_rsp;
	uint32_t connect_reqs;
	uint32_t disconnect_reqs;
	uint32_t put_reqs;
	uint32_t get_reqs;
	uint32_t other_reqs;
};

static struct net_buf *test_buf(void)
{
	struct net_buf *buf = net_buf_alloc(&test_pool, K_NO_WAIT);

	zassert_not_null(buf, "No test buffer");
	net_buf_reserve(buf, BT_OBEX_SEND_BUF_RESERVE);
	return buf;
}

/* The peer sends a packet. */
static void test_recv(struct obex_mid_operation_fixture *f, uint8_t code, const uint8_t *data,
		      size_t len)
{
	struct net_buf *buf = net_buf_alloc(&test_pool, K_NO_WAIT);
	int err;

	zassert_not_null(buf, "No test buffer");
	(void)net_buf_add_u8(buf, code);
	net_buf_add_be16(buf, TEST_PKT_HDR_LEN + len);
	if (len > 0U) {
		(void)net_buf_add_mem(buf, data, len);
	}

	err = bt_obex_recv(&f->obex, buf);
	net_buf_unref(buf);
	zassert_ok(err, "Packet not accepted (err %d)", err);
}

#define TEST_RECV(f, code, ...)                                                                    \
	do {                                                                                       \
		const uint8_t data[] = {__VA_ARGS__};                                              \
		test_recv(f, code, data, sizeof(data));                                            \
	} while (false)

#define TEST_RECV_EMPTY(f, code) test_recv(f, code, NULL, 0U)

static struct net_buf *transport_alloc_buf(struct bt_obex *obex, struct net_buf_pool *pool)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(obex, struct obex_mid_operation_fixture, obex);

	void (*on_alloc)(struct obex_mid_operation_fixture *f) = f->on_alloc;

	ARG_UNUSED(pool);

	if (on_alloc != NULL) {
		f->on_alloc = NULL;
		on_alloc(f);
	}

	return test_buf();
}

static int transport_send(struct bt_obex *obex, struct net_buf *buf)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(obex, struct obex_mid_operation_fixture, obex);

	void (*on_send)(struct obex_mid_operation_fixture *f) = f->on_send;

	if (on_send != NULL) {
		f->on_send = NULL;
		on_send(f);
	}

	if (f->send_err != 0) {
		return f->send_err;
	}

	k_fifo_put(&f->sent, buf);
	return 0;
}

static int transport_disconnect(struct bt_obex *obex)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(obex, struct obex_mid_operation_fixture, obex);

	f->transport_disconnects++;
	return 0;
}

static void client_connect(struct bt_obex_client *client, uint8_t rsp_code, uint8_t version,
			   uint16_t mopl, struct net_buf *buf)
{
	zassert_equal(rsp_code, BT_OBEX_RSP_CODE_SUCCESS);
}

static void client_disconnect(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
}

static void test_rsp_record(struct test_rsp *rsp, uint8_t rsp_code)
{
	rsp->count++;
	rsp->code = rsp_code;
}

static void client_put(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(client, struct obex_mid_operation_fixture, client);

	test_rsp_record(&f->put_rsp, rsp_code);
}

static void client_get(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(client, struct obex_mid_operation_fixture, client);

	test_rsp_record(&f->get_rsp, rsp_code);
}

static void client_action(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(client, struct obex_mid_operation_fixture, client);

	test_rsp_record(&f->action_rsp, rsp_code);
}

static void client_abort(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(client, struct obex_mid_operation_fixture, client);

	test_rsp_record(&f->abort_rsp, rsp_code);
}

static void server_connect(struct bt_obex_server *server, uint8_t version, uint16_t mopl,
			   struct net_buf *buf)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(server, struct obex_mid_operation_fixture, server);

	f->connect_reqs++;
}

static void server_disconnect(struct bt_obex_server *server, struct net_buf *buf)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(server, struct obex_mid_operation_fixture, server);

	f->disconnect_reqs++;
}

static void server_put(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(server, struct obex_mid_operation_fixture, server);

	f->put_reqs++;
}

static void server_get(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(server, struct obex_mid_operation_fixture, server);

	f->get_reqs++;
}

static void server_abort(struct bt_obex_server *server, struct net_buf *buf)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(server, struct obex_mid_operation_fixture, server);

	f->other_reqs++;
}

static void server_setpath(struct bt_obex_server *server, uint8_t flags, struct net_buf *buf)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(server, struct obex_mid_operation_fixture, server);

	f->other_reqs++;
}

static void server_action(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct obex_mid_operation_fixture *f =
		CONTAINER_OF(server, struct obex_mid_operation_fixture, server);

	f->other_reqs++;
}

static void test_expect_sent(struct obex_mid_operation_fixture *f, uint8_t code)
{
	struct net_buf *buf = k_fifo_get(&f->sent, K_NO_WAIT);
	uint8_t sent;

	zassert_not_null(buf, "Nothing sent, expected 0x%02x", code);
	sent = buf->data[0];
	net_buf_unref(buf);
	zassert_equal(sent, code, "Sent 0x%02x, expected 0x%02x", sent, code);
}

static void test_expect_nothing_sent(struct obex_mid_operation_fixture *f)
{
	zassert_true(k_fifo_is_empty(&f->sent), "Unexpected packet sent");
}

static void test_client_connect(struct obex_mid_operation_fixture *f)
{
	zassert_ok(bt_obex_connect(&f->client, TEST_MTU, NULL));
	test_expect_sent(f, BT_OBEX_OPCODE_CONNECT);
	TEST_RECV(f, BT_OBEX_RSP_CODE_SUCCESS, CONNECT_FIELDS);
}

static void test_server_connect(struct obex_mid_operation_fixture *f)
{
	uint32_t connect_reqs = f->connect_reqs;

	TEST_RECV(f, BT_OBEX_OPCODE_CONNECT, CONNECT_FIELDS);
	zassert_equal(f->connect_reqs, connect_reqs + 1U);
	zassert_ok(bt_obex_connect_rsp(&f->server, BT_OBEX_RSP_CODE_SUCCESS, TEST_MTU, NULL));
	test_expect_sent(f, BT_OBEX_RSP_CODE_SUCCESS);
}

static void test_send_srm_get(struct obex_mid_operation_fixture *f)
{
	struct net_buf *buf = test_buf();

	zassert_ok(bt_obex_add_header_srm(buf, TEST_SRM_ENABLE));
	zassert_ok(bt_obex_get(&f->client, true, buf));
	test_expect_sent(f, BT_OBEX_OPCODE_GET_F);
}

static void test_send_srm_put(struct obex_mid_operation_fixture *f)
{
	struct net_buf *buf = test_buf();

	zassert_ok(bt_obex_add_header_srm(buf, TEST_SRM_ENABLE));
	zassert_ok(bt_obex_put(&f->client, false, buf));
	test_expect_sent(f, BT_OBEX_OPCODE_PUT);
}

static void test_recv_end_of_get(struct obex_mid_operation_fixture *f)
{
	TEST_RECV(f, BT_OBEX_RSP_CODE_SUCCESS, HDR_END_BODY);
}

static void test_teardown(struct obex_mid_operation_fixture *f)
{
	zassert_ok(bt_obex_transport_disconnected(&f->obex));
}

static void test_abort_again(struct obex_mid_operation_fixture *f)
{
	struct net_buf *buf = test_buf();

	zassert_ok(bt_obex_abort(&f->client, buf));
	test_expect_sent(f, BT_OBEX_OPCODE_ABORT);
}

static void test_recv_disconnect(struct obex_mid_operation_fixture *f)
{
	TEST_RECV_EMPTY(f, BT_OBEX_OPCODE_DISCONN);
}

struct test_server_op {
	uint8_t code;
	size_t len;
	int (*rsp)(struct bt_obex_server *server, uint8_t rsp_code, struct net_buf *buf);
};

/* Requests that a DISCONNECT can overtake, with SETPATH flags and constants as their data */
static const struct test_server_op test_server_ops[] = {
	{ BT_OBEX_OPCODE_PUT_F, 0U, bt_obex_put_rsp },
	{ BT_OBEX_OPCODE_GET_F, 0U, bt_obex_get_rsp },
	{ BT_OBEX_OPCODE_SETPATH, sizeof(struct bt_obex_setpath_req_hdr), bt_obex_setpath_rsp },
	{ BT_OBEX_OPCODE_ACTION_F, 0U, bt_obex_action_rsp },
	{ BT_OBEX_OPCODE_ABORT, 0U, bt_obex_abort_rsp },
};

static void test_recv_server_op(struct obex_mid_operation_fixture *f,
				const struct test_server_op *op)
{
	const uint8_t data[sizeof(struct bt_obex_setpath_req_hdr)] = { 0 };

	test_recv(f, op->code, data, op->len);
}

static void *suite_setup(void)
{
	static struct obex_mid_operation_fixture fixture;

	return &fixture;
}

static void before_each(void *data)
{
	static const struct bt_obex_transport_ops transport_ops = {
		.alloc_buf = transport_alloc_buf,
		.send = transport_send,
		.disconnect = transport_disconnect,
	};
	static const struct bt_obex_client_ops client_ops = {
		.connect = client_connect,
		.disconnect = client_disconnect,
		.put = client_put,
		.get = client_get,
		.action = client_action,
		.abort = client_abort,
	};
	static const struct bt_obex_server_ops server_ops = {
		.connect = server_connect,
		.disconnect = server_disconnect,
		.put = server_put,
		.get = server_get,
		.abort = server_abort,
		.setpath = server_setpath,
		.action = server_action,
	};
	struct obex_mid_operation_fixture *f = data;

	(void)memset(f, 0, sizeof(*f));
	k_fifo_init(&f->sent);

	f->obex.rx.mtu = TEST_MTU;
	f->obex.tx.mtu = TEST_MTU;
	f->client.obex = &f->obex;
	f->client.ops = &client_ops;
	f->server.obex = &f->obex;
	f->server.ops = &server_ops;

	bt_obex_reg_transport(&f->obex, &transport_ops);
	zassert_ok(bt_obex_transport_connected(&f->obex));
}

static void after_each(void *data)
{
	struct obex_mid_operation_fixture *f = data;
	struct net_buf *buf;

	while ((buf = k_fifo_get(&f->sent, K_NO_WAIT)) != NULL) {
		net_buf_unref(buf);
	}

	zassert_equal(net_buf_get_available(&test_pool), TEST_BUF_COUNT, "Leaked buffer");
}

static ZTEST_F(obex_mid_operation, test_abort_waits_for_rsp)
{
	test_client_connect(fixture);

	zassert_ok(bt_obex_get(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_GET_F);

	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EBUSY);
	test_expect_nothing_sent(fixture);

	/* A CONTINUE without headers is the ordinary response to the request */
	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_CONTINUE);
	zassert_equal(fixture->get_rsp.count, 1U);
	zassert_equal(fixture->transport_disconnects, 0U);

	zassert_ok(bt_obex_abort(&fixture->client, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_ABORT);

	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_SUCCESS);
	zassert_equal(fixture->abort_rsp.count, 1U);
	zassert_equal(fixture->abort_rsp.code, BT_OBEX_RSP_CODE_SUCCESS);
	zassert_equal(fixture->get_rsp.count, 1U);

	/* The next response belongs to the next request */
	zassert_ok(bt_obex_get(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_GET_F);

	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_NOT_FOUND);
	zassert_equal(fixture->get_rsp.count, 2U);
	zassert_equal(fixture->get_rsp.code, BT_OBEX_RSP_CODE_NOT_FOUND);
	zassert_equal(fixture->abort_rsp.count, 1U);
	zassert_equal(fixture->transport_disconnects, 0U);
}

static ZTEST_F(obex_mid_operation, test_abort_of_final_put_waits_for_rsp)
{
	test_client_connect(fixture);

	zassert_ok(bt_obex_put(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_PUT_F);

	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EBUSY);
	test_expect_nothing_sent(fixture);

	/* A SUCCESS without headers is the response to the PUT, nothing is left to abort */
	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_SUCCESS);
	zassert_equal(fixture->put_rsp.count, 1U);
	zassert_equal(fixture->abort_rsp.count, 0U);

	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EINVAL);
	test_expect_nothing_sent(fixture);
}

static ZTEST_F(obex_mid_operation, test_abort_rsp_with_headers)
{
	test_client_connect(fixture);

	zassert_ok(bt_obex_get(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_GET_F);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_BODY);
	zassert_equal(fixture->get_rsp.count, 1U);

	zassert_ok(bt_obex_abort(&fixture->client, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_ABORT);

	TEST_RECV(fixture, BT_OBEX_RSP_CODE_SUCCESS, HDR_DESC);
	zassert_equal(fixture->abort_rsp.count, 1U);
	zassert_equal(fixture->get_rsp.count, 1U, "Taken as the response to the GET");
}

static ZTEST_F(obex_mid_operation, test_abort_rsp_failure_closes_transport)
{
	test_client_connect(fixture);

	zassert_ok(bt_obex_get(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_GET_F);
	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_CONTINUE);

	zassert_ok(bt_obex_abort(&fixture->client, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_ABORT);

	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_FORBIDDEN);
	zassert_equal(fixture->transport_disconnects, 1U);
	zassert_equal(fixture->abort_rsp.count, 0U);

	/* A response could still be on its way, so nothing starts until the transport is down */
	zassert_equal(bt_obex_get(&fixture->client, true, NULL), -EBUSY);
	test_expect_nothing_sent(fixture);

	zassert_ok(bt_obex_transport_disconnected(&fixture->obex));
	zassert_equal(bt_obex_get(&fixture->client, true, NULL), -ENOTCONN);
}

static ZTEST_F(obex_mid_operation, test_abort_send_failure)
{
	test_client_connect(fixture);

	zassert_ok(bt_obex_get(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_GET_F);
	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_CONTINUE);

	fixture->send_err = -EIO;
	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EIO);
	fixture->send_err = 0;

	/* The operation goes on */
	zassert_ok(bt_obex_get(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_GET_F);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_SUCCESS, HDR_END_BODY);
	zassert_equal(fixture->get_rsp.count, 2U);
	zassert_equal(fixture->get_rsp.code, BT_OBEX_RSP_CODE_SUCCESS);
	zassert_equal(fixture->abort_rsp.count, 0U);
}

static ZTEST_F(obex_mid_operation, test_abort_overtaken_by_final_rsp)
{
	test_client_connect(fixture);

	test_send_srm_get(fixture);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_SRM, HDR_BODY);

	/* The final response arrives while bt_obex_abort() is allocating its buffer */
	fixture->on_alloc = test_recv_end_of_get;
	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EBUSY);
	test_expect_nothing_sent(fixture);
	zassert_equal(fixture->get_rsp.count, 2U);
	zassert_equal(fixture->get_rsp.code, BT_OBEX_RSP_CODE_SUCCESS);

	/* Nothing is left to abort and the client is free */
	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EINVAL);

	zassert_ok(bt_obex_get(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_GET_F);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_SUCCESS, HDR_END_BODY);
	zassert_equal(fixture->get_rsp.count, 3U);
}

static ZTEST_F(obex_mid_operation, test_abort_send_failure_after_final_rsp)
{
	test_client_connect(fixture);

	test_send_srm_get(fixture);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_SRM, HDR_BODY);

	/* The final response arrives while the abort request fails to go out */
	fixture->on_send = test_recv_end_of_get;
	fixture->send_err = -EIO;
	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EIO);
	fixture->send_err = 0;
	zassert_equal(fixture->get_rsp.count, 2U);

	/* The operation is not brought back */
	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EINVAL);

	zassert_ok(bt_obex_get(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_GET_F);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_SUCCESS, HDR_END_BODY);
	zassert_equal(fixture->get_rsp.count, 3U);
}

static ZTEST_F(obex_mid_operation, test_abort_send_failure_after_teardown)
{
	const struct bt_obex_transport_ops *ops = fixture->obex._transport_ops;

	test_client_connect(fixture);

	zassert_ok(bt_obex_get(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_GET_F);
	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_CONTINUE);

	/* The transport goes away while the abort request fails to go out */
	fixture->on_send = test_teardown;
	fixture->send_err = -EIO;
	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EIO);
	fixture->send_err = 0;

	/* The operation is not brought back, so a new connection can start */
	bt_obex_reg_transport(&fixture->obex, ops);
	zassert_ok(bt_obex_transport_connected(&fixture->obex));
	test_client_connect(fixture);
}

static ZTEST_F(obex_mid_operation, test_abort_overlapping_calls)
{
	test_client_connect(fixture);

	test_send_srm_get(fixture);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_SRM, HDR_BODY);

	/* Another call sends the abort request while this one is allocating its buffer */
	fixture->on_alloc = test_abort_again;
	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EINPROGRESS);
	test_expect_nothing_sent(fixture);

	/* The abort that was sent still knows which operation it aborts */
	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_CONTINUE);
	zassert_equal(fixture->get_rsp.count, 2U);
	zassert_equal(fixture->transport_disconnects, 0U);

	TEST_RECV(fixture, BT_OBEX_RSP_CODE_SUCCESS, HDR_END_BODY);
	zassert_equal(fixture->get_rsp.count, 3U);
	zassert_equal(fixture->abort_rsp.count, 0U);

	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_SUCCESS);
	zassert_equal(fixture->abort_rsp.count, 1U);
}

static ZTEST_F(obex_mid_operation, test_abort_of_action_waits_for_rsp)
{
	test_client_connect(fixture);

	/* An SRM operation that ended early does not make the ACTION an SRM operation */
	test_send_srm_put(fixture);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_SRM);
	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_FORBIDDEN);

	zassert_ok(bt_obex_action(&fixture->client, false, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_ACTION);

	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EBUSY);
	test_expect_nothing_sent(fixture);

	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_CONTINUE);
	zassert_equal(fixture->action_rsp.count, 1U);

	/* Every ACTION request gets a response */
	zassert_ok(bt_obex_action(&fixture->client, false, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_ACTION);

	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EBUSY);
	test_expect_nothing_sent(fixture);

	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_CONTINUE);
	zassert_equal(fixture->action_rsp.count, 2U);

	zassert_ok(bt_obex_abort(&fixture->client, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_ABORT);

	TEST_RECV(fixture, BT_OBEX_RSP_CODE_SUCCESS, HDR_DESC);
	zassert_equal(fixture->abort_rsp.count, 1U);
	zassert_equal(fixture->action_rsp.count, 2U);
}

static ZTEST_F(obex_mid_operation, test_abort_srm_get_crossing_rsps)
{
	test_client_connect(fixture);

	test_send_srm_get(fixture);

	/* SRM is not in effect until the server has enabled it */
	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EBUSY);
	test_expect_nothing_sent(fixture);

	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_SRM, HDR_BODY);
	zassert_equal(fixture->get_rsp.count, 1U);

	zassert_ok(bt_obex_abort(&fixture->client, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_ABORT);

	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_CONTINUE);
	zassert_equal(fixture->get_rsp.count, 2U);
	zassert_equal(fixture->transport_disconnects, 0U);

	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_BODY);
	zassert_equal(fixture->get_rsp.count, 3U);

	TEST_RECV(fixture, BT_OBEX_RSP_CODE_SUCCESS, HDR_END_BODY);
	zassert_equal(fixture->get_rsp.count, 4U);
	zassert_equal(fixture->get_rsp.code, BT_OBEX_RSP_CODE_SUCCESS);
	zassert_equal(fixture->abort_rsp.count, 0U);

	/* The abort response is due, it must not be taken as the response to a new request */
	zassert_equal(bt_obex_get(&fixture->client, true, NULL), -EBUSY);
	zassert_equal(bt_obex_put(&fixture->client, true, NULL), -EBUSY);
	test_expect_nothing_sent(fixture);

	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_SUCCESS);
	zassert_equal(fixture->abort_rsp.count, 1U);
	zassert_equal(fixture->get_rsp.count, 4U);

	zassert_ok(bt_obex_get(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_GET_F);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_SUCCESS, HDR_END_BODY);
	zassert_equal(fixture->get_rsp.count, 5U);
}

static ZTEST_F(obex_mid_operation, test_abort_srm_get_only_abort_rsp)
{
	test_client_connect(fixture);

	test_send_srm_get(fixture);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_SRM, HDR_BODY);

	zassert_ok(bt_obex_abort(&fixture->client, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_ABORT);

	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_SUCCESS);
	zassert_equal(fixture->abort_rsp.count, 1U);
	zassert_equal(fixture->get_rsp.count, 1U);

	/* A header other than the object body does not make it a response to the GET */
	test_send_srm_get(fixture);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_SRM, HDR_BODY);

	zassert_ok(bt_obex_abort(&fixture->client, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_ABORT);

	TEST_RECV(fixture, BT_OBEX_RSP_CODE_SUCCESS, HDR_DESC);
	zassert_equal(fixture->abort_rsp.count, 2U);
	zassert_equal(fixture->get_rsp.count, 2U);
}

static ZTEST_F(obex_mid_operation, test_abort_srm_get_wait)
{
	test_client_connect(fixture);

	test_send_srm_get(fixture);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_SRM, HDR_SRMP);

	zassert_ok(bt_obex_get(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_GET_F);

	/* The SRMP wait makes this exchange a request and a response */
	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EBUSY);
	test_expect_nothing_sent(fixture);

	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_SRMP);
	zassert_equal(fixture->get_rsp.count, 2U);

	zassert_ok(bt_obex_abort(&fixture->client, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_ABORT);

	/* The server is holding its responses, so this can only be the abort response */
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_SUCCESS, HDR_END_BODY);
	zassert_equal(fixture->abort_rsp.count, 1U);
	zassert_equal(fixture->get_rsp.count, 2U);
}

static ZTEST_F(obex_mid_operation, test_abort_srm_put)
{
	test_client_connect(fixture);

	test_send_srm_put(fixture);

	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EBUSY);
	test_expect_nothing_sent(fixture);

	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_SRM);

	/* No response is due for the requests before the final one */
	zassert_ok(bt_obex_put(&fixture->client, false, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_PUT);

	zassert_ok(bt_obex_abort(&fixture->client, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_ABORT);

	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_SUCCESS);
	zassert_equal(fixture->abort_rsp.count, 1U);
	zassert_equal(fixture->put_rsp.count, 1U);

	/* The final request gets a response */
	test_send_srm_put(fixture);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_SRM);

	zassert_ok(bt_obex_put(&fixture->client, true, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_PUT_F);

	zassert_equal(bt_obex_abort(&fixture->client, NULL), -EBUSY);
	test_expect_nothing_sent(fixture);

	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_SUCCESS);
	zassert_equal(fixture->put_rsp.count, 3U);
	zassert_equal(fixture->abort_rsp.count, 1U);
}

static ZTEST_F(obex_mid_operation, test_abort_srm_early_failure_closes_transport)
{
	test_client_connect(fixture);

	test_send_srm_put(fixture);
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_CONTINUE, HDR_SRM);

	zassert_ok(bt_obex_put(&fixture->client, false, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_PUT);

	zassert_ok(bt_obex_abort(&fixture->client, NULL));
	test_expect_sent(fixture, BT_OBEX_OPCODE_ABORT);

	/* Cannot be told apart from a failed abort */
	TEST_RECV(fixture, BT_OBEX_RSP_CODE_FORBIDDEN, HDR_DESC);
	zassert_equal(fixture->transport_disconnects, 1U);
	zassert_equal(fixture->abort_rsp.count, 0U);
	zassert_equal(bt_obex_put(&fixture->client, true, NULL), -EBUSY);

	/* It was an early response: the abort response arrives while the transport is closing */
	TEST_RECV_EMPTY(fixture, BT_OBEX_RSP_CODE_SUCCESS);
	zassert_equal(fixture->abort_rsp.count, 1U);
	zassert_equal(fixture->put_rsp.count, 1U);
}

static ZTEST_F(obex_mid_operation, test_disconnect_during_put)
{
	zassert_ok(bt_obex_server_register(&fixture->server, NULL));
	test_server_connect(fixture);

	TEST_RECV(fixture, BT_OBEX_OPCODE_PUT, HDR_BODY);
	zassert_equal(fixture->put_reqs, 1U);
	zassert_ok(bt_obex_put_rsp(&fixture->server, BT_OBEX_RSP_CODE_CONTINUE, NULL));
	test_expect_sent(fixture, BT_OBEX_RSP_CODE_CONTINUE);

	TEST_RECV_EMPTY(fixture, BT_OBEX_OPCODE_DISCONN);
	zassert_equal(fixture->disconnect_reqs, 1U);
	test_expect_nothing_sent(fixture);

	/* The operation is over */
	zassert_equal(bt_obex_put_rsp(&fixture->server, BT_OBEX_RSP_CODE_CONTINUE, NULL), -EINVAL);
	test_expect_nothing_sent(fixture);

	zassert_ok(bt_obex_disconnect_rsp(&fixture->server, BT_OBEX_RSP_CODE_SUCCESS, NULL));
	test_expect_sent(fixture, BT_OBEX_RSP_CODE_SUCCESS);

	/* A new connection starts clean */
	test_server_connect(fixture);

	TEST_RECV(fixture, BT_OBEX_OPCODE_PUT_F, HDR_END_BODY);
	zassert_equal(fixture->put_reqs, 2U);
	zassert_ok(bt_obex_put_rsp(&fixture->server, BT_OBEX_RSP_CODE_SUCCESS, NULL));
	test_expect_sent(fixture, BT_OBEX_RSP_CODE_SUCCESS);
}

static ZTEST_F(obex_mid_operation, test_disconnect_during_get)
{
	zassert_ok(bt_obex_server_register(&fixture->server, NULL));
	test_server_connect(fixture);

	TEST_RECV_EMPTY(fixture, BT_OBEX_OPCODE_GET_F);
	zassert_equal(fixture->get_reqs, 1U);
	zassert_ok(bt_obex_get_rsp(&fixture->server, BT_OBEX_RSP_CODE_CONTINUE, NULL));
	test_expect_sent(fixture, BT_OBEX_RSP_CODE_CONTINUE);

	TEST_RECV_EMPTY(fixture, BT_OBEX_OPCODE_DISCONN);
	zassert_equal(fixture->disconnect_reqs, 1U);
	test_expect_nothing_sent(fixture);

	zassert_equal(bt_obex_get_rsp(&fixture->server, BT_OBEX_RSP_CODE_SUCCESS, NULL), -EINVAL);
	test_expect_nothing_sent(fixture);

	zassert_ok(bt_obex_disconnect_rsp(&fixture->server, BT_OBEX_RSP_CODE_SUCCESS, NULL));
	test_expect_sent(fixture, BT_OBEX_RSP_CODE_SUCCESS);
}

static ZTEST_F(obex_mid_operation, test_disconnect_with_request_unanswered)
{
	zassert_ok(bt_obex_server_register(&fixture->server, NULL));
	test_server_connect(fixture);

	TEST_RECV(fixture, BT_OBEX_OPCODE_PUT, HDR_BODY);
	zassert_equal(fixture->put_reqs, 1U);

	TEST_RECV_EMPTY(fixture, BT_OBEX_OPCODE_DISCONN);
	zassert_equal(fixture->disconnect_reqs, 1U);
	test_expect_nothing_sent(fixture);

	zassert_equal(bt_obex_put_rsp(&fixture->server, BT_OBEX_RSP_CODE_CONTINUE, NULL), -EINVAL);
	test_expect_nothing_sent(fixture);

	zassert_ok(bt_obex_disconnect_rsp(&fixture->server, BT_OBEX_RSP_CODE_SUCCESS, NULL));
	test_expect_sent(fixture, BT_OBEX_RSP_CODE_SUCCESS);
}

static ZTEST_F(obex_mid_operation, test_disconnect_while_rsp_allocates)
{
	zassert_ok(bt_obex_server_register(&fixture->server, NULL));

	ARRAY_FOR_EACH_PTR(test_server_ops, op) {
		test_server_connect(fixture);
		test_recv_server_op(fixture, op);

		/* The response is dropped and the DISCONNECT still gets its own */
		fixture->on_alloc = test_recv_disconnect;
		zassert_equal(op->rsp(&fixture->server, BT_OBEX_RSP_CODE_SUCCESS, NULL), -EINVAL,
			      "op 0x%02x", op->code);
		test_expect_nothing_sent(fixture);
		zassert_equal(net_buf_get_available(&test_pool), TEST_BUF_COUNT,
			      "op 0x%02x leaked its buffer", op->code);

		zassert_ok(bt_obex_disconnect_rsp(&fixture->server, BT_OBEX_RSP_CODE_SUCCESS, NULL),
			   "op 0x%02x", op->code);
		test_expect_sent(fixture, BT_OBEX_RSP_CODE_SUCCESS);
	}
}

static ZTEST_F(obex_mid_operation, test_disconnect_while_rsp_is_sent)
{
	zassert_ok(bt_obex_server_register(&fixture->server, NULL));

	ARRAY_FOR_EACH_PTR(test_server_ops, op) {
		test_server_connect(fixture);
		test_recv_server_op(fixture, op);

		/* The response is on its way and does not end the DISCONNECT */
		fixture->on_send = test_recv_disconnect;
		zassert_ok(op->rsp(&fixture->server, BT_OBEX_RSP_CODE_SUCCESS, NULL), "op 0x%02x",
			   op->code);
		test_expect_sent(fixture, BT_OBEX_RSP_CODE_SUCCESS);

		zassert_ok(bt_obex_disconnect_rsp(&fixture->server, BT_OBEX_RSP_CODE_SUCCESS, NULL),
			   "op 0x%02x", op->code);
		test_expect_sent(fixture, BT_OBEX_RSP_CODE_SUCCESS);
	}
}

static ZTEST_F(obex_mid_operation, test_second_disconnect)
{
	zassert_ok(bt_obex_server_register(&fixture->server, NULL));
	test_server_connect(fixture);

	TEST_RECV_EMPTY(fixture, BT_OBEX_OPCODE_DISCONN);
	zassert_equal(fixture->disconnect_reqs, 1U);

	TEST_RECV_EMPTY(fixture, BT_OBEX_OPCODE_DISCONN);
	zassert_equal(fixture->disconnect_reqs, 1U);
	test_expect_sent(fixture, BT_OBEX_RSP_CODE_FORBIDDEN);

	zassert_ok(bt_obex_disconnect_rsp(&fixture->server, BT_OBEX_RSP_CODE_SUCCESS, NULL));
	test_expect_sent(fixture, BT_OBEX_RSP_CODE_SUCCESS);
}

ZTEST_SUITE(obex_mid_operation, NULL, suite_setup, before_each, after_each, NULL);
