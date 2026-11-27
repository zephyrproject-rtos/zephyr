/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/ztest.h>

#include <host/classic/at.h>
#include <host/classic/avdtp_internal.h>
#include <host/conn_internal.h>

#define REQUEST_TID 4U

NET_BUF_POOL_DEFINE(rx_pool, 2, 64, 0, NULL);

static struct bt_conn conn;
static struct bt_avdtp session;
static struct bt_avdtp_sep sep;
static struct bt_avdtp_req request;
static struct bt_l2cap_server *server;
static struct bt_l2cap_chan *signal_chan;
static struct bt_l2cap_chan *media_chan;
static struct bt_l2cap_chan *outgoing_chan;
static struct bt_conn *outgoing_conn;
static uint16_t outgoing_psm;
static int accept_error;
static unsigned int connected_count;
static unsigned int disconnected_count;
static unsigned int media_connected_count;
static unsigned int media_disconnected_count;
static unsigned int media_received_count;
static unsigned int request_count;
static unsigned int at_finish_count;
static uint8_t request_status;
static uint8_t response[8];
static size_t response_len;
static struct net_buf *media_buf;
static enum at_result at_result;
static enum at_cme at_error;

/* These global guards cause a link failure if the production callbacks lose static linkage. */
void bt_avdtp_l2cap_connected(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);
	zassert_unreachable("Signaling connected callback resolved to the test guard");
}

void bt_avdtp_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);
	zassert_unreachable("Signaling disconnected callback resolved to the test guard");
}

int bt_avdtp_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	ARG_UNUSED(chan);
	ARG_UNUSED(buf);
	zassert_unreachable("Signaling receive callback resolved to the test guard");
	return -EINVAL;
}

int bt_l2cap_br_server_register(struct bt_l2cap_server *registered_server)
{
	server = registered_server;
	return 0;
}

int bt_l2cap_chan_connect(struct bt_conn *connection, struct bt_l2cap_chan *chan, uint16_t psm)
{
	outgoing_conn = connection;
	outgoing_chan = chan;
	outgoing_psm = psm;
	return 0;
}

static int accept(struct bt_conn *connection, struct bt_avdtp **accepted_session)
{
	zassert_equal(connection, &conn);
	if (accept_error != 0) {
		return accept_error;
	}
	*accepted_session = &session;
	return 0;
}

static void connected(struct bt_avdtp *connected_session)
{
	zassert_equal(connected_session, &session);
	connected_count++;
}

static void disconnected(struct bt_avdtp *disconnected_session)
{
	zassert_equal(disconnected_session, &session);
	disconnected_count++;
}

static int request_complete(struct bt_avdtp_req *completed_request, struct net_buf *buf)
{
	zassert_equal(completed_request, &request);
	zassert_is_null(session.req);
	request_count++;
	request_status = completed_request->status;
	if (buf != NULL) {
		zassert_true(buf->len <= sizeof(response));
		response_len = buf->len;
		memcpy(response, buf->data, buf->len);
	}
	return 0;
}

static void media_connected(struct bt_avdtp_sep *connected_sep)
{
	zassert_equal(connected_sep, &sep);
	media_connected_count++;
}

static void media_disconnected(struct bt_avdtp_sep *disconnected_sep)
{
	zassert_equal(disconnected_sep, &sep);
	media_disconnected_count++;
}

static void media_received(struct bt_avdtp_sep *received_sep, struct net_buf *buf)
{
	zassert_equal(received_sep, &sep);
	media_received_count++;
	media_buf = buf;
}

static int at_finish(struct at_client *client, enum at_result result, enum at_cme error)
{
	ARG_UNUSED(client);

	at_finish_count++;
	at_result = result;
	at_error = error;
	return 0;
}

static const struct bt_avdtp_ops_cb session_ops = {
	.connected = connected,
	.disconnected = disconnected,
};

static const struct bt_avdtp_sep_ops media_ops = {
	.connected = media_connected,
	.disconnected = media_disconnected,
	.media_data_cb = media_received,
};

static struct bt_avdtp_event_cb event_cb = {
	.accept = accept,
};

static struct net_buf *alloc_buf(const void *data, size_t len)
{
	struct net_buf *buf = net_buf_alloc(&rx_pool, K_NO_WAIT);

	zassert_not_null(buf);
	if (len > 0) {
		net_buf_add_mem(buf, data, len);
	}
	return buf;
}

static void set_request(void)
{
	request = (struct bt_avdtp_req){
		.sig = BT_AVDTP_DISCOVER,
		.tid = REQUEST_TID,
		.func = request_complete,
	};
	session.req = &request;
}

static void *setup(void)
{
	zassert_ok(bt_avdtp_register(&event_cb));
	zassert_not_null(server);
	sep.ops = &media_ops;
	zassert_ok(bt_avdtp_register_sep(BT_AVDTP_AUDIO, A2DP_SRC_ROLE, &sep));
	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&session, 0, sizeof(session));
	session.ops = &session_ops;
	accept_error = 0;
	zassert_ok(server->accept(&conn, server, &signal_chan));
	session.current_sep = &sep;
	zassert_ok(server->accept(&conn, server, &media_chan));
	connected_count = 0;
	disconnected_count = 0;
	media_connected_count = 0;
	media_disconnected_count = 0;
	media_received_count = 0;
	request_count = 0;
	at_finish_count = 0;
	response_len = 0;
	media_buf = NULL;
	outgoing_chan = NULL;
	outgoing_conn = NULL;
}

static void after(void *fixture)
{
	struct k_work_sync sync;

	ARG_UNUSED(fixture);

	k_work_cancel_sync(&session._release_work, &sync);
	if (sep.sep_info.inuse != 0) {
		media_chan->ops->disconnected(media_chan);
	}
	k_work_cancel_delayable_sync(&session.timeout_work, &sync);
	k_work_cancel_delayable_sync(&sep._delay_work, &sync);
	net_buf_drop(&session.reasm_buf);
	sep.session = NULL;
}

ZTEST_SUITE(classic_static_linkage, NULL, setup, before, after, NULL);

ZTEST(classic_static_linkage, test_signal_connected)
{
	signal_chan->ops->connected(signal_chan);
	zassert_equal(connected_count, 1);
}

ZTEST(classic_static_linkage, test_signal_disconnected_clears_pending_request)
{
	set_request();
	session.reasm_buf = alloc_buf(NULL, 0);

	signal_chan->ops->disconnected(signal_chan);

	zassert_equal(disconnected_count, 1);
	zassert_equal(request_count, 1);
	zassert_equal(request_status, BT_AVDTP_BAD_STATE);
	zassert_is_null(session.req);
	zassert_is_null(session.reasm_buf);
	zassert_equal(response_len, 0);
}

ZTEST(classic_static_linkage, test_signal_receive_completes_request)
{
	static const uint8_t packet[] = {
		(REQUEST_TID << 4) | BT_AVDTP_ACCEPT, BT_AVDTP_DISCOVER, 0x04, 0x00,
	};
	struct net_buf *buf = alloc_buf(packet, sizeof(packet));

	set_request();
	zassert_true(k_work_reschedule(&session.timeout_work, K_SECONDS(30)) >= 0);

	zassert_ok(signal_chan->ops->recv(signal_chan, buf));
	zassert_equal(request_count, 1);
	zassert_equal(request_status, BT_AVDTP_SUCCESS);
	zassert_equal(response_len, sizeof(packet) - sizeof(struct bt_avdtp_single_sig_hdr));
	zassert_mem_equal(response, packet + sizeof(struct bt_avdtp_single_sig_hdr), response_len);
	zassert_false(k_work_delayable_is_pending(&session.timeout_work));
	net_buf_unref(buf);
}

ZTEST(classic_static_linkage, test_signal_receive_ignores_unexpected_tid)
{
	static const uint8_t packet[] = {
		((REQUEST_TID + 1U) << 4) | BT_AVDTP_ACCEPT, BT_AVDTP_DISCOVER,
	};
	struct net_buf *buf = alloc_buf(packet, sizeof(packet));

	set_request();
	zassert_ok(signal_chan->ops->recv(signal_chan, buf));
	zassert_equal(request_count, 0);
	zassert_equal(session.req, &request);
	net_buf_unref(buf);
}

ZTEST(classic_static_linkage, test_signal_receive_rejects_truncated_header)
{
	static const uint8_t packet[] = {BT_AVDTP_ACCEPT};

	set_request();
	for (size_t len = 0; len < sizeof(struct bt_avdtp_single_sig_hdr); len++) {
		struct net_buf *buf = alloc_buf(packet, len);

		zassert_equal(signal_chan->ops->recv(signal_chan, buf), -EINVAL);
		zassert_equal(session.req, &request);
		net_buf_unref(buf);
	}
	zassert_equal(request_count, 0);
}

ZTEST(classic_static_linkage, test_media_connected_completes_pending_request)
{
	set_request();
	media_chan->ops->connected(media_chan);

	zassert_equal(media_connected_count, 1);
	zassert_equal(request_count, 1);
	zassert_equal(request_status, BT_AVDTP_SUCCESS);
	zassert_equal(sep.sep_info.inuse, 1);
}

ZTEST(classic_static_linkage, test_media_disconnected)
{
	media_chan->ops->connected(media_chan);
	media_chan->ops->disconnected(media_chan);

	zassert_equal(media_disconnected_count, 1);
	zassert_equal(sep.sep_info.inuse, 0);
}

ZTEST(classic_static_linkage, test_media_receive_forwards_buffer)
{
	static const uint8_t payload[] = {0x80, 0x60, 0x00, 0x01};
	struct net_buf *buf = alloc_buf(payload, sizeof(payload));

	zassert_ok(media_chan->ops->recv(media_chan, buf));
	zassert_equal(media_received_count, 1);
	zassert_equal(media_buf, buf);
	zassert_equal(buf->len, sizeof(payload));
	zassert_mem_equal(buf->data, payload, sizeof(payload));
	net_buf_unref(buf);
}

ZTEST(classic_static_linkage, test_accept_propagates_upper_layer_error)
{
	struct bt_l2cap_chan *chan = NULL;

	accept_error = -ENOMEM;
	zassert_equal(server->accept(&conn, server, &chan), -ENOMEM);
	zassert_is_null(chan);
}

ZTEST(classic_static_linkage, test_outgoing_connect_uses_signal_callbacks)
{
	session.br_chan.chan.conn = NULL;
	zassert_ok(bt_avdtp_connect(&conn, &session));
	zassert_equal(outgoing_conn, &conn);
	zassert_equal(outgoing_chan, &session.br_chan.chan);
	zassert_equal(outgoing_psm, BT_L2CAP_PSM_AVDTP);

	outgoing_chan->ops->connected(outgoing_chan);
	zassert_equal(connected_count, 1);
}

ZTEST(classic_static_linkage, test_at_cme_error)
{
	static const char packet[] = "\r\n+CME ERROR: 3\r\n";
	struct at_client client = {0};
	struct net_buf *buf = alloc_buf(packet, sizeof(packet) - 1U);

	at_register(&client, NULL, at_finish);
	zassert_ok(at_parse_input(&client, buf));
	zassert_equal(at_finish_count, 1);
	zassert_equal(at_result, AT_RESULT_CME_ERROR);
	zassert_equal(at_error, CME_ERROR_OPERATION_NOT_ALLOWED);
	net_buf_unref(buf);
}
