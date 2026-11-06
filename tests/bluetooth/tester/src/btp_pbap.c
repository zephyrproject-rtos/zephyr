/* btp_pbap.c - Bluetooth PBAP Tester */

/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/pbap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <sys/types.h>

#include "btp/btp.h"

#define BTP_PBAP_TRANSPORT_RFCOMM 0x00
#define BTP_PBAP_TRANSPORT_L2CAP  0x01

#define PBAP_MAX_INSTANCES CONFIG_BT_MAX_CONN

NET_BUF_POOL_FIXED_DEFINE(pbap_tx_pool, CONFIG_BT_MAX_CONN, 1024, CONFIG_BT_CONN_TX_USER_DATA_SIZE,
			  NULL);

NET_BUF_POOL_DEFINE(sdp_discover_pool, PBAP_MAX_INSTANCES,
		    BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU), CONFIG_BT_CONN_TX_USER_DATA_SIZE,
		    NULL);

struct pbap_instance {
	struct bt_pbap_pce pbap_pce;
	struct bt_pbap_pse pbap_pse;
	bt_addr_t address;
	uint32_t conn_id;
	struct net_buf *tx_buf;
	struct bt_conn *conn;
	uint16_t peer_mopl;
	uint16_t local_mopl;
	uint8_t transport_type;
	bool in_use;
};

static struct pbap_instance pbap_instances[PBAP_MAX_INSTANCES];
static struct bt_pbap_pse_rfcomm rfcomm_server;
static struct bt_pbap_pse_l2cap l2cap_server;

#if defined(CONFIG_BT_PBAP_PSE)
/* PSE SDP record variables */
static uint16_t pse_l2cap_psm = 0x1025;
static uint8_t pse_rfcomm_channel = 0x13;
static uint8_t pse_supported_repositories = 0x0f;
static uint32_t pse_supported_features = 0x000003ff;

static struct bt_sdp_attribute pbap_pse_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_PBAP_PSE_SVCLASS)
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
				&pse_rfcomm_channel
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
	BT_SDP_SERVICE_NAME("Phonebook Access PSE"),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PBAP_SVCLASS)
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
		BT_SDP_ATTR_GOEP_L2CAP_PSM,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			&pse_l2cap_psm
		}
	},
	{
		BT_SDP_ATTR_SUPPORTED_REPOSITORIES,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			&pse_supported_repositories
		}
	},
	{
		BT_SDP_ATTR_PBAP_SUPPORTED_FEATURES,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT32),
			&pse_supported_features
		}
	},
};

static struct bt_sdp_record pbap_pse_rec = BT_SDP_RECORD(pbap_pse_attrs);
#endif /* CONFIG_BT_PBAP_PSE */

#if defined(CONFIG_BT_PBAP_PCE)
static uint16_t peer_sdp_rfcomm_channel;
static uint16_t peer_sdp_l2cap_psm;
static uint32_t peer_supported_features;
static uint8_t peer_supported_repositories;
/* PCE SDP record variables */
static struct bt_sdp_attribute pbap_pce_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_PBAP_PCE_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_PBAP_PCE_SVCLASS)
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
				BT_SDP_ARRAY_16(BT_SDP_PBAP_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("Phonebook Access PCE"),
};

static struct bt_sdp_record pbap_pce_rec = BT_SDP_RECORD(pbap_pce_attrs);
#endif /* CONFIG_BT_PBAP_PCE */

/* Forward declaration of PCE callbacks */
static struct bt_pbap_pce_cb pce_callbacks;

static struct pbap_instance *find_instance_by_address(const bt_addr_t *address)
{
	for (uint8_t i = 0; i < PBAP_MAX_INSTANCES; i++) {
		if (pbap_instances[i].conn != NULL &&
		    bt_addr_eq(&pbap_instances[i].address, address)) {
			return &pbap_instances[i];
		}
	}
	return NULL;
}

static struct pbap_instance *pbap_instance_allocate(struct bt_conn *conn)
{
	for (uint8_t i = 0; i < PBAP_MAX_INSTANCES; i++) {
		if (pbap_instances[i].conn == NULL) {
			memset(&pbap_instances[i], 0, sizeof(struct pbap_instance));
			pbap_instances[i].conn = conn;
			bt_addr_copy(&pbap_instances[i].address, bt_conn_get_dst_br(conn));
			return &pbap_instances[i];
		}
	}
	return NULL;
}

static void pbap_instance_free(struct pbap_instance *inst)
{
	if (inst->tx_buf) {
		net_buf_unref(inst->tx_buf);
		inst->tx_buf = NULL;
	}
	inst->conn = NULL;
}

/* PCE Callbacks */
static void pbap_pce_connected_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, uint8_t version,
				  uint16_t mopl, struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pce, struct pbap_instance, pbap_pce);
	struct btp_pbap_pce_connected_ev *ev;
	uint8_t *event_buf;
	int err;

	inst->peer_mopl = mopl;

	err = bt_obex_get_header_conn_id(buf, &inst->conn_id);
	if (err != 0) {
		/* No connection ID header found */
	}

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pce_connected_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->version = version;
	ev->mopl = sys_cpu_to_le16(mopl);
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PCE_EV_CONNECTED, ev, sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pce_disconnected_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				     struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pce, struct pbap_instance, pbap_pce);
	struct btp_pbap_pce_disconnected_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pce_disconnected_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PCE_EV_DISCONNECTED, ev, sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pce_pull_phone_book_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
					struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pce, struct pbap_instance, pbap_pce);
	struct btp_pbap_pce_pull_phone_book_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pce_pull_phone_book_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PCE_EV_PULL_PHONE_BOOK, ev,
		     sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pce_pull_vcard_listing_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
					   struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pce, struct pbap_instance, pbap_pce);
	struct btp_pbap_pce_pull_vcard_listing_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pce_pull_vcard_listing_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PCE_EV_PULL_VCARD_LISTING, ev,
		     sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pce_pull_vcard_entry_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
					 struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pce, struct pbap_instance, pbap_pce);
	struct btp_pbap_pce_pull_vcard_entry_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pce_pull_vcard_entry_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PCE_EV_PULL_VCARD_ENTRY, ev,
		     sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pce_set_phone_book_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				       struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pce, struct pbap_instance, pbap_pce);
	struct btp_pbap_pce_set_path_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pce_set_path_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PCE_EV_SET_PATH, ev, sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pce_abort_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pce, struct pbap_instance, pbap_pce);
	struct btp_pbap_pce_abort_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pce_abort_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->rsp_code = rsp_code;
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PCE_EV_ABORT, ev, sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pce_rfcomm_transport_connected(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pce, struct pbap_instance, pbap_pce);
	struct btp_pbap_pce_rfcomm_connected_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev), &event_buf);
	ev = (struct btp_pbap_pce_rfcomm_connected_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PCE_EV_RFCOMM_CONNECTED, ev, sizeof(*ev));

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pce_rfcomm_transport_disconnected(struct bt_pbap_pce *pbap_pce)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pce, struct pbap_instance, pbap_pce);
	struct btp_pbap_pce_rfcomm_disconnected_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev), &event_buf);
	ev = (struct btp_pbap_pce_rfcomm_disconnected_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PCE_EV_RFCOMM_DISCONNECTED, ev, sizeof(*ev));

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	pbap_instance_free(inst);
}

static void pbap_pce_l2cap_transport_connected(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pce, struct pbap_instance, pbap_pce);
	struct btp_pbap_pce_l2cap_connected_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev), &event_buf);
	ev = (struct btp_pbap_pce_l2cap_connected_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PCE_EV_L2CAP_CONNECTED, ev, sizeof(*ev));

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pce_l2cap_transport_disconnected(struct bt_pbap_pce *pbap_pce)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pce, struct pbap_instance, pbap_pce);
	struct btp_pbap_pce_l2cap_disconnected_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev), &event_buf);
	ev = (struct btp_pbap_pce_l2cap_disconnected_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PCE_EV_L2CAP_DISCONNECTED, ev, sizeof(*ev));

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	pbap_instance_free(inst);
}

static struct bt_pbap_pce_cb pce_callbacks = {
	.rfcomm_connected = pbap_pce_rfcomm_transport_connected,
	.rfcomm_disconnected = pbap_pce_rfcomm_transport_disconnected,
	.l2cap_connected = pbap_pce_l2cap_transport_connected,
	.l2cap_disconnected = pbap_pce_l2cap_transport_disconnected,
	.connect = pbap_pce_connected_cb,
	.disconnect = pbap_pce_disconnected_cb,
	.pull_phone_book = pbap_pce_pull_phone_book_cb,
	.pull_vcard_listing = pbap_pce_pull_vcard_listing_cb,
	.pull_vcard_entry = pbap_pce_pull_vcard_entry_cb,
	.set_phone_book = pbap_pce_set_phone_book_cb,
	.abort = pbap_pce_abort_cb,
};

/* PSE Callbacks */
static void pbap_pse_connected_cb(struct bt_pbap_pse *pbap_pse, uint8_t version, uint16_t mopl,
				  struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pse, struct pbap_instance, pbap_pse);
	struct btp_pbap_pse_connect_ev *ev;
	uint8_t *event_buf;
	uint16_t buf_len = 0;
	int err;

	inst->peer_mopl = mopl;

	if (buf && buf->len > 0) {
		buf_len = buf->len;
	}

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pse_connect_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->version = version;
	ev->mopl = sys_cpu_to_le16(mopl);
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PSE_EV_CONNECT, ev, sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pse_disconnected_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pse, struct pbap_instance, pbap_pse);
	struct btp_pbap_pse_disconnect_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pse_disconnect_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PSE_EV_DISCONNECT, ev, sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pse_pull_phone_book_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pse, struct pbap_instance, pbap_pse);
	struct btp_pbap_pse_pull_phone_book_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pse_pull_phone_book_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PSE_EV_PULL_PHONE_BOOK, ev,
		     sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pse_pull_vcard_listing_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pse, struct pbap_instance, pbap_pse);
	struct btp_pbap_pse_pull_vcard_listing_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pse_pull_vcard_listing_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PSE_EV_PULL_VCARD_LISTING, ev,
		     sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pse_pull_vcard_entry_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pse, struct pbap_instance, pbap_pse);
	struct btp_pbap_pse_pull_vcard_entry_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pse_pull_vcard_entry_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PSE_EV_PULL_VCARD_ENTRY, ev,
		     sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pse_set_phone_book_cb(struct bt_pbap_pse *pbap_pse, uint8_t flags,
				       struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pse, struct pbap_instance, pbap_pse);
	struct btp_pbap_pse_set_phone_book_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pse_set_phone_book_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->flags = flags;
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PSE_EV_SET_PHONE_BOOK, ev,
		     sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pse_abort_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pse, struct pbap_instance, pbap_pse);
	struct btp_pbap_pse_abort_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + buf->len, &event_buf);
	ev = (struct btp_pbap_pse_abort_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->buf_len = sys_cpu_to_le16(buf->len);

	if (buf->len > 0) {
		memcpy(ev->buf, buf->data, buf->len);
	}

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PSE_EV_ABORT, ev, sizeof(*ev) + buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pse_rfcomm_transport_connected(struct bt_conn *conn, struct bt_pbap_pse *pbap_pse)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pse, struct pbap_instance, pbap_pse);
	struct btp_pbap_pse_rfcomm_connected_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev), &event_buf);
	ev = (struct btp_pbap_pse_rfcomm_connected_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PSE_EV_RFCOMM_CONNECTED, ev, sizeof(*ev));

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pse_rfcomm_transport_disconnected(struct bt_pbap_pse *pbap_pse)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pse, struct pbap_instance, pbap_pse);
	struct btp_pbap_pse_rfcomm_disconnected_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev), &event_buf);
	ev = (struct btp_pbap_pse_rfcomm_disconnected_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PSE_EV_RFCOMM_DISCONNECTED, ev, sizeof(*ev));

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	pbap_instance_free(inst);
}

static void pbap_pse_l2cap_transport_connected(struct bt_conn *conn, struct bt_pbap_pse *pbap_pse)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pse, struct pbap_instance, pbap_pse);
	struct btp_pbap_pse_l2cap_connected_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev), &event_buf);
	ev = (struct btp_pbap_pse_l2cap_connected_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PSE_EV_L2CAP_CONNECTED, ev, sizeof(*ev));

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void pbap_pse_l2cap_transport_disconnected(struct bt_pbap_pse *pbap_pse)
{
	struct pbap_instance *inst = CONTAINER_OF(pbap_pse, struct pbap_instance, pbap_pse);
	struct btp_pbap_pse_l2cap_disconnected_ev *ev;
	uint8_t *event_buf;
	int err;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev), &event_buf);
	ev = (struct btp_pbap_pse_l2cap_disconnected_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &inst->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_PSE_EV_L2CAP_DISCONNECTED, ev, sizeof(*ev));

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	pbap_instance_free(inst);
}

static struct bt_pbap_pse_cb pse_callbacks = {
	.rfcomm_connected = pbap_pse_rfcomm_transport_connected,
	.rfcomm_disconnected = pbap_pse_rfcomm_transport_disconnected,
	.l2cap_connected = pbap_pse_l2cap_transport_connected,
	.l2cap_disconnected = pbap_pse_l2cap_transport_disconnected,
	.connect = pbap_pse_connected_cb,
	.disconnect = pbap_pse_disconnected_cb,
	.pull_phone_book = pbap_pse_pull_phone_book_cb,
	.pull_vcard_listing = pbap_pse_pull_vcard_listing_cb,
	.pull_vcard_entry = pbap_pse_pull_vcard_entry_cb,
	.set_phone_book = pbap_pse_set_phone_book_cb,
	.abort = pbap_pse_abort_cb,
};

/* PSE Accept Callbacks */
static int rfcomm_accept_cb(struct bt_conn *conn, struct bt_pbap_pse_rfcomm *pbap_pse_rfcomm,
			    struct bt_pbap_pse **pbap_pse)
{
	struct pbap_instance *inst = NULL;

	/* Find the instance pre-allocated by pse_register() */
	for (uint8_t i = 0; i < PBAP_MAX_INSTANCES; i++) {
		if (pbap_instances[i].in_use && pbap_instances[i].conn == NULL) {
			inst = &pbap_instances[i];
			break;
		}
	}

	if (inst == NULL) {
		return -ENOMEM;
	}

	/* Update the instance with connection information */
	inst->conn = conn;
	bt_addr_copy(&inst->address, bt_conn_get_dst_br(conn));
	inst->transport_type = BTP_PBAP_TRANSPORT_RFCOMM;
	*pbap_pse = &inst->pbap_pse;

	return 0;
}

static int l2cap_accept_cb(struct bt_conn *conn, struct bt_pbap_pse_l2cap *pbap_pse_l2cap,
			   struct bt_pbap_pse **pbap_pse)
{
	struct pbap_instance *inst = NULL;

	/* Find the instance pre-allocated by pse_register() */
	for (uint8_t i = 0; i < PBAP_MAX_INSTANCES; i++) {
		if (pbap_instances[i].in_use && pbap_instances[i].conn == NULL) {
			inst = &pbap_instances[i];
			break;
		}
	}

	if (inst == NULL) {
		return -ENOMEM;
	}

	/* Update the instance with connection information */
	inst->conn = conn;
	bt_addr_copy(&inst->address, bt_conn_get_dst_br(conn));
	inst->transport_type = BTP_PBAP_TRANSPORT_L2CAP;
	*pbap_pse = &inst->pbap_pse;

	return 0;
}

/* BTP Command Handlers */
static uint8_t read_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	struct btp_pbap_read_supported_commands_rp *rp = rsp;

	tester_set_bit(rp->data, BTP_PBAP_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_PBAP_PCE_RFCOMM_CONNECT);
	tester_set_bit(rp->data, BTP_PBAP_PCE_RFCOMM_DISCONNECT);
	tester_set_bit(rp->data, BTP_PBAP_PCE_L2CAP_CONNECT);
	tester_set_bit(rp->data, BTP_PBAP_PCE_L2CAP_DISCONNECT);
	tester_set_bit(rp->data, BTP_PBAP_PCE_CONNECT);
	tester_set_bit(rp->data, BTP_PBAP_PCE_DISCONNECT);
	tester_set_bit(rp->data, BTP_PBAP_PCE_PULL_PHONE_BOOK);
	tester_set_bit(rp->data, BTP_PBAP_PCE_PULL_VCARD_LISTING);
	tester_set_bit(rp->data, BTP_PBAP_PCE_PULL_VCARD_ENTRY);
	tester_set_bit(rp->data, BTP_PBAP_PCE_SET_PHONE_BOOK);
	tester_set_bit(rp->data, BTP_PBAP_PCE_ABORT);
	tester_set_bit(rp->data, BTP_PBAP_PSE_CONNECT_RSP);
	tester_set_bit(rp->data, BTP_PBAP_PSE_DISCONNECT_RSP);
	tester_set_bit(rp->data, BTP_PBAP_PSE_PULL_PHONE_BOOK_RSP);
	tester_set_bit(rp->data, BTP_PBAP_PSE_PULL_VCARD_LISTING_RSP);
	tester_set_bit(rp->data, BTP_PBAP_PSE_PULL_VCARD_ENTRY_RSP);
	tester_set_bit(rp->data, BTP_PBAP_PSE_SET_PHONE_BOOK_RSP);
	tester_set_bit(rp->data, BTP_PBAP_PSE_ABORT_RSP);
	tester_set_bit(rp->data, BTP_PBAP_SDP_DISCOVER);

	*rsp_len = sizeof(*rp) + 3;

	return BTP_STATUS_SUCCESS;
}

static uint8_t pce_rfcomm_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_pce_rfcomm_connect_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	inst = pbap_instance_allocate(conn);
	if (inst == NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	inst->in_use = true;
	inst->transport_type = BTP_PBAP_TRANSPORT_RFCOMM;

	err = bt_pbap_pce_rfcomm_connect(conn, &inst->pbap_pce, &pce_callbacks, cp->channel);

	bt_conn_unref(conn);

	if (err != 0) {
		pbap_instance_free(inst);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pce_rfcomm_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_pbap_pce_rfcomm_disconnect_cmd *cp = cmd;
	struct pbap_instance *inst;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_pbap_pce_rfcomm_disconnect(&inst->pbap_pce);

	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pce_l2cap_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_pce_l2cap_connect_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	inst = pbap_instance_allocate(conn);
	if (inst == NULL) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	inst->in_use = true;
	inst->transport_type = BTP_PBAP_TRANSPORT_L2CAP;

	err = bt_pbap_pce_l2cap_connect(conn, &inst->pbap_pce, &pce_callbacks,
					sys_le16_to_cpu(cp->psm));

	bt_conn_unref(conn);

	if (err != 0) {
		pbap_instance_free(inst);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pce_l2cap_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_pce_l2cap_disconnect_cmd *cp = cmd;
	struct pbap_instance *inst;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_pbap_pce_l2cap_disconnect(&inst->pbap_pce);

	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pce_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_pce_connect_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_pbap_pce_create_pdu(&inst->pbap_pce, &pbap_tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pce_connect(&inst->pbap_pce, sys_le16_to_cpu(cp->mopl), buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pce_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_pce_disconnect_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf = NULL;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		buf = bt_pbap_pce_create_pdu(&inst->pbap_pce, &pbap_tx_pool);
		if (!buf) {
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pce_disconnect(&inst->pbap_pce, buf);
	if (err) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pce_pull_phonebook(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_pce_pull_phone_book_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_pbap_pce_create_pdu(&inst->pbap_pce, &pbap_tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pce_pull_phone_book(&inst->pbap_pce, buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pce_pull_vcard_listing(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_pbap_pce_pull_vcard_listing_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_pbap_pce_create_pdu(&inst->pbap_pce, &pbap_tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pce_pull_vcard_listing(&inst->pbap_pce, buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pce_pull_vcard_entry(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_pce_pull_vcard_entry_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_pbap_pce_create_pdu(&inst->pbap_pce, &pbap_tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pce_pull_vcard_entry(&inst->pbap_pce, buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pce_set_phone_book(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_pce_set_phone_book_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_pbap_pce_create_pdu(&inst->pbap_pce, &pbap_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pce_set_phone_book(&inst->pbap_pce, cp->flags, buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pce_abort(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_pce_abort_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf = NULL;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		buf = bt_pbap_pce_create_pdu(&inst->pbap_pce, &pbap_tx_pool);
		if (!buf) {
			return BTP_STATUS_FAILED;
		}
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pce_abort(&inst->pbap_pce, buf);
	if (err) {
		if (buf) {
			net_buf_unref(buf);
		}
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static int pse_register_rfcomm(void)
{
	int err;

	rfcomm_server.server.rfcomm.channel = pse_rfcomm_channel;
	rfcomm_server.accept = rfcomm_accept_cb;

	err = bt_pbap_pse_rfcomm_register(&rfcomm_server);
	if (err != 0) {
		rfcomm_server.server.rfcomm.channel = 0;
	}

	return err;
}

static uint8_t pse_register_l2cap(void)
{
	int err;

	l2cap_server.server.l2cap.psm = pse_l2cap_psm;
	l2cap_server.accept = l2cap_accept_cb;

	err = bt_pbap_pse_l2cap_register(&l2cap_server);
	if (err != 0) {
		l2cap_server.server.l2cap.psm = 0;
	}

	return err;
}

static int pse_register(void)
{
	struct pbap_instance *inst = NULL;
	int err;

	/* Find an available instance for PSE registration */
	for (uint8_t i = 0; i < PBAP_MAX_INSTANCES; i++) {
		if (!pbap_instances[i].in_use) {
			memset(&pbap_instances[i], 0, sizeof(struct pbap_instance));
			pbap_instances[i].in_use = true;
			pbap_instances[i].conn = NULL;
			inst = &pbap_instances[i];
			break;
		}
	}

	if (inst == NULL) {
		return -EINVAL;
	}

	err = bt_pbap_pse_register(&inst->pbap_pse, &pse_callbacks);
	if (err) {
		inst->in_use = false;
	}

	return err;
}

static uint8_t pse_connect_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_pse_connect_rsp_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_pbap_pse_create_pdu(&inst->pbap_pse, &pbap_tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pse_connect_rsp(&inst->pbap_pse, sys_le16_to_cpu(cp->mopl), cp->rsp_code,
				      buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pse_disconnect_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_pse_disconnect_rsp_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_pbap_pse_create_pdu(&inst->pbap_pse, &pbap_tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pse_disconnect_rsp(&inst->pbap_pse, cp->rsp_code, buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pse_pull_phonebook_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_pbap_pse_pull_phone_book_rsp_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_pbap_pse_create_pdu(&inst->pbap_pse, &pbap_tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pse_pull_phone_book_rsp(&inst->pbap_pse, cp->rsp_code, buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pse_pull_vcard_listing_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
					  uint16_t *rsp_len)
{
	const struct btp_pbap_pse_pull_vcard_listing_rsp_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_pbap_pse_create_pdu(&inst->pbap_pse, &pbap_tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pse_pull_vcard_listing_rsp(&inst->pbap_pse, cp->rsp_code, buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pse_pull_vcard_entry_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_pbap_pse_pull_vcard_entry_rsp_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_pbap_pse_create_pdu(&inst->pbap_pse, &pbap_tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pse_pull_vcard_entry_rsp(&inst->pbap_pse, cp->rsp_code, buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pse_set_phone_book_rsp(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_pbap_pse_set_phone_book_rsp_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_pbap_pse_create_pdu(&inst->pbap_pse, &pbap_tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pse_set_phone_book_rsp(&inst->pbap_pse, cp->rsp_code, buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pse_abort_rsp(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_pse_abort_rsp_cmd *cp = cmd;
	struct pbap_instance *inst;
	struct net_buf *buf;
	int err;

	inst = find_instance_by_address(&cp->address.a);
	if (inst == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_pbap_pse_create_pdu(&inst->pbap_pse, &pbap_tx_pool);
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	if (cp->buf_len > 0) {
		net_buf_add_mem(buf, cp->buf, sys_le16_to_cpu(cp->buf_len));
	}

	err = bt_pbap_pse_abort_rsp(&inst->pbap_pse, cp->rsp_code, buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static int pbap_sdp_get_goep_l2cap_psm(const struct net_buf *buf, uint16_t *psm)
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

static int pbap_sdp_get_features(const struct net_buf *buf, uint32_t *feature)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_MAP_SUPPORTED_FEATURES, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*feature))) {
		return -EINVAL;
	}

	*feature = value.uint.u32;
	return 0;
}

static int pbap_sdp_get_repositories(const struct net_buf *buf, uint8_t *repositories)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_SUPPORTED_REPOSITORIES, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) ||
	    (value.uint.size != sizeof(*repositories))) {
		return -EINVAL;
	}

	*repositories = value.uint.u8;
	return 0;
}

struct btp_pbap_sdp_discover_ev rp;

static uint8_t sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
			       const struct bt_sdp_discover_params *params)
{

	if (result && result->resp_buf) {
		/* Parse SDP attributes for PBAP service */
		bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM,
				       &peer_sdp_rfcomm_channel);
		rp.rfcomm_channel = (uint8_t)peer_sdp_rfcomm_channel;

		pbap_sdp_get_goep_l2cap_psm(result->resp_buf, &peer_sdp_l2cap_psm);
		rp.l2cap_psm = peer_sdp_l2cap_psm;

		pbap_sdp_get_features(result->resp_buf, &peer_supported_features);
		rp.supported_features = peer_supported_features;

		pbap_sdp_get_repositories(result->resp_buf, &peer_supported_repositories);
		rp.supported_repositories = peer_supported_repositories;

		bt_addr_copy(&rp.address.a, bt_conn_get_dst_br(conn));
		rp.address.type = BTP_BR_ADDRESS_TYPE;
		/* Discovery complete, send response */
		tester_event(BTP_SERVICE_ID_PBAP, BTP_PBAP_EV_SDP_DISCOVER, &rp, sizeof(rp));
	}

	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static struct bt_sdp_discover_params discov_pbap = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.uuid = BT_UUID_DECLARE_16(BT_SDP_PBAP_PSE_SVCLASS),
	.func = sdp_discover_cb,
	.pool = &sdp_discover_pool,
};

static uint8_t sdp_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_pbap_sdp_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	err = bt_sdp_discover(conn, &discov_pbap);

	bt_conn_unref(conn);

	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_PBAP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = read_supported_commands,
	},
	{
		.opcode = BTP_PBAP_PCE_RFCOMM_CONNECT,
		.expect_len = sizeof(struct btp_pbap_pce_rfcomm_connect_cmd),
		.func = pce_rfcomm_connect,
	},
	{
		.opcode = BTP_PBAP_PCE_RFCOMM_DISCONNECT,
		.expect_len = sizeof(struct btp_pbap_pce_rfcomm_disconnect_cmd),
		.func = pce_rfcomm_disconnect,
	},
	{
		.opcode = BTP_PBAP_PCE_L2CAP_CONNECT,
		.expect_len = sizeof(struct btp_pbap_pce_l2cap_connect_cmd),
		.func = pce_l2cap_connect,
	},
	{
		.opcode = BTP_PBAP_PCE_L2CAP_DISCONNECT,
		.expect_len = sizeof(struct btp_pbap_pce_l2cap_disconnect_cmd),
		.func = pce_l2cap_disconnect,
	},
	{
		.opcode = BTP_PBAP_PCE_CONNECT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pce_connect,
	},
	{
		.opcode = BTP_PBAP_PCE_DISCONNECT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pce_disconnect,
	},
	{
		.opcode = BTP_PBAP_PCE_PULL_PHONE_BOOK,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pce_pull_phonebook,
	},
	{
		.opcode = BTP_PBAP_PCE_PULL_VCARD_LISTING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pce_pull_vcard_listing,
	},
	{
		.opcode = BTP_PBAP_PCE_PULL_VCARD_ENTRY,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pce_pull_vcard_entry,
	},
	{
		.opcode = BTP_PBAP_PCE_SET_PHONE_BOOK,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pce_set_phone_book,
	},
	{
		.opcode = BTP_PBAP_PCE_ABORT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pce_abort,
	},
	{
		.opcode = BTP_PBAP_PSE_CONNECT_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pse_connect_rsp,
	},
	{
		.opcode = BTP_PBAP_PSE_DISCONNECT_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pse_disconnect_rsp,
	},
	{
		.opcode = BTP_PBAP_PSE_PULL_PHONE_BOOK_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pse_pull_phonebook_rsp,
	},
	{
		.opcode = BTP_PBAP_PSE_PULL_VCARD_LISTING_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pse_pull_vcard_listing_rsp,
	},
	{
		.opcode = BTP_PBAP_PSE_PULL_VCARD_ENTRY_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pse_pull_vcard_entry_rsp,
	},
	{
		.opcode = BTP_PBAP_PSE_SET_PHONE_BOOK_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pse_set_phone_book_rsp,
	},
	{
		.opcode = BTP_PBAP_PSE_ABORT_RSP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pse_abort_rsp,
	},
	{
		.opcode = BTP_PBAP_SDP_DISCOVER,
		.expect_len = sizeof(struct btp_pbap_sdp_discover_cmd),
		.func = sdp_discover,
	},
};

uint8_t tester_init_pbap(void)
{
	int err;

	memset(pbap_instances, 0, sizeof(pbap_instances));
	memset(&rfcomm_server, 0, sizeof(rfcomm_server));
	memset(&l2cap_server, 0, sizeof(l2cap_server));

#if defined(CONFIG_BT_PBAP_PSE)
	/* Register PSE SDP record */
	err = bt_sdp_register_service(&pbap_pse_rec);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	/* Initialize PSE server registrations */
	err = pse_register_rfcomm();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	err = pse_register_l2cap();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	err = pse_register();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

#endif /* CONFIG_BT_PBAP_PSE */

#if defined(CONFIG_BT_PBAP_PCE)
	/* Register PCE SDP record */
	err = bt_sdp_register_service(&pbap_pce_rec);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}
#endif /* CONFIG_BT_PBAP_PCE */

	tester_register_command_handlers(BTP_SERVICE_ID_PBAP, handlers, ARRAY_SIZE(handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_pbap(void)
{
	for (uint8_t i = 0; i < PBAP_MAX_INSTANCES; i++) {
		if (pbap_instances[i].in_use) {
			pbap_instance_free(&pbap_instances[i]);
		}
	}

	return BTP_STATUS_SUCCESS;
}
