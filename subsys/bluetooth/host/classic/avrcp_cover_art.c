/** @file
 *  @brief Audio Video Remote Control Profile
 */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/bip.h>
#include <zephyr/bluetooth/classic/avrcp_cover_art.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"
#include "avctp_internal.h"
#include "avrcp_internal.h"

#define LOG_LEVEL CONFIG_BT_AVRCP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_avrcp_cover_art);

/** @brief AVRCP Cover Art service UUID */
#define BT_AVRCP_COVER_ART_UUID                                                                    \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0x7163DD54, 0x4A7E, 0x11E2, 0xB47C, 0x0050C2490048))

static const struct bt_uuid_128 *cover_art_uuid = BT_AVRCP_COVER_ART_UUID;

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
static struct bt_avrcp_cover_art_ct avrcp_cover_art_ct[CONFIG_BT_MAX_CONN];

static struct bt_avrcp_cover_art_ct_cb *cover_art_ct_cb;

#define COVER_ART_CT_FROM_BIP(_bip)  CONTAINER_OF(_bip, struct bt_avrcp_cover_art_ct, bip);
#define COVER_ART_CT_FROM_CLIENT(_c) CONTAINER_OF(_c, struct bt_avrcp_cover_art_ct, client);
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART */
#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
static struct bt_avrcp_cover_art_tg avrcp_cover_art_tg[CONFIG_BT_MAX_CONN];

static struct bt_avrcp_cover_art_tg_cb *cover_art_tg_cb;

#define COVER_ART_TG_FROM_BIP(_bip)  CONTAINER_OF(_bip, struct bt_avrcp_cover_art_tg, bip);
#define COVER_ART_TG_FROM_SERVER(_s) CONTAINER_OF(_s, struct bt_avrcp_cover_art_tg, server);
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

#if CONFIG_BT_AVRCP_TG_COVER_ART

static void tg_bip_connect(struct bt_bip_server *server, uint8_t version, uint16_t mopl,
			   struct net_buf *buf)
{
	struct bt_avrcp_cover_art_tg *tg = COVER_ART_TG_FROM_SERVER(server);

	LOG_DBG("BIP server %p conn req, version %02x, mopl %04x", server, version, mopl);

	if ((cover_art_tg_cb != NULL) && (cover_art_tg_cb->connect != NULL)) {
		cover_art_tg_cb->connect(tg, version, mopl, buf);
	}
}

static void tg_bip_disconnect(struct bt_bip_server *server, struct net_buf *buf)
{
	struct bt_avrcp_cover_art_tg *tg = COVER_ART_TG_FROM_SERVER(server);

	LOG_DBG("BIP server %p disconn req", server);

	if ((cover_art_tg_cb != NULL) && (cover_art_tg_cb->disconnect != NULL)) {
		cover_art_tg_cb->disconnect(tg, buf);
	}
}

static void tg_bip_abort(struct bt_bip_server *server, struct net_buf *buf)
{
	struct bt_avrcp_cover_art_tg *tg = COVER_ART_TG_FROM_SERVER(server);

	LOG_DBG("BIP server %p abort req", server);

	if ((cover_art_tg_cb != NULL) && (cover_art_tg_cb->abort != NULL)) {
		cover_art_tg_cb->abort(tg, buf);
	}
}

static void tg_bip_get_image_properties(struct bt_bip_server *server, bool final,
					struct net_buf *buf)
{
	struct bt_avrcp_cover_art_tg *tg = COVER_ART_TG_FROM_SERVER(server);

	LOG_DBG("BIP server %p get_image_properties req, final %s, data len %d", server,
		final ? "true" : "false", buf->len);

	if ((cover_art_tg_cb != NULL) && (cover_art_tg_cb->get_image_properties != NULL)) {
		cover_art_tg_cb->get_image_properties(tg, final, buf);
	}
}

static void tg_bip_get_image(struct bt_bip_server *server, bool final, struct net_buf *buf)
{
	struct bt_avrcp_cover_art_tg *tg = COVER_ART_TG_FROM_SERVER(server);

	LOG_DBG("BIP server %p get_image req, final %s, data len %d", server,
		final ? "true" : "false", buf->len);

	if ((cover_art_tg_cb != NULL) && (cover_art_tg_cb->get_image != NULL)) {
		cover_art_tg_cb->get_image(tg, final, buf);
	}
}

static void tg_bip_get_linked_thumbnail(struct bt_bip_server *server, bool final,
					struct net_buf *buf)
{
	struct bt_avrcp_cover_art_tg *tg = COVER_ART_TG_FROM_SERVER(server);

	LOG_DBG("BIP server %p get_linked_thumbnail req, final %s, data len %d", server,
		final ? "true" : "false", buf->len);

	if ((cover_art_tg_cb != NULL) && (cover_art_tg_cb->get_linked_thumbnail != NULL)) {
		cover_art_tg_cb->get_linked_thumbnail(tg, final, buf);
	}
}

static struct bt_bip_server_cb tg_bip_server_cb = {
	.connect = tg_bip_connect,
	.disconnect = tg_bip_disconnect,
	.abort = tg_bip_abort,
	.get_image_properties = tg_bip_get_image_properties,
	.get_image = tg_bip_get_image,
	.get_linked_thumbnail = tg_bip_get_linked_thumbnail,
};

static void tg_bip_transport_connected(struct bt_conn *conn, struct bt_bip *bip)
{
	struct bt_avrcp_cover_art_tg *tg = COVER_ART_TG_FROM_BIP(bip);
	int err;

	LOG_DBG("BIP %p transport connected on %p", bip, conn);

	tg->conn = bt_conn_ref(conn);

	err = bt_bip_primary_image_pull_server_register(&tg->bip, &tg->server, cover_art_uuid,
							&tg_bip_server_cb);
	if (err != 0) {
		LOG_ERR("Failed to register BIP server: %d", err);
		goto failed;
	}

	if ((cover_art_tg_cb != NULL) && (cover_art_tg_cb->l2cap_connected != NULL)) {
		cover_art_tg_cb->l2cap_connected(tg->tg, tg);
	}

	return;

failed:
	err = bt_avrcp_cover_art_tg_l2cap_disconnect(tg);
	if (err != 0) {
		LOG_ERR("Failed to send l2cap disconnect: %d", err);
	}
}

static void tg_bip_transport_disconnected(struct bt_bip *bip)
{
	struct bt_avrcp_cover_art_tg *tg = COVER_ART_TG_FROM_BIP(bip);
	int err;

	LOG_DBG("BIP %p transport disconnected", bip);

	if (tg->conn != NULL) {
		bt_conn_unref(tg->conn);
	}

	err = bt_bip_server_unregister(&tg->server);
	if (err != 0) {
		LOG_ERR("Failed to unregister BIP server: %d", err);
	}

	if ((cover_art_tg_cb != NULL) && (cover_art_tg_cb->l2cap_disconnected != NULL)) {
		cover_art_tg_cb->l2cap_disconnected(tg);
	}
}

static struct bt_bip_transport_ops tg_bip_transport_ops = {
	.connected = tg_bip_transport_connected,
	.disconnected = tg_bip_transport_disconnected,
};

static int bt_avrcp_tg_cover_art_accept(struct bt_conn *conn, struct bt_bip_l2cap_server *server,
					struct bt_bip **bip)
{
	uint8_t index = bt_conn_index(conn);

	__ASSERT(index < ARRAY_SIZE(avrcp_cover_art_tg), "Conn index out of bounds");

	avrcp_cover_art_tg[index].tg = bt_avrcp_get_tg(conn, server->server.l2cap.psm);
	if (avrcp_cover_art_tg[index].tg == NULL) {
		LOG_ERR("Failed to get AVRCP target for connection");
		return -ENOTCONN;
	}
	avrcp_cover_art_tg[index].bip.ops = &tg_bip_transport_ops;
	*bip = &avrcp_cover_art_tg[index].bip;

	return 0;
}

struct bt_bip_l2cap_server tg_cover_art_server = {
	.server.l2cap.psm = 0,
	.server.l2cap.sec_level = BT_SECURITY_L2,
	.accept = bt_avrcp_tg_cover_art_accept,
};

int bt_avrcp_tg_cover_art_init(uint16_t *psm)
{
	int err;

	if (psm == NULL) {
		return -EINVAL;
	}

	tg_cover_art_server.server.l2cap.psm = 0;
	err = bt_bip_l2cap_register(&tg_cover_art_server);
	if (err < 0) {
		LOG_ERR("Failed to register AVRCP Cover Art BIP transport server %d", err);
		return err;
	}

	LOG_DBG("AVRCP Cover Art PSM 0x%04x", tg_cover_art_server.server.l2cap.psm);
	*psm = tg_cover_art_server.server.l2cap.psm;

	return 0;
}

int bt_avrcp_cover_art_tg_cb_register(struct bt_avrcp_cover_art_tg_cb *cb)
{
	if (cb == NULL) {
		LOG_ERR("Invalid TG cb");
		return -EINVAL;
	}

	if (cover_art_tg_cb != NULL) {
		LOG_ERR("TG cb has been registered");
		return -EBUSY;
	}

	cover_art_tg_cb = cb;

	return 0;
}

int bt_avrcp_cover_art_tg_l2cap_disconnect(struct bt_avrcp_cover_art_tg *cover_art_tg)
{
	if (cover_art_tg == NULL) {
		LOG_ERR("Invalid cover art target");
		return -EINVAL;
	}

	if (cover_art_tg->conn == NULL) {
		LOG_ERR("TG is not connected");
		return -ENOTCONN;
	}

	return bt_bip_l2cap_disconnect(&cover_art_tg->bip);
}

struct net_buf *bt_avrcp_cover_art_tg_create_pdu(struct bt_avrcp_cover_art_tg *tg,
						 struct net_buf_pool *pool)
{
	if (tg == NULL) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	return bt_goep_create_pdu(&tg->bip.goep, pool);
}

int bt_avrcp_cover_art_tg_connect(struct bt_avrcp_cover_art_tg *tg, uint8_t rsp_code)
{
	int err;

	if (tg == NULL) {
		LOG_ERR("Invalid cover art target");
		return -EINVAL;
	}

	if (tg->conn == NULL) {
		LOG_ERR("TG is not connected");
		return -ENOTCONN;
	}

	err = bt_bip_connect_rsp(&tg->server, rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send OBEX connect rsp %d", err);
	}
	return err;
}

int bt_avrcp_cover_art_tg_disconnect(struct bt_avrcp_cover_art_tg *tg, uint8_t rsp_code)
{
	int err;

	if (tg == NULL) {
		LOG_ERR("Invalid cover art target");
		return -EINVAL;
	}

	if (tg->conn == NULL) {
		LOG_ERR("TG is not connected");
		return -ENOTCONN;
	}

	err = bt_bip_disconnect_rsp(&tg->server, rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send OBEX disconnect rsp %d", err);
	}
	return err;
}

int bt_avrcp_cover_art_tg_abort(struct bt_avrcp_cover_art_tg *tg, uint8_t rsp_code,
				struct net_buf *buf)
{
	int err;

	if (tg == NULL) {
		LOG_ERR("Invalid cover art target");
		return -EINVAL;
	}

	if (tg->conn == NULL) {
		LOG_ERR("TG is not connected");
		return -ENOTCONN;
	}

	err = bt_bip_abort_rsp(&tg->server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send OBEX abort rsp %d", err);
	}
	return err;
}

int bt_avrcp_cover_art_tg_get_image_properties(struct bt_avrcp_cover_art_tg *tg, uint8_t rsp_code,
					       struct net_buf *buf)
{
	int err;

	if (tg == NULL) {
		LOG_ERR("Invalid cover art target");
		return -EINVAL;
	}

	if (tg->conn == NULL) {
		LOG_ERR("TG is not connected");
		return -ENOTCONN;
	}

	err = bt_bip_get_image_properties_rsp(&tg->server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send OBEX get image properties rsp %d", err);
	}
	return err;
}

int bt_avrcp_cover_art_tg_get_image(struct bt_avrcp_cover_art_tg *tg, uint8_t rsp_code,
				    struct net_buf *buf)
{
	int err;

	if (tg == NULL) {
		LOG_ERR("Invalid cover art target");
		return -EINVAL;
	}

	if (tg->conn == NULL) {
		LOG_ERR("TG is not connected");
		return -ENOTCONN;
	}

	err = bt_bip_get_image_rsp(&tg->server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send OBEX get image rsp %d", err);
	}
	return err;
}

int bt_avrcp_cover_art_tg_get_linked_thumbnail(struct bt_avrcp_cover_art_tg *tg, uint8_t rsp_code,
					       struct net_buf *buf)
{
	int err;

	if (tg == NULL) {
		LOG_ERR("Invalid cover art target");
		return -EINVAL;
	}

	if (tg->conn == NULL) {
		LOG_ERR("TG is not connected");
		return -ENOTCONN;
	}

	err = bt_bip_get_linked_thumbnail_rsp(&tg->server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send OBEX get linked thumbnail rsp %d", err);
	}
	return err;
}
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
static void ct_bip_connect(struct bt_bip_client *client, uint8_t rsp_code, uint8_t version,
			   uint16_t mopl, struct net_buf *buf)
{
	struct bt_avrcp_cover_art_ct *ct = COVER_ART_CT_FROM_CLIENT(client);

	LOG_DBG("BIP client %p conn rsp, rsp_code %s, version %02x, mopl %04x", client,
		bt_obex_rsp_code_to_str(rsp_code), version, mopl);

	if ((cover_art_ct_cb != NULL) && (cover_art_ct_cb->connect != NULL)) {
		cover_art_ct_cb->connect(ct, rsp_code, version, mopl, buf);
	}
}

static void ct_bip_disconnect(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_avrcp_cover_art_ct *ct = COVER_ART_CT_FROM_CLIENT(client);

	LOG_DBG("BIP client %p disconn rsp, rsp_code %s", client,
		bt_obex_rsp_code_to_str(rsp_code));

	if ((cover_art_ct_cb != NULL) && (cover_art_ct_cb->disconnect != NULL)) {
		cover_art_ct_cb->disconnect(ct, rsp_code, buf);
	}
}

static void ct_bip_abort(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_avrcp_cover_art_ct *ct = COVER_ART_CT_FROM_CLIENT(client);

	LOG_DBG("BIP client %p abort rsp, rsp_code %s", client, bt_obex_rsp_code_to_str(rsp_code));

	if ((cover_art_ct_cb != NULL) && (cover_art_ct_cb->abort != NULL)) {
		cover_art_ct_cb->abort(ct, rsp_code, buf);
	}
}

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
static void ct_bip_get_image_properties(struct bt_bip_client *client, uint8_t rsp_code,
					struct net_buf *buf)
{
	struct bt_avrcp_cover_art_ct *ct = COVER_ART_CT_FROM_CLIENT(client);

	LOG_DBG("BIP client %p get_image_properties rsp, rsp_code %s, data len %d", client,
		bt_obex_rsp_code_to_str(rsp_code), buf->len);

	if ((cover_art_ct_cb != NULL) && (cover_art_ct_cb->get_image_properties != NULL)) {
		cover_art_ct_cb->get_image_properties(ct, rsp_code, buf);
	}
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE)
static void ct_bip_get_image(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_avrcp_cover_art_ct *ct = COVER_ART_CT_FROM_CLIENT(client);

	LOG_DBG("BIP client %p get_image rsp, rsp_code %s, data len %d", client,
		bt_obex_rsp_code_to_str(rsp_code), buf->len);

	if ((cover_art_ct_cb != NULL) && (cover_art_ct_cb->get_image != NULL)) {
		cover_art_ct_cb->get_image(ct, rsp_code, buf);
	}
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL)
static void ct_bip_get_linked_thumbnail(struct bt_bip_client *client, uint8_t rsp_code,
					struct net_buf *buf)
{
	struct bt_avrcp_cover_art_ct *ct = COVER_ART_CT_FROM_CLIENT(client);

	LOG_DBG("BIP client %p get_linked_thumbnail rsp, rsp_code %s, data len %d", client,
		bt_obex_rsp_code_to_str(rsp_code), buf->len);

	if ((cover_art_ct_cb != NULL) && (cover_art_ct_cb->get_linked_thumbnail != NULL)) {
		cover_art_ct_cb->get_linked_thumbnail(ct, rsp_code, buf);
	}
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL */

static struct bt_bip_client_cb ct_bip_client_cb = {
	.connect = ct_bip_connect,
	.disconnect = ct_bip_disconnect,
	.abort = ct_bip_abort,
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
	.get_image_properties = ct_bip_get_image_properties,
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES */
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE)
	.get_image = ct_bip_get_image,
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE */
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL)
	.get_linked_thumbnail = ct_bip_get_linked_thumbnail,
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL */
};

static void ct_bip_transport_connected(struct bt_conn *conn, struct bt_bip *bip)
{
	struct bt_avrcp_cover_art_ct *ct = COVER_ART_CT_FROM_BIP(bip);
	int err;
	uint32_t functions;

	LOG_DBG("Cover Art CT %p transport connected on %p", ct, conn);

	ct->conn = bt_conn_ref(conn);

	err = bt_bip_set_supported_capabilities(&ct->bip, BIT(BT_BIP_SUPP_CAP_GENERIC_IMAGE));
	if (err != 0) {
		LOG_ERR("Failed to set supported capabilities: %d", err);
		goto failed;
	}

	err = bt_bip_set_supported_features(&ct->bip, BIT(BT_BIP_SUPP_FEAT_IMAGE_PULL));
	if (err != 0) {
		LOG_ERR("Failed to set supported features: %d", err);
		goto failed;
	}

	functions = BIT(BT_BIP_SUPP_FUNC_GET_IMAGE_PROPERTIES) | BIT(BT_BIP_SUPP_FUNC_GET_IMAGE) |
		    BIT(BT_BIP_SUPP_FUNC_GET_LINKED_THUMBNAIL);
	err = bt_bip_set_supported_functions(&ct->bip, functions);
	if (err != 0) {
		LOG_ERR("Failed to set supported functions: %d", err);
		goto failed;
	}

	if ((cover_art_ct_cb != NULL) && (cover_art_ct_cb->l2cap_connected != NULL)) {
		cover_art_ct_cb->l2cap_connected(ct->ct, ct);
	}

	return;

failed:
	err = bt_avrcp_cover_art_ct_l2cap_disconnect(ct);
	if (err != 0) {
		LOG_ERR("Failed to send l2cap disconnect: %d", err);
	}
}

static void ct_bip_transport_disconnected(struct bt_bip *bip)
{
	struct bt_avrcp_cover_art_ct *ct = COVER_ART_CT_FROM_BIP(bip);

	LOG_DBG("Cover Art CT %p  transport disconnected", ct);

	if (ct->conn != NULL) {
		bt_conn_unref(ct->conn);
	}

	if ((cover_art_ct_cb != NULL) && (cover_art_ct_cb->l2cap_disconnected != NULL)) {
		cover_art_ct_cb->l2cap_disconnected(ct);
	}
}

static struct bt_bip_transport_ops ct_bip_transport_ops = {
	.connected = ct_bip_transport_connected,
	.disconnected = ct_bip_transport_disconnected,
};

int bt_avrcp_cover_art_ct_cb_register(struct bt_avrcp_cover_art_ct_cb *cb)
{
	if (cb == NULL) {
		LOG_ERR("Invalid CT cb");
		return -EINVAL;
	}

	if (cover_art_ct_cb != NULL) {
		LOG_ERR("CT cb has been registered");
		return -EBUSY;
	}

	cover_art_ct_cb = cb;

	return 0;
}

int bt_avrcp_cover_art_ct_l2cap_connect(struct bt_avrcp_ct *ct,
					struct bt_avrcp_cover_art_ct **cover_art_ct, uint16_t psm)
{
	uint8_t index;
	int err;
	struct bt_conn *conn;

	conn = bt_avrcp_ct_get_acl_conn(ct);

	if (conn == NULL) {
		LOG_ERR("No ACL connect");
		return -ENOTCONN;
	}

	bt_conn_unref(conn);

	index = bt_conn_index(conn);

	if (index >= ARRAY_SIZE(avrcp_cover_art_ct)) {
		LOG_ERR("Conn index out of bounds");
		return -EINVAL;
	}

	avrcp_cover_art_ct[index].bip.ops = &ct_bip_transport_ops;

	err = bt_bip_l2cap_connect(conn, &avrcp_cover_art_ct[index].bip, psm);
	if (err != 0) {
		LOG_ERR("Failed to connect AVRCP Cover Art L2CAP channel (err %d)", err);
		return err;
	}

	if (cover_art_ct != NULL) {
		*cover_art_ct = &avrcp_cover_art_ct[index];
	}

	return 0;
}

int bt_avrcp_cover_art_ct_l2cap_disconnect(struct bt_avrcp_cover_art_ct *ct)
{
	if (ct == NULL) {
		LOG_ERR("Invalid cover art controller");
		return -EINVAL;
	}

	if (ct->conn == NULL) {
		LOG_ERR("CT is not connected");
		return -ENOTCONN;
	}

	return bt_bip_l2cap_disconnect(&ct->bip);
}

struct net_buf *bt_avrcp_cover_art_ct_create_pdu(struct bt_avrcp_cover_art_ct *ct,
						 struct net_buf_pool *pool)
{
	if (ct == NULL) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	return bt_goep_create_pdu(&ct->bip.goep, pool);
}

int bt_avrcp_cover_art_ct_connect(struct bt_avrcp_cover_art_ct *ct)
{
	int err;
	struct net_buf *buf;
	uint8_t val[BT_UUID_SIZE_128];

	if (ct == NULL) {
		LOG_ERR("Invalid cover art controller");
		return -EINVAL;
	}

	if (ct->conn == NULL) {
		LOG_ERR("CT is not connected");
		return -ENOTCONN;
	}

	buf = bt_avrcp_cover_art_ct_create_pdu(ct, NULL);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	sys_memcpy_swap(val, cover_art_uuid->val, sizeof(val));
	err = bt_obex_add_header_target(buf, sizeof(val), val);
	if (err != 0) {
		LOG_ERR("Failed to add target header: %d", err);
		goto cleanup;
	}

	err = bt_bip_primary_image_pull_client_connect(&ct->bip, &ct->client, &ct_bip_client_cb,
						       buf);
	if (err != 0) {
		LOG_ERR("Failed to send OBEX connect %d", err);
		goto cleanup;
	}

	return 0;

cleanup:
	net_buf_unref(buf);
	return err;
}

int bt_avrcp_cover_art_ct_disconnect(struct bt_avrcp_cover_art_ct *ct)
{
	int err;

	if (ct == NULL) {
		LOG_ERR("Invalid cover art controller");
		return -EINVAL;
	}

	if (ct->conn == NULL) {
		LOG_ERR("CT is not connected");
		return -ENOTCONN;
	}

	err = bt_bip_disconnect(&ct->client, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send OBEX disconnect %d", err);
	}
	return err;
}

int bt_avrcp_cover_art_ct_abort(struct bt_avrcp_cover_art_ct *ct, struct net_buf *buf)
{
	int err;

	if (ct == NULL) {
		LOG_ERR("Invalid cover art controller");
		return -EINVAL;
	}

	if (ct->conn == NULL) {
		LOG_ERR("CT is not connected");
		return -ENOTCONN;
	}

	err = bt_bip_abort(&ct->client, buf);
	if (err != 0) {
		LOG_ERR("Failed to send OBEX abort %d", err);
	}
	return err;
}

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
int bt_avrcp_cover_art_ct_get_image_properties(struct bt_avrcp_cover_art_ct *ct, bool final,
					       struct net_buf *buf)
{
	int err;

	if (ct == NULL) {
		LOG_ERR("Invalid cover art controller");
		return -EINVAL;
	}

	if (ct->conn == NULL) {
		LOG_ERR("CT is not connected");
		return -ENOTCONN;
	}

	err = bt_bip_get_image_properties(&ct->client, final, buf);
	if (err != 0) {
		LOG_ERR("Failed to send OBEX get image properties %d", err);
	}
	return err;
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE)
int bt_avrcp_cover_art_ct_get_image(struct bt_avrcp_cover_art_ct *ct, bool final,
				    struct net_buf *buf)
{
	int err;

	if (ct == NULL) {
		LOG_ERR("Invalid cover art controller");
		return -EINVAL;
	}

	if (ct->conn == NULL) {
		LOG_ERR("CT is not connected");
		return -ENOTCONN;
	}

	err = bt_bip_get_image(&ct->client, final, buf);
	if (err != 0) {
		LOG_ERR("Failed to send OBEX get image %d", err);
	}
	return err;
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL)
int bt_avrcp_cover_art_ct_get_linked_thumbnail(struct bt_avrcp_cover_art_ct *ct, bool final,
					       struct net_buf *buf)
{
	int err;

	if (ct == NULL) {
		LOG_ERR("Invalid cover art controller");
		return -EINVAL;
	}

	if (ct->conn == NULL) {
		LOG_ERR("CT is not connected");
		return -ENOTCONN;
	}

	err = bt_bip_get_linked_thumbnail(&ct->client, final, buf);
	if (err != 0) {
		LOG_ERR("Failed to send OBEX get linked thumbnail %d", err);
	}
	return err;
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL */
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART */
