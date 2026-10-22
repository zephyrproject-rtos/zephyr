/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/classic/obex.h>

#include "host/classic/obex_internal.h"

#include "test_link.h"

#define TEST_BUF_COUNT     8U
#define TEST_RAW_BUF_COUNT 2U
#define TEST_BUF_SIZE      255U

/* Value of the SRM header to enable SRM */
#define TEST_SRM_ENABLE 0x01

NET_BUF_POOL_FIXED_DEFINE(test_tx_pool, TEST_BUF_COUNT, TEST_BUF_SIZE, 0, NULL);
/* Buffers from this pool have no head room */
NET_BUF_POOL_FIXED_DEFINE(test_raw_pool, TEST_RAW_BUF_COUNT, TEST_BUF_SIZE, 0, NULL);

struct test_link cli_link;
struct test_link srv_link;

static size_t tx_pool_avail;
static size_t raw_pool_avail;

/* Name "a" and Dest Name "b", null-terminated UTF-16BE */
static const uint8_t test_name[] = {0x00, 'a', 0x00, 0x00};
static const uint8_t test_dest_name[] = {0x00, 'b', 0x00, 0x00};

static struct net_buf *alloc_reserved(void)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&test_tx_pool, K_NO_WAIT);
	if (buf != NULL) {
		net_buf_reserve(buf, BT_OBEX_SEND_BUF_RESERVE);
	}

	return buf;
}

static struct net_buf *link_alloc_buf(struct bt_obex *obex, struct net_buf_pool *pool)
{
	struct test_link *link = CONTAINER_OF(obex, struct test_link, obex);

	ARG_UNUSED(pool);

	if (link->alloc_fail) {
		return NULL;
	}

	return alloc_reserved();
}

static int link_send(struct bt_obex *obex, struct net_buf *buf)
{
	struct test_link *link = CONTAINER_OF(obex, struct test_link, obex);

	if (link->send_err != 0) {
		return link->send_err;
	}

	k_fifo_put(&link->txq, buf);
	link->pending++;
	return 0;
}

static int link_disconnect(struct bt_obex *obex)
{
	ARG_UNUSED(obex);

	zassert_unreachable("Unexpected transport disconnection");
	return 0;
}

static const struct bt_obex_transport_ops link_ops = {
	.alloc_buf = link_alloc_buf,
	.send = link_send,
	.disconnect = link_disconnect,
};

static void link_init(struct test_link *link)
{
	memset(link, 0, sizeof(*link));
	k_fifo_init(&link->txq);
	link->obex.rx.mtu = TEST_MTU;
	link->obex.tx.mtu = TEST_MTU;

	bt_obex_reg_transport(&link->obex, &link_ops);
	zassert_ok(bt_obex_transport_connected(&link->obex));
}

static void link_drain(struct test_link *link)
{
	struct net_buf *buf;

	while ((buf = k_fifo_get(&link->txq, K_NO_WAIT)) != NULL) {
		link->pending--;
		net_buf_unref(buf);
	}
}

void test_link_setup(void)
{
	tx_pool_avail = net_buf_get_available(&test_tx_pool);
	raw_pool_avail = net_buf_get_available(&test_raw_pool);

	link_init(&cli_link);
	link_init(&srv_link);
}

void test_link_cleanup(void)
{
	link_drain(&cli_link);
	link_drain(&srv_link);
}

void test_link_check_idle(void)
{
	zassert_equal(test_link_pending(&cli_link), 0, "Unexpected client packet");
	zassert_equal(test_link_pending(&srv_link), 0, "Unexpected server packet");
	zassert_equal(net_buf_get_available(&test_tx_pool), tx_pool_avail, "Leaked buffer");
	zassert_equal(net_buf_get_available(&test_raw_pool), raw_pool_avail, "Leaked buffer");
}

size_t test_link_pending(struct test_link *link)
{
	return link->pending;
}

void test_link_pump(struct test_link *from, struct test_link *to)
{
	struct net_buf *buf;
	int err;

	buf = k_fifo_get(&from->txq, K_NO_WAIT);
	zassert_not_null(buf, "No packet to deliver");
	from->pending--;

	/* The OBEX layer does not keep the received buffer */
	err = bt_obex_recv(&to->obex, buf);
	net_buf_unref(buf);
	zassert_ok(err, "Packet not accepted (err %d)", err);
}

struct net_buf *test_buf(void)
{
	struct net_buf *buf = alloc_reserved();

	zassert_not_null(buf, "No test buffer");
	return buf;
}

struct net_buf *test_buf_no_headroom(void)
{
	struct net_buf *buf = net_buf_alloc(&test_raw_pool, K_NO_WAIT);

	zassert_not_null(buf, "No test buffer");
	zassert_equal(net_buf_headroom(buf), 0);
	return buf;
}

void test_add_action_headers(struct net_buf *buf, const struct test_client *c)
{
	if (c->use_conn_id) {
		zassert_ok(bt_obex_add_header_conn_id(buf, c->conn_id));
	}

	zassert_ok(bt_obex_add_header_action_id(buf, BT_OBEX_ACTION_COPY));
	zassert_ok(bt_obex_add_header_name(buf, sizeof(test_name), test_name));
	zassert_ok(bt_obex_add_header_dest_name(buf, sizeof(test_dest_name), test_dest_name));
}

struct net_buf *test_action_req(const struct test_client *c)
{
	struct net_buf *buf = test_buf();

	test_add_action_headers(buf, c);
	return buf;
}

void test_buf_state_save(const struct net_buf *buf, struct test_buf_state *state)
{
	state->data = buf->data;
	state->len = buf->len;
}

void test_buf_state_check(const struct net_buf *buf, const struct test_buf_state *state)
{
	zassert_equal_ptr(buf->data, state->data, "Buffer data changed");
	zassert_equal(buf->len, state->len, "Buffer length changed");
}

static void client_connect(struct bt_obex_client *client, uint8_t rsp_code, uint8_t version,
			   uint16_t mopl, struct net_buf *buf)
{
	struct test_client *c = CONTAINER_OF(client, struct test_client, client);

	c->connect_count++;
	c->connect_rsp = rsp_code;
}

static void client_disconnect(struct bt_obex_client *client, uint8_t rsp_code,
			      struct net_buf *buf)
{
}

static void client_get(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct test_client *c = CONTAINER_OF(client, struct test_client, client);

	c->get_count++;
	c->get_rsp = rsp_code;
}

static void client_action(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct test_client *c = CONTAINER_OF(client, struct test_client, client);

	c->action_count++;
	c->action_rsp = rsp_code;
}

static const struct bt_obex_client_ops test_client_ops = {
	.connect = client_connect,
	.disconnect = client_disconnect,
	.get = client_get,
	.action = client_action,
};

static void server_connect(struct bt_obex_server *server, uint8_t version, uint16_t mopl,
			   struct net_buf *buf)
{
	struct test_server *s = CONTAINER_OF(server, struct test_server, server);

	s->connect_count++;
}

static void server_disconnect(struct bt_obex_server *server, struct net_buf *buf)
{
}

static void server_get(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct test_server *s = CONTAINER_OF(server, struct test_server, server);

	s->get_count++;
	s->get_final = final;
}

static void server_action(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct test_server *s = CONTAINER_OF(server, struct test_server, server);

	s->action_count++;
	s->action_final = final;
	if (bt_obex_get_header_action_id(buf, &s->action_id) != 0) {
		s->action_id = UINT8_MAX;
	}
}

static const struct bt_obex_server_ops test_server_ops = {
	.connect = server_connect,
	.disconnect = server_disconnect,
	.get = server_get,
	.action = server_action,
};

void test_client_init(struct test_client *c)
{
	memset(c, 0, sizeof(*c));
	c->client.obex = &cli_link.obex;
	c->client.ops = &test_client_ops;
}

void test_server_init(struct test_server *s)
{
	memset(s, 0, sizeof(*s));
	s->server.obex = &srv_link.obex;
	s->server.ops = &test_server_ops;
}

void test_connect(struct test_client *c, struct test_server *s, const uint8_t *target,
		  uint32_t conn_id)
{
	struct net_buf *buf = NULL;

	if (target != NULL) {
		buf = test_buf();
		zassert_ok(bt_obex_add_header_target(buf, BT_UUID_SIZE_128, target));
	}

	zassert_ok(bt_obex_connect(&c->client, TEST_MTU, buf));
	test_link_pump(&cli_link, &srv_link);
	zassert_equal(s->connect_count, 1, "Connect request not received by the server");

	buf = NULL;
	if (target != NULL) {
		buf = test_buf();
		zassert_ok(bt_obex_add_header_who(buf, BT_UUID_SIZE_128, target));
		zassert_ok(bt_obex_add_header_conn_id(buf, conn_id));
	}

	zassert_ok(bt_obex_connect_rsp(&s->server, BT_OBEX_RSP_CODE_SUCCESS, TEST_MTU, buf));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(c->connect_count, 1, "Connect response not received by the client");
	zassert_equal(c->connect_rsp, BT_OBEX_RSP_CODE_SUCCESS);

	c->conn_id = conn_id;
	c->use_conn_id = (target != NULL);
}

int test_send_action(struct test_client *c, bool final)
{
	struct net_buf *buf = test_action_req(c);
	int err;

	err = bt_obex_action(&c->client, final, buf);
	if (err != 0) {
		net_buf_unref(buf);
	}

	return err;
}

void test_continue_action(struct test_client *c, struct test_server *s)
{
	size_t c_count = c->action_count;
	size_t s_count = s->action_count;

	zassert_ok(test_send_action(c, false));
	test_link_pump(&cli_link, &srv_link);
	zassert_equal(s->action_count, s_count + 1, "Action request not received by the server");
	zassert_false(s->action_final);
	zassert_equal(s->action_id, BT_OBEX_ACTION_COPY);

	zassert_ok(bt_obex_action_rsp(&s->server, BT_OBEX_RSP_CODE_CONTINUE, NULL));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(c->action_count, c_count + 1, "Action response not received by the client");
	zassert_equal(c->action_rsp, BT_OBEX_RSP_CODE_CONTINUE);
}

void test_finish_action(struct test_client *c, struct test_server *s)
{
	size_t c_count = c->action_count;
	size_t s_count = s->action_count;

	zassert_ok(test_send_action(c, true));
	test_link_pump(&cli_link, &srv_link);
	zassert_equal(s->action_count, s_count + 1, "Action request not received by the server");
	zassert_true(s->action_final);
	zassert_equal(s->action_id, BT_OBEX_ACTION_COPY);

	zassert_ok(bt_obex_action_rsp(&s->server, BT_OBEX_RSP_CODE_SUCCESS, NULL));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(c->action_count, c_count + 1, "Action response not received by the client");
	zassert_equal(c->action_rsp, BT_OBEX_RSP_CODE_SUCCESS);
}

void test_srm_get(struct test_client *c, struct test_server *s)
{
	size_t c_count = c->get_count;
	size_t s_count = s->get_count;
	struct net_buf *buf;

	buf = test_buf();
	if (c->use_conn_id) {
		zassert_ok(bt_obex_add_header_conn_id(buf, c->conn_id));
	}
	zassert_ok(bt_obex_add_header_srm(buf, TEST_SRM_ENABLE));
	zassert_ok(bt_obex_get(&c->client, true, buf));
	test_link_pump(&cli_link, &srv_link);
	zassert_equal(s->get_count, s_count + 1, "Get request not received by the server");
	zassert_true(s->get_final);

	buf = test_buf();
	zassert_ok(bt_obex_add_header_srm(buf, TEST_SRM_ENABLE));
	zassert_ok(bt_obex_get_rsp(&s->server, BT_OBEX_RSP_CODE_SUCCESS, buf));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(c->get_count, c_count + 1, "Get response not received by the client");
	zassert_equal(c->get_rsp, BT_OBEX_RSP_CODE_SUCCESS);
}
