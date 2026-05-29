/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"
#include "common/bt_str.h"

#define HELP_NONE "[none]"
#define SPP_RFCOMM_MTU     CONFIG_BT_RFCOMM_L2CAP_MTU
#define SDP_CLIENT_BUF_LEN 512

#define SPP_EP_FROM_DLC(_dlc) CONTAINER_OF(_dlc, struct bt_spp_endpoint, rfcomm_dlc)


enum __packed spp_state {
	/* SPP_STATE_DISCONNECTED:
	 * No active SPP connection. Resources are released or not allocated.
	 */
	SPP_STATE_DISCONNECTED,
	/* SPP_STATE_CONNECTING:
	 * Connection is in progress (e.g., SDP discovery or RFCOMM setup).
	 * Not yet available for data transfer.
	 */
	SPP_STATE_CONNECTING,
	/* SPP_STATE_CONNECTED:
	 * RFCOMM DLC is established and SPP is ready for data transfer.
	 */
	SPP_STATE_CONNECTED,
	/* SPP_STATE_DISCONNECTING:
	 * Disconnection is in progress; cleanup and resource release pending.
	 */
	SPP_STATE_DISCONNECTING,
};

enum bt_spp_mode {
	/** SPP mode is unknown. */
	BT_SPP_MODE_UNKNOWN,
	/** SPP mode using UUID for service discovery. */
	BT_SPP_MODE_UUID,
	/** SPP mode using RFCOMM channel for service connection. */
	BT_SPP_MODE_CHANNEL,
};

struct bt_spp_endpoint {
	/** RFCOMM Data Link Connection (DLC) */
	struct bt_rfcomm_dlc rfcomm_dlc;

	union {
		/** UUID (service) to discovery remote SPP service */
		const struct bt_uuid *uuid;
		/** Rfcomm channel to connect remote SPP server */
		uint8_t channel;
	};

	enum bt_spp_mode mode;

	/** Connection state (atomic) */
	atomic_t state;
};

struct bt_spp_client {
	struct bt_spp_endpoint ep;

	/** SPP SDP discover params */
	struct bt_sdp_discover_params sdp_discover;
};

struct bt_spp_server {
	struct bt_spp_endpoint ep;

	/** Pointer to the SDP record exposed for this server.
	 *
	 *  If NULL when registering, the registration will fail. The record must
	 *  remain valid for the lifetime of the registration.
	 */
	struct bt_sdp_record *sdp_record;

	/** RFCOMM server instance.
	 *
	 *  Populated/used by the stack to accept incoming RFCOMM connections
	 *  for this SPP service. Do not modify directly from application code.
	 */
	struct bt_rfcomm_server rfcomm_server;
};

NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, SDP_CLIENT_BUF_LEN, 8, NULL);
NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_RFCOMM_BUF_SIZE(SPP_RFCOMM_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_uuid_any spp_uuid;

static struct bt_spp_server default_spp_server;

static uint16_t spp_sdp_uuid16_val = BT_SDP_SERIAL_PORT_SVCLASS;
static uint32_t spp_sdp_uuid32_val;
static uint8_t spp_sdp_uuid128_val[BT_UUID_SIZE_128];

static const struct bt_sdp_attribute spp_attr_svclass_id_list_uuid16[] = {
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			&spp_sdp_uuid16_val,
		},
		)
	),
};

static const struct bt_sdp_attribute spp_attr_svclass_id_list_uuid32[] = {
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID32),
			&spp_sdp_uuid32_val,
		},
		)
	),
};

static const struct bt_sdp_attribute spp_attr_svclass_id_list_uuid128[] = {
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID128),
			spp_sdp_uuid128_val,
		},
		)
	),
};

static struct bt_sdp_attribute spp_sdp_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			&spp_sdp_uuid16_val
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
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
				&default_spp_server.rfcomm_server.channel
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
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("Serial Port"),
};

static struct bt_sdp_record spp_sdp_rec = BT_SDP_RECORD(spp_sdp_attrs);

static int spp_parse_rfcomm_channel(const char *str, uint8_t *channel)
{
	int err = 0;
	unsigned long val;

	val = shell_strtoul(str, 10, &err);
	if (err != 0) {
		return err;
	}

	*channel = (uint8_t)val;
	return 0;
}

static void bt_spp_connected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp_endpoint *ep = SPP_EP_FROM_DLC(dlci);

	atomic_set(&ep->state, SPP_STATE_CONNECTED);
	bt_shell_print("SPP: connected (ep=%p, channel=%u)", ep, ep->channel);
}

static void bt_spp_disconnected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp_endpoint *ep = SPP_EP_FROM_DLC(dlci);

	atomic_set(&ep->state, SPP_STATE_DISCONNECTED);
	bt_shell_print("SPP: disconnected (ep=%p)", ep);
}

static void bt_spp_recv(struct bt_rfcomm_dlc *dlci, struct net_buf *buf)
{
	struct bt_spp_endpoint *ep = SPP_EP_FROM_DLC(dlci);

	bt_shell_print("SPP: rx data (ep=%p, len=%u)", ep, (unsigned int)buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static int spp_sdp_set_uuid(const struct bt_uuid *uuid)
{
	struct bt_sdp_attribute *attr = NULL;
	const struct bt_sdp_attribute *attr_source;

	ARRAY_FOR_EACH(spp_sdp_attrs, i) {
		if (spp_sdp_attrs[i].id == BT_SDP_ATTR_SVCLASS_ID_LIST) {
			attr = &spp_sdp_attrs[i];
			break;
		}
	}

	if (attr == NULL) {
		return -ENOENT;
	}

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		spp_sdp_uuid16_val = BT_UUID_16(uuid)->val;
		attr_source = spp_attr_svclass_id_list_uuid16;
		break;
	case BT_UUID_TYPE_32:
		spp_sdp_uuid32_val = BT_UUID_32(uuid)->val;
		attr_source = spp_attr_svclass_id_list_uuid32;
		break;
	case BT_UUID_TYPE_128:
		memcpy(spp_sdp_uuid128_val, BT_UUID_128(uuid)->val, BT_UUID_SIZE_128);
		attr_source = spp_attr_svclass_id_list_uuid128;
		break;
	default:
		return -EINVAL;
	}

	memcpy(attr, attr_source, sizeof(*attr));
	return 0;
}

static struct bt_spp_client default_spp_client;

static struct bt_rfcomm_dlc_ops spp_rfcomm_ops = {
	.recv = bt_spp_recv,
	.connected = bt_spp_connected,
	.disconnected = bt_spp_disconnected,
};

static int bt_spp_accept(struct bt_conn *conn, struct bt_rfcomm_server *server,
			 struct bt_rfcomm_dlc **dlc)
{
	struct bt_spp_server *spp = CONTAINER_OF(server, struct bt_spp_server, rfcomm_server);

	if (atomic_get(&spp->ep.state) != SPP_STATE_DISCONNECTED) {
		bt_shell_error("SPP: reject incoming RFCOMM (state=%ld)",
			       (long)atomic_get(&spp->ep.state));
		return -EALREADY;
	}

	atomic_set(&spp->ep.state, SPP_STATE_CONNECTING);
	spp->ep.rfcomm_dlc.ops = &spp_rfcomm_ops;
	spp->ep.rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	spp->ep.rfcomm_dlc.required_sec_level = BT_SECURITY_L2;
	spp->ep.channel = spp->rfcomm_server.channel;
	*dlc = &spp->ep.rfcomm_dlc;

	bt_shell_print("SPP: accepted incoming connection (conn=%p)", conn);
	return 0;
}

static int bt_spp_server_register(struct bt_spp_server *server)
{
	int err;

	server->rfcomm_server.accept = bt_spp_accept;
	err = bt_rfcomm_server_register(&server->rfcomm_server);
	if (err < 0) {
		bt_shell_error("SPP: RFCOMM server register failed (err=%d)", err);
		return err;
	}

	err = bt_sdp_register_service(server->sdp_record);
	if (err < 0) {
		bt_shell_error("SPP: SDP service register failed (err=%d)", err);
		bt_rfcomm_server_unregister(&server->rfcomm_server);
		return err;
	}

	return 0;
}

static int cmd_spp_register_with_uuid(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_uuid_any uuid_any;
	const char *uuid = argv[1];

	if (default_spp_server.rfcomm_server.channel != 0) {
		shell_error(sh, "SPP: server already registered");
		return -EALREADY;
	}

	if (bt_uuid_from_str(uuid, &uuid_any) < 0) {
		shell_error(sh, "SPP: invalid UUID string: '%s'", uuid);
		return -ENOEXEC;
	}

	if (spp_sdp_set_uuid(&uuid_any.uuid) < 0) {
		shell_error(sh, "SPP: failed to set SDP UUID: '%s'", uuid);
		return -ENOEXEC;
	}

	default_spp_server.sdp_record = &spp_sdp_rec;

	err = bt_spp_server_register(&default_spp_server);
	if (err < 0) {
		shell_error(sh, "SPP: register server failed (uuid=%s, err=%d)", uuid, err);
		default_spp_server.sdp_record = NULL;
		default_spp_server.rfcomm_server.channel = 0U;
		return -ENOEXEC;
	}

	shell_print(sh, "SPP: server registered (uuid=%s, channel=%u)", uuid,
		    default_spp_server.rfcomm_server.channel);
	return 0;
}

static int cmd_spp_register_with_channel(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t channel = 0;

	if (default_spp_server.rfcomm_server.channel != 0) {
		shell_error(sh, "SPP: server already registered");
		return -EALREADY;
	}

	err = spp_parse_rfcomm_channel(argv[1], &channel);
	if (err < 0) {
		shell_error(sh, "SPP: invalid RFCOMM channel:%s", argv[1]);
		return -EINVAL;
	}

	(void)spp_sdp_set_uuid(BT_UUID_DECLARE_16(BT_SDP_SERIAL_PORT_SVCLASS));

	default_spp_server.sdp_record = &spp_sdp_rec;
	default_spp_server.rfcomm_server.channel = channel;

	err = bt_spp_server_register(&default_spp_server);
	if (err < 0) {
		shell_error(sh, "SPP: register server failed (channel=%u, err=%d)", channel,
			    err);
		default_spp_server.sdp_record = NULL;
		default_spp_server.rfcomm_server.channel = 0U;
		return -ENOEXEC;
	}

	shell_print(sh, "SPP: server registered (channel=%u)",
		    default_spp_server.rfcomm_server.channel);
	return 0;
}

static int bt_spp_connect_rfcomm(struct bt_conn *conn, struct bt_spp_endpoint *ep)
{
	int err;

	ep->rfcomm_dlc.ops = &spp_rfcomm_ops;
	ep->rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	ep->rfcomm_dlc.required_sec_level = BT_SECURITY_L2;

	err = bt_rfcomm_dlc_connect(conn, &ep->rfcomm_dlc, ep->channel);
	if (err < 0) {
		bt_shell_error("SPP: RFCOMM DLC connect failed (conn=%p, channel=%u, err=%d)", conn,
			       ep->channel, err);
		return err;
	}

	return 0;
}

static uint8_t sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *response,
			       const struct bt_sdp_discover_params *params)
{
	uint16_t channel;
	int err;
	struct bt_spp_client *spp = CONTAINER_OF(params, struct bt_spp_client, sdp_discover);

	if (response == NULL || response->resp_buf == NULL) {
		bt_shell_error("SPP: SDP service not found (conn=%p, uuid=%s)", conn,
			       bt_uuid_str(params->uuid));
		goto exit;
	}

	err = bt_sdp_get_proto_param(response->resp_buf, BT_SDP_PROTO_RFCOMM, &channel);
	if (err < 0) {
		bt_shell_error(
			"SPP: SDP parse failed to get RFCOMM channel (conn=%p, uuid=%s, err=%d)",
			conn, bt_uuid_str(params->uuid), err);
		goto exit;
	}

	bt_shell_print("SPP: SDP found RFCOMM channel (conn=%p, uuid=%s, channel=%u)", conn,
		       bt_uuid_str(params->uuid), channel);

	spp->ep.channel = channel;
	err = bt_spp_connect_rfcomm(conn, &spp->ep);
	if (err < 0) {
		bt_shell_error("SPP: connect failed after SDP (conn=%p, channel=%u, err=%d)", conn,
			       spp->ep.channel, err);
		goto exit;
	}

	return BT_SDP_DISCOVER_UUID_STOP;

exit:
	atomic_set(&spp->ep.state, SPP_STATE_DISCONNECTED);
	return BT_SDP_DISCOVER_UUID_STOP;
}

static int bt_spp_connect(struct bt_conn *conn, struct bt_spp_client *spp)
{
	int err;

	if (atomic_get(&spp->ep.state) != SPP_STATE_DISCONNECTED) {
		bt_shell_error("SPP: connect rejected (state=%ld)",
			       (long)atomic_get(&spp->ep.state));
		return -EALREADY;
	}

	if (spp->ep.mode == BT_SPP_MODE_CHANNEL) {
		err = bt_spp_connect_rfcomm(conn, &spp->ep);
		if (err < 0) {
			bt_shell_error(
				"SPP: connect by channel failed (conn=%p, channel=%u, err=%d)",
				conn, spp->ep.channel, err);
			return err;
		}
	} else if (spp->ep.mode == BT_SPP_MODE_UUID) {
		spp->sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
		spp->sdp_discover.func = sdp_discover_cb;
		spp->sdp_discover.pool = &sdp_pool;
		spp->sdp_discover.uuid = spp->ep.uuid;

		err = bt_sdp_discover(conn, &spp->sdp_discover);
		if (err < 0) {
			bt_shell_error("SPP: SDP discover failed (conn=%p, uuid=%s, err=%d)", conn,
				       bt_uuid_str(spp->ep.uuid), err);
			return err;
		}
	} else {
		bt_shell_error("SPP: invalid mode (mode=%d)", spp->ep.mode);
		return -EINVAL;
	}

	atomic_set(&spp->ep.state, SPP_STATE_CONNECTING);
	return 0;
}

static int cmd_spp_connect_by_uuid(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "SPP: no default connection");
		return -ENOEXEC;
	}

	if (bt_uuid_from_str(argv[1], &spp_uuid) < 0) {
		shell_error(sh, "SPP: invalid UUID string: '%s'", argv[1]);
		return -ENOEXEC;
	}

	if (atomic_get(&default_spp_client.ep.state) != SPP_STATE_DISCONNECTED) {
		shell_error(sh, "SPP: connect rejected (state=%ld)",
			    (long)atomic_get(&default_spp_client.ep.state));
		return -EALREADY;
	}

	default_spp_client.ep.mode = BT_SPP_MODE_UUID;
	default_spp_client.ep.uuid = &spp_uuid.uuid;

	err = bt_spp_connect(default_conn, &default_spp_client);
	if (err < 0) {
		shell_error(sh, "SPP: connect by UUID failed (uuid=%s, err=%d)",
			    bt_uuid_str(&spp_uuid.uuid), err);
		default_spp_client.ep.mode = BT_SPP_MODE_UNKNOWN;
		default_spp_client.ep.uuid = NULL;
		return -ENOEXEC;
	}

	shell_print(sh, "SPP: connect started (uuid=%s)", bt_uuid_str(&spp_uuid.uuid));
	return 0;
}

static int cmd_spp_connect_by_channel(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t channel = 0;

	if (default_conn == NULL) {
		shell_error(sh, "SPP: no default connection");
		return -ENOEXEC;
	}

	err = spp_parse_rfcomm_channel(argv[1], &channel);
	if (err < 0) {
		shell_error(sh, "SPP: invalid RFCOMM channel:%s", argv[1]);
		return -EINVAL;
	}

	if (atomic_get(&default_spp_client.ep.state) != SPP_STATE_DISCONNECTED) {
		shell_error(sh, "SPP: connect rejected (state=%ld)",
			    (long)atomic_get(&default_spp_client.ep.state));
		return -EALREADY;
	}

	default_spp_client.ep.mode = BT_SPP_MODE_CHANNEL;
	default_spp_client.ep.channel = channel;

	err = bt_spp_connect(default_conn, &default_spp_client);
	if (err < 0) {
		shell_error(sh, "SPP: connect by channel failed (channel=%u, err=%d)", channel,
			    err);
		default_spp_client.ep.mode = BT_SPP_MODE_UNKNOWN;
		default_spp_client.ep.channel = 0;
		return -ENOEXEC;
	}

	shell_print(sh, "SPP: connect started (channel=%u)", channel);
	return 0;
}

static struct bt_spp_endpoint *bt_spp_get_active_ep(void)
{
	if (atomic_get(&default_spp_client.ep.state) != SPP_STATE_DISCONNECTED) {
		return &default_spp_client.ep;
	}

	if (atomic_get(&default_spp_server.ep.state) != SPP_STATE_DISCONNECTED) {
		return &default_spp_server.ep;
	}

	return NULL;
}

static int cmd_spp_send(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct net_buf *buf;
	uint8_t *data;
	unsigned long len = SPP_RFCOMM_MTU;
	size_t tailroom;
	struct bt_spp_endpoint *ep;

	ep = bt_spp_get_active_ep();
	if (ep == NULL) {
		shell_error(sh, "SPP: no active connection");
		return -ENOEXEC;
	}

	if (argc > 1) {
		err = 0;
		len = shell_strtoul(argv[1], 10, &err);
		if (err != 0) {
			shell_error(sh, "SPP: invalid length: '%s'", argv[1]);
			return -EINVAL;
		}
	}

	len = MIN(SPP_RFCOMM_MTU, len);

	buf = bt_rfcomm_create_pdu(&tx_pool);
	if (buf == NULL) {
		shell_error(sh, "SPP: TX buffer allocation failed");
		return -ENOEXEC;
	}

	shell_print(sh, "SPP: tx data (len=%lu)", len);

	tailroom = net_buf_tailroom(buf);
	if (len > tailroom) {
		shell_error(sh, "SPP: tx len %lu exceeds buffer tailroom %zu", len, tailroom);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	data = net_buf_add(buf, len);
	memset(data, 0xff, len);

	err = bt_rfcomm_dlc_send(&ep->rfcomm_dlc, buf);
	if (err < 0) {
		shell_error(sh, "SPP: RFCOMM send failed (len=%u, err=%d)", (unsigned int)buf->len,
			    err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_spp_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_spp_endpoint *ep;

	ep = bt_spp_get_active_ep();
	if (ep == NULL) {
		shell_error(sh, "SPP: no active connection");
		return -ENOEXEC;
	}

	err = bt_rfcomm_dlc_disconnect(&ep->rfcomm_dlc);
	if (err < 0) {
		shell_error(sh, "SPP: RFCOMM disconnect failed (err=%d)", err);
		return -ENOEXEC;
	}

	atomic_set(&ep->state, SPP_STATE_DISCONNECTING);

	shell_print(sh, "SPP: disconnecting...");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	spp_cmds,
	SHELL_CMD_ARG(register_with_channel, NULL, "<channel>",
		      cmd_spp_register_with_channel, 2, 0),
	SHELL_CMD_ARG(register_with_uuid, NULL,
		      "<bt-uuid16|bt-uuid32|bt-uuid128> e.g. 1101 or 00001101-0000-1000-8000-00805F9B34FB",
		      cmd_spp_register_with_uuid, 2, 0),
	SHELL_CMD_ARG(connect_by_channel, NULL, "<channel>",
		      cmd_spp_connect_by_channel, 2, 0),
	SHELL_CMD_ARG(connect_by_uuid, NULL,
		      "<bt-uuid16|bt-uuid32|bt-uuid128> e.g. 1101 or 00001101-0000-1000-8000-00805F9B34FB",
		      cmd_spp_connect_by_uuid, 2, 0),
	SHELL_CMD_ARG(send, NULL, "send [length of packet(s)]", cmd_spp_send, 1, 1),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_spp_disconnect, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_spp(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "SPP: unknown argument '%s'", argv[1]);
	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(spp, &spp_cmds, "Bluetooth SPP sh commands", cmd_spp, 1, 1);
