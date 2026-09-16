/* btp_opp.c - Bluetooth OPP Tester */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/obex.h>
#include <zephyr/bluetooth/classic/opp.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME btp_opp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

/* 2 MB push: OBEX header overhead per packet (opcode 1 + length 2 + Body hdr 3) */
#define PUSH_2MB_TOTAL_SIZE     (2U * 1024U * 1024U)
#define PUSH_2MB_HDR_OVERHEAD   6U
#define PUSH_2MB_FALLBACK_CHUNK 512U
/* Zero-filled source buffer for body data; chunk size is capped to this. */
#define PUSH_2MB_ZERO_BUF_SIZE  512U
static const uint8_t push_2mb_zero_buf[PUSH_2MB_ZERO_BUF_SIZE];

/* Current BR/EDR ACL connection (set by conn_cb, used before RFCOMM connects). */
static struct bt_conn *current_br_conn;

/* Single OPP Push Client instance. */
static struct bt_opp_client opp_client;
static uint16_t opp_client_mopl;

/* Autonomous 2 MB push state. */
static bool push_2mb_active;
static uint32_t push_2mb_remaining;
static struct k_work push_2mb_work;

/* Single OPP Push Server instance. */
static struct bt_opp_server opp_server;
static bool opp_server_in_use;

/* Prepared push-response state. */
static uint8_t push_rsp_code;
static bool push_rsp_override;

static struct bt_opp_server_rfcomm opp_rfcomm_server;

static void br_conn_connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	if (err != 0) {
		return;
	}

	if (bt_conn_get_info(conn, &info) != 0 ||
	    info.type != BT_CONN_TYPE_BR) {
		return;
	}

	current_br_conn = bt_conn_ref(conn);
}

static void br_conn_disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn == current_br_conn) {
		bt_conn_unref(current_br_conn);
		current_br_conn = NULL;
	}
}

BT_CONN_CB_DEFINE(opp_br_conn_cb) = {
	.connected    = br_conn_connected,
	.disconnected = br_conn_disconnected,
};

static void client_rfcomm_connected_cb(struct bt_conn *conn,
					struct bt_opp_client *client)
{
	ARG_UNUSED(conn);
	LOG_DBG("OPP client RFCOMM connected");

	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_CLIENT_TRANSPORT_CONNECTED, NULL, 0);
}

static void client_rfcomm_disconnected_cb(struct bt_opp_client *client)
{
	LOG_DBG("OPP client RFCOMM disconnected");

	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_CLIENT_TRANSPORT_DISCONNECTED, NULL, 0);
}

static void client_connect_cb(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			      uint8_t version, uint16_t mopl, struct net_buf *buf)
{
	struct btp_opp_client_connected_ev ev;

	LOG_DBG("OPP client connect rsp 0x%02x", rsp_code);

	opp_client_mopl = mopl;

	ev.rsp_code = (uint8_t)rsp_code;
	ev.version = version;
	ev.mopl = sys_cpu_to_le16(mopl);

	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_CLIENT_CONNECTED, &ev, sizeof(ev));
}

static void client_disconnect_cb(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
				 struct net_buf *buf)
{
	struct btp_opp_client_disconnected_ev ev;

	LOG_DBG("OPP client disconnect rsp 0x%02x", rsp_code);

	ev.rsp_code = (uint8_t)rsp_code;

	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_CLIENT_DISCONNECTED, &ev, sizeof(ev));
}

/*
 * Send the next Body (or End-of-Body) chunk of the 2 MB push.
 * Called from the system work queue so it runs outside any BT stack callback.
 */
static void push_2mb_send_next(struct k_work *work)
{
	struct net_buf *buf;
	uint16_t chunk;
	bool is_first_pkt;
	bool is_final_pkt;
	int err;

	if (!push_2mb_active) {
		return;
	}

	is_first_pkt = (push_2mb_remaining == PUSH_2MB_TOTAL_SIZE);

	{
		uint16_t effective_mopl = (opp_client_mopl > PUSH_2MB_HDR_OVERHEAD)
					  ? opp_client_mopl
					  : PUSH_2MB_FALLBACK_CHUNK + PUSH_2MB_HDR_OVERHEAD;
		uint16_t max_body = effective_mopl - PUSH_2MB_HDR_OVERHEAD;

		if (is_first_pkt) {
			/*
			 * The first PUT packet also carries Name, Type, and Length
			 * headers in addition to the Body header.  Deduct their
			 * encoded sizes from the available body room so that the
			 * total buf->len does not exceed mopl.
			 *
			 *   Name  : 3-byte hdr + 26-byte UTF-16 payload = 29
			 *   Type  : 3-byte hdr + 10-byte "image/bmp\0"  = 13
			 *   Length: 1-byte ID  +  4-byte uint32 value   =  5
			 *   Total extra                                  = 47
			 */
			const uint16_t first_pkt_extra = 29U + 13U + 5U;

			if (max_body > first_pkt_extra) {
				max_body -= first_pkt_extra;
			} else {
				max_body = 0U;
			}
		}

		if (max_body > PUSH_2MB_ZERO_BUF_SIZE) {
			max_body = PUSH_2MB_ZERO_BUF_SIZE;
		}

		chunk = (push_2mb_remaining <= max_body)
			? (uint16_t)push_2mb_remaining
			: max_body;
	}

	is_final_pkt = (chunk >= push_2mb_remaining);

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (buf == NULL) {
		LOG_ERR("push_2mb: failed to create PDU");
		goto error;
	}

	if (is_first_pkt) {
		static const uint8_t name_utf16[] = {
			0x00, 't', 0x00, 'e', 0x00, 's', 0x00, 't',
			0x00, '_', 0x00, '2', 0x00, 'm', 0x00, 'b',
			0x00, '.', 0x00, 'b', 0x00, 'm', 0x00, 'p',
			0x00, 0x00,
		};
		static const char type_str[] = "image/bmp";
		uint8_t type_len = (uint8_t)(sizeof(type_str));

		err = bt_obex_add_header_name(buf, (uint16_t)sizeof(name_utf16), name_utf16);
		if (err != 0) {
			LOG_ERR("push_2mb: add Name header failed (err %d)", err);
			net_buf_unref(buf);
			goto error;
		}

		err = bt_obex_add_header_type(buf, type_len, (const uint8_t *)type_str);
		if (err != 0) {
			LOG_ERR("push_2mb: add Type header failed (err %d)", err);
			net_buf_unref(buf);
			goto error;
		}

		err = bt_obex_add_header_len(buf, PUSH_2MB_TOTAL_SIZE);
		if (err != 0) {
			LOG_ERR("push_2mb: add Length header failed (err %d)", err);
			net_buf_unref(buf);
			goto error;
		}
	}

	{
		size_t avail = net_buf_tailroom(buf);
		uint16_t max_now = (avail > 3U) ? (uint16_t)(avail - 3U) : 0U;

		if (chunk > max_now) {
			chunk = max_now;
		}
		is_final_pkt = (chunk >= push_2mb_remaining);
	}

	if (is_final_pkt) {
		err = bt_obex_add_header_end_body(buf, chunk, push_2mb_zero_buf);
	} else {
		err = bt_obex_add_header_body(buf, chunk, push_2mb_zero_buf);
	}
	if (err != 0) {
		LOG_ERR("push_2mb: add Body header failed chunk=%u (err %d)", chunk, err);
		net_buf_unref(buf);
		goto error;
	}

	err = bt_opp_client_push(&opp_client, is_final_pkt, buf);
	if (err != 0) {
		LOG_ERR("push_2mb: bt_opp_client_push failed (err %d)", err);
		net_buf_unref(buf);
		goto error;
	}

	push_2mb_remaining -= chunk;
	LOG_DBG("push_2mb: sent chunk=%u remaining=%u is_final=%d",
		chunk, push_2mb_remaining, (int)is_final_pkt);
	return;

error:
	push_2mb_active = false;
	push_2mb_remaining = 0U;

	{
		struct btp_opp_client_push_ev ev;

		ev.rsp_code = BT_OPP_RSP_CODE_INTER_ERROR;
		tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_CLIENT_PUSH, &ev, sizeof(ev));
	}
}

static void client_push_cb(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			   struct net_buf *buf)
{
	struct btp_opp_client_push_ev ev;

	LOG_DBG("OPP client push rsp 0x%02x", rsp_code);

	if (push_2mb_active) {
		if (rsp_code == BT_OPP_RSP_CODE_CONTINUE && push_2mb_remaining > 0U) {
			k_work_submit(&push_2mb_work);
			return;
		}
		push_2mb_active = false;
		push_2mb_remaining = 0U;
	}

	ev.rsp_code = (uint8_t)rsp_code;
	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_CLIENT_PUSH, &ev, sizeof(ev));
}

static void client_pull_bcard_cb(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
				 struct net_buf *buf)
{
	struct btp_opp_client_pull_bcard_ev *ev;
	uint8_t *ev_data;
	const uint8_t *body = NULL;
	uint16_t body_len = 0;

	LOG_DBG("OPP client pull bcard rsp 0x%02x", rsp_code);

	if (buf != NULL) {
		uint16_t hdr_len = 0;

		if (bt_obex_get_header_end_body(buf, &hdr_len, &body) == 0) {
			body_len = hdr_len;
		} else {
			hdr_len = 0;
			if (bt_obex_get_header_body(buf, &hdr_len, &body) == 0) {
				body_len = hdr_len;
			}
		}
	}

	{
		size_t ev_len = sizeof(*ev) + body_len;

		tester_rsp_buffer_lock();
		tester_rsp_buffer_allocate(ev_len, &ev_data);

		if (ev_data == NULL) {
			LOG_ERR("Failed to allocate pull bcard event buffer");
			tester_rsp_buffer_unlock();
		} else {
			ev = (struct btp_opp_client_pull_bcard_ev *)ev_data;
			ev->rsp_code = (uint8_t)rsp_code;
			ev->data_len = sys_cpu_to_le16(body_len);
			if (body_len > 0 && body != NULL) {
				memcpy(ev->data, body, body_len);
			}

			tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_CLIENT_PULL_BCARD,
				     ev, ev_len);

			tester_rsp_buffer_free();
			tester_rsp_buffer_unlock();
		}
	}

	if (rsp_code == BT_OPP_RSP_CODE_CONTINUE) {
		struct net_buf *next_buf;
		int err;

		next_buf = bt_opp_client_create_pdu(client, NULL);
		if (next_buf == NULL) {
			LOG_ERR("pull bcard continuation: failed to create PDU");
			return;
		}

		err = bt_opp_client_pull_bcard(client, next_buf);
		if (err != 0) {
			LOG_ERR("pull bcard continuation: pull_bcard failed (err %d)", err);
			net_buf_unref(next_buf);
		}
	}
}

static void client_abort_cb(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			    struct net_buf *buf)
{
	struct btp_opp_client_abort_ev ev;

	LOG_DBG("OPP client abort rsp 0x%02x", rsp_code);

	ev.rsp_code = (uint8_t)rsp_code;
	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_CLIENT_ABORT, &ev, sizeof(ev));
}

static const struct bt_opp_client_cb opp_client_cb = {
	.rfcomm_connected    = client_rfcomm_connected_cb,
	.rfcomm_disconnected = client_rfcomm_disconnected_cb,
	.connect             = client_connect_cb,
	.disconnect          = client_disconnect_cb,
	.push                = client_push_cb,
	.pull_bcard          = client_pull_bcard_cb,
	.abort               = client_abort_cb,
};

static void server_rfcomm_connected_cb(struct bt_conn *conn,
					struct bt_opp_server *server)
{
	ARG_UNUSED(conn);
	LOG_DBG("OPP server RFCOMM connected");

	opp_server_in_use = true;

	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_SERVER_TRANSPORT_CONNECTED, NULL, 0);
}

static void server_rfcomm_disconnected_cb(struct bt_opp_server *server)
{
	LOG_DBG("OPP server RFCOMM disconnected");

	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_SERVER_TRANSPORT_DISCONNECTED, NULL, 0);

	opp_server_in_use = false;
}

static void server_connect_cb(struct bt_opp_server *server, uint8_t version,
			      uint16_t mopl, struct net_buf *buf)
{
	struct btp_opp_server_connected_ev ev;

	LOG_DBG("OPP server connect request version 0x%02x mopl %u", version, mopl);

	ev.version = version;
	ev.mopl = sys_cpu_to_le16(mopl);

	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_SERVER_CONNECTED, &ev, sizeof(ev));
}

static void server_disconnect_cb(struct bt_opp_server *server, struct net_buf *buf)
{
	LOG_DBG("OPP server disconnect request");

	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_SERVER_DISCONNECTED, NULL, 0);
}

static void server_push_cb(struct bt_opp_server *server, bool final, struct net_buf *buf)
{
	static bool is_first = true;
	uint8_t is_final = final ? 1U : 0U;

	/* Fast path for intermediate packets without an override. */
	if (!is_first && !is_final && !push_rsp_override) {
		(void)bt_opp_server_push_rsp(server, BT_OPP_RSP_CODE_CONTINUE, NULL);
		return;
	}

	{
		struct btp_opp_server_push_ev *ev;
		uint8_t *ev_data;
		size_t ev_len;

		const uint8_t *name = NULL;
		uint16_t name_len = 0;
		const uint8_t *type = NULL;
		uint16_t type_len = 0;
		const uint8_t *body = NULL;
		uint16_t body_len = 0;
		uint32_t total_length = 0;

		if (buf != NULL) {
			uint16_t hdr_len = 0;
			uint32_t obj_len = 0;

			if (bt_obex_get_header_name(buf, &hdr_len, &name) == 0) {
				name_len = hdr_len;
			}
			hdr_len = 0;
			if (bt_obex_get_header_type(buf, &hdr_len, &type) == 0) {
				type_len = hdr_len;
			}
			if (bt_obex_get_header_len(buf, &obj_len) == 0) {
				total_length = obj_len;
			}
			hdr_len = 0;
			if (is_final) {
				(void)bt_obex_get_header_end_body(buf, &hdr_len, &body);
			} else {
				(void)bt_obex_get_header_body(buf, &hdr_len, &body);
			}
			body_len = hdr_len;
		}

		ev_len = sizeof(*ev) + name_len + type_len + body_len;

		tester_rsp_buffer_lock();
		tester_rsp_buffer_allocate(ev_len, &ev_data);

		if (ev_data == NULL) {
			LOG_ERR("Failed to allocate push event buffer");
			tester_rsp_buffer_unlock();
		} else {
			ev = (struct btp_opp_server_push_ev *)ev_data;
			ev->total_length = sys_cpu_to_le32(total_length);
			ev->is_final = is_final;
			ev->name_len = (uint8_t)name_len;
			ev->type_len = (uint8_t)type_len;
			ev->body_len = sys_cpu_to_le16(body_len);

			uint8_t *ptr = ev->data;

			if (name_len > 0) {
				memcpy(ptr, name, name_len);
				ptr += name_len;
			}
			if (type_len > 0) {
				memcpy(ptr, type, type_len);
				ptr += type_len;
			}
			if (body_len > 0 && body != NULL) {
				memcpy(ptr, body, body_len);
			}

			tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_SERVER_PUSH, ev, ev_len);

			tester_rsp_buffer_free();
			tester_rsp_buffer_unlock();
		}

		is_first = (bool)is_final;
	}

	{
		uint8_t rsp_code;

		if (push_rsp_override) {
			rsp_code = push_rsp_code;
			push_rsp_override = false;
			LOG_DBG("OPP server push - using prepared rsp 0x%02x", rsp_code);
		} else {
			rsp_code = is_final ? BT_OPP_RSP_CODE_SUCCESS
					    : BT_OPP_RSP_CODE_CONTINUE;
			LOG_DBG("OPP server push - auto reply 0x%02x", rsp_code);
		}

		(void)bt_opp_server_push_rsp(server, rsp_code, NULL);
	}
}

static void server_pull_bcard_cb(struct bt_opp_server *server, struct net_buf *buf)
{
	LOG_DBG("OPP server pull bcard request");

	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_SERVER_PULL_BCARD, NULL, 0);
}

static void server_abort_cb(struct bt_opp_server *server, struct net_buf *buf)
{
	LOG_DBG("OPP server abort request - auto reply SUCCESS");

	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_SERVER_ABORT, NULL, 0);

	(void)bt_opp_server_abort_rsp(server, BT_OPP_RSP_CODE_SUCCESS, NULL);
}

static const struct bt_opp_server_cb opp_server_cb = {
	.rfcomm_connected    = server_rfcomm_connected_cb,
	.rfcomm_disconnected = server_rfcomm_disconnected_cb,
	.connect             = server_connect_cb,
	.disconnect          = server_disconnect_cb,
	.push                = server_push_cb,
	.pull_bcard          = server_pull_bcard_cb,
	.abort               = server_abort_cb,
};

static int rfcomm_server_accept(struct bt_conn *conn,
				struct bt_opp_server_rfcomm *rfcomm_server,
				struct bt_opp_server **out_server)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(rfcomm_server);
	if (opp_server_in_use) {
		LOG_ERR("OPP server already in use");
		return -ENOMEM;
	}

	int err = bt_opp_server_register(&opp_server, &opp_server_cb);

	if (err != 0) {
		LOG_ERR("bt_opp_server_register failed (err %d)", err);
		return err;
	}

	*out_server = &opp_server;
	return 0;
}

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	struct btp_opp_read_supported_commands_rp *rp = rsp;

	/* Octet 0 */
	tester_set_bit(rp->data, BTP_OPP_READ_SUPPORTED_COMMANDS);

	/* Octet 2 (commands 0x10 - 0x18) */
	tester_set_bit(rp->data, BTP_OPP_DISCOVER);
	tester_set_bit(rp->data, BTP_OPP_CLIENT_TRANSPORT_CONNECT);
	tester_set_bit(rp->data, BTP_OPP_CLIENT_TRANSPORT_DISCONNECT);
	tester_set_bit(rp->data, BTP_OPP_CLIENT_CONNECT);
	tester_set_bit(rp->data, BTP_OPP_CLIENT_DISCONNECT);
	tester_set_bit(rp->data, BTP_OPP_CLIENT_PUSH);
	tester_set_bit(rp->data, BTP_OPP_CLIENT_PULL_BCARD);
	tester_set_bit(rp->data, BTP_OPP_CLIENT_ABORT);
	tester_set_bit(rp->data, BTP_OPP_CLIENT_PUSH_2MB);

	/* Octet 4 (commands 0x20 - 0x26) */
	tester_set_bit(rp->data, BTP_OPP_SERVER_REGISTER);
	tester_set_bit(rp->data, BTP_OPP_SERVER_CONNECT_RSP);
	tester_set_bit(rp->data, BTP_OPP_SERVER_DISCONNECT_RSP);
	tester_set_bit(rp->data, BTP_OPP_SERVER_PUSH_RSP);
	tester_set_bit(rp->data, BTP_OPP_SERVER_PULL_BCARD_RSP);
	tester_set_bit(rp->data, BTP_OPP_SERVER_ABORT_RSP);
	tester_set_bit(rp->data, BTP_OPP_SERVER_PREPARE_PUSH_RSP);

	*rsp_len = sizeof(*rp) + 5;

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_transport_connect(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	const struct btp_opp_client_transport_connect_cmd *cp = cmd;
	int err;

	if (current_br_conn == NULL) {
		LOG_ERR("No BR/EDR connection found");
		return BTP_STATUS_FAILED;
	}

	memset(&opp_client, 0, sizeof(opp_client));

	err = bt_opp_client_connect_rfcomm(current_br_conn, &opp_client, &opp_client_cb,
					   cp->channel);
	if (err != 0) {
		LOG_ERR("bt_opp_client_connect_rfcomm failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_transport_disconnect(const void *cmd, uint16_t cmd_len,
					   void *rsp, uint16_t *rsp_len)
{
	int err = bt_opp_client_disconnect_rfcomm(&opp_client);

	if (err != 0) {
		LOG_ERR("bt_opp_client_disconnect_rfcomm failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_connect(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_opp_client_connect_cmd *cp = cmd;
	int err;

	err = bt_opp_client_connect(&opp_client, sys_le16_to_cpu(cp->mopl), NULL);
	if (err != 0) {
		LOG_ERR("bt_opp_client_connect failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_disconnect(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	int err = bt_opp_client_disconnect(&opp_client, NULL);

	if (err != 0) {
		LOG_ERR("bt_opp_client_disconnect failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_push(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_opp_client_push_cmd *cp = cmd;
	struct net_buf *buf;
	const uint8_t *ptr;
	uint16_t name_len;
	uint16_t type_len;
	uint16_t body_len;
	uint32_t total_length;
	int err;

	name_len = cp->name_len;
	type_len = cp->type_len;
	body_len = sys_le16_to_cpu(cp->body_len);
	total_length = sys_le32_to_cpu(cp->total_length);
	ptr = cp->data;

	LOG_DBG("OPP client push: name_len=%u type_len=%u body_len=%u total=%u is_final=%u",
		name_len, type_len, body_len, total_length, cp->is_final);

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (buf == NULL) {
		LOG_ERR("Failed to create OPP PDU");
		return BTP_STATUS_FAILED;
	}

	if (name_len > 0) {
		err = bt_obex_add_header_name(buf, name_len, ptr);
		if (err != 0) {
			LOG_ERR("Failed to add Name header (err %d)", err);
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		ptr += name_len;
	}

	if (type_len > 0) {
		err = bt_obex_add_header_type(buf, type_len, ptr);
		if (err != 0) {
			LOG_ERR("Failed to add Type header (err %d)", err);
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		ptr += type_len;
	}

	if (total_length != 0) {
		err = bt_obex_add_header_len(buf, total_length);
		if (err != 0) {
			LOG_ERR("Failed to add Length header (err %d)", err);
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
	}

	if (body_len > 0) {
		if (cp->is_final) {
			err = bt_obex_add_header_end_body(buf, body_len, ptr);
		} else {
			err = bt_obex_add_header_body(buf, body_len, ptr);
		}
		if (err != 0) {
			LOG_ERR("Failed to add Body header (err %d)", err);
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
	}

	err = bt_opp_client_push(&opp_client, (bool)cp->is_final, buf);
	if (err != 0) {
		LOG_ERR("bt_opp_client_push failed (err %d)", err);
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_pull_bcard(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	struct net_buf *buf;
	const char *type_str = BT_OPP_TYPE_VCARD;
	int err;

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (buf == NULL) {
		LOG_ERR("Failed to create OPP PDU");
		return BTP_STATUS_FAILED;
	}

	err = bt_obex_add_header_type(buf, strlen(type_str) + 1,
				      (const uint8_t *)type_str);
	if (err != 0) {
		LOG_ERR("Failed to add Type header (err %d)", err);
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	err = bt_opp_client_pull_bcard(&opp_client, buf);
	if (err != 0) {
		LOG_ERR("bt_opp_client_pull_bcard failed (err %d)", err);
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_abort(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	int err = bt_opp_client_abort(&opp_client, NULL);

	if (err != 0) {
		LOG_ERR("bt_opp_client_abort failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_push_2mb(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	if (push_2mb_active) {
		LOG_ERR("push_2mb already in progress");
		return BTP_STATUS_FAILED;
	}

	push_2mb_remaining = PUSH_2MB_TOTAL_SIZE;
	push_2mb_active = true;

	LOG_DBG("push_2mb: starting autonomous 2 MB push (mopl=%u)", opp_client_mopl);

	k_work_submit(&push_2mb_work);

	return BTP_STATUS_SUCCESS;
}

#define BTP_OPP_RFCOMM_CHANNEL 12U

static struct bt_sdp_attribute opp_sdp_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(0x1105)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8(BTP_OPP_RFCOMM_CHANNEL)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_OBEX)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(0x1105)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0100) /* v1.1 */
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_OPP_SDP_ATTR_SUPPORTED_FORMATS,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 14),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_VCARD_2_1)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_VCARD_3_0)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_VCAL_1_0)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_ICAL_2_0)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_VNOTE)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_VMESSAGE)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_ANY)
		},
		)
	),
};

static struct bt_sdp_record opp_sdp_record = BT_SDP_RECORD(opp_sdp_attrs);

static uint8_t server_register(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	int err;

	opp_rfcomm_server.server.rfcomm.channel = BTP_OPP_RFCOMM_CHANNEL;
	opp_rfcomm_server.accept = rfcomm_server_accept;

	err = bt_opp_server_rfcomm_register(&opp_rfcomm_server);
	if (err != 0) {
		LOG_ERR("bt_opp_server_rfcomm_register failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	err = bt_sdp_register_service(&opp_sdp_record);
	if (err != 0) {
		LOG_ERR("bt_sdp_register_service failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t server_connect_rsp(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_opp_server_connect_rsp_cmd *cp = cmd;
	int err;

	err = bt_opp_server_connect_rsp(&opp_server,
					sys_le16_to_cpu(cp->mopl),
					cp->rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("bt_opp_server_connect_rsp failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t server_disconnect_rsp(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_opp_server_disconnect_rsp_cmd *cp = cmd;
	int err;

	err = bt_opp_server_disconnect_rsp(&opp_server, cp->rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("bt_opp_server_disconnect_rsp failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t server_push_rsp(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_opp_server_push_rsp_cmd *cp = cmd;
	int err;

	err = bt_opp_server_push_rsp(&opp_server, cp->rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("bt_opp_server_push_rsp failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t server_pull_bcard_rsp(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_opp_server_pull_bcard_rsp_cmd *cp = cmd;
	struct net_buf *buf = NULL;
	uint16_t body_len;
	int err;

	body_len = sys_le16_to_cpu(cp->body_len);

	if (body_len > 0) {
		buf = bt_opp_server_create_pdu(&opp_server, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to create OPP server PDU");
			return BTP_STATUS_FAILED;
		}

		if (cp->is_final) {
			err = bt_obex_add_header_end_body(buf, body_len, cp->body);
		} else {
			err = bt_obex_add_header_body(buf, body_len, cp->body);
		}
		if (err != 0) {
			LOG_ERR("Failed to add Body header (err %d)", err);
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
	}

	err = bt_opp_server_pull_bcard_rsp(&opp_server, cp->rsp_code, buf);
	if (err != 0) {
		LOG_ERR("bt_opp_server_pull_bcard_rsp failed (err %d)", err);
		if (buf != NULL) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t server_abort_rsp(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_opp_server_abort_rsp_cmd *cp = cmd;
	int err;

	err = bt_opp_server_abort_rsp(&opp_server, cp->rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("bt_opp_server_abort_rsp failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t server_prepare_push_rsp(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	const struct btp_opp_server_prepare_push_rsp_cmd *cp = cmd;

	push_rsp_code     = cp->rsp_code;
	push_rsp_override = true;

	LOG_DBG("OPP server prepare push rsp: 0x%02x", cp->rsp_code);

	return BTP_STATUS_SUCCESS;
}

#define OPP_SDP_MAX_FORMATS 8

NET_BUF_POOL_DEFINE(opp_sdp_pool, 1,
		    BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static uint8_t opp_sdp_search_cb(struct bt_conn *conn,
				  struct bt_sdp_client_result *result,
				  const struct bt_sdp_discover_params *params)
{
	uint8_t ev_buf[sizeof(struct btp_opp_discovered_ev) + OPP_SDP_MAX_FORMATS];
	struct btp_opp_discovered_ev *ev = (struct btp_opp_discovered_ev *)ev_buf;
	uint16_t rfcomm_ch = 0;
	uint8_t formats[OPP_SDP_MAX_FORMATS];
	uint8_t formats_count = 0;

	if (result != NULL && result->resp_buf != NULL) {
		if (bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM,
					   &rfcomm_ch) != 0) {
			rfcomm_ch = 0;
		}

		struct bt_sdp_attribute attr;

		if (bt_sdp_get_attr(result->resp_buf,
				    BT_OPP_SDP_ATTR_SUPPORTED_FORMATS,
				    &attr) == 0 && attr.val.data != NULL) {
			const uint8_t *d = (const uint8_t *)attr.val.data;
			uint16_t remaining = attr.val.data_size;

			while (remaining >= 2 && formats_count < OPP_SDP_MAX_FORMATS) {
				formats[formats_count++] = d[1];
				d += 2;
				remaining -= 2;
			}
		}
	}

	ev->rfcomm_channel = (uint8_t)rfcomm_ch;
	ev->formats_count = formats_count;
	memcpy(ev->formats, formats, formats_count);

	tester_event(BTP_SERVICE_ID_OPP, BTP_OPP_EV_DISCOVERED,
		     ev, sizeof(*ev) + formats_count);

	return BT_SDP_DISCOVER_UUID_STOP;
}

static struct bt_sdp_discover_params opp_sdp_discover_params = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.uuid = BT_OPP_SDP_UUID,
	.func = opp_sdp_search_cb,
	.pool = &opp_sdp_pool,
};

static uint8_t sdp_discover(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	int err;

	if (current_br_conn == NULL) {
		LOG_ERR("No BR/EDR connection found");
		return BTP_STATUS_FAILED;
	}

	err = bt_sdp_discover(current_br_conn, &opp_sdp_discover_params);
	if (err != 0) {
		LOG_ERR("bt_sdp_discover failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_OPP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
	{
		.opcode = BTP_OPP_DISCOVER,
		.expect_len = 0,
		.func = sdp_discover,
	},
	{
		.opcode = BTP_OPP_CLIENT_TRANSPORT_CONNECT,
		.expect_len = sizeof(struct btp_opp_client_transport_connect_cmd),
		.func = client_transport_connect,
	},
	{
		.opcode = BTP_OPP_CLIENT_TRANSPORT_DISCONNECT,
		.expect_len = 0,
		.func = client_transport_disconnect,
	},
	{
		.opcode = BTP_OPP_CLIENT_CONNECT,
		.expect_len = sizeof(struct btp_opp_client_connect_cmd),
		.func = client_connect,
	},
	{
		.opcode = BTP_OPP_CLIENT_DISCONNECT,
		.expect_len = 0,
		.func = client_disconnect,
	},
	{
		.opcode = BTP_OPP_CLIENT_PUSH,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = client_push,
	},
	{
		.opcode = BTP_OPP_CLIENT_PULL_BCARD,
		.expect_len = 0,
		.func = client_pull_bcard,
	},
	{
		.opcode = BTP_OPP_CLIENT_ABORT,
		.expect_len = 0,
		.func = client_abort,
	},
	{
		.opcode = BTP_OPP_CLIENT_PUSH_2MB,
		.expect_len = 0,
		.func = client_push_2mb,
	},
	{
		.opcode = BTP_OPP_SERVER_REGISTER,
		.expect_len = 0,
		.func = server_register,
	},
	{
		.opcode = BTP_OPP_SERVER_CONNECT_RSP,
		.expect_len = sizeof(struct btp_opp_server_connect_rsp_cmd),
		.func = server_connect_rsp,
	},
	{
		.opcode = BTP_OPP_SERVER_DISCONNECT_RSP,
		.expect_len = sizeof(struct btp_opp_server_disconnect_rsp_cmd),
		.func = server_disconnect_rsp,
	},
	{
		.opcode = BTP_OPP_SERVER_PUSH_RSP,
		.expect_len = sizeof(struct btp_opp_server_push_rsp_cmd),
		.func = server_push_rsp,
	},
	{
		.opcode = BTP_OPP_SERVER_PULL_BCARD_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = server_pull_bcard_rsp,
	},
	{
		.opcode = BTP_OPP_SERVER_ABORT_RSP,
		.expect_len = sizeof(struct btp_opp_server_abort_rsp_cmd),
		.func = server_abort_rsp,
	},
	{
		.opcode = BTP_OPP_SERVER_PREPARE_PUSH_RSP,
		.expect_len = sizeof(struct btp_opp_server_prepare_push_rsp_cmd),
		.func = server_prepare_push_rsp,
	},
};

uint8_t tester_init_opp(void)
{
	k_work_init(&push_2mb_work, push_2mb_send_next);

	tester_register_command_handlers(BTP_SERVICE_ID_OPP, handlers,
					 ARRAY_SIZE(handlers));
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_opp(void)
{
	return BTP_STATUS_SUCCESS;
}
