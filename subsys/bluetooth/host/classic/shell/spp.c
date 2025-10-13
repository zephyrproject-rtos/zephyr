/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <ctype.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
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

#define SPP_RFCOMM_CHANNEL_MIN 1U
#define SPP_RFCOMM_CHANNEL_MAX 31U

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

struct bt_spp;

enum bt_spp_mode {
	/** SPP mode is unknown. */
	BT_SPP_MODE_UNKNOWN,
	/** SPP mode using UUID for service discovery. */
	BT_SPP_MODE_UUID,
	/** SPP mode using RFCOMM channel for service connection. */
	BT_SPP_MODE_CHANNEL,
};

struct bt_spp_client {
	/** @internal SPP SDP discover params */
	struct bt_sdp_discover_params sdp_discover;
};

struct bt_spp_server {
	/** Pointer to the SDP record exposed for this server.
	 *
	 *  If NULL when registering, the registration will fail. The record must
	 *  remain valid for the lifetime of the registration.
	 */
	struct bt_sdp_record *sdp_record;

	union {
		/** UUID (service) to discovery remote SPP service */
		const struct bt_uuid *uuid;
		/** Rfcomm channel to connect remote SPP server */
		uint8_t channel;
	};

	/*  SPP mode: UUID or channel */
	enum bt_spp_mode mode;

	/** RFCOMM server instance.
	 *
	 *  Populated/used by the stack to accept incoming RFCOMM connections
	 *  for this SPP service. Do not modify directly from application code.
	 */
	struct bt_rfcomm_server rfcomm_server;
};

struct bt_spp {
	/** @internal RFCOMM Data Link Connection (DLC) */
	struct bt_rfcomm_dlc rfcomm_dlc;

	union {
		/** UUID (service) to discovery remote SPP service */
		const struct bt_uuid *uuid;
		/** Rfcomm channel to connect remote SPP server */
		uint8_t channel;
	};

	enum bt_spp_mode mode;

	/** @internal Connection state (atomic) */
	atomic_t state;

	/** @internal Role-specific context
	 *
	 *  Union containing context specific to the role of this SPP instance:
	 *   - client: SDP discovery parameters and UUID used when initiating a
	 *             connection to a remote SPP service.
	 *   - server: server registration and accept callback context for
	 *             accepted incoming connections.
	 *
	 *  Access the appropriate member only when the instance role is known.
	 */
	union {
		struct bt_spp_client client;
		struct bt_spp_server server;
	};
};

NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, SDP_CLIENT_BUF_LEN, 8, NULL);
NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(SPP_RFCOMM_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static uint8_t spp_channel;

static union {
	struct bt_uuid_16 u16;
	struct bt_uuid_32 u32;
	struct bt_uuid_128 u128;
} spp_uuid;

static struct bt_sdp_attribute spp_attrs_channel[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
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
				&spp_channel
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

static struct bt_sdp_record spp_rec_channel = BT_SDP_RECORD(spp_attrs_channel);

static uint8_t spp_uuid_buf[BT_UUID_SIZE_128];

static struct bt_sdp_attribute spp_attrs_uuid128[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID128),
			spp_uuid_buf
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
				&spp_channel
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

static struct bt_sdp_record spp_rec_uuid128 = BT_SDP_RECORD(spp_attrs_uuid128);

static int spp_parse_rfcomm_channel(const char *str, uint8_t *channel)
{
	char *end;
	unsigned long val;

	if (str == NULL || channel == NULL || str[0] == '-') {
		return -EINVAL;
	}

	val = strtoul(str, &end, 10);
	if (end == str || *end != '\0') {
		return -EINVAL;
	}

	*channel = (uint8_t)val;
	return 0;
}

static void bt_spp_connected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp *spp = CONTAINER_OF(dlci, struct bt_spp, rfcomm_dlc);

	atomic_set(&spp->state, SPP_STATE_CONNECTED);
	bt_shell_print("SPP: connected (spp=%p, channel=%u)", spp, spp->channel);
}

static void bt_spp_disconnected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp *spp = CONTAINER_OF(dlci, struct bt_spp, rfcomm_dlc);

	atomic_set(&spp->state, SPP_STATE_DISCONNECTED);
	bt_shell_print("SPP: disconnected (spp=%p)", spp);
}

static void bt_spp_recv(struct bt_rfcomm_dlc *dlci, struct net_buf *buf)
{
	struct bt_spp *spp = CONTAINER_OF(dlci, struct bt_spp, rfcomm_dlc);

	bt_shell_print("SPP: rx data (spp=%p, len=%u)", spp, (unsigned int)buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static int bt_spp_uuid_from_str(const char *str, struct bt_uuid *uuid)
{
	if (str == NULL || uuid == NULL) {
		return -EINVAL;
	}

	switch (strlen(str)) {
	case BT_UUID_SIZE_16 * 2: {
		uint16_t *p = &BT_UUID_16(uuid)->val;

		if (sscanf(str, "%04hx", p) != 1) {
			return -EINVAL;
		}

		uuid->type = BT_UUID_TYPE_16;
		return 0;
	}
	case BT_UUID_SIZE_32 * 2: {
		uint32_t *p = &BT_UUID_32(uuid)->val;

		if (sscanf(str, "%08x", p) != 1) {
			return -EINVAL;
		}

		uuid->type = BT_UUID_TYPE_32;
		return 0;
	}
	case BT_UUID_STR_LEN - 1: {
		uint8_t *p = BT_UUID_128(uuid)->val;

		if (sscanf(str,
			   "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx"
			   "-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
			   &p[15], &p[14], &p[13], &p[12], &p[11], &p[10], &p[9], &p[8], &p[7],
			   &p[6], &p[5], &p[4], &p[3], &p[2], &p[1], &p[0]) != BT_UUID_SIZE_128) {
			return -EINVAL;
		}

		uuid->type = BT_UUID_TYPE_128;
		return 0;
	}
	default:
		return -EINVAL;
	}
}

static int bt_spp_uuid_to_uuid128(const struct bt_uuid *uuid, struct bt_uuid_128 *uuid128)
{
	if (uuid == NULL || uuid128 == NULL) {
		return -EINVAL;
	}

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		*uuid128 = (struct bt_uuid_128)BT_UUID_INIT_128(BT_UUID_128_ENCODE(
			BT_UUID_16(uuid)->val, 0x0000, 0x1000, 0x8000, 0x00805F9B34FBULL));
		return 0;
	case BT_UUID_TYPE_32:
		*uuid128 = (struct bt_uuid_128)BT_UUID_INIT_128(BT_UUID_128_ENCODE(
			BT_UUID_32(uuid)->val, 0x0000, 0x1000, 0x8000, 0x00805F9B34FBULL));
		return 0;
	case BT_UUID_TYPE_128:
		uuid128->uuid.type = BT_UUID_TYPE_128;
		memcpy(uuid128->val, BT_UUID_128(uuid)->val, BT_UUID_SIZE_128);
		return 0;
	default:
		return -EINVAL;
	}
}

static struct bt_spp default_spp;

static struct bt_rfcomm_dlc_ops spp_rfcomm_ops = {
	.recv = bt_spp_recv,
	.connected = bt_spp_connected,
	.disconnected = bt_spp_disconnected,
};

static int bt_spp_accept(struct bt_conn *conn, struct bt_rfcomm_server *server,
			 struct bt_rfcomm_dlc **dlc)
{
	struct bt_spp_server *spp_srv = CONTAINER_OF(server, struct bt_spp_server, rfcomm_server);

	if (atomic_get(&default_spp.state) != SPP_STATE_DISCONNECTED) {
		bt_shell_error("SPP: reject incoming RFCOMM (state=%ld)",
			       (long)atomic_get(&default_spp.state));
		return -EALREADY;
	}

	atomic_set(&default_spp.state, SPP_STATE_CONNECTING);
	default_spp.rfcomm_dlc.ops = &spp_rfcomm_ops;
	default_spp.rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	default_spp.rfcomm_dlc.required_sec_level = BT_SECURITY_L2;
	*dlc = &default_spp.rfcomm_dlc;

	bt_shell_print("SPP: accepted incoming connection (conn=%p, mode=%u)", conn, spp_srv->mode);
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
		return err;
	}

	return 0;
}

static int cmd_spp_register_with_uuid(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	const char *uuid;
	struct bt_uuid_128 uuid128;
	union {
		struct bt_uuid_16 u16;
		struct bt_uuid_32 u32;
		struct bt_uuid_128 u128;
	} uuid_any;

	uuid = argv[1];

	if (bt_spp_uuid_from_str(uuid, (struct bt_uuid *)&uuid_any) < 0) {
		shell_error(sh, "SPP: invalid UUID string: '%s'", uuid);
		return -ENOEXEC;
	}

	if (bt_spp_uuid_to_uuid128((struct bt_uuid *)&uuid_any, &uuid128) < 0) {
		shell_error(sh, "SPP: failed to normalize UUID: '%s'", uuid);
		return -ENOEXEC;
	}

	memcpy(spp_uuid_buf, uuid128.val, BT_UUID_SIZE_128);

	default_spp.server.sdp_record = &spp_rec_uuid128;
	default_spp.server.rfcomm_server.channel = 0U;

	err = bt_spp_server_register(&default_spp.server);
	if (err < 0) {
		shell_error(sh, "SPP: register server failed (uuid=%s, err=%d)", uuid, err);
		default_spp.server.sdp_record = NULL;
		default_spp.server.rfcomm_server.channel = 0U;
		return -ENOEXEC;
	}

	spp_channel = default_spp.server.rfcomm_server.channel;

	shell_print(sh, "SPP: server registered (uuid=%s, channel=%u)", uuid, spp_channel);

	return 0;
}

static int cmd_spp_register_with_channel(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t channel;

	err = spp_parse_rfcomm_channel(argv[1], &channel);
	if (err < 0) {
		shell_error(sh, "SPP: invalid RFCOMM channel:%s", argv[1]);
		return -EINVAL;
	}

	spp_channel = channel;

	default_spp.server.sdp_record = &spp_rec_channel;
	default_spp.server.rfcomm_server.channel = spp_channel;

	err = bt_spp_server_register(&default_spp.server);
	if (err < 0) {
		shell_error(sh, "SPP: register server failed (channel=%u, err=%d)", spp_channel,
			    err);
		default_spp.server.sdp_record = NULL;
		default_spp.server.rfcomm_server.channel = 0U;
		return -ENOEXEC;
	}

	shell_print(sh, "SPP: server registered (channel=%u)", spp_channel);
	return 0;
}

static int bt_spp_connect_rfcomm(struct bt_conn *conn, struct bt_spp *spp)
{
	int err;

	spp->rfcomm_dlc.ops = &spp_rfcomm_ops;
	spp->rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	spp->rfcomm_dlc.required_sec_level = BT_SECURITY_L2;

	err = bt_rfcomm_dlc_connect(conn, &spp->rfcomm_dlc, spp->channel);
	if (err < 0) {
		bt_shell_error("SPP: RFCOMM DLC connect failed (conn=%p, channel=%u, err=%d)", conn,
			       spp->channel, err);
		return err;
	}

	return 0;
}

static uint8_t sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *response,
			       const struct bt_sdp_discover_params *params)
{
	uint16_t channel;
	int err;
	struct bt_spp *spp = CONTAINER_OF(params, struct bt_spp, client.sdp_discover);

	if (response == NULL) {
		bt_shell_error("SPP: SDP service not found (conn=%p, uuid=%s)", conn,
			       bt_uuid_str(params->uuid));
		goto exit;
	}

	if (response->resp_buf == NULL) {
		bt_shell_error("SPP: SDP response buffer is NULL (conn=%p, uuid=%s)", conn,
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

	spp->channel = channel;
	err = bt_spp_connect_rfcomm(conn, spp);
	if (err < 0) {
		bt_shell_error("SPP: connect failed after SDP (conn=%p, channel=%u, err=%d)", conn,
			       spp->channel, err);
		goto exit;
	}

	return BT_SDP_DISCOVER_UUID_STOP;

exit:
	atomic_set(&spp->state, SPP_STATE_DISCONNECTED);
	return BT_SDP_DISCOVER_UUID_STOP;
}

static int bt_spp_connect(struct bt_conn *conn, struct bt_spp *spp)
{
	int err;

	if (atomic_get(&spp->state) != SPP_STATE_DISCONNECTED) {
		bt_shell_error("SPP: connect rejected (state=%ld)", (long)atomic_get(&spp->state));
		return -EALREADY;
	}

	if (spp->mode == BT_SPP_MODE_CHANNEL) {
		err = bt_spp_connect_rfcomm(conn, spp);
		if (err < 0) {
			bt_shell_error(
				"SPP: connect by channel failed (conn=%p, channel=%u, err=%d)",
				conn, spp->channel, err);
			return err;
		}
	} else if (spp->mode == BT_SPP_MODE_UUID) {
		spp->client.sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
		spp->client.sdp_discover.func = sdp_discover_cb;
		spp->client.sdp_discover.pool = &sdp_pool;
		spp->client.sdp_discover.uuid = spp->uuid;

		err = bt_sdp_discover(conn, &spp->client.sdp_discover);
		if (err < 0) {
			bt_shell_error("SPP: SDP discover failed (conn=%p, uuid=%s, err=%d)", conn,
				       bt_uuid_str(spp->uuid), err);
			return err;
		}
	}

	atomic_set(&spp->state, SPP_STATE_CONNECTING);
	return 0;
}

static int cmd_spp_connect_by_uuid(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	size_t len;

	if (default_conn == NULL) {
		shell_error(sh, "SPP: no default connection (run 'bt connect' first)");
		return -ENOEXEC;
	}

	len = strlen(argv[1]);

	if (len == (BT_UUID_SIZE_16 * 2)) {
		uint16_t val;

		spp_uuid.u16.uuid.type = BT_UUID_TYPE_16;
		hex2bin(argv[1], len, (uint8_t *)&val, sizeof(val));
		spp_uuid.u16.val = sys_be16_to_cpu(val);
	} else if (len == (BT_UUID_SIZE_32 * 2)) {
		uint32_t val;

		spp_uuid.u32.uuid.type = BT_UUID_TYPE_32;
		hex2bin(argv[1], len, (uint8_t *)&val, sizeof(val));
		spp_uuid.u32.val = sys_be32_to_cpu(val);
	} else if (len == (BT_UUID_STR_LEN - 1)) {
		if (bt_spp_uuid_from_str(argv[1], (struct bt_uuid *)&spp_uuid) < 0) {
			shell_error(sh, "SPP: invalid UUID string: '%s'", argv[1]);
			return -ENOEXEC;
		}
	} else {
		shell_error(sh,
			    "SPP: invalid UUID '%s' (expected 16/32-bit hex or UUID-128 string)",
			    argv[1]);
		return -ENOEXEC;
	}

	default_spp.mode = BT_SPP_MODE_UUID;
	default_spp.uuid = (const struct bt_uuid *)&spp_uuid;

	err = bt_spp_connect(default_conn, &default_spp);
	if (err < 0) {
		shell_error(sh, "SPP: connect by UUID failed (uuid=%s, err=%d)",
			    bt_uuid_str((const struct bt_uuid *)&spp_uuid), err);
		default_spp.mode = BT_SPP_MODE_UNKNOWN;
		default_spp.uuid = NULL;
		return -ENOEXEC;
	}

	shell_print(sh, "SPP: connect started (uuid=%s)",
		    bt_uuid_str((const struct bt_uuid *)&spp_uuid));
	return 0;
}

static int cmd_spp_connect_by_channel(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t channel;

	if (default_conn == NULL) {
		shell_error(sh, "SPP: no default connection (run 'bt connect' first)");
		return -ENOEXEC;
	}

	err = spp_parse_rfcomm_channel(argv[1], &channel);
	if (err < 0) {
		shell_error(sh, "SPP: invalid RFCOMM channel:%s", argv[1]);
		return -EINVAL;
	}

	default_spp.mode = BT_SPP_MODE_CHANNEL;
	default_spp.channel = channel;

	err = bt_spp_connect(default_conn, &default_spp);
	if (err < 0) {
		shell_error(sh, "SPP: connect by channel failed (channel=%u, err=%d)", channel,
			    err);
		default_spp.mode = BT_SPP_MODE_UNKNOWN;
		default_spp.uuid = NULL;
		return -ENOEXEC;
	}

	shell_print(sh, "SPP: connect started (channel=%u)", channel);
	return 0;
}

static int cmd_spp_send(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct net_buf *buf;
	uint8_t *data;
	int len = SPP_RFCOMM_MTU;
	size_t tailroom;

	buf = bt_rfcomm_create_pdu(&tx_pool);
	if (buf == NULL) {
		shell_error(sh, "SPP: TX buffer allocation failed");
		return -ENOEXEC;
	}

	if (argc > 1) {
		len = strtoul(argv[1], NULL, 10);
	}

	len = MIN(SPP_RFCOMM_MTU, len);
	shell_print(sh, "SPP: tx data (len=%d)", len);

	tailroom = net_buf_tailroom(buf);
	if (len > tailroom) {
		shell_error(sh, "SPP: tx len %d exceeds buffer tailroom %zu", len, tailroom);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	data = net_buf_add(buf, len);
	memset(data, 0xff, len);

	err = bt_rfcomm_dlc_send(&default_spp.rfcomm_dlc, buf);
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

	err = bt_rfcomm_dlc_disconnect(&default_spp.rfcomm_dlc);
	if (err < 0) {
		shell_error(sh, "SPP: RFCOMM disconnect failed (err=%d)", err);
		return -ENOEXEC;
	}

	atomic_set(&default_spp.state, SPP_STATE_DISCONNECTING);

	shell_print(sh, "SPP: disconnecting...");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	spp_cmds,
	SHELL_CMD_ARG(register_with_channel, NULL,
		      "<channel (" STRINGIFY(SPP_RFCOMM_CHANNEL_MIN) "-"
		      STRINGIFY(SPP_RFCOMM_CHANNEL_MAX) ")>",
		      cmd_spp_register_with_channel, 2, 0),
	SHELL_CMD_ARG(register_with_uuid, NULL,
		      "<bt-uuid16|bt-uuid32|bt-uuid128> e.g. 1101 or 00001101-0000-1000-8000-00805F9B34FB",
		      cmd_spp_register_with_uuid, 2, 0),
	SHELL_CMD_ARG(connect_by_channel, NULL,
		      "<channel (" STRINGIFY(SPP_RFCOMM_CHANNEL_MIN) "-"
		      STRINGIFY(SPP_RFCOMM_CHANNEL_MAX) ")>",
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
