/** @file
 * @brief Bluetooth BIP shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */
/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <stdlib.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/bip.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define BIP_MOPL CONFIG_BT_GOEP_RFCOMM_MTU

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_RFCOMM_BUF_SIZE(BIP_MOPL),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static uint8_t add_head_buffer[BIP_MOPL];

struct bt_bip_app {
	struct bt_bip_client client;
	struct bt_bip bip;
	struct bt_bip_server server;
	struct bt_conn *conn;
	struct net_buf *tx_buf;
	uint16_t client_mopl;
	uint16_t server_mopl;
	uint32_t conn_id;
};

static struct bt_bip_app bip_app;

static struct bt_bip_rfcomm_server rfcomm_server;
static struct bt_bip_l2cap_server l2cap_server;

#define TLV_COUNT       6
#define TLV_BUFFER_SIZE 64

static struct bt_obex_tlv tlvs[TLV_COUNT];
static uint8_t tlv_buffers[TLV_COUNT][TLV_BUFFER_SIZE];
static uint8_t tlv_count;

static uint8_t bip_supported_caps;
static uint16_t bip_supported_features;
static uint32_t bip_supported_functions;
static uint64_t bip_max_memory_space = 1024; /* 1024 bytes */

static struct bt_sdp_attribute bip_responder_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_IMAGING_RESPONDER_SVCLASS)
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
				&rfcomm_server.server.rfcomm.channel
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
	BT_SDP_SERVICE_NAME("imaging"),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_IMAGING_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	{
		BT_SDP_ATTR_SUPPORTED_CAPABILITIES,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			&bip_supported_caps,
		},
	},
	{
		BT_SDP_ATTR_SUPPORTED_FEATURES,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			&bip_supported_features,
		},
	},
	{
		BT_SDP_ATTR_SUPPORTED_FUNCTIONS,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT32),
			&bip_supported_functions,
		},
	},
	{
		BT_SDP_ATTR_TOTAL_IMAGING_DATA_CAPACITY,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT64),
			&bip_max_memory_space,
		},
	},
	{
		BT_SDP_ATTR_GOEP_L2CAP_PSM,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			&l2cap_server.server.l2cap.psm,
		},
	},
};

static struct bt_sdp_record bip_responder_rec = BT_SDP_RECORD(bip_responder_attrs);

static struct bt_bip_app *bip_alloc(struct bt_conn *conn)
{
	if (bip_app.conn != NULL) {
		return NULL;
	}

	bip_app.conn = conn;
	return &bip_app;
}

static void bip_free(struct bt_bip_app *bip)
{
	bip->conn = NULL;
}

static void bip_transport_connected(struct bt_conn *conn, struct bt_bip *bip)
{
	bt_shell_print("BIP %p transport connected on %p", bip, conn);
}

static void bip_transport_disconnected(struct bt_bip *bip)
{
	struct bt_bip_app *g_app = CONTAINER_OF(bip, struct bt_bip_app, bip);

	bip_free(g_app);
	bt_shell_print("BIP %p transport disconnected", bip);
}

static struct bt_bip_transport_ops bip_transport_ops = {
	.connected = bip_transport_connected,
	.disconnected = bip_transport_disconnected,
};

static bool bip_parse_tlvs_cb(struct bt_obex_tlv *tlv, void *user_data)
{
	bt_shell_print("T %02x L %d", tlv->type, tlv->data_len);
	bt_shell_hexdump(tlv->data, tlv->data_len);

	return true;
}

static bool bip_parse_headers_cb(struct bt_obex_hdr *hdr, void *user_data)
{
	bt_shell_print("HI %02x Len %d", hdr->id, hdr->len);

	switch (hdr->id) {
	case BT_OBEX_HEADER_ID_APP_PARAM:
	case BT_OBEX_HEADER_ID_AUTH_CHALLENGE:
	case BT_OBEX_HEADER_ID_AUTH_RSP: {
		int err;

		err = bt_obex_tlv_parse(hdr->len, hdr->data, bip_parse_tlvs_cb, NULL);
		if (err != 0) {
			bt_shell_error("Fail to parse BIP TLV triplet");
		}
	} break;
	default:
		bt_shell_hexdump(hdr->data, hdr->len);
		break;
	}

	return true;
}

static int bip_parse_headers(struct net_buf *buf)
{
	int err;

	if (buf == NULL) {
		return 0;
	}

	err = bt_obex_header_parse(buf, bip_parse_headers_cb, NULL);
	if (err != 0) {
		bt_shell_warn("Fail to parse BIP Headers");
	}

	return err;
}

static void bip_server_connect(struct bt_bip_server *server, uint8_t version, uint16_t mopl,
			       struct net_buf *buf)
{
	bt_shell_print("BIP server %p conn req, version %02x, mopl %04x", server, version, mopl);
	bip_parse_headers(buf);
	bip_app.client_mopl = mopl;
}

static void bip_server_disconnect(struct bt_bip_server *server, struct net_buf *buf)
{
	bt_shell_print("BIP server %p disconn req", server);
	bip_parse_headers(buf);
}

static void bip_server_get_caps(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("BIP server %p get_caps req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static void bip_server_get_image_list(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("BIP server %p get_image_list req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static void bip_server_get_image_properties(struct bt_bip_server *server, bool final,
					    struct net_buf *buf)
{
	bt_shell_print("BIP server %p get_image_properties req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static void bip_server_get_image(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("BIP server %p get_image req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static void bip_server_get_linked_thumbnail(struct bt_bip_server *server, bool final,
					    struct net_buf *buf)
{
	bt_shell_print("BIP server %p get_linked_thumbnail req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static void bip_server_get_linked_attachment(struct bt_bip_server *server, bool final,
					     struct net_buf *buf)
{
	bt_shell_print("BIP server %p get_linked_attachment req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static void bip_server_get_partial_image(struct bt_bip_server *server, bool final,
					 struct net_buf *buf)
{
	bt_shell_print("BIP server %p get_partial_image req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static void bip_server_get_monitoring_image(struct bt_bip_server *server, bool final,
					    struct net_buf *buf)
{
	bt_shell_print("BIP server %p get_monitoring_image req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static void bip_server_get_status(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("BIP server %p get_status req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static bool server_put_image_final;
static void bip_server_put_image(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	server_put_image_final = final;
	bt_shell_print("BIP server %p put_image req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static bool server_put_linked_thumbnail_final;
static void bip_server_put_linked_thumbnail(struct bt_bip_server *server, bool final,
					    struct net_buf *buf)
{
	server_put_linked_thumbnail_final = final;
	bt_shell_print("BIP server %p put_linked_thumbnail req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static bool server_put_linked_attachment_final;
static void bip_server_put_linked_attachment(struct bt_bip_server *server, bool final,
					     struct net_buf *buf)
{
	server_put_linked_attachment_final = final;
	bt_shell_print("BIP server %p put_linked_attachment req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static bool server_remote_display_final;
static void bip_server_remote_display(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	server_remote_display_final = final;
	bt_shell_print("BIP server %p remote_display req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static bool server_delete_image_final;
static void bip_server_delete_image(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	server_delete_image_final = final;
	bt_shell_print("BIP server %p delete_image req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static bool server_start_print_final;
static void bip_server_start_print(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	server_start_print_final = final;
	bt_shell_print("BIP server %p start_print req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static bool server_start_archive_final;
static void bip_server_start_archive(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	server_start_archive_final = final;
	bt_shell_print("BIP server %p start_archive req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	bip_parse_headers(buf);
}

static void bip_server_abort(struct bt_bip_server *server, struct net_buf *buf)
{
	server_put_image_final = false;
	server_put_linked_thumbnail_final = false;
	server_put_linked_attachment_final = false;
	server_remote_display_final = false;
	server_delete_image_final = false;
	server_start_print_final = false;
	server_start_archive_final = false;
	bt_shell_print("BIP server %p abort req", server);
	bip_parse_headers(buf);
}

struct bt_bip_server_cb bip_server_cb = {
	.connect = bip_server_connect,
	.disconnect = bip_server_disconnect,
	.abort = bip_server_abort,
	.get_caps = bip_server_get_caps,
	.get_image_list = bip_server_get_image_list,
	.get_image_properties = bip_server_get_image_properties,
	.get_image = bip_server_get_image,
	.get_linked_thumbnail = bip_server_get_linked_thumbnail,
	.get_linked_attachment = bip_server_get_linked_attachment,
	.get_partial_image = bip_server_get_partial_image,
	.get_monitoring_image = bip_server_get_monitoring_image,
	.get_status = bip_server_get_status,
	.put_image = bip_server_put_image,
	.put_linked_thumbnail = bip_server_put_linked_thumbnail,
	.put_linked_attachment = bip_server_put_linked_attachment,
	.remote_display = bip_server_remote_display,
	.delete_image = bip_server_delete_image,
	.start_print = bip_server_start_print,
	.start_archive = bip_server_start_archive,
};

static void bip_client_connect(struct bt_bip_client *client, uint8_t rsp_code, uint8_t version,
			       uint16_t mopl, struct net_buf *buf)
{
	int err;

	bt_shell_print("BIP client %p conn rsp, rsp_code %s, version %02x, mopl %04x", client,
		       bt_obex_rsp_code_to_str(rsp_code), version, mopl);
	bip_parse_headers(buf);

	bip_app.server_mopl = mopl;

	err = bt_obex_get_header_conn_id(buf, &bip_app.conn_id);
	if (err != 0) {
		bt_shell_error("Failed to get connection id");
	} else {
		bt_shell_print("connect id %u", bip_app.conn_id);
	}
}

static void bip_client_disconnect(struct bt_bip_client *client, uint8_t rsp_code,
				  struct net_buf *buf)
{
	bt_shell_print("BIP client %p disconn rsp, rsp_code %s", client,
		       bt_obex_rsp_code_to_str(rsp_code));
	bip_parse_headers(buf);
}

static uint8_t client_get_caps_rsp_code;
static void bip_client_get_caps(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	client_get_caps_rsp_code = rsp_code;
	bt_shell_print("BIP client %p get_caps rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_get_image_list_rsp_code;
static void bip_client_get_image_list(struct bt_bip_client *client, uint8_t rsp_code,
				      struct net_buf *buf)
{
	client_get_image_list_rsp_code = rsp_code;
	bt_shell_print("BIP client %p get_image_list rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_get_image_properties_rsp_code;
static void bip_client_get_image_properties(struct bt_bip_client *client, uint8_t rsp_code,
					    struct net_buf *buf)
{
	client_get_image_properties_rsp_code = rsp_code;
	bt_shell_print("BIP client %p get_image_properties rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_get_image_rsp_code;
static void bip_client_get_image(struct bt_bip_client *client, uint8_t rsp_code,
				 struct net_buf *buf)
{
	client_get_image_rsp_code = rsp_code;
	bt_shell_print("BIP client %p get_image rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_get_linked_thumbnail_rsp_code;
static void bip_client_get_linked_thumbnail(struct bt_bip_client *client, uint8_t rsp_code,
					    struct net_buf *buf)
{
	client_get_linked_thumbnail_rsp_code = rsp_code;
	bt_shell_print("BIP client %p get_linked_thumbnail rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_get_linked_attachment_rsp_code;
static void bip_client_get_linked_attachment(struct bt_bip_client *client, uint8_t rsp_code,
					     struct net_buf *buf)
{
	client_get_linked_attachment_rsp_code = rsp_code;
	bt_shell_print("BIP client %p get_linked_attachment rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_get_partial_image_rsp_code;
static void bip_client_get_partial_image(struct bt_bip_client *client, uint8_t rsp_code,
					 struct net_buf *buf)
{
	client_get_partial_image_rsp_code = rsp_code;
	bt_shell_print("BIP client %p get_partial_image rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_get_monitoring_image_rsp_code;
static void bip_client_get_monitoring_image(struct bt_bip_client *client, uint8_t rsp_code,
					    struct net_buf *buf)
{
	client_get_monitoring_image_rsp_code = rsp_code;
	bt_shell_print("BIP client %p get_monitoring_image rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_get_status_rsp_code;
static void bip_client_get_status(struct bt_bip_client *client, uint8_t rsp_code,
				  struct net_buf *buf)
{
	client_get_status_rsp_code = rsp_code;
	bt_shell_print("BIP client %p get_status rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_put_image_rsp_code;
static bool client_put_image_final;
static void bip_client_put_image(struct bt_bip_client *client, uint8_t rsp_code,
				 struct net_buf *buf)
{
	client_put_image_rsp_code = rsp_code;
	if (client_put_image_rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		client_put_image_final = false;
	}
	bt_shell_print("BIP client %p put_image rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_put_linked_thumbnail_rsp_code;
static bool client_put_linked_thumbnail_final;
static void bip_client_put_linked_thumbnail(struct bt_bip_client *client, uint8_t rsp_code,
					    struct net_buf *buf)
{
	client_put_linked_thumbnail_rsp_code = rsp_code;
	if (client_put_linked_thumbnail_rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		client_put_linked_thumbnail_final = false;
	}
	bt_shell_print("BIP client %p put_linked_thumbnail rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_put_linked_attachment_rsp_code;
static bool client_put_linked_attachment_final;
static void bip_client_put_linked_attachment(struct bt_bip_client *client, uint8_t rsp_code,
					     struct net_buf *buf)
{
	client_put_linked_attachment_rsp_code = rsp_code;
	if (client_put_linked_attachment_rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		client_put_linked_attachment_final = false;
	}
	bt_shell_print("BIP client %p put_linked_attachment rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_remote_display_rsp_code;
static bool client_remote_display_final;
static void bip_client_remote_display(struct bt_bip_client *client, uint8_t rsp_code,
				      struct net_buf *buf)
{
	client_remote_display_rsp_code = rsp_code;
	if (client_remote_display_rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		client_remote_display_final = false;
	}
	bt_shell_print("BIP client %p remote_display rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_delete_image_rsp_code;
static bool client_delete_image_final;
static void bip_client_delete_image(struct bt_bip_client *client, uint8_t rsp_code,
				    struct net_buf *buf)
{
	client_delete_image_rsp_code = rsp_code;
	if (client_delete_image_rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		client_delete_image_final = false;
	}
	bt_shell_print("BIP client %p delete_image rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_start_print_rsp_code;
static bool client_start_print_final;
static void bip_client_start_print(struct bt_bip_client *client, uint8_t rsp_code,
				   struct net_buf *buf)
{
	client_start_print_rsp_code = rsp_code;
	if (client_start_print_rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		client_start_print_final = false;
	}
	bt_shell_print("BIP client %p start_print rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static uint8_t client_start_archive_rsp_code;
static bool client_start_archive_final;
static void bip_client_start_archive(struct bt_bip_client *client, uint8_t rsp_code,
				     struct net_buf *buf)
{
	client_start_archive_rsp_code = rsp_code;
	if (client_start_archive_rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		client_start_archive_final = false;
	}
	bt_shell_print("BIP client %p start_archive rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	bip_parse_headers(buf);
}

static void bip_client_abort(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	client_put_image_final = false;
	client_put_linked_thumbnail_final = false;
	client_put_linked_attachment_final = false;
	client_remote_display_final = false;
	client_delete_image_final = false;
	client_start_print_final = false;
	client_start_archive_final = false;

	client_get_caps_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_get_image_list_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_get_image_properties_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_get_image_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_get_linked_thumbnail_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_get_linked_attachment_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_get_partial_image_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_get_monitoring_image_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_get_status_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_put_image_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_put_linked_thumbnail_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_put_linked_attachment_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_remote_display_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_delete_image_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_start_print_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	client_start_archive_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;

	bt_shell_print("BIP client %p abort rsp, rsp_code %s", client,
		       bt_obex_rsp_code_to_str(rsp_code));
	bip_parse_headers(buf);
}

struct bt_bip_client_cb bip_client_cb = {
	.connect = bip_client_connect,
	.disconnect = bip_client_disconnect,
	.abort = bip_client_abort,
	.get_caps = bip_client_get_caps,
	.get_image_list = bip_client_get_image_list,
	.get_image_properties = bip_client_get_image_properties,
	.get_image = bip_client_get_image,
	.get_linked_thumbnail = bip_client_get_linked_thumbnail,
	.get_linked_attachment = bip_client_get_linked_attachment,
	.get_partial_image = bip_client_get_partial_image,
	.get_monitoring_image = bip_client_get_monitoring_image,
	.get_status = bip_client_get_status,
	.put_image = bip_client_put_image,
	.put_linked_thumbnail = bip_client_put_linked_thumbnail,
	.put_linked_attachment = bip_client_put_linked_attachment,
	.remote_display = bip_client_remote_display,
	.delete_image = bip_client_delete_image,
	.start_print = bip_client_start_print,
	.start_archive = bip_client_start_archive,
};

static int rfcomm_accept(struct bt_conn *conn, struct bt_bip_rfcomm_server *server,
			 struct bt_bip **bip)
{
	struct bt_bip_app *g_app;

	g_app = bip_alloc(conn);
	if (g_app == NULL) {
		bt_shell_print("Cannot allocate bip instance");
		return -ENOMEM;
	}

	g_app->bip.ops = &bip_transport_ops;
	*bip = &g_app->bip;
	return 0;
}

static int cmd_register_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t channel;

	if (rfcomm_server.server.rfcomm.channel != 0) {
		shell_error(sh, "RFCOMM has been registered");
		return -EBUSY;
	}

	channel = (uint8_t)strtoul(argv[1], NULL, 16);

	rfcomm_server.server.rfcomm.channel = channel;
	rfcomm_server.accept = rfcomm_accept;
	err = bt_bip_rfcomm_register(&rfcomm_server);
	if (err != 0) {
		shell_error(sh, "Fail to register RFCOMM server (error %d)", err);
		rfcomm_server.server.rfcomm.channel = 0;
		return -ENOEXEC;
	}
	shell_print(sh, "RFCOMM server (channel %02x) is registered",
		    rfcomm_server.server.rfcomm.channel);
	return 0;
}

static int cmd_connect_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_bip_app *g_app;
	uint8_t channel;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	channel = (uint8_t)strtoul(argv[1], NULL, 16);
	if (channel == 0) {
		shell_error(sh, "Invalid channel");
		return -ENOEXEC;
	}

	g_app = bip_alloc(default_conn);
	if (g_app == NULL) {
		shell_error(sh, "Cannot allocate bip instance");
		return -ENOMEM;
	}

	g_app->bip.ops = &bip_transport_ops;

	err = bt_bip_rfcomm_connect(default_conn, &g_app->bip, channel);
	if (err != 0) {
		bip_free(g_app);
		shell_error(sh, "Fail to connect to channel %d (err %d)", channel, err);
	} else {
		shell_print(sh, "BIP RFCOMM connection pending");
	}

	return err;
}

static int cmd_disconnect_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	err = bt_bip_rfcomm_disconnect(&bip_app.bip);
	if (err != 0) {
		shell_error(sh, "Fail to disconnect to channel (err %d)", err);
	} else {
		shell_print(sh, "BIP RFCOMM disconnection pending");
	}
	return err;
}

static int l2cap_accept(struct bt_conn *conn, struct bt_bip_l2cap_server *server,
			struct bt_bip **bip)
{
	struct bt_bip_app *g_app;

	g_app = bip_alloc(conn);
	if (g_app == NULL) {
		bt_shell_print("Cannot allocate bip instance");
		return -ENOMEM;
	}

	g_app->bip.ops = &bip_transport_ops;
	*bip = &g_app->bip;
	return 0;
}

static int cmd_register_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint16_t psm;

	if (l2cap_server.server.l2cap.psm != 0) {
		shell_error(sh, "L2CAP server has been registered");
		return -EBUSY;
	}

	psm = (uint16_t)strtoul(argv[1], NULL, 16);

	l2cap_server.server.l2cap.psm = psm;
	l2cap_server.accept = l2cap_accept;
	err = bt_bip_l2cap_register(&l2cap_server);
	if (err != 0) {
		shell_error(sh, "Fail to register L2CAP server (error %d)", err);
		l2cap_server.server.l2cap.psm = 0;
		return -ENOEXEC;
	}
	shell_print(sh, "L2CAP server (psm %04x) is registered", l2cap_server.server.l2cap.psm);
	return 0;
}

static int cmd_connect_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_bip_app *g_app;
	uint16_t psm;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	psm = (uint16_t)strtoul(argv[1], NULL, 16);
	if (psm == 0) {
		shell_error(sh, "Invalid psm");
		return -ENOEXEC;
	}

	g_app = bip_alloc(default_conn);
	if (g_app == NULL) {
		shell_error(sh, "Cannot allocate bip instance");
		return -ENOMEM;
	}

	g_app->bip.ops = &bip_transport_ops;

	err = bt_bip_l2cap_connect(default_conn, &g_app->bip, psm);
	if (err != 0) {
		bip_free(g_app);
		shell_error(sh, "Fail to connect to PSM %d (err %d)", psm, err);
	} else {
		shell_print(sh, "BIP L2CAP connection pending");
	}

	return err;
}

static int cmd_disconnect_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	err = bt_bip_l2cap_disconnect(&bip_app.bip);
	if (err != 0) {
		shell_error(sh, "Fail to disconnect L2CAP conn (err %d)", err);
	} else {
		shell_print(sh, "BIP L2CAP disconnection pending");
	}
	return err;
}

static int cmd_add_header_count(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t count;
	int err;

	count = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_count(bip_app.tx_buf, count);
	if (err != 0) {
		shell_error(sh, "Fail to add header count");
	}
	return err;
}

static int cmd_add_header_name(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload;
	size_t hex_payload_size;

	if (argc > 1) {
		hex_payload = argv[1];
		hex_payload_size = strlen(hex_payload);

		len = hex2bin(hex_payload, hex_payload_size, add_head_buffer,
			      sizeof(add_head_buffer));
		if (len > UINT16_MAX) {
			shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
			return -ENOEXEC;
		}
	} else {
		len = 0;
	}

	err = bt_obex_add_header_name(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header name");
	}
	return err;
}

static int cmd_add_header_type(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header type");
	}
	return err;
}

static int cmd_add_header_len(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t len;
	int err;

	len = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_len(bip_app.tx_buf, len);
	if (err != 0) {
		shell_error(sh, "Fail to add header len");
	}
	return err;
}

static int cmd_add_header_time_iso_8601(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_time_iso_8601(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header time_iso_8601");
	}
	return err;
}

static int cmd_add_header_time(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t t;
	int err;

	t = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_time(bip_app.tx_buf, t);
	if (err != 0) {
		shell_error(sh, "Fail to add header time");
	}
	return err;
}

static int cmd_add_header_description(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_description(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header description");
	}
	return err;
}

static int cmd_add_header_target(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_target(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header target");
	}
	return err;
}

static int cmd_add_header_http(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_http(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header http");
	}
	return err;
}

static int cmd_add_header_body(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_body(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header body");
	}
	return err;
}

static int cmd_add_header_end_body(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_end_body(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header end_body");
	}
	return err;
}

static int cmd_add_header_who(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_who(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header who");
	}
	return err;
}

static int cmd_add_header_conn_id(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t conn_id;
	int err;

	conn_id = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add header conn_id");
	}
	return err;
}

static int cmd_add_header_app_param(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	uint8_t tag;
	bool last = false;

	if (tlv_count >= (uint8_t)ARRAY_SIZE(tlvs)) {
		shell_warn(sh, "No space of TLV array, add app_param and clear tlvs");
		goto add_header;
	}

	len = hex2bin(argv[1], strlen(argv[1]), &tag, sizeof(tag));
	if (len < 1) {
		shell_error(sh, "Length should not be zero");
		return -ENOEXEC;
	}

	len = hex2bin(argv[2], strlen(argv[2]), &tlv_buffers[tlv_count][0], TLV_BUFFER_SIZE);
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	if ((argc > 3) && !strcmp(argv[3], "last")) {
		last = true;
	}

	tlvs[tlv_count].type = tag;
	tlvs[tlv_count].data_len = len;
	tlvs[tlv_count].data = &tlv_buffers[tlv_count][0];

	tlv_count++;

	if (!last) {
		return 0;
	}

add_header:
	err = bt_obex_add_header_app_param(bip_app.tx_buf, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add header app_param");
	}
	tlv_count = 0;
	return err;
}

static int cmd_add_header_auth_challenge(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	uint8_t tag;
	bool last = false;

	if (tlv_count >= (uint8_t)ARRAY_SIZE(tlvs)) {
		shell_warn(sh, "No space of TLV array, add auth_challenge and clear tlvs");
		goto add_header;
	}

	len = hex2bin(argv[1], strlen(argv[1]), &tag, sizeof(tag));
	if (len < 1) {
		shell_error(sh, "Length should not be zero");
		return -ENOEXEC;
	}

	len = hex2bin(argv[2], strlen(argv[2]), &tlv_buffers[tlv_count][0], TLV_BUFFER_SIZE);
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	if ((argc > 3) && !strcmp(argv[3], "last")) {
		last = true;
	}

	tlvs[tlv_count].type = tag;
	tlvs[tlv_count].data_len = len;
	tlvs[tlv_count].data = &tlv_buffers[tlv_count][0];

	tlv_count++;

	if (!last) {
		return 0;
	}

add_header:
	err = bt_obex_add_header_auth_challenge(bip_app.tx_buf, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add header auth_challenge");
	}
	tlv_count = 0;
	return err;
}

static int cmd_add_header_auth_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	uint8_t tag;
	bool last = false;

	if (tlv_count >= (uint8_t)ARRAY_SIZE(tlvs)) {
		shell_warn(sh, "No space of TLV array, add auth_rsp and clear tlvs");
		goto add_header;
	}

	len = hex2bin(argv[1], strlen(argv[1]), &tag, sizeof(tag));
	if (len < 1) {
		shell_error(sh, "Length should not be zero");
		return -ENOEXEC;
	}

	len = hex2bin(argv[2], strlen(argv[2]), &tlv_buffers[tlv_count][0], TLV_BUFFER_SIZE);
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	if ((argc > 3) && !strcmp(argv[3], "last")) {
		last = true;
	}

	tlvs[tlv_count].type = tag;
	tlvs[tlv_count].data_len = len;
	tlvs[tlv_count].data = &tlv_buffers[tlv_count][0];

	tlv_count++;

	if (!last) {
		return 0;
	}

add_header:
	err = bt_obex_add_header_auth_rsp(bip_app.tx_buf, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add header auth_rsp");
	}
	tlv_count = 0;
	return err;
}

static int cmd_add_header_creator_id(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t creator_id;
	int err;

	creator_id = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_creator_id(bip_app.tx_buf, creator_id);
	if (err != 0) {
		shell_error(sh, "Fail to add header creator_id");
	}
	return err;
}

static int cmd_add_header_wan_uuid(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_wan_uuid(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header wan_uuid");
	}
	return err;
}

static int cmd_add_header_obj_class(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_obj_class(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header obj_class");
	}
	return err;
}

static int cmd_add_header_session_param(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_session_param(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header session_param");
	}
	return err;
}

static int cmd_add_header_session_seq_number(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t session_seq_number;
	int err;

	session_seq_number = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_session_seq_number(bip_app.tx_buf, session_seq_number);
	if (err != 0) {
		shell_error(sh, "Fail to add header session_seq_number");
	}
	return err;
}

static int cmd_add_header_action_id(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t action_id;
	int err;

	action_id = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_action_id(bip_app.tx_buf, (uint8_t)action_id);
	if (err != 0) {
		shell_error(sh, "Fail to add header action_id");
	}
	return err;
}

static int cmd_add_header_dest_name(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_dest_name(bip_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err != 0) {
		shell_error(sh, "Fail to add header dest_name");
	}
	return err;
}

static int cmd_add_header_perm(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t perm;
	int err;

	perm = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_perm(bip_app.tx_buf, perm);
	if (err != 0) {
		shell_error(sh, "Fail to add header perm");
	}
	return err;
}

static int cmd_add_header_srm(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t srm;
	int err;

	srm = strtoul(argv[1], NULL, 16);

	if (srm > UINT8_MAX) {
		shell_error(sh, "Value exceeds max value (%x > %x)", srm, UINT8_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_srm(bip_app.tx_buf, (uint8_t)srm);
	if (err != 0) {
		shell_error(sh, "Fail to add header srm");
	}
	return err;
}

static int cmd_add_header_srm_param(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t srm_param;
	int err;

	srm_param = strtoul(argv[1], NULL, 16);

	if (srm_param > UINT8_MAX) {
		shell_error(sh, "Value exceeds max value (%x > %x)", srm_param, UINT8_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_srm_param(bip_app.tx_buf, (uint8_t)srm_param);
	if (err != 0) {
		shell_error(sh, "Fail to add header srm_param");
	}
	return err;
}

static int cmd_bip_client_set_feats_funcs(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_bip_set_supported_features(&bip_app.bip, bip_supported_features);
	if (err != 0) {
		shell_error(sh, "Failed to set BIP supported features");
	}

	err = bt_bip_set_supported_functions(&bip_app.bip, bip_supported_functions);
	if (err != 0) {
		shell_error(sh, "Failed to set BIP supported functions");
	}

	return 0;
}

static int cmd_bip_client_conn(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t uuid128[BT_UUID_SIZE_128];
	struct bt_uuid_128 *u = NULL;
	uint8_t type;

	static struct bt_uuid_128 uuid;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	err = 0;
	type = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid type %s", argv[1]);
		return -ENOEXEC;
	}

	if (argc > 2) {
		hex2bin(argv[2], strlen(argv[2]), uuid128, sizeof(uuid128));
		uuid.uuid.type = BT_UUID_TYPE_128;
		sys_memcpy_swap(uuid.val, uuid128, BT_UUID_SIZE_128);
		u = &uuid;
	}

	err = bt_bip_primary_client_connect(&bip_app.bip, &bip_app.client, type, &bip_client_cb,
					    bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send conn req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_client_disconn(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	err = bt_bip_disconnect(&bip_app.client, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send disconn req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_client_abort(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	err = bt_bip_abort(&bip_app.client, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send abort req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_client_get_caps(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_get_caps_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_GET_CAPS),
				      BT_BIP_HDR_TYPE_GET_CAPS);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

continue_req:
	err = bt_bip_get_capabilities(&bip_app.client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get caps req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

#define IMAGE_HANDLES_DESC                                                                         \
	"<image-handles-descriptor version=\"1.0\">"                                               \
	"<filtering-parameters created=\"20000101T000000Z-20000101T235959Z\" />"                   \
	"</image-handles-descriptor>"

static int cmd_bip_client_get_image_list(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint16_t returned_handles = sys_cpu_to_be16(1);
	uint16_t list_start_offset = sys_cpu_to_be16(0);
	uint8_t latest_captured = 0;
	struct bt_obex_tlv appl_params[] = {
		{BT_BIP_APPL_PARAM_TAG_ID_RETURNED_HANDLES, sizeof(returned_handles),
		 (const uint8_t *)&returned_handles},
		{BT_BIP_APPL_PARAM_TAG_ID_LIST_START_OFFSET, sizeof(list_start_offset),
		 (const uint8_t *)&list_start_offset},
		{BT_BIP_APPL_PARAM_TAG_ID_LATEST_CAPTURED_IMAGES, sizeof(latest_captured),
		 (const uint8_t *)&latest_captured},
	};

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_get_image_list_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_GET_IMAGE_LIST),
				      BT_BIP_HDR_TYPE_GET_IMAGE_LIST);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

	err = bt_obex_add_header_app_param(bip_app.tx_buf, ARRAY_SIZE(appl_params), appl_params);
	if (err != 0) {
		shell_error(sh, "Fail to add appl param header %d", err);
		return err;
	}

	err = bt_bip_add_header_image_desc(bip_app.tx_buf, sizeof(IMAGE_HANDLES_DESC),
					   IMAGE_HANDLES_DESC);
	if (err != 0) {
		shell_error(sh, "Fail to add image desc header %d", err);
		return err;
	}

continue_req:
	err = bt_bip_get_image_list(&bip_app.client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get image list req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

#define IMAGE_HANDLE "\x00\x31\x00\x30\x00\x30\x00\x30\x00\x30\x00\x30\x00\x31\x00\x00"

static int cmd_bip_client_get_image_properties(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_get_image_properties_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES),
				      BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

	err = bt_bip_add_header_image_handle(bip_app.tx_buf, sizeof(IMAGE_HANDLE) - 1,
					     IMAGE_HANDLE);
	if (err != 0) {
		shell_error(sh, "Fail to add image handle header %d", err);
		return err;
	}

continue_req:
	err = bt_bip_get_image_properties(&bip_app.client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_image_properties req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

#define IMAGE_DESC                                                                                 \
	"<image-descriptor version=\"1.0\">"                                                       \
	"<image encoding=\"JPEG\" pixel=\"640*480\" size=\"100000\"/>"                             \
	"</image-descriptor>"

static int cmd_bip_client_get_image(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_get_image_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_GET_IMAGE),
				      BT_BIP_HDR_TYPE_GET_IMAGE);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

	err = bt_bip_add_header_image_handle(bip_app.tx_buf, sizeof(IMAGE_HANDLE) - 1,
					     IMAGE_HANDLE);
	if (err != 0) {
		shell_error(sh, "Fail to add image handle header %d", err);
		return err;
	}

	err = bt_bip_add_header_image_desc(bip_app.tx_buf, sizeof(IMAGE_DESC), IMAGE_DESC);
	if (err != 0) {
		shell_error(sh, "Fail to add image descriptor header %d", err);
		return err;
	}

continue_req:
	err = bt_bip_get_image(&bip_app.client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_image req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_client_get_linked_thumbnail(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_get_linked_thumbnail_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL),
				      BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

	err = bt_bip_add_header_image_handle(bip_app.tx_buf, sizeof(IMAGE_HANDLE) - 1,
					     IMAGE_HANDLE);
	if (err != 0) {
		shell_error(sh, "Fail to add image handle header %d", err);
		return err;
	}

continue_req:
	err = bt_bip_get_linked_thumbnail(&bip_app.client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_linked_thumbnail req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

#define IMAGE_ATTCHMENT_FILE_NAME "\x00\x31\x00\x30\x00\x30\x00\x30\x00\x30\x00\x30\x00\x31\x00\x00"

static int cmd_bip_client_get_linked_attachment(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_get_linked_attachment_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_GET_LINKED_ATTACHMENT),
				      BT_BIP_HDR_TYPE_GET_LINKED_ATTACHMENT);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

	err = bt_bip_add_header_image_handle(bip_app.tx_buf, sizeof(IMAGE_HANDLE) - 1,
					     IMAGE_HANDLE);
	if (err != 0) {
		shell_error(sh, "Fail to add image handle header %d", err);
		return err;
	}

	err = bt_obex_add_header_name(bip_app.tx_buf, sizeof(IMAGE_ATTCHMENT_FILE_NAME) - 1,
				      IMAGE_ATTCHMENT_FILE_NAME);
	if (err != 0) {
		shell_error(sh, "Fail to add image name header %d", err);
		return err;
	}

continue_req:
	err = bt_bip_get_linked_attachment(&bip_app.client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_linked_attachment req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

#define IMAGE_PARTIAL_FILE_NAME "\x00\x31\x00\x30\x00\x30\x00\x30\x00\x30\x00\x30\x00\x31\x00\x00"

static int cmd_bip_client_get_partial_image(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint32_t partial_file_len = 0xffffffff;
	uint32_t partial_file_start_offset = 0;
	struct bt_obex_tlv appl_params[] = {
		{BT_BIP_APPL_PARAM_TAG_ID_PARTIAL_FILE_LEN, sizeof(partial_file_len),
		 (const uint8_t *)&partial_file_len},
		{BT_BIP_APPL_PARAM_TAG_ID_PARTIAL_FILE_START_OFFSET,
		 sizeof(partial_file_start_offset), (const uint8_t *)&partial_file_start_offset},
	};

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_GET_PARTIAL_IMAGE),
				      BT_BIP_HDR_TYPE_GET_PARTIAL_IMAGE);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

	err = bt_obex_add_header_name(bip_app.tx_buf, sizeof(IMAGE_PARTIAL_FILE_NAME) - 1,
				      IMAGE_PARTIAL_FILE_NAME);
	if (err != 0) {
		shell_error(sh, "Fail to add image name header %d", err);
		return err;
	}

	err = bt_obex_add_header_app_param(bip_app.tx_buf, ARRAY_SIZE(appl_params), appl_params);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header %d", err);
		return err;
	}

	if (client_get_partial_image_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

continue_req:
	err = bt_bip_get_partial_image(&bip_app.client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_partial_image req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_client_get_monitoring_image(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t store_flag = 0;
	struct bt_obex_tlv appl_params[] = {
		{BT_BIP_APPL_PARAM_TAG_ID_STORE_FLAG, sizeof(store_flag),
		 (const uint8_t *)&store_flag},
	};

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_get_monitoring_image_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_GET_MONITORING_IMAGE),
				      BT_BIP_HDR_TYPE_GET_MONITORING_IMAGE);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

	err = bt_obex_add_header_app_param(bip_app.tx_buf, ARRAY_SIZE(appl_params), appl_params);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header %d", err);
		return err;
	}

continue_req:
	err = bt_bip_get_monitoring_image(&bip_app.client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_monitoring_image req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_client_get_status(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_GET_STATUS),
				      BT_BIP_HDR_TYPE_GET_STATUS);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

	if (client_get_status_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

continue_req:
	err = bt_bip_get_status(&bip_app.client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_status req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

#define IMAGE_PUT_FILE_NAME "\x00\x31\x00\x30\x00\x30\x00\x30\x00\x30\x00\x30\x00\x31\x00\x00"

/* Minimal 1x1 pixel white JPEG image data */
static const uint8_t jpeg_1000001[] = {
	/* SOI (Start of Image) */
	0xFF, 0xD8,

	/* APP0 segment */
	0xFF, 0xE0,                   /* APP0 marker */
	0x00, 0x10,                   /* Length (16 bytes) */
	0x4A, 0x46, 0x49, 0x46, 0x00, /* "JFIF\0" */
	0x01, 0x01,                   /* Version 1.1 */
	0x01,                         /* Density units (1 = pixels per inch) */
	0x00, 0x48,                   /* X density (72) */
	0x00, 0x48,                   /* Y density (72) */
	0x00, 0x00,                   /* Thumbnail width and height (0,0 = no thumbnail) */

	/* DQT (Define Quantization Table) - Luminance quantization table */
	0xFF, 0xDB, /* DQT marker */
	0x00, 0x43, /* Length (67 bytes) */
	0x00,       /* Table ID (0) and precision (8bit) */
	/* 8x8 quantization table (simplified version) */
	0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, 0x0E, 0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18,
	0x28, 0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25, 0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C,
	0x39, 0x33, 0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 0x44, 0x57, 0x45, 0x37, 0x38, 0x50,
	0x6D, 0x51, 0x57, 0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 0x71, 0x79, 0x70, 0x64, 0x78,
	0x5C, 0x65, 0x67, 0x63,

	/* SOF0 (Start of Frame - Baseline DCT) */
	0xFF, 0xC0, /* SOF0 marker */
	0x00, 0x11, /* Length (17 bytes) */
	0x08,       /* Data precision (8 bits) */
	0x00, 0x01, /* Image height (1) */
	0x00, 0x01, /* Image width (1) */
	0x01,       /* Number of color components (1 = grayscale) */
	0x01,       /* Component ID (1) */
	0x11,       /* Sampling factors (1x1) */
	0x00,       /* Quantization table selector (0) */

	/* DHT (Define Huffman Table) - DC table */
	0xFF, 0xC4, /* DHT marker */
	0x00, 0x1F, /* Length (31 bytes) */
	0x00,       /* Table class (0=DC) and table ID (0) */
	/* Symbol length counts (16 bytes) */
	0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,
	/* Symbol values (12 bytes) */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,

	/* DHT (Define Huffman Table) - AC table */
	0xFF, 0xC4, /* DHT marker */
	0x00, 0xB5, /* Length (181 bytes) */
	0x10,       /* Table class (1=AC) and table ID (0) */
	/* AC Huffman table data (simplified version) */
	0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01,
	0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51,
	0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15,
	0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A,
	0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44,
	0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63,
	0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
	0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5,
	0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2,
	0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
	0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,

	/* SOS (Start of Scan) */
	0xFF, 0xDA, /* SOS marker */
	0x00, 0x08, /* Length (8 bytes) */
	0x01,       /* Number of components (1) */
	0x01,       /* Component ID (1) */
	0x00,       /* DC and AC Huffman table selectors */
	0x00,       /* Spectral selection start */
	0x3F,       /* Spectral selection end */
	0x00,       /* Successive approximation bit position */

	/* Image data (encoded data for 1 white pixel) */
	0xFF, 0x00, /* Stuffed DC coefficient (white) */

	/* EOI (End of Image) */
	0xFF, 0xD9
};

static int cmd_bip_client_put_image(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint16_t len = 0;

	static uint16_t offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_put_image_final) {
		goto continue_req;
	}

	if (client_put_image_rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		offset = 0;
	}

	if (offset == 0) {
		err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
		if (err != 0) {
			shell_error(sh, "Fail to add conn id header %d", err);
			return err;
		}

		err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_PUT_IMAGE),
					      BT_BIP_HDR_TYPE_PUT_IMAGE);
		if (err != 0) {
			shell_error(sh, "Fail to add type header %d", err);
			return err;
		}

		err = bt_obex_add_header_name(bip_app.tx_buf, sizeof(IMAGE_PUT_FILE_NAME) - 1,
					      IMAGE_PUT_FILE_NAME);
		if (err != 0) {
			shell_error(sh, "Fail to add name header %d", err);
			return err;
		}

		err = bt_bip_add_header_image_desc(bip_app.tx_buf, sizeof(IMAGE_DESC), IMAGE_DESC);
		if (err != 0) {
			shell_error(sh, "Fail to add image desc header %d", err);
			return err;
		}
	}

	err = bt_obex_add_header_body_or_end_body(bip_app.tx_buf, bip_app.server_mopl,
						  sizeof(jpeg_1000001) - offset,
						  jpeg_1000001 + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		return -ENOEXEC;
	}

	if (bt_obex_has_header(bip_app.tx_buf, BT_OBEX_HEADER_ID_END_BODY)) {
		client_put_image_final = true;
	}

continue_req:
	err = bt_bip_put_image(&bip_app.client, client_put_image_final, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send put_image req %d", err);
	} else {
		bip_app.tx_buf = NULL;
		if (!client_put_image_final) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

static int cmd_bip_client_put_linked_thumbnail(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint16_t len = 0;

	static uint16_t offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_put_linked_thumbnail_final) {
		goto continue_req;
	}

	if (client_put_linked_thumbnail_rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		offset = 0;
	}

	if (offset == 0) {
		err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
		if (err != 0) {
			shell_error(sh, "Fail to add conn id header %d", err);
			return err;
		}

		err = bt_obex_add_header_type(bip_app.tx_buf,
					      sizeof(BT_BIP_HDR_TYPE_PUT_LINKED_THUMBNAIL),
					      BT_BIP_HDR_TYPE_PUT_LINKED_THUMBNAIL);
		if (err != 0) {
			shell_error(sh, "Fail to add type header %d", err);
			return err;
		}

		err = bt_bip_add_header_image_handle(bip_app.tx_buf, sizeof(IMAGE_HANDLE) - 1,
						     IMAGE_HANDLE);
		if (err != 0) {
			shell_error(sh, "Fail to add image handle header %d", err);
			return err;
		}
	}

	err = bt_obex_add_header_body_or_end_body(bip_app.tx_buf, bip_app.server_mopl,
						  sizeof(jpeg_1000001) - offset,
						  jpeg_1000001 + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		return -ENOEXEC;
	}

	if (bt_obex_has_header(bip_app.tx_buf, BT_OBEX_HEADER_ID_END_BODY)) {
		client_put_linked_thumbnail_final = true;
	}

continue_req:
	err = bt_bip_put_linked_thumbnail(&bip_app.client, client_put_linked_thumbnail_final,
					  bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send put_linked_thumbnail req %d", err);
	} else {
		bip_app.tx_buf = NULL;
		if (!client_put_linked_thumbnail_final) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

static int cmd_bip_client_put_linked_attachment(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint16_t len = 0;

	static uint16_t offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_put_linked_attachment_final) {
		goto continue_req;
	}

	if (client_put_linked_attachment_rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		offset = 0;
	}

	if (offset == 0) {
		err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
		if (err != 0) {
			shell_error(sh, "Fail to add conn id header %d", err);
			return err;
		}

		err = bt_obex_add_header_type(bip_app.tx_buf,
					      sizeof(BT_BIP_HDR_TYPE_PUT_LINKED_ATTACHMENT),
					      BT_BIP_HDR_TYPE_PUT_LINKED_ATTACHMENT);
		if (err != 0) {
			shell_error(sh, "Fail to add type header %d", err);
			return err;
		}

		err = bt_bip_add_header_image_handle(bip_app.tx_buf, sizeof(IMAGE_HANDLE) - 1,
						     IMAGE_HANDLE);
		if (err != 0) {
			shell_error(sh, "Fail to add image handle header %d", err);
			return err;
		}

		err = bt_bip_add_header_image_desc(bip_app.tx_buf, sizeof(IMAGE_DESC), IMAGE_DESC);
		if (err != 0) {
			shell_error(sh, "Fail to add image descriptor header %d", err);
			return err;
		}
	}

	err = bt_obex_add_header_body_or_end_body(bip_app.tx_buf, bip_app.server_mopl,
						  sizeof(jpeg_1000001) - offset,
						  jpeg_1000001 + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		return -ENOEXEC;
	}

	if (bt_obex_has_header(bip_app.tx_buf, BT_OBEX_HEADER_ID_END_BODY)) {
		client_put_linked_attachment_final = true;
	}

continue_req:
	err = bt_bip_put_linked_attachment(&bip_app.client, client_put_linked_attachment_final,
					   bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send put_linked_attachment req %d", err);
	} else {
		bip_app.tx_buf = NULL;
		if (!client_put_linked_attachment_final) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

static int cmd_bip_client_remote_display(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t remote_display_id = 0x04;

	struct bt_obex_tlv appl_params[] = {
		{BT_BIP_APPL_PARAM_TAG_ID_REMOTE_DISPLAY, sizeof(remote_display_id),
		 (const uint8_t *)&remote_display_id},
	};

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_remote_display_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_REMOTE_DISPLAY),
				      BT_BIP_HDR_TYPE_REMOTE_DISPLAY);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

	err = bt_bip_add_header_image_handle(bip_app.tx_buf, sizeof(IMAGE_HANDLE) - 1,
					     IMAGE_HANDLE);
	if (err != 0) {
		shell_error(sh, "Fail to add image handle header %d", err);
		return err;
	}

	err = bt_obex_add_header_app_param(bip_app.tx_buf, ARRAY_SIZE(appl_params), appl_params);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header %d", err);
		return err;
	}

continue_req:
	err = bt_bip_remote_display(&bip_app.client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send remote_display req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_client_delete_image(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_delete_image_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_DELETE_IMAGE),
				      BT_BIP_HDR_TYPE_DELETE_IMAGE);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

	err = bt_bip_add_header_image_handle(bip_app.tx_buf, sizeof(IMAGE_HANDLE) - 1,
					     IMAGE_HANDLE);
	if (err != 0) {
		shell_error(sh, "Fail to add image handle header %d", err);
		return err;
	}

continue_req:
	err = bt_bip_delete_image(&bip_app.client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send delete_image req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

#define PRINT_CONTROL_OBJECT                                                                       \
	"[HDR]"                                                                                    \
	"GEN REV = 01.10"                                                                          \
	"GEN CRT = \"Bluetooth camera\" -01.00"                                                    \
	"GEN DTM = 2001:01:01:12:00:00"                                                            \
	"[JOB]"                                                                                    \
	"PRT PID = 001"                                                                            \
	"PRT TYP = STD"                                                                            \
	"PRT QTY = 001"                                                                            \
	"IMG FMT = EXIF2 -J"                                                                       \
	"<IMG SRC = \"../DCIM/100ABCDE/ABCD0001.JPG\">"                                            \
	"CFG DSC = \"100-0001\" -ATR FID"                                                          \
	"[JOB]"                                                                                    \
	"PRT PID = 002"                                                                            \
	"PRT TYP = STD"                                                                            \
	"PRT QTY = 002"                                                                            \
	"IMG FMT = EXIF2 -J"                                                                       \
	"<IMG SRC = \"../DCIM/100ABCDE/ABCD0002.JPG\">"                                            \
	"CFG DSC = \"2000.12.24\" -ATR DTM"                                                        \
	"CFG DSC = \"100-0002\" -ATR FID"                                                          \
	"[JOB]"                                                                                    \
	"PRT PID = 003"                                                                            \
	"PRT TYP = STD"                                                                            \
	"PRT QTY = 001"                                                                            \
	"IMG FMT = EXIF2 -J"                                                                       \
	"<IMG SRC = \"../DCIM/100ABCDE/ABCD0003.JPG\">"                                            \
	"CFG DSC = \"2000.12.25\" -ATR DTM"                                                        \
	"CFG DSC = \"100-0003\" -ATR FID"                                                          \
	"[JOB]"                                                                                    \
	"PRT PID = 004"                                                                            \
	"PRT TYP = STD"                                                                            \
	"PRT QTY = 003"                                                                            \
	"IMG FMT = EXIF2 -J"                                                                       \
	"<IMG SRC = \"../DCIM/102_BLUE/BLUE0001.JPG\">"                                            \
	"CFG DSC = \"102-0001\" -ATR FID"                                                          \
	"[JOB]"                                                                                    \
	"PRT PID = 100"                                                                            \
	"PRT TYP = IDX"                                                                            \
	"PRT QTY = 001"                                                                            \
	"IMG FMT = EXIF2 -J"                                                                       \
	"IMG SRC = \"../DCIM/100ABCDE/ABCD0001.JPG\""                                              \
	"IMG SRC = \"../DCIM/100ABCDE/ABCD0002.JPG\""                                              \
	"IMG SRC = \"../DCIM/100ABCDE/ABCD0003.JPG\""                                              \
	"IMG SRC = \"../DCIM/102_BLUE/BLUE0001.JPG\""

static int cmd_bip_client_start_print(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t uuid_128[] = {
		BT_UUID_128_ENCODE(0x8E61F95D, 0x1A79, 0x11D4, 0x8EA4, 0x00805F9B9834)};
	struct bt_obex_tlv appl_params[] = {
		{BT_BIP_APPL_PARAM_TAG_ID_STORE_FLAG, sizeof(uuid_128), (const uint8_t *)uuid_128},
	};
	uint16_t len = 0;
	const uint8_t *print_control_obj = (const uint8_t *)PRINT_CONTROL_OBJECT;

	static uint16_t offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_start_print_final) {
		goto continue_req;
	}

	if (client_start_print_rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		offset = 0;
	}

	if (offset == 0) {
		err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
		if (err != 0) {
			shell_error(sh, "Fail to add conn id header %d", err);
			return err;
		}

		err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_START_PRINT),
					      BT_BIP_HDR_TYPE_START_PRINT);
		if (err != 0) {
			shell_error(sh, "Fail to add type header %d", err);
			return err;
		}

		err = bt_obex_add_header_app_param(bip_app.tx_buf, ARRAY_SIZE(appl_params),
						   appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add app param header %d", err);
			return err;
		}
	}

	err = bt_obex_add_header_body_or_end_body(bip_app.tx_buf, bip_app.server_mopl,
						  sizeof(PRINT_CONTROL_OBJECT) - offset,
						  print_control_obj + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		return -ENOEXEC;
	}

	if (bt_obex_has_header(bip_app.tx_buf, BT_OBEX_HEADER_ID_END_BODY)) {
		client_start_print_final = true;
	}

continue_req:
	err = bt_bip_start_print(&bip_app.client, client_start_print_final, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send start_print req %d", err);
	} else {
		bip_app.tx_buf = NULL;
		if (!client_start_print_final) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

static int cmd_bip_client_start_archive(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t uuid_128[] = {
		BT_UUID_128_ENCODE(0x8E61F95E, 0x1A79, 0x11D4, 0x8EA4, 0x00805F9B9834)};
	struct bt_obex_tlv appl_params[] = {
		{BT_BIP_APPL_PARAM_TAG_ID_STORE_FLAG, sizeof(uuid_128), (const uint8_t *)uuid_128},
	};

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (client_start_archive_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_START_ARCHIVE),
				      BT_BIP_HDR_TYPE_START_ARCHIVE);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

	err = bt_obex_add_header_app_param(bip_app.tx_buf, ARRAY_SIZE(appl_params), appl_params);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header %d", err);
		return err;
	}

continue_req:
	err = bt_bip_start_archive(&bip_app.client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send start_archive req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_server_reg(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t uuid128[BT_UUID_SIZE_128];
	struct bt_uuid_128 *u = NULL;
	uint8_t type;

	static struct bt_uuid_128 uuid;

	err = 0;
	type = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid type %s", argv[1]);
		return -ENOEXEC;
	}

	if (argc > 2) {
		hex2bin(argv[2], strlen(argv[2]), uuid128, sizeof(uuid128));
		uuid.uuid.type = BT_UUID_TYPE_128;
		sys_memcpy_swap(uuid.val, uuid128, BT_UUID_SIZE_128);
		u = &uuid;
	}

	err = bt_bip_primary_server_register(&bip_app.bip, &bip_app.server, type, u,
					     &bip_server_cb);
	if (err != 0) {
		shell_error(sh, "Fail to register server %d", err);
	}

	return err;
}

static int cmd_bip_server_unreg(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_bip_server_unregister(&bip_app.server);
	if (err != 0) {
		shell_error(sh, "Fail to unregister obex server %d", err);
	}

	return err;
}

static int cmd_bip_server_conn(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_bip_connect_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send conn rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_server_disconn(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_bip_disconnect_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send disconn rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_server_abort(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_bip_abort_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send abort rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

#define IMAGING_CAPS_BODY                                                                          \
	"<?xml version=\"1.0\"?>"                                                                  \
	"<!DOCTYPE imaging-capabilities SYSTEM \"obex-capability.dtd\">"                           \
	"<imaging-capabilities version=\"1.0\">"                                                   \
	"    <preferred-format encoding=\"JPEG\" pixel=\"160*120\" size=\"5000\"/>"                \
	"    <image-formats encoding=\"JPEG\" pixel=\"640*480-1600*1200\" size=\"50000\"/>"        \
	"    <image-formats encoding=\"GIF\" pixel=\"80*60-640*480\" size=\"20000\"/>"             \
	"    <image-formats encoding=\"BMP\" pixel=\"160*120-1600*1200\" size=\"100000\"/>"        \
	"    <image-formats encoding=\"PNG\" pixel=\"160*120-1600*1200\" size=\"100000\"/>"        \
	"    <image-formats encoding=\"WBMP\" pixel=\"160*120\" size=\"5000\"/>"                   \
	"    <attachment-formats encoding=\"text/plain\" size=\"1000\"/>"                          \
	"    <attachment-formats encoding=\"text/x-vCard\" size=\"1000\"/>"                        \
	"    <attachment-formats encoding=\"text/x-vCalendar\" size=\"5000\"/>"                    \
	"    <attachment-formats encoding=\"application/vnd.wap.wmlc\" size=\"1000\"/>"            \
	"    <filtering-parameters encoding=\"JPEG\" pixel=\"yes\" transformation=\"yes\"/>"       \
	"    <dpof-options printing=\"yes\" paper-types=\"yes\"/>"                                 \
	"</imaging-capabilities>"

static int cmd_bip_server_get_caps(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t len = 0;
	const uint8_t *caps_body = (const uint8_t *)IMAGING_CAPS_BODY;

	static uint16_t offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_body_or_end_body(bip_app.tx_buf, bip_app.client_mopl,
						  sizeof(IMAGING_CAPS_BODY) - offset,
						  caps_body + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		return -ENOEXEC;
	}

	if (bt_obex_has_header(bip_app.tx_buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_bip_get_capabilities_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_capabilities rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

#define IMAGE_LIST_BODY                                                                            \
	"<images-listing version=\"1.0\">"                                                         \
	"<image handle=\"1000001\" created=\"20000801T060000Z\"/>"                                 \
	"<image handle=\"1000003\" created=\"20000801T060115Z\" modified=\"20000808T071500Z\" />"  \
	"<image handle=\"1000004\" created=\"20000801T060137Z\" />"                                \
	"</images-listing>"

static int cmd_bip_server_get_image_list(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t len = 0;
	const uint8_t *image_list_body = (const uint8_t *)IMAGE_LIST_BODY;

	static uint16_t offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (offset == 0) {
		uint16_t returned_handles = sys_cpu_to_be16(1);
		struct bt_obex_tlv appl_params[] = {
			{BT_BIP_APPL_PARAM_TAG_ID_RETURNED_HANDLES, sizeof(returned_handles),
			 (const uint8_t *)&returned_handles},
		};

		err = bt_obex_add_header_app_param(bip_app.tx_buf, ARRAY_SIZE(appl_params),
						   appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add appl param header %d", err);
			return err;
		}

		err = bt_bip_add_header_image_desc(bip_app.tx_buf, sizeof(IMAGE_HANDLES_DESC),
						   IMAGE_HANDLES_DESC);
		if (err != 0) {
			shell_error(sh, "Fail to add image desc header %d", err);
			return err;
		}
	}

	err = bt_obex_add_header_body_or_end_body(bip_app.tx_buf, bip_app.client_mopl,
						  sizeof(IMAGE_LIST_BODY) - offset,
						  image_list_body + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		return -ENOEXEC;
	}

	if (bt_obex_has_header(bip_app.tx_buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_bip_get_image_list_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_image_list rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

#define IMAGE_PROPERTIES_BODY                                                                      \
	"<image-properties version=\"1.0\" handle=\"1000001\">"                                    \
	"<native encoding=\"JPEG\" pixel=\"1280*1024\" size=\"1048576\"/>"                         \
	"<variant encoding=\"JPEG\" pixel=\"640*480\" />"                                          \
	"<variant encoding=\"JPEG\" pixel=\"160*120\" />"                                          \
	"<variant encoding=\"GIF\" pixel=\"80*60-640*480\"/>"                                      \
	"<attachment content-type=\"text/plain\" name=\"ABCD0001.txt\" size=\"5120\"/>"            \
	"<attachment content-type=\"audio/basic\" name=\"ABCD0001.wav\" size=\"102400\"/>"         \
	"</image-properties>"

static int cmd_bip_server_get_image_properties(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t len = 0;
	const uint8_t *image_properties_body = (const uint8_t *)IMAGE_PROPERTIES_BODY;

	static uint16_t offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_body_or_end_body(bip_app.tx_buf, bip_app.client_mopl,
						  sizeof(IMAGE_PROPERTIES_BODY) - offset,
						  image_properties_body + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		return -ENOEXEC;
	}

	if (bt_obex_has_header(bip_app.tx_buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_bip_get_image_properties_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_image_properties rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

static int cmd_bip_server_get_image(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t len = 0;

	static uint16_t offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (offset == 0) {
		err = bt_obex_add_header_len(bip_app.tx_buf, sizeof(jpeg_1000001));
		if (err != 0) {
			shell_error(sh, "Fail to add len header %d", err);
			return err;
		}
	}

	err = bt_obex_add_header_body_or_end_body(bip_app.tx_buf, bip_app.client_mopl,
						  sizeof(jpeg_1000001) - offset,
						  jpeg_1000001 + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		return -ENOEXEC;
	}

	if (bt_obex_has_header(bip_app.tx_buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_bip_get_image_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_image rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

static int cmd_bip_server_get_linked_thumbnail(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t len = 0;

	static uint16_t offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_body_or_end_body(bip_app.tx_buf, bip_app.client_mopl,
						  sizeof(jpeg_1000001) - offset,
						  jpeg_1000001 + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		return -ENOEXEC;
	}

	if (bt_obex_has_header(bip_app.tx_buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_bip_get_linked_thumbnail_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_linked_thumbnail rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

static int cmd_bip_server_get_linked_attachment(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t len = 0;

	static uint16_t offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_body_or_end_body(bip_app.tx_buf, bip_app.client_mopl,
						  sizeof(jpeg_1000001) - offset,
						  jpeg_1000001 + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		return -ENOEXEC;
	}

	if (bt_obex_has_header(bip_app.tx_buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_bip_get_linked_attachment_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_linked_attachment rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

static int cmd_bip_server_get_partial_image(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t len = 0;

	static uint16_t offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (offset == 0) {
		uint32_t total_file_size = sys_cpu_to_be32(sizeof(jpeg_1000001));
		uint8_t end_flag = 0x01;
		struct bt_obex_tlv appl_params[] = {
			{BT_BIP_APPL_PARAM_TAG_ID_TOTAL_FILE_SIZE, sizeof(total_file_size),
			 (const uint8_t *)&total_file_size},
			{BT_BIP_APPL_PARAM_TAG_ID_END_FLAG, sizeof(end_flag),
			 (const uint8_t *)&end_flag},
		};

		err = bt_obex_add_header_len(bip_app.tx_buf, sizeof(jpeg_1000001));
		if (err != 0) {
			shell_error(sh, "Fail to add appl param header %d", err);
			return err;
		}

		err = bt_obex_add_header_app_param(bip_app.tx_buf, ARRAY_SIZE(appl_params),
						   appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add appl param header %d", err);
			return err;
		}
	}

	err = bt_obex_add_header_body_or_end_body(bip_app.tx_buf, bip_app.client_mopl,
						  sizeof(jpeg_1000001) - offset,
						  jpeg_1000001 + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		return -ENOEXEC;
	}

	if (bt_obex_has_header(bip_app.tx_buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_bip_get_partial_image_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_partial_image rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

static int cmd_bip_server_get_monitoring_image(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t len = 0;

	static uint16_t offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (offset == 0) {
		err = bt_bip_add_header_image_handle(bip_app.tx_buf, sizeof(IMAGE_HANDLE) - 1,
						     IMAGE_HANDLE);
		if (err != 0) {
			shell_error(sh, "Fail to add image handle header %d", err);
			return err;
		}
	}

	err = bt_obex_add_header_body_or_end_body(bip_app.tx_buf, bip_app.client_mopl,
						  sizeof(jpeg_1000001) - offset,
						  jpeg_1000001 + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		return -ENOEXEC;
	}

	if (bt_obex_has_header(bip_app.tx_buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_bip_get_monitoring_image_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_monitoring_image rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

static int cmd_bip_server_get_status(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_bip_get_status_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send get_status rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_server_put_image(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (server_put_image_final) {
		err = bt_bip_add_header_image_handle(bip_app.tx_buf, sizeof(IMAGE_HANDLE) - 1,
						     IMAGE_HANDLE);
		if (err != 0) {
			shell_error(sh, "Fail to add image handle header %d", err);
			return err;
		}
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_bip_put_image_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send put_image rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
		server_put_image_final = false;
	}
	return err;
}

static int cmd_bip_server_put_linked_thumbnail(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (server_put_linked_thumbnail_final) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_bip_put_linked_thumbnail_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send put_linked_thumbnail rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
		server_put_linked_thumbnail_final = false;
	}
	return err;
}

static int cmd_bip_server_put_linked_attachment(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (server_put_linked_attachment_final) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_bip_put_linked_attachment_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send put_linked_attachment rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
		server_put_linked_attachment_final = false;
	}
	return err;
}

static int cmd_bip_server_remote_display(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	rsp_code = BT_OBEX_RSP_CODE_SUCCESS;

	err = bt_bip_add_header_image_handle(bip_app.tx_buf, sizeof(IMAGE_HANDLE) - 1,
					     IMAGE_HANDLE);
	if (err != 0) {
		shell_error(sh, "Fail to add image handle header %d", err);
		return err;
	}

error_rsp:
	err = bt_bip_remote_display_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send remote_display rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_server_delete_image(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	rsp_code = BT_OBEX_RSP_CODE_SUCCESS;

error_rsp:
	err = bt_bip_delete_image_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send delete_image rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_server_start_print(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (server_start_print_final) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}
error_rsp:
	err = bt_bip_start_print_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send start_print rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
		server_start_print_final = false;
	}
	return err;
}

static int cmd_bip_server_start_archive(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	if (bip_app.tx_buf != NULL) {
		net_buf_unref(bip_app.tx_buf);
		bip_app.tx_buf = NULL;
	}

	if (bip_app.tx_buf == NULL) {
		bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	rsp_code = BT_OBEX_RSP_CODE_SUCCESS;

error_rsp:
	err = bt_bip_start_archive_rsp(&bip_app.server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send start_archive rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_bip_sdp_reg(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	static bool registered;

	if (registered) {
		shell_error(sh, "SDP record has been registered");
		return -EINVAL;
	}

	err = bt_sdp_register_service(&bip_responder_rec);
	if (err != 0) {
		shell_error(sh, "Failed to register SDP record");
		return err;
	}

	registered = true;

	return err;
}

static int cmd_sdp_set_caps(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	bip_supported_caps = shell_strtol(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid capabilities %d", err);
		return err;
	}

	return 0;
}

static int cmd_sdp_set_features(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	bip_supported_features = shell_strtol(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid features %d", err);
		return err;
	}

	return 0;
}

static int cmd_sdp_set_functions(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	bip_supported_functions = shell_strtol(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid functions %d", err);
		return err;
	}

	return 0;
}

#define BIP_SDP_DISCOVER_BUF_LEN 512
NET_BUF_POOL_FIXED_DEFINE(sdp_client_pool, CONFIG_BT_MAX_CONN, BIP_SDP_DISCOVER_BUF_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static int bip_sdp_get_goep_l2cap_psm(const struct net_buf *buf, uint16_t *psm)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_GOEP_L2CAP_PSM, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*psm))) {
		return -EINVAL;
	}

	*psm = value.uint.u16;
	return 0;
}

static int bip_sdp_get_functions(const struct net_buf *buf, uint32_t *funcs)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_SUPPORTED_FUNCTIONS, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*funcs))) {
		return -EINVAL;
	}

	*funcs = value.uint.u32;
	return 0;
}

static uint8_t bip_discover_func(struct bt_conn *conn, struct bt_sdp_client_result *result,
				 const struct bt_sdp_discover_params *params)
{
	int err;
	uint16_t features = 0;
	uint32_t functions = 0;
	uint16_t rfcomm_channel = 0;
	uint16_t l2cap_psm = 0;

	if (result == NULL || result->resp_buf == NULL || conn == NULL || params == NULL) {
		bt_shell_info("No record found");
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &rfcomm_channel);
	if (err != 0) {
		bt_shell_error("Failed to get RFCOMM channel: %d", err);
	} else {
		bt_shell_info("Found RFCOMM channel %u", rfcomm_channel);
	}

	err = bip_sdp_get_goep_l2cap_psm(result->resp_buf, &l2cap_psm);
	if (err != 0) {
		bt_shell_error("Failed to get GOEP L2CAP PSM: %d", err);
	} else {
		bt_shell_info("Found GOEP L2CAP PSM %u", l2cap_psm);
	}

	err = bt_sdp_get_features(result->resp_buf, &features);
	if (err != 0) {
		bt_shell_error("Failed to get BIP features: %d", err);
	} else {
		bt_shell_info("Found BIP features 0x%08x", features);
		bip_supported_features = features;
	}

	err = bip_sdp_get_functions(result->resp_buf, &functions);
	if (err != 0) {
		bt_shell_error("Failed to get BIP functions: %d", err);
	} else {
		bt_shell_info("Found BIP functions 0x%08x", functions);
		bip_supported_functions = functions;
	}

	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static int cmd_sdp_discover(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	static struct bt_sdp_discover_params sdp_bip_params;
	static struct bt_uuid_16 uuid;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	uuid.uuid.type = BT_UUID_TYPE_16;
	uuid.val = BT_SDP_IMAGING_SVCLASS;
	sdp_bip_params.uuid = &uuid.uuid;
	sdp_bip_params.func = bip_discover_func;
	sdp_bip_params.pool = &sdp_client_pool;
	sdp_bip_params.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;

	err = bt_sdp_discover(default_conn, &sdp_bip_params);
	if (err != 0) {
		shell_error(sh, "Failed to start SDP discovery %d", err);
		return err;
	}

	return 0;
}

#define HELP_NONE ""

SHELL_STATIC_SUBCMD_SET_CREATE(
	obex_add_header_cmds,
	SHELL_CMD_ARG(count, NULL, "<number of objects (used by Connect)>", cmd_add_header_count, 2,
		      0),
	SHELL_CMD_ARG(name, NULL, "[name of the object (often a file name)]", cmd_add_header_name,
		      1, 1),
	SHELL_CMD_ARG(type, NULL,
		      "<type of object - e.g. text, html, binary, manufacturer specific>",
		      cmd_add_header_type, 2, 0),
	SHELL_CMD_ARG(len, NULL, "<length of the object in bytes>", cmd_add_header_len, 2, 0),
	SHELL_CMD_ARG(time_iso_8601, NULL, "<date/time stamp - ISO 8601 version - preferred>",
		      cmd_add_header_time_iso_8601, 2, 0),
	SHELL_CMD_ARG(time, NULL, "<date/time stamp - 4 byte version (for compatibility only)>",
		      cmd_add_header_time, 2, 0),
	SHELL_CMD_ARG(description, NULL, "<text description of the object>",
		      cmd_add_header_description, 2, 0),
	SHELL_CMD_ARG(target, NULL, "<name of service that operation is targeted to>",
		      cmd_add_header_target, 2, 0),
	SHELL_CMD_ARG(http, NULL, "<an HTTP 1.x header>", cmd_add_header_http, 2, 0),
	SHELL_CMD_ARG(body, NULL, "<a chunk of the object body>", cmd_add_header_body, 2, 0),
	SHELL_CMD_ARG(end_body, NULL, "<the final chunk of the object body>",
		      cmd_add_header_end_body, 2, 0),
	SHELL_CMD_ARG(who, NULL,
		      "<identifies the BIP application, used to tell if talking to a peer>",
		      cmd_add_header_who, 2, 0),
	SHELL_CMD_ARG(conn_id, NULL, "<an identifier used for BIP connection multiplexing>",
		      cmd_add_header_conn_id, 2, 0),
	SHELL_CMD_ARG(app_param, NULL, "application parameter: <tag> <value> [last]",
		      cmd_add_header_app_param, 3, 1),
	SHELL_CMD_ARG(auth_challenge, NULL, "authentication digest-challenge: <tag> <value> [last]",
		      cmd_add_header_auth_challenge, 3, 1),
	SHELL_CMD_ARG(auth_rsp, NULL, "authentication digest-response: <tag> <value> [last]",
		      cmd_add_header_auth_rsp, 3, 1),
	SHELL_CMD_ARG(creator_id, NULL, "<indicates the creator of an object>",
		      cmd_add_header_creator_id, 2, 0),
	SHELL_CMD_ARG(wan_uuid, NULL, "<uniquely identifies the network client (BIP server)>",
		      cmd_add_header_wan_uuid, 2, 0),
	SHELL_CMD_ARG(obj_class, NULL, "<BIP Object class of object>", cmd_add_header_obj_class, 2,
		      0),
	SHELL_CMD_ARG(session_param, NULL, "<parameters used in session commands/responses>",
		      cmd_add_header_session_param, 2, 0),
	SHELL_CMD_ARG(session_seq_number, NULL,
		      "<sequence number used in each BIP packet for reliability>",
		      cmd_add_header_session_seq_number, 2, 0),
	SHELL_CMD_ARG(action_id, NULL,
		      "<specifies the action to be performed (used in ACTION operation)>",
		      cmd_add_header_action_id, 2, 0),
	SHELL_CMD_ARG(dest_name, NULL,
		      "<the destination object name (used in certain ACTION operations)>",
		      cmd_add_header_dest_name, 2, 0),
	SHELL_CMD_ARG(perm, NULL, "<4-byte bit mask for setting permissions>", cmd_add_header_perm,
		      2, 0),
	SHELL_CMD_ARG(srm, NULL, "<1-byte value to setup Single Response Mode (SRM)>",
		      cmd_add_header_srm, 2, 0),
	SHELL_CMD_ARG(srm_param, NULL, "<Single Response Mode (SRM) Parameter>",
		      cmd_add_header_srm_param, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	obex_client_cmds,
	SHELL_CMD_ARG(set_feats_funcs, NULL, HELP_NONE, cmd_bip_client_set_feats_funcs, 1, 0),
	SHELL_CMD_ARG(conn, NULL, "<type> [UUID 128]", cmd_bip_client_conn, 2, 1),
	SHELL_CMD_ARG(disconn, NULL, HELP_NONE, cmd_bip_client_disconn, 1, 0),
	SHELL_CMD_ARG(abort, NULL, HELP_NONE, cmd_bip_client_abort, 1, 0),
	SHELL_CMD_ARG(get_caps, NULL, HELP_NONE, cmd_bip_client_get_caps, 1, 0),
	SHELL_CMD_ARG(get_image_list, NULL, HELP_NONE, cmd_bip_client_get_image_list, 1, 0),
	SHELL_CMD_ARG(get_image_properties, NULL, HELP_NONE, cmd_bip_client_get_image_properties, 1,
		      0),
	SHELL_CMD_ARG(get_image, NULL, HELP_NONE, cmd_bip_client_get_image, 1, 0),
	SHELL_CMD_ARG(get_linked_thumbnail, NULL, HELP_NONE, cmd_bip_client_get_linked_thumbnail, 1,
		      0),
	SHELL_CMD_ARG(get_linked_attachment, NULL, HELP_NONE, cmd_bip_client_get_linked_attachment,
		      1, 0),
	SHELL_CMD_ARG(get_partial_image, NULL, HELP_NONE, cmd_bip_client_get_partial_image, 1, 0),
	SHELL_CMD_ARG(get_monitoring_image, NULL, HELP_NONE, cmd_bip_client_get_monitoring_image, 1,
		      0),
	SHELL_CMD_ARG(get_status, NULL, HELP_NONE, cmd_bip_client_get_status, 1, 0),
	SHELL_CMD_ARG(put_image, NULL, HELP_NONE, cmd_bip_client_put_image, 1, 0),
	SHELL_CMD_ARG(put_linked_thumbnail, NULL, HELP_NONE, cmd_bip_client_put_linked_thumbnail, 1,
		      0),
	SHELL_CMD_ARG(put_linked_attachment, NULL, HELP_NONE, cmd_bip_client_put_linked_attachment,
		      1, 0),
	SHELL_CMD_ARG(remote_display, NULL, HELP_NONE, cmd_bip_client_remote_display, 1, 0),
	SHELL_CMD_ARG(delete_image, NULL, HELP_NONE, cmd_bip_client_delete_image, 1, 0),
	SHELL_CMD_ARG(start_print, NULL, HELP_NONE, cmd_bip_client_start_print, 1, 0),
	SHELL_CMD_ARG(start_archive, NULL, HELP_NONE, cmd_bip_client_start_archive, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	obex_server_cmds, SHELL_CMD_ARG(reg, NULL, "<type> [UUID 128]", cmd_bip_server_reg, 2, 1),
	SHELL_CMD_ARG(unreg, NULL, HELP_NONE, cmd_bip_server_unreg, 1, 0),
	SHELL_CMD_ARG(conn, NULL, "<rsp: continue, success, error> [rsp_code]", cmd_bip_server_conn,
		      2, 1),
	SHELL_CMD_ARG(disconn, NULL, "<rsp: continue, success, error> [rsp_code]",
		      cmd_bip_server_disconn, 2, 1),
	SHELL_CMD_ARG(abort, NULL, "<rsp: continue, success, error> [rsp_code]",
		      cmd_bip_server_abort, 2, 1),
	SHELL_CMD_ARG(get_caps, NULL, "<rsp: noerror, error> [rsp_code]", cmd_bip_server_get_caps,
		      2, 1),
	SHELL_CMD_ARG(get_image_list, NULL, "<rsp: noerror, error> [rsp_code]",
		      cmd_bip_server_get_image_list, 2, 1),
	SHELL_CMD_ARG(get_image_properties, NULL, "<rsp: noerror, error> [rsp_code]",
		      cmd_bip_server_get_image_properties, 2, 1),
	SHELL_CMD_ARG(get_image, NULL, "<rsp: noerror, error> [rsp_code]", cmd_bip_server_get_image,
		      2, 1),
	SHELL_CMD_ARG(get_linked_thumbnail, NULL, "<rsp: noerror, error> [rsp_code]",
		      cmd_bip_server_get_linked_thumbnail, 2, 1),
	SHELL_CMD_ARG(get_linked_attachment, NULL, "<rsp: noerror, error> [rsp_code]",
		      cmd_bip_server_get_linked_attachment, 2, 1),
	SHELL_CMD_ARG(get_partial_image, NULL, "<rsp: noerror, error> [rsp_code]",
		      cmd_bip_server_get_partial_image, 2, 1),
	SHELL_CMD_ARG(get_monitoring_image, NULL, "<rsp: noerror, error> [rsp_code]",
		      cmd_bip_server_get_monitoring_image, 2, 1),
	SHELL_CMD_ARG(get_status, NULL, "<rsp: continue, success, error> [rsp_code]",
		      cmd_bip_server_get_status, 2, 1),
	SHELL_CMD_ARG(put_image, NULL, "<rsp: noerror, error> [rsp_code]", cmd_bip_server_put_image,
		      2, 1),
	SHELL_CMD_ARG(put_linked_thumbnail, NULL, "<rsp: noerror, error> [rsp_code]",
		      cmd_bip_server_put_linked_thumbnail, 2, 1),
	SHELL_CMD_ARG(put_linked_attachment, NULL, "<rsp: noerror, error> [rsp_code]",
		      cmd_bip_server_put_linked_attachment, 2, 1),
	SHELL_CMD_ARG(remote_display, NULL, "<rsp: noerror, error> [rsp_code]",
		      cmd_bip_server_remote_display, 2, 1),
	SHELL_CMD_ARG(delete_image, NULL, "<rsp: noerror, error> [rsp_code]",
		      cmd_bip_server_delete_image, 2, 1),
	SHELL_CMD_ARG(start_print, NULL, "<rsp: noerror, error> [rsp_code]",
		      cmd_bip_server_start_print, 2, 1),
	SHELL_CMD_ARG(start_archive, NULL, "<rsp: noerror, error> [rsp_code]",
		      cmd_bip_server_start_archive, 2, 1),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	bip_sdp_cmds, SHELL_CMD_ARG(reg, NULL, HELP_NONE, cmd_bip_sdp_reg, 1, 0),
	SHELL_CMD_ARG(set_caps, NULL, "<capabilities>", cmd_sdp_set_caps, 2, 0),
	SHELL_CMD_ARG(set_features, NULL, "<features>", cmd_sdp_set_features, 2, 0),
	SHELL_CMD_ARG(set_functions, NULL, "<functions>", cmd_sdp_set_functions, 2, 0),
	SHELL_CMD_ARG(discover, NULL, HELP_NONE, cmd_sdp_discover, 1, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_alloc_buf(const struct shell *sh, size_t argc, char **argv)
{
	if (bip_app.tx_buf != NULL) {
		shell_error(sh, "Buf %p is using", bip_app.tx_buf);
		return -EBUSY;
	}

	bip_app.tx_buf = bt_goep_create_pdu(&bip_app.bip.goep, &tx_pool);
	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	return 0;
}

static int cmd_release_buf(const struct shell *sh, size_t argc, char **argv)
{
	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "No buf is using");
		return -EINVAL;
	}

	net_buf_unref(bip_app.tx_buf);
	bip_app.tx_buf = NULL;

	return 0;
}

static int cmd_common(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	bip_cmds, SHELL_CMD_ARG(register-rfcomm, NULL, "<channel>", cmd_register_rfcomm, 2, 0),
	SHELL_CMD_ARG(connect-rfcomm, NULL, "<channel>", cmd_connect_rfcomm, 2, 0),
	SHELL_CMD_ARG(disconnect-rfcomm, NULL, HELP_NONE, cmd_disconnect_rfcomm, 1, 0),
	SHELL_CMD_ARG(register-l2cap, NULL, "<psm>", cmd_register_l2cap, 2, 0),
	SHELL_CMD_ARG(connect-l2cap, NULL, "<psm>", cmd_connect_l2cap, 2, 0),
	SHELL_CMD_ARG(disconnect-l2cap, NULL, HELP_NONE, cmd_disconnect_l2cap, 1, 0),
	SHELL_CMD_ARG(alloc-buf, NULL, "Alloc tx buffer", cmd_alloc_buf, 1, 0),
	SHELL_CMD_ARG(release-buf, NULL, "Free allocated tx buffer", cmd_release_buf, 1, 0),
	SHELL_CMD_ARG(add-header, &obex_add_header_cmds, "Adding header sets", cmd_common, 1, 0),
	SHELL_CMD_ARG(client, &obex_client_cmds, "Client sets", cmd_common, 1, 0),
	SHELL_CMD_ARG(server, &obex_server_cmds, "Server sets", cmd_common, 1, 0),
	SHELL_CMD_ARG(sdp, &bip_sdp_cmds, "SDP sets", cmd_common, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(bip, &bip_cmds, "Bluetooth BIP shell commands", cmd_common, 1, 1);
