/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/classic/obex.h>
#include <zephyr/bluetooth/uuid.h>

#include "test_link.h"

#define CONN_ID_A 0x00000001U
#define CONN_ID_B 0x00000002U

/* Target header values of the two sessions */
static const uint8_t target_a[BT_UUID_SIZE_128] = {
	0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
};
static const uint8_t target_b[BT_UUID_SIZE_128] = {
	0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
};

static union bt_obex_uuid uuid_a;
static union bt_obex_uuid uuid_b;

/* Two OBEX sessions on the same transport */
static struct test_client client_a;
static struct test_client client_b;
static struct test_server server_a;
static struct test_server server_b;

static void obex_action_multi_before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_link_setup();
	test_client_init(&client_a);
	test_client_init(&client_b);
	test_server_init(&server_a);
	test_server_init(&server_b);

	zassert_ok(bt_obex_make_uuid(&uuid_a, target_a, sizeof(target_a)));
	zassert_ok(bt_obex_make_uuid(&uuid_b, target_b, sizeof(target_b)));
	zassert_ok(bt_obex_server_register(&server_a.server, &uuid_a.u128));
	zassert_ok(bt_obex_server_register(&server_b.server, &uuid_b.u128));

	test_connect(&client_a, &server_a, target_a, CONN_ID_A);
	test_connect(&client_b, &server_b, target_b, CONN_ID_B);
}

static void obex_action_multi_after(void *fixture)
{
	ARG_UNUSED(fixture);

	test_link_cleanup();
}

ZTEST_SUITE(obex_action_multi, NULL, NULL, obex_action_multi_before, obex_action_multi_after,
	    NULL);

/* Another client cannot send a request until the ongoing action operation completes */
ZTEST(obex_action_multi, test_multi_busy)
{
	struct test_buf_state state;
	struct net_buf *buf;

	zassert_ok(test_send_action(&client_a, false));

	buf = test_action_req(&client_b);
	test_buf_state_save(buf, &state);
	zassert_equal(bt_obex_action(&client_b.client, true, buf), -EBUSY);
	test_buf_state_check(buf, &state);
	zassert_equal(test_link_pending(&cli_link), 1);

	test_link_pump(&cli_link, &srv_link);
	zassert_equal(server_a.action_count, 1);
	zassert_equal(server_b.action_count, 0);
	zassert_ok(bt_obex_action_rsp(&server_a.server, BT_OBEX_RSP_CODE_CONTINUE, NULL));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client_a.action_rsp, BT_OBEX_RSP_CODE_CONTINUE);
	zassert_equal(client_b.action_count, 0);

	/* Client A keeps the transport until its operation completes */
	zassert_equal(bt_obex_action(&client_b.client, true, buf), -EBUSY);
	test_buf_state_check(buf, &state);
	test_finish_action(&client_a, &server_a);

	zassert_ok(bt_obex_action(&client_b.client, true, buf));
	test_link_pump(&cli_link, &srv_link);
	zassert_equal(server_b.action_count, 1);
	zassert_true(server_b.action_final);
	zassert_equal(server_a.action_count, 2);
	zassert_ok(bt_obex_action_rsp(&server_b.server, BT_OBEX_RSP_CODE_SUCCESS, NULL));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client_b.action_count, 1);
	zassert_equal(client_b.action_rsp, BT_OBEX_RSP_CODE_SUCCESS);
	zassert_equal(client_a.action_count, 2);
	test_link_check_idle();
}

/* A failed first request releases the transport for the other client */
ZTEST(obex_action_multi, test_multi_first_req_failure)
{
	struct net_buf *buf;

	cli_link.send_err = -EIO;
	zassert_equal(test_send_action(&client_a, true), -EIO);
	cli_link.send_err = 0;
	test_finish_action(&client_b, &server_b);

	buf = test_buf_no_headroom();
	test_add_action_headers(buf, &client_a);
	zassert_equal(bt_obex_action(&client_a.client, true, buf), -EINVAL);
	net_buf_unref(buf);
	test_finish_action(&client_b, &server_b);

	cli_link.alloc_fail = true;
	zassert_equal(bt_obex_action(&client_a.client, true, NULL), -ENOBUFS);
	cli_link.alloc_fail = false;
	test_finish_action(&client_b, &server_b);

	zassert_equal(server_a.action_count, 0);
	test_finish_action(&client_a, &server_a);
	test_link_check_idle();
}

/* A failed following request keeps the transport for the ongoing operation */
ZTEST(obex_action_multi, test_multi_following_req_failure)
{
	test_continue_action(&client_a, &server_a);

	cli_link.send_err = -EIO;
	zassert_equal(test_send_action(&client_a, true), -EIO);
	cli_link.send_err = 0;

	zassert_equal(test_send_action(&client_b, true), -EBUSY);
	zassert_equal(test_link_pending(&cli_link), 0);

	test_finish_action(&client_a, &server_a);
	test_finish_action(&client_b, &server_b);
	test_link_check_idle();
}

/* A server can only respond to its own action requests */
ZTEST(obex_action_multi, test_multi_server_isolation)
{
	zassert_ok(test_send_action(&client_a, false));
	test_link_pump(&cli_link, &srv_link);
	zassert_equal(server_a.action_count, 1);
	zassert_equal(server_b.action_count, 0);

	zassert_equal(bt_obex_action_rsp(&server_b.server, BT_OBEX_RSP_CODE_CONTINUE, NULL),
		      -EINVAL);
	zassert_ok(bt_obex_action_rsp(&server_a.server, BT_OBEX_RSP_CODE_CONTINUE, NULL));
	zassert_equal(bt_obex_action_rsp(&server_a.server, BT_OBEX_RSP_CODE_CONTINUE, NULL),
		      -EPROTO);
	zassert_equal(test_link_pending(&srv_link), 1);
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client_a.action_rsp, BT_OBEX_RSP_CODE_CONTINUE);
	test_finish_action(&client_a, &server_a);

	zassert_ok(test_send_action(&client_b, true));
	test_link_pump(&cli_link, &srv_link);
	zassert_equal(server_b.action_count, 1);
	zassert_equal(bt_obex_action_rsp(&server_a.server, BT_OBEX_RSP_CODE_SUCCESS, NULL),
		      -EINVAL);
	zassert_ok(bt_obex_action_rsp(&server_b.server, BT_OBEX_RSP_CODE_SUCCESS, NULL));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client_b.action_rsp, BT_OBEX_RSP_CODE_SUCCESS);
	test_link_check_idle();
}

/* The SRM state of a previous GET operation of a server does not allow a second response */
ZTEST(obex_action_multi, test_multi_server_stale_srm_flags)
{
	test_srm_get(&client_b, &server_b);

	/* The other session is not affected */
	test_finish_action(&client_a, &server_a);

	zassert_ok(test_send_action(&client_b, true));
	test_link_pump(&cli_link, &srv_link);
	zassert_equal(server_b.action_count, 1);
	zassert_ok(bt_obex_action_rsp(&server_b.server, BT_OBEX_RSP_CODE_CONTINUE, NULL));
	zassert_equal(bt_obex_action_rsp(&server_b.server, BT_OBEX_RSP_CODE_CONTINUE, NULL),
		      -EPROTO);
	zassert_equal(test_link_pending(&srv_link), 1);
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client_b.action_rsp, BT_OBEX_RSP_CODE_CONTINUE);
	test_finish_action(&client_b, &server_b);
	test_link_check_idle();
}
