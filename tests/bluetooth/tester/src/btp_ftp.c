/* btp_ftp.c - Bluetooth FTP Tester */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/ftp.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_ftp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

/*
 * TX buffer size for FTP PDUs.
 * 6 bytes - L2CAP I-frame overhead (4-byte extended control field + 2-byte FCS).
 * BT_OBEX_SEND_BUF_RESERVE - OBEX send-side headroom.
 * BT_OBEX_HDR_LEN           - OBEX packet header length.
 */
#define FTP_MOPL        CONFIG_BT_GOEP_RFCOMM_MTU
#define FTP_TX_BUF_SIZE BT_L2CAP_BUF_SIZE(FTP_MOPL + 6 + BT_OBEX_SEND_BUF_RESERVE - BT_OBEX_HDR_LEN)

NET_BUF_POOL_FIXED_DEFINE(ftp_tx_pool, CONFIG_BT_MAX_CONN, FTP_TX_BUF_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#define SDP_CLIENT_USER_BUF_LEN 512
NET_BUF_POOL_DEFINE(sdp_client_pool, 1, SDP_CLIENT_USER_BUF_LEN, 8, NULL);

#define FTP_CLIENT_CONNECT_RFCOMM 0
#define FTP_CLIENT_CONNECT_L2CAP  1

struct ftp_client_instance {
	struct bt_ftp_client client;
	struct bt_conn *conn;
};

struct ftp_server_instance {
	struct bt_ftp_server server;
	struct bt_conn *conn;
};

static struct ftp_client_instance ftp_client_instances[CONFIG_BT_MAX_CONN];
static struct ftp_server_instance ftp_server_instances[CONFIG_BT_MAX_CONN];
static uint8_t ftp_client_connect_flags;

static struct ftp_client_instance *ftp_client_alloc(struct bt_conn *conn)
{
	uint8_t index = bt_conn_index(conn);

	if (index >= CONFIG_BT_MAX_CONN) {
		return NULL;
	}

	if (ftp_client_instances[index].conn != NULL) {
		return NULL;
	}

	ftp_client_instances[index].conn = bt_conn_ref(conn);
	return &ftp_client_instances[index];
}

static void ftp_client_free(struct bt_ftp_client *client)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);

	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		memset(&inst->conn, 0, sizeof(*inst) - offsetof(struct ftp_client_instance, conn));
	}
}

static struct ftp_client_instance *ftp_client_find(const bt_addr_le_t *addr)
{
	struct bt_conn *conn;
	uint8_t index;

	conn = bt_conn_lookup_addr_br(&addr->a);
	if (conn == NULL) {
		return NULL;
	}

	index = bt_conn_index(conn);
	bt_conn_unref(conn);

	if (index >= CONFIG_BT_MAX_CONN) {
		return NULL;
	}

	if (ftp_client_instances[index].conn == NULL) {
		return NULL;
	}

	return &ftp_client_instances[index];
}

static void client_rfcomm_connected_cb(struct bt_conn *conn, struct bt_ftp_client *client)
{
	struct btp_ftp_client_rfcomm_connected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_RFCOMM_CONNECTED, &ev, sizeof(ev));
}

static void client_rfcomm_disconnected_cb(struct bt_ftp_client *client)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_rfcomm_disconnected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(inst->conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_RFCOMM_DISCONNECTED, &ev, sizeof(ev));
	ftp_client_free(client);
}

static void client_l2cap_connected_cb(struct bt_conn *conn, struct bt_ftp_client *client)
{
	struct btp_ftp_client_l2cap_connected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_L2CAP_CONNECTED, &ev, sizeof(ev));
}

static void client_l2cap_disconnected_cb(struct bt_ftp_client *client)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_l2cap_disconnected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(inst->conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_L2CAP_DISCONNECTED, &ev, sizeof(ev));
	ftp_client_free(client);
}

static void client_connect_cb(struct bt_ftp_client *client, uint8_t rsp_code, uint8_t version,
			      uint16_t mopl, struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_connect_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->version = version;
	mopl = MIN(FTP_MOPL, mopl);
	ev->mopl = sys_cpu_to_le16(mopl);
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_CONNECT, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void client_disconnect_cb(struct bt_ftp_client *client, uint8_t rsp_code,
				 struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_disconnect_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_DISCONNECT, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void client_abort_cb(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_abort_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_ABORT, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void client_set_folder_cb(struct bt_ftp_client *client, uint8_t rsp_code,
				 struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_set_folder_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_SET_FOLDER, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void client_pull_folder_listing_cb(struct bt_ftp_client *client, uint8_t rsp_code,
					  struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_pull_folder_listing_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_PULL_FOLDER_LISTING, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void client_push_file_cb(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_push_file_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_PUSH_FILE, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void client_pull_file_cb(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_pull_file_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_PULL_FILE, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void client_delete_cb(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_delete_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_DELETE, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void client_rename_cb(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_rename_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_RENAME, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void client_copy_cb(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_copy_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_COPY, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void client_set_permission_cb(struct bt_ftp_client *client, uint8_t rsp_code,
				     struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	struct btp_ftp_client_set_permission_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_CLIENT_EV_SET_PERMISSION, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static struct bt_ftp_client_cb ftp_client_cb = {
	.rfcomm_connected = client_rfcomm_connected_cb,
	.rfcomm_disconnected = client_rfcomm_disconnected_cb,
	.l2cap_connected = client_l2cap_connected_cb,
	.l2cap_disconnected = client_l2cap_disconnected_cb,
	.connect = client_connect_cb,
	.disconnect = client_disconnect_cb,
	.abort = client_abort_cb,
	.set_folder = client_set_folder_cb,
	.pull_folder_listing = client_pull_folder_listing_cb,
	.push_file = client_push_file_cb,
	.pull_file = client_pull_file_cb,
	.delete = client_delete_cb,
	.rename = client_rename_cb,
	.copy = client_copy_cb,
	.set_permission = client_set_permission_cb,
};

static int ftp_sdp_get_goep_l2cap_psm(const struct net_buf *buf, uint16_t *psm)
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

static uint8_t ftp_sdp_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
			  const struct bt_sdp_discover_params *params)
{
	struct ftp_client_instance *inst;
	uint16_t rfcomm_channel = 0;
	uint16_t l2cap_psm = 0;
	int err;

	if (result == NULL || result->resp_buf == NULL || conn == NULL) {
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	/* Extract RFCOMM channel */
	(void)bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &rfcomm_channel);

	/* Extract L2CAP PSM */
	(void)ftp_sdp_get_goep_l2cap_psm(result->resp_buf, &l2cap_psm);

	/* Allocate client instance indexed by conn */
	inst = ftp_client_alloc(conn);
	if (inst == NULL) {
		return BT_SDP_DISCOVER_UUID_STOP;
	}
	memset(&inst->client, 0, sizeof(inst->client));

	if (ftp_client_connect_flags == FTP_CLIENT_CONNECT_L2CAP) {
		if (l2cap_psm != 0) {
			err = bt_ftp_client_l2cap_connect(conn, &inst->client, &ftp_client_cb,
							  l2cap_psm);
			if (err != 0) {
				ftp_client_free(&inst->client);
			}
		}

		return BT_SDP_DISCOVER_UUID_STOP;
	}

	/* RFCOMM path (also L2CAP fallback) */
	if (rfcomm_channel == 0) {
		ftp_client_free(&inst->client);
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	err = bt_ftp_client_rfcomm_connect(conn, &inst->client, &ftp_client_cb,
					   (uint8_t)rfcomm_channel);
	if (err != 0) {
		ftp_client_free(&inst->client);
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

static struct bt_sdp_discover_params discov_ftp = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.uuid = BT_UUID_DECLARE_16(BT_SDP_OBEX_FILETRANS_SVCLASS),
	.func = ftp_sdp_cb,
	.pool = &sdp_client_pool,
};

static uint8_t client_rfcomm_connect(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_ftp_client_rfcomm_connect_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	ftp_client_connect_flags = FTP_CLIENT_CONNECT_RFCOMM;

	err = bt_sdp_discover(conn, &discov_ftp);
	bt_conn_unref(conn);

	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_rfcomm_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_ftp_client_rfcomm_disconnect_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	int err;

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_client_rfcomm_disconnect(&inst->client);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t client_l2cap_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_client_l2cap_connect_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	ftp_client_connect_flags = FTP_CLIENT_CONNECT_L2CAP;

	err = bt_sdp_discover(conn, &discov_ftp);
	bt_conn_unref(conn);

	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_l2cap_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_ftp_client_l2cap_disconnect_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	int err;

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_client_l2cap_disconnect(&inst->client);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t client_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_client_connect_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_client_connect(&inst->client, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_client_disconnect_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	int err;

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_client_disconnect(&inst->client, NULL);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t client_abort(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_client_abort_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	int err;

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_client_abort(&inst->client, NULL);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t client_set_folder(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_client_set_folder_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_client_set_folder(&inst->client, cp->flags, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_pull_folder_listing(const void *cmd, uint16_t cmd_len, void *rsp,
					  uint16_t *rsp_len)
{
	const struct btp_ftp_client_pull_folder_listing_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_client_pull_folder_listing(&inst->client, (bool)cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_push_file(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_client_push_file_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_client_push_file(&inst->client, (bool)cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_pull_file(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_client_pull_file_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_client_pull_file(&inst->client, (bool)cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_delete(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_client_delete_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_client_delete(&inst->client, (bool)cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_rename(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_client_rename_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_client_rename(&inst->client, (bool)cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_copy(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_client_copy_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_client_copy(&inst->client, (bool)cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t client_set_permission(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_ftp_client_set_permission_cmd *cp = cmd;
	struct ftp_client_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_client_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_client_set_permission(&inst->client, (bool)cp->final, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static struct ftp_server_instance *ftp_server_alloc(struct bt_conn *conn)
{
	uint8_t index = bt_conn_index(conn);

	if (index >= CONFIG_BT_MAX_CONN) {
		return NULL;
	}

	if (ftp_server_instances[index].conn != NULL) {
		return NULL;
	}

	ftp_server_instances[index].conn = bt_conn_ref(conn);
	return &ftp_server_instances[index];
}

static void ftp_server_free(struct bt_ftp_server *server)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);

	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		memset(&inst->conn, 0, sizeof(*inst) - offsetof(struct ftp_server_instance, conn));
	}
}

static struct ftp_server_instance *ftp_server_find(const bt_addr_le_t *addr)
{
	struct bt_conn *conn;
	uint8_t index;

	conn = bt_conn_lookup_addr_br(&addr->a);
	if (conn == NULL) {
		return NULL;
	}

	index = bt_conn_index(conn);
	bt_conn_unref(conn);

	if (index >= CONFIG_BT_MAX_CONN) {
		return NULL;
	}

	if (ftp_server_instances[index].conn == NULL) {
		return NULL;
	}

	return &ftp_server_instances[index];
}

static void server_rfcomm_connected_cb(struct bt_conn *conn, struct bt_ftp_server *server)
{
	struct btp_ftp_server_rfcomm_connected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_RFCOMM_CONNECTED, &ev, sizeof(ev));
}

static void server_rfcomm_disconnected_cb(struct bt_ftp_server *server)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_rfcomm_disconnected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(inst->conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_RFCOMM_DISCONNECTED, &ev, sizeof(ev));
	ftp_server_free(server);
}

static void server_l2cap_connected_cb(struct bt_conn *conn, struct bt_ftp_server *server)
{
	struct btp_ftp_server_l2cap_connected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_L2CAP_CONNECTED, &ev, sizeof(ev));
}

static void server_l2cap_disconnected_cb(struct bt_ftp_server *server)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_l2cap_disconnected_ev ev;

	bt_addr_copy(&ev.address.a, bt_conn_get_dst_br(inst->conn));
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_L2CAP_DISCONNECTED, &ev, sizeof(ev));
	ftp_server_free(server);
}

static void server_connect_cb(struct bt_ftp_server *server, uint8_t version, uint16_t mopl,
			      struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_connect_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->version = version;
	mopl = MIN(FTP_MOPL, mopl);
	ev->mopl = sys_cpu_to_le16(mopl);
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_CONNECT, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void server_disconnect_cb(struct bt_ftp_server *server, struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_disconnect_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_DISCONNECT, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void server_abort_cb(struct bt_ftp_server *server, struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_abort_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_ABORT, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void server_set_folder_cb(struct bt_ftp_server *server, uint8_t flags, struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_set_folder_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->flags = flags;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_SET_FOLDER, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void server_pull_folder_listing_cb(struct bt_ftp_server *server, bool final,
					  struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_pull_folder_listing_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_PULL_FOLDER_LISTING, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void server_push_file_cb(struct bt_ftp_server *server, bool final, struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_push_file_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_PUSH_FILE, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void server_pull_file_cb(struct bt_ftp_server *server, bool final, struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_pull_file_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_PULL_FILE, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void server_delete_cb(struct bt_ftp_server *server, bool final, struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_delete_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_DELETE, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void server_rename_cb(struct bt_ftp_server *server, bool final, struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_rename_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_RENAME, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void server_copy_cb(struct bt_ftp_server *server, bool final, struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_copy_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_COPY, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void server_set_permission_cb(struct bt_ftp_server *server, bool final, struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	struct btp_ftp_server_set_permission_ev *ev;
	uint16_t buf_len = buf ? buf->len : 0;
	uint16_t ev_len = sizeof(*ev) + buf_len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, (uint8_t **)&ev);

	bt_addr_copy(&ev->address.a, bt_conn_get_dst_br(inst->conn));
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->final = final ? 1 : 0;
	ev->buf_len = sys_cpu_to_le16(buf_len);
	if (buf_len > 0) {
		memcpy(ev->buf, buf->data, buf_len);
	}

	tester_event(BTP_SERVICE_ID_FTP, BTP_FTP_SERVER_EV_SET_PERMISSION, ev, ev_len);
	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static struct bt_ftp_server_cb ftp_server_cb = {
	.rfcomm_connected = server_rfcomm_connected_cb,
	.rfcomm_disconnected = server_rfcomm_disconnected_cb,
	.l2cap_connected = server_l2cap_connected_cb,
	.l2cap_disconnected = server_l2cap_disconnected_cb,
	.connect = server_connect_cb,
	.disconnect = server_disconnect_cb,
	.abort = server_abort_cb,
	.set_folder = server_set_folder_cb,
	.pull_folder_listing = server_pull_folder_listing_cb,
	.push_file = server_push_file_cb,
	.pull_file = server_pull_file_cb,
	.delete = server_delete_cb,
	.rename = server_rename_cb,
	.copy = server_copy_cb,
	.set_permission = server_set_permission_cb,
};

static uint8_t server_rfcomm_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_ftp_server_rfcomm_disconnect_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	int err;

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_server_rfcomm_disconnect(&inst->server);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t server_l2cap_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_ftp_server_l2cap_disconnect_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	int err;

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_server_l2cap_disconnect(&inst->server);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t server_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_server_connect_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_server_create_pdu(&inst->server, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_server_connect(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t server_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_server_disconnect_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	int err;

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_server_disconnect(&inst->server, cp->rsp_code, NULL);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t server_abort(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_server_abort_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	int err;

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_server_abort(&inst->server, cp->rsp_code, NULL);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t server_set_folder(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_server_set_folder_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	int err;

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_server_set_folder(&inst->server, cp->rsp_code, NULL);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t server_pull_folder_listing(const void *cmd, uint16_t cmd_len, void *rsp,
					  uint16_t *rsp_len)
{
	const struct btp_ftp_server_pull_folder_listing_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_server_create_pdu(&inst->server, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_server_pull_folder_listing(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t server_push_file(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_server_push_file_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_server_create_pdu(&inst->server, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_server_push_file(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t server_pull_file(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_server_pull_file_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	struct net_buf *buf;
	uint16_t buf_len;
	int err;

	if (cmd_len < sizeof(*cp)) {
		return BTP_STATUS_FAILED;
	}

	buf_len = sys_le16_to_cpu(cp->buf_len);
	if (cmd_len != sizeof(*cp) + buf_len) {
		return BTP_STATUS_FAILED;
	}

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_ftp_server_create_pdu(&inst->server, &ftp_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (buf_len > 0) {
		if (net_buf_tailroom(buf) < buf_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, buf_len);
	}

	err = bt_ftp_server_pull_file(&inst->server, cp->rsp_code, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t server_delete(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_server_delete_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	int err;

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_server_delete(&inst->server, cp->rsp_code, NULL);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t server_rename(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_server_rename_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	int err;

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_server_rename(&inst->server, cp->rsp_code, NULL);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t server_copy(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ftp_server_copy_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	int err;

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_server_copy(&inst->server, cp->rsp_code, NULL);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t server_set_permission(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_ftp_server_set_permission_cmd *cp = cmd;
	struct ftp_server_instance *inst;
	int err;

	inst = ftp_server_find(&cp->address);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_ftp_server_set_permission(&inst->server, cp->rsp_code, NULL);
	return err == 0 ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED;
}

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct btp_ftp_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_FTP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_FTP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
	{
		.opcode = BTP_FTP_CLIENT_RFCOMM_CONNECT,
		.expect_len = sizeof(struct btp_ftp_client_rfcomm_connect_cmd),
		.func = client_rfcomm_connect,
	},
	{
		.opcode = BTP_FTP_CLIENT_RFCOMM_DISCONNECT,
		.expect_len = sizeof(struct btp_ftp_client_rfcomm_disconnect_cmd),
		.func = client_rfcomm_disconnect,
	},
	{
		.opcode = BTP_FTP_CLIENT_L2CAP_CONNECT,
		.expect_len = sizeof(struct btp_ftp_client_l2cap_connect_cmd),
		.func = client_l2cap_connect,
	},
	{
		.opcode = BTP_FTP_CLIENT_L2CAP_DISCONNECT,
		.expect_len = sizeof(struct btp_ftp_client_l2cap_disconnect_cmd),
		.func = client_l2cap_disconnect,
	},
	{
		.opcode = BTP_FTP_CLIENT_CONNECT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = client_connect,
	},
	{
		.opcode = BTP_FTP_CLIENT_DISCONNECT,
		.expect_len = sizeof(struct btp_ftp_client_disconnect_cmd),
		.func = client_disconnect,
	},
	{
		.opcode = BTP_FTP_CLIENT_ABORT,
		.expect_len = sizeof(struct btp_ftp_client_abort_cmd),
		.func = client_abort,
	},
	{
		.opcode = BTP_FTP_CLIENT_SET_FOLDER,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = client_set_folder,
	},
	{
		.opcode = BTP_FTP_CLIENT_PULL_FOLDER_LISTING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = client_pull_folder_listing,
	},
	{
		.opcode = BTP_FTP_CLIENT_PUSH_FILE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = client_push_file,
	},
	{
		.opcode = BTP_FTP_CLIENT_PULL_FILE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = client_pull_file,
	},
	{
		.opcode = BTP_FTP_CLIENT_DELETE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = client_delete,
	},
	{
		.opcode = BTP_FTP_CLIENT_RENAME,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = client_rename,
	},
	{
		.opcode = BTP_FTP_CLIENT_COPY,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = client_copy,
	},
	{
		.opcode = BTP_FTP_CLIENT_SET_PERMISSION,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = client_set_permission,
	},
	{
		.opcode = BTP_FTP_SERVER_RFCOMM_DISCONNECT,
		.expect_len = sizeof(struct btp_ftp_server_rfcomm_disconnect_cmd),
		.func = server_rfcomm_disconnect,
	},
	{
		.opcode = BTP_FTP_SERVER_L2CAP_DISCONNECT,
		.expect_len = sizeof(struct btp_ftp_server_l2cap_disconnect_cmd),
		.func = server_l2cap_disconnect,
	},
	{
		.opcode = BTP_FTP_SERVER_CONNECT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = server_connect,
	},
	{
		.opcode = BTP_FTP_SERVER_DISCONNECT,
		.expect_len = sizeof(struct btp_ftp_server_disconnect_cmd),
		.func = server_disconnect,
	},
	{
		.opcode = BTP_FTP_SERVER_ABORT,
		.expect_len = sizeof(struct btp_ftp_server_abort_cmd),
		.func = server_abort,
	},
	{
		.opcode = BTP_FTP_SERVER_SET_FOLDER,
		.expect_len = sizeof(struct btp_ftp_server_set_folder_cmd),
		.func = server_set_folder,
	},
	{
		.opcode = BTP_FTP_SERVER_PULL_FOLDER_LISTING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = server_pull_folder_listing,
	},
	{
		.opcode = BTP_FTP_SERVER_PUSH_FILE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = server_push_file,
	},
	{
		.opcode = BTP_FTP_SERVER_PULL_FILE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = server_pull_file,
	},
	{
		.opcode = BTP_FTP_SERVER_DELETE,
		.expect_len = sizeof(struct btp_ftp_server_delete_cmd),
		.func = server_delete,
	},
	{
		.opcode = BTP_FTP_SERVER_RENAME,
		.expect_len = sizeof(struct btp_ftp_server_rename_cmd),
		.func = server_rename,
	},
	{
		.opcode = BTP_FTP_SERVER_COPY,
		.expect_len = sizeof(struct btp_ftp_server_copy_cmd),
		.func = server_copy,
	},
	{
		.opcode = BTP_FTP_SERVER_SET_PERMISSION,
		.expect_len = sizeof(struct btp_ftp_server_set_permission_cmd),
		.func = server_set_permission,
	},
};

static struct bt_ftp_server_rfcomm ftp_rfcomm_server;
static struct bt_ftp_server_l2cap ftp_l2cap_server;

static int ftp_server_rfcomm_accept(struct bt_conn *conn,
				    struct bt_ftp_server_rfcomm *rfcomm_server,
				    struct bt_ftp_server **ftp_server)
{
	struct ftp_server_instance *inst;

	inst = ftp_server_alloc(conn);
	if (inst == NULL) {
		return -ENOMEM;
	}

	*ftp_server = &inst->server;
	return 0;
}

static int ftp_server_l2cap_accept(struct bt_conn *conn, struct bt_ftp_server_l2cap *l2cap_server,
				   struct bt_ftp_server **ftp_server)
{
	struct ftp_server_instance *inst;

	inst = ftp_server_alloc(conn);
	if (inst == NULL) {
		return -ENOMEM;
	}

	*ftp_server = &inst->server;
	return 0;
}

uint8_t tester_init_ftp(void)
{
	int err;

	tester_register_command_handlers(BTP_SERVICE_ID_FTP, handlers, ARRAY_SIZE(handlers));

	ARRAY_FOR_EACH(ftp_server_instances, i) {
		err = bt_ftp_server_register(&ftp_server_instances[i].server, &ftp_server_cb);
		if (err != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	ftp_rfcomm_server.server.rfcomm.channel = 0;
	ftp_rfcomm_server.accept = ftp_server_rfcomm_accept;
	err = bt_ftp_server_rfcomm_register(&ftp_rfcomm_server);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	ftp_l2cap_server.server.l2cap.psm = 0;
	ftp_l2cap_server.accept = ftp_server_l2cap_accept;
	err = bt_ftp_server_l2cap_register(&ftp_l2cap_server);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_ftp(void)
{
	return BTP_STATUS_SUCCESS;
}
