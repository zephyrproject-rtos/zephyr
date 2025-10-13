/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
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

	/** @internal ACL connection reference */
	struct bt_conn *acl_conn;

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
NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN,
			  BT_L2CAP_BUF_SIZE(SPP_RFCOMM_MTU), 16, NULL);

static uint8_t spp_channel;
static uint8_t spp_buf[SPP_RFCOMM_MTU];

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

static void bt_spp_connected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp *spp = CONTAINER_OF(dlci, struct bt_spp, rfcomm_dlc);

	atomic_set(&spp->state, SPP_STATE_CONNECTED);

	bt_shell_print("SPP:%p, conn:%p, channel:%d connected ", spp, spp->acl_conn, spp->channel);
}

static void bt_spp_disconnected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp *spp = CONTAINER_OF(dlci, struct bt_spp, rfcomm_dlc);

	atomic_set(&spp->state, SPP_STATE_DISCONNECTED);

	bt_shell_print("SPP:%p disconnected", spp);
}

static void bt_spp_recv(struct bt_rfcomm_dlc *dlci, struct net_buf *buf)
{
	struct bt_spp *spp = CONTAINER_OF(dlci, struct bt_spp, rfcomm_dlc);

	bt_shell_print("SPP:%p, data len:%d", spp, buf->len);

	bt_shell_hexdump(buf->data, buf->len);
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
	struct bt_spp_server *spp_srv =
		CONTAINER_OF(server, struct bt_spp_server, rfcomm_server);

	if (atomic_get(&default_spp.state) != SPP_STATE_DISCONNECTED) {
		bt_shell_print("SPP already connected or connecting");
		return -EALREADY;
	}

	default_spp.acl_conn = bt_conn_ref(conn);
	default_spp.rfcomm_dlc.ops = &spp_rfcomm_ops;
	default_spp.rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	default_spp.rfcomm_dlc.required_sec_level = BT_SECURITY_L2;
	*dlc = &default_spp.rfcomm_dlc;

	bt_shell_print("SPP accept a conn %p, mode:%d", conn, spp_srv->mode);
	return 0;
}

static int bt_spp_server_register(struct bt_spp_server *server)
{
	int err;

	server->rfcomm_server.accept = bt_spp_accept;

	err = bt_rfcomm_server_register(&server->rfcomm_server);
	if (err < 0) {
		bt_shell_print("fail to register rfcomm service, err:%d", err);
		return err;
	}

	err = bt_sdp_register_service(server->sdp_record);
	if (err < 0) {
		bt_shell_print("fail to register sdp service, err:%d", err);
		return err;
	}

	return 0;
}

static int cmd_spp_register_with_uuid(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;
	const char *uuid;
	struct bt_uuid_128 uuid128;

	uuid = argv[1];

	if (!bt_str_to_uuid(uuid, (struct bt_uuid *)&uuid128)) {
		bt_shell_print("fail to parse uuid");
		return -ENOEXEC;
	}

	if (uuid128.uuid.type != BT_UUID_TYPE_128) {
		bt_shell_print("only support 128bit uuid");
		return -ENOEXEC;
	}

	sys_memcpy_swap(spp_uuid_buf, uuid128.val, BT_UUID_SIZE_128);

	default_spp.server.sdp_record = &spp_rec_uuid128;
	default_spp.server.rfcomm_server.channel = 0U;

	err = bt_spp_server_register(&default_spp.server);
	if (err < 0) {
		shell_print(sh, "fail to register SPP service, err:%d", err);
		default_spp.server.sdp_record = NULL;
		default_spp.server.rfcomm_server.channel = 0U;
		return -ENOEXEC;
	}

	spp_channel = default_spp.server.rfcomm_server.channel;

	shell_print(sh, "register SPP srv uuid:%s(channel:%d) success", uuid, spp_channel);

	return 0;
}

static int cmd_spp_register_with_channel(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	spp_channel = strtoul(argv[1], NULL, 16);

	default_spp.server.sdp_record = &spp_rec_channel;
	default_spp.server.rfcomm_server.channel = spp_channel;

	err = bt_spp_server_register(&default_spp.server);
	if (err < 0) {
		shell_print(sh, "fail to register SPP service, err:%d", err);
		default_spp.server.sdp_record = NULL;
		default_spp.server.rfcomm_server.channel = 0U;
		return -ENOEXEC;
	}

	shell_print(sh, "register SPP srv channel:%d success", spp_channel);

	return 0;
}

static int bt_spp_connect_rfcomm(struct bt_conn *conn, struct bt_spp *spp)
{
	int err;

	spp->acl_conn = bt_conn_ref(conn);
	spp->rfcomm_dlc.ops = &spp_rfcomm_ops;
	spp->rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	spp->rfcomm_dlc.required_sec_level = BT_SECURITY_L2;

	err = bt_rfcomm_dlc_connect(conn, &spp->rfcomm_dlc, spp->channel);
	if (err < 0) {
		bt_shell_print("SPP rfcomm dlc connect fail, err:%d", err);
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
		bt_shell_print("SPP response is null");
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	if (response->resp_buf == NULL) {
		bt_shell_print("SPP sdp resp_buf is null");
		goto exit;
	}

	err = bt_sdp_get_proto_param(response->resp_buf, BT_SDP_PROTO_RFCOMM, &channel);
	if (err < 0) {
		bt_shell_print("SPP sdp get proto fail");
		goto exit;
	}

	bt_shell_print("SPP SDP record channel:%d found for uuid:%s", channel,
		       bt_uuid_str(params->uuid));

	spp->channel = channel;
	err = bt_spp_connect_rfcomm(conn, spp);
	if (err < 0) {
		bt_shell_print("SPP connect fail, err:%d", err);
		goto exit;
	}

	return BT_SDP_DISCOVER_UUID_STOP;

exit:
	return BT_SDP_DISCOVER_UUID_STOP;
}

static int bt_spp_connect(struct bt_conn *conn, struct bt_spp *spp)
{
	int err;

	if (atomic_get(&spp->state) != SPP_STATE_DISCONNECTED) {
		bt_shell_print("SPP was not in disconnected state");
		return -EALREADY;
	}

	if (spp->mode == BT_SPP_MODE_CHANNEL) {
		err = bt_spp_connect_rfcomm(conn, spp);
		if (err < 0) {
			bt_shell_print("SPP connect fail, err:%d", err);
			return err;
		}
	} else if (spp->mode == BT_SPP_MODE_UUID) {
		spp->client.sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
		spp->client.sdp_discover.func = sdp_discover_cb;
		spp->client.sdp_discover.pool = &sdp_pool;
		spp->client.sdp_discover.uuid = spp->uuid;

		err = bt_sdp_discover(conn, &spp->client.sdp_discover);
		if (err < 0) {
			bt_shell_print("SPP sdp discover fail, err:%d", err);
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
		shell_error(sh, "please connect bt first");
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
	} else if (len == (BT_UUID_SIZE_128 * 2)) {
		uint8_t uuid128[BT_UUID_SIZE_128];

		spp_uuid.u128.uuid.type = BT_UUID_TYPE_128;
		hex2bin(argv[1], len, &uuid128[0], sizeof(uuid128));
		sys_memcpy_swap(spp_uuid.u128.val, uuid128, sizeof(uuid128));
	} else {
		shell_error(sh, "Invalid UUID");
		return -ENOEXEC;
	}

	default_spp.mode = BT_SPP_MODE_UUID;
	default_spp.uuid = (const struct bt_uuid *)&spp_uuid;

	err = bt_spp_connect(default_conn, &default_spp);
	if (err < 0) {
		shell_error(sh, "fail to connect SPP device, err:%d", err);
		default_spp.mode = BT_SPP_MODE_UNKNOWN;
		default_spp.uuid = NULL;
		return -ENOEXEC;
	}

	shell_print(sh, "SPP connect uuid:%s ", bt_uuid_str((const struct bt_uuid *)&spp_uuid));
	return 0;
}

static int cmd_spp_connect_by_channel(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t channel;

	if (default_conn == NULL) {
		shell_error(sh, "please connect bt first");
		return -ENOEXEC;
	}

	channel = strtoul(argv[1], NULL, 16);

	default_spp.mode = BT_SPP_MODE_CHANNEL;
	default_spp.channel = channel;

	err = bt_spp_connect(default_conn, &default_spp);
	if (err < 0) {
		shell_error(sh, "fail to connect SPP device, err:%d", err);
		default_spp.mode = BT_SPP_MODE_UNKNOWN;
		default_spp.uuid = NULL;
		return -ENOEXEC;
	}

	shell_print(sh, "SPP connect channel:%d", channel);
	return 0;
}

static int bt_spp_send(struct bt_spp *spp, struct net_buf *buf)
{
	int err;

	if (atomic_get(&spp->state) != SPP_STATE_CONNECTED) {
		bt_shell_print("Cannot send data while not connected");
		return -ENOTCONN;
	}

	err = bt_rfcomm_dlc_send(&spp->rfcomm_dlc, buf);
	if (err < 0) {
		bt_shell_print("rfcomm unable to send: %d", err);
		return err;
	}

	return 0;
}

static int cmd_spp_send(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct net_buf *buf;
	uint8_t *data = spp_buf;
	int len = ARRAY_SIZE(spp_buf);

	memset(data, 0xff, sizeof(spp_buf));

	buf = bt_rfcomm_create_pdu(&tx_pool);
	if (buf == NULL) {
		shell_error(sh, "SPP buf is NULL");
		return -ENOEXEC;
	}

	if (argc > 1) {
		len = strtoul(argv[1], NULL, 10);
	}

	len = MIN(SPP_RFCOMM_MTU, len);
	shell_print(sh, "Send data len:%d", len);

	net_buf_add_mem(buf, data, len);

	err = bt_spp_send(&default_spp, buf);
	if (err < 0) {
		shell_error(sh, "fail to send data, err:%d", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	return 0;
}

static int bt_spp_disconnect(struct bt_spp *spp)
{
	int err;

	if (spp->acl_conn == NULL) {
		bt_shell_print("SPP conn invalid");
		return -EINVAL;
	}

	if (atomic_get(&spp->state) != SPP_STATE_CONNECTED) {
		bt_shell_print("Cannot disconnect SPP connection while not connected");
		return -ENOTCONN;
	}

	err = bt_rfcomm_dlc_disconnect(&spp->rfcomm_dlc);
	if (err < 0) {
		bt_shell_print("bt rfcomm disconnect err:%d", err);
		return err;
	}

	atomic_set(&spp->state, SPP_STATE_DISCONNECTING);
	return 0;
}

static int cmd_spp_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_spp_disconnect(&default_spp);
	if (err < 0) {
		shell_error(sh, "fail to disconnect SPP, err:%d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "SPP disconnecting");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	spp_cmds,
	SHELL_CMD_ARG(register_with_channel, NULL, "<channel>",
		      cmd_spp_register_with_channel, 2, 0),
	SHELL_CMD_ARG(register_with_uuid, NULL,
		      "<uuid:uuid16 or uuid32 or uuid128, "
		      "eg: uuid128:00001101-0000-1000-8000-00805F9B34FB>",
		      cmd_spp_register_with_uuid, 2, 0),
	SHELL_CMD_ARG(connect_by_channel, NULL, "<channel>",
		      cmd_spp_connect_by_channel, 2, 0),
	SHELL_CMD_ARG(connect_by_uuid, NULL,
		      "<uuid:uuid16 or uuid32 or uuid128, "
		      "eg: uuid128:00001101-0000-1000-8000-00805F9B34FB>",
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

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(spp, &spp_cmds, "Bluetooth SPP sh commands", cmd_spp, 1, 1);
