/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/classic/obex.h>

#include "test_link.h"

/* One OBEX session, connected without Target header */
static struct test_client client;
static struct test_server server;

static void obex_action_before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_link_setup();
	test_client_init(&client);
	test_server_init(&server);

	zassert_ok(bt_obex_server_register(&server.server, NULL));
	test_connect(&client, &server, NULL, 0);
}

static void obex_action_after(void *fixture)
{
	ARG_UNUSED(fixture);

	test_link_cleanup();
}

ZTEST_SUITE(obex_action, NULL, NULL, obex_action_before, obex_action_after, NULL);

/* A final request answered with Success completes the action operation */
ZTEST(obex_action, test_client_action)
{
	test_finish_action(&client, &server);

	/* The next action operation can start */
	test_finish_action(&client, &server);
	test_link_check_idle();
}

/* The next request is rejected until the response of the previous one is received */
ZTEST(obex_action, test_client_next_req_before_rsp)
{
	struct test_buf_state state;
	struct net_buf *buf;

	zassert_ok(test_send_action(&client, false));

	buf = test_action_req(&client);
	test_buf_state_save(buf, &state);
	zassert_equal(bt_obex_action(&client.client, false, buf), -EPROTO);
	test_buf_state_check(buf, &state);
	zassert_equal(test_link_pending(&cli_link), 1);

	test_link_pump(&cli_link, &srv_link);
	zassert_equal(server.action_count, 1);
	zassert_ok(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, NULL));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client.action_rsp, BT_OBEX_RSP_CODE_CONTINUE);

	/* The rejected buffer can be sent once the response has been received */
	zassert_ok(bt_obex_action(&client.client, true, buf));
	test_link_pump(&cli_link, &srv_link);
	zassert_equal(server.action_count, 2);
	zassert_true(server.action_final);
	zassert_ok(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_SUCCESS, NULL));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client.action_count, 2);
	zassert_equal(client.action_rsp, BT_OBEX_RSP_CODE_SUCCESS);
	test_link_check_idle();
}

/* A buffer without head room for the request header is rejected and left unchanged */
ZTEST(obex_action, test_client_no_headroom)
{
	struct test_buf_state state;
	struct net_buf *buf;

	buf = test_buf_no_headroom();
	test_add_action_headers(buf, &client);
	test_buf_state_save(buf, &state);
	zassert_equal(bt_obex_action(&client.client, true, buf), -EINVAL);
	test_buf_state_check(buf, &state);
	zassert_equal(test_link_pending(&cli_link), 0);
	net_buf_unref(buf);

	test_finish_action(&client, &server);
	test_link_check_idle();
}

/* A request that fails to be sent leaves the buffer and the request state unchanged */
ZTEST(obex_action, test_client_send_failure)
{
	struct test_buf_state state;
	struct net_buf *buf;

	test_continue_action(&client, &server);

	buf = test_action_req(&client);
	test_buf_state_save(buf, &state);
	cli_link.send_err = -EIO;
	zassert_equal(bt_obex_action(&client.client, true, buf), -EIO);
	cli_link.send_err = 0;
	test_buf_state_check(buf, &state);
	zassert_equal(test_link_pending(&cli_link), 0);

	/* The response of the previous request is still known as received */
	zassert_ok(bt_obex_action(&client.client, true, buf));
	test_link_pump(&cli_link, &srv_link);
	zassert_equal(server.action_count, 2);
	zassert_true(server.action_final);
	zassert_ok(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_SUCCESS, NULL));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client.action_rsp, BT_OBEX_RSP_CODE_SUCCESS);
	test_link_check_idle();
}

/* A failed buffer allocation does not change the request state */
ZTEST(obex_action, test_client_alloc_failure)
{
	/* First request of the operation */
	cli_link.alloc_fail = true;
	zassert_equal(bt_obex_action(&client.client, true, NULL), -ENOBUFS);
	cli_link.alloc_fail = false;
	zassert_equal(test_link_pending(&cli_link), 0);
	test_finish_action(&client, &server);

	/* Following request of the operation */
	test_continue_action(&client, &server);
	cli_link.alloc_fail = true;
	zassert_equal(bt_obex_action(&client.client, true, NULL), -ENOBUFS);
	cli_link.alloc_fail = false;
	zassert_equal(test_link_pending(&cli_link), 0);
	test_finish_action(&client, &server);
	test_link_check_idle();
}

/* Only one response can be sent for each received request */
ZTEST(obex_action, test_server_second_rsp)
{
	struct test_buf_state state;
	struct net_buf *buf;

	zassert_ok(test_send_action(&client, false));
	test_link_pump(&cli_link, &srv_link);
	zassert_equal(server.action_count, 1);

	zassert_ok(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, NULL));
	zassert_equal(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, NULL), -EPROTO);

	buf = test_buf();
	test_buf_state_save(buf, &state);
	zassert_equal(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, buf), -EPROTO);
	test_buf_state_check(buf, &state);
	net_buf_unref(buf);
	zassert_equal(test_link_pending(&srv_link), 1);

	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client.action_rsp, BT_OBEX_RSP_CODE_CONTINUE);
	test_finish_action(&client, &server);
	test_link_check_idle();
}

/* The SRM state of a previous GET operation does not allow a second response */
ZTEST(obex_action, test_server_stale_srm_flags)
{
	test_srm_get(&client, &server);

	zassert_ok(test_send_action(&client, true));
	test_link_pump(&cli_link, &srv_link);
	zassert_equal(server.action_count, 1);
	zassert_true(server.action_final);

	zassert_ok(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, NULL));
	zassert_equal(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, NULL), -EPROTO);
	zassert_equal(test_link_pending(&srv_link), 1);

	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client.action_rsp, BT_OBEX_RSP_CODE_CONTINUE);
	test_finish_action(&client, &server);
	test_link_check_idle();
}

/* Success cannot be sent for a request without final bit */
ZTEST(obex_action, test_server_success_without_final)
{
	struct test_buf_state state;
	struct net_buf *buf;

	zassert_ok(test_send_action(&client, false));
	test_link_pump(&cli_link, &srv_link);
	zassert_false(server.action_final);

	zassert_equal(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_SUCCESS, NULL), -EINVAL);

	buf = test_buf();
	test_buf_state_save(buf, &state);
	zassert_equal(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_SUCCESS, buf), -EINVAL);
	test_buf_state_check(buf, &state);
	net_buf_unref(buf);
	zassert_equal(test_link_pending(&srv_link), 0);

	zassert_ok(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, NULL));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client.action_rsp, BT_OBEX_RSP_CODE_CONTINUE);
	test_finish_action(&client, &server);
	test_link_check_idle();
}

/* No response can be sent without an action operation */
ZTEST(obex_action, test_server_no_action)
{
	zassert_equal(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_SUCCESS, NULL), -EINVAL);
	zassert_equal(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, NULL), -EINVAL);
	zassert_equal(test_link_pending(&srv_link), 0);

	/* After the action operation has completed */
	test_finish_action(&client, &server);
	zassert_equal(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_SUCCESS, NULL), -EINVAL);
	zassert_equal(test_link_pending(&srv_link), 0);
	test_link_check_idle();
}

/* A response that fails to be sent leaves the buffer and the response state unchanged */
ZTEST(obex_action, test_server_send_failure)
{
	struct test_buf_state state;
	struct net_buf *buf;

	/* Response of the first request */
	zassert_ok(test_send_action(&client, false));
	test_link_pump(&cli_link, &srv_link);

	buf = test_buf();
	test_buf_state_save(buf, &state);
	srv_link.send_err = -EIO;
	zassert_equal(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, buf), -EIO);
	srv_link.send_err = 0;
	test_buf_state_check(buf, &state);
	zassert_equal(test_link_pending(&srv_link), 0);

	zassert_ok(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, buf));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client.action_rsp, BT_OBEX_RSP_CODE_CONTINUE);

	/* Response of a following request */
	zassert_ok(test_send_action(&client, false));
	test_link_pump(&cli_link, &srv_link);

	buf = test_buf();
	test_buf_state_save(buf, &state);
	srv_link.send_err = -EIO;
	zassert_equal(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, buf), -EIO);
	srv_link.send_err = 0;
	test_buf_state_check(buf, &state);
	zassert_equal(test_link_pending(&srv_link), 0);

	zassert_ok(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, buf));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client.action_count, 2);
	zassert_equal(client.action_rsp, BT_OBEX_RSP_CODE_CONTINUE);

	test_finish_action(&client, &server);
	test_link_check_idle();
}

/* A buffer without head room for the response header is rejected and left unchanged */
ZTEST(obex_action, test_server_no_headroom)
{
	struct test_buf_state state;
	struct net_buf *buf;

	zassert_ok(test_send_action(&client, false));
	test_link_pump(&cli_link, &srv_link);

	buf = test_buf_no_headroom();
	test_buf_state_save(buf, &state);
	zassert_equal(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, buf), -EINVAL);
	test_buf_state_check(buf, &state);
	net_buf_unref(buf);
	zassert_equal(test_link_pending(&srv_link), 0);

	zassert_ok(bt_obex_action_rsp(&server.server, BT_OBEX_RSP_CODE_CONTINUE, NULL));
	test_link_pump(&srv_link, &cli_link);
	zassert_equal(client.action_rsp, BT_OBEX_RSP_CODE_CONTINUE);
	test_finish_action(&client, &server);
	test_link_check_idle();
}
