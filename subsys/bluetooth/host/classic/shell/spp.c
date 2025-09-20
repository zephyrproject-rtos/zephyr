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
#include <zephyr/bluetooth/classic/spp.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"
#include "common/bt_str.h"

#define HELP_NONE "[none]"
#define DATA_MTU  CONFIG_BT_SPP_L2CAP_MTU

enum {
	SPP_DISCONNECTED,
	SPP_REGISTERED,
	SPP_CONNECTING,
	SPP_CONNECTED,
};

static int spp_accept(struct bt_conn *conn, struct bt_spp_server *server, struct bt_spp **spp);

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(CONFIG_BT_SPP_L2CAP_MTU),
			  16, NULL);

static uint8_t spp_channel;
static struct net_buf *tx_buf;
static uint8_t spp_state;
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

static uint8_t spp_uuid_buf[16];

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
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 22),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 20),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID128),
				spp_uuid_buf
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

static struct bt_spp_server spp_server = {
	.accept = spp_accept,
};

static void spp_connected(struct bt_conn *conn, struct bt_spp *spp)
{
	spp_state = SPP_CONNECTED;

	bt_shell_print("SPP:%p, conn:%p connected ", spp, conn);
}

static void spp_disconnected(struct bt_spp *spp)
{
	spp_state = SPP_DISCONNECTED;

	bt_shell_print("SPP:%p disconnected", spp);
}

static void spp_recv(struct bt_spp *spp, struct net_buf *buf)
{
	bt_shell_print("SPP:%p, data len:%d", spp, buf->len);

	bt_shell_hexdump(buf->data, buf->len);
}

static struct bt_spp_ops spp_ops = {
	.connected = spp_connected,
	.disconnected = spp_disconnected,
	.recv = spp_recv,
};

static struct bt_spp default_spp = {
	.ops = &spp_ops,
};

static int spp_accept(struct bt_conn *conn, struct bt_spp_server *server, struct bt_spp **spp)
{
	if (spp_state == SPP_CONNECTED) {
		bt_shell_print("SPP already connected");
		return -EALREADY;
	}

	spp_state = SPP_CONNECTED;
	*spp = &default_spp;

	return 0;
}

static int spp_uuid_register(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;
	const char *uuid;
	char hex[33];
	static uint8_t uuid_buf[16];

	uuid = argv[2];

	if ((strlen(uuid) != 36) || (uuid[8] != '-') || (uuid[13] != '-') || (uuid[18] != '-') ||
	    (uuid[23] != '-')) {
		shell_print(sh, "Usage: <uuid128>\nExample: 00001101-0000-1000-8000-00805F9B34FB");
		return -ENOEXEC;
	}

	/* remove dashes into contiguous hex string */
	for (size_t i = 0, j = 0; i < 36; i++) {
		if (uuid[i] == '-') {
			continue;
		}

		hex[j++] = uuid[i];
	}
	hex[32] = '\0';

	/* convert 32 hex chars -> 16 bytes into spp_uuid_buf */
	err = hex2bin(hex, 32, uuid_buf, sizeof(uuid_buf));
	if (err <= 0) {
		shell_print(sh, "fail parse uuid hex");
		return -ENOEXEC;
	}

	sys_memcpy_swap(spp_uuid_buf, uuid_buf, sizeof(uuid_buf));

	spp_server.sdp_record = &spp_rec_uuid128;
	spp_server.rfcomm_server.channel = 0;

	err = bt_spp_server_register(&spp_server);
	if (err < 0) {
		shell_print(sh, "fail to register SPP service, err:%d", err);
		spp_server.sdp_record = NULL;
		spp_server.rfcomm_server.channel = 0U;
		return -ENOEXEC;
	}

	spp_channel = spp_server.rfcomm_server.channel;

	shell_print(sh, "register SPP srv uuid:%s(channel:%d) success", uuid, spp_channel);

	return 0;
}

static int spp_channel_register(const struct shell *sh, int32_t argc, char *argv[])
{
	int err = -1;

	spp_channel = strtoul(argv[2], NULL, 16);

	spp_server.sdp_record = &spp_rec_channel;
	spp_server.rfcomm_server.channel = spp_channel;

	err = bt_spp_server_register(&spp_server);
	if (err < 0) {
		shell_print(sh, "fail to register SPP service, err:%d", err);
		spp_server.sdp_record = NULL;
		spp_server.rfcomm_server.channel = 0U;
		return -ENOEXEC;
	}

	shell_print(sh, "register SPP srv channel:%d success", spp_channel);

	return 0;
}

static int cmd_register_server(const struct shell *sh, int32_t argc, char *argv[])
{
	int err = -1;
	const char *type;

	if (argc < 3) {
		return -ENOEXEC;
	}

	type = argv[1];

	if (!strcmp(type, "channel")) {
		err = spp_channel_register(sh, argc, argv);
	} else if (!strcmp(type, "uuid")) {
		err = spp_uuid_register(sh, argc, argv);
	} else {
		shell_print(sh, "invalid type:%s", type);
		return -ENOEXEC;
	}

	return err;
}

static int spp_uuid_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	size_t len;

	len = strlen(argv[2]);

	if (len == (BT_UUID_SIZE_16 * 2)) {
		uint16_t val;

		spp_uuid.u16.uuid.type = BT_UUID_TYPE_16;
		hex2bin(argv[2], len, (uint8_t *)&val, sizeof(val));
		spp_uuid.u16.val = sys_be16_to_cpu(val);
	} else if (len == (BT_UUID_SIZE_32 * 2)) {
		uint32_t val;

		spp_uuid.u32.uuid.type = BT_UUID_TYPE_32;
		hex2bin(argv[2], len, (uint8_t *)&val, sizeof(val));
		spp_uuid.u32.val = sys_be32_to_cpu(val);
	} else if (len == (BT_UUID_SIZE_128 * 2)) {
		uint8_t uuid128[BT_UUID_SIZE_128];

		spp_uuid.u128.uuid.type = BT_UUID_TYPE_128;
		hex2bin(argv[2], len, &uuid128[0], sizeof(uuid128));
		sys_memcpy_swap(spp_uuid.u128.val, uuid128, sizeof(uuid128));
	} else {
		shell_error(sh, "Invalid UUID");
		return -ENOEXEC;
	}

	default_spp.mode = BT_SPP_MODE_UUID;
	default_spp.uuid = (const struct bt_uuid *)&spp_uuid;
	spp_state = SPP_CONNECTING;

	err = bt_spp_connect(default_conn, &default_spp);
	if (err < 0) {
		shell_error(sh, "fail to connect SPP device, err:%d", err);
		default_spp.mode = BT_SPP_MODE_UNKNOWN;
		default_spp.uuid = NULL;
		spp_state = SPP_DISCONNECTED;
		return -ENOEXEC;
	}

	shell_print(sh, "SPP connect uuid:%s ", bt_uuid_str((const struct bt_uuid *)&spp_uuid));
	return 0;
}

static int spp_channel_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t channel;

	channel = strtoul(argv[2], NULL, 16);

	default_spp.mode = BT_SPP_MODE_CHANNEL;
	default_spp.channel = channel;
	spp_state = SPP_CONNECTING;

	err = bt_spp_connect(default_conn, &default_spp);
	if (err < 0) {
		shell_error(sh, "fail to connect SPP device, err:%d", err);
		default_spp.mode = BT_SPP_MODE_UNKNOWN;
		default_spp.uuid = NULL;
		spp_state = SPP_DISCONNECTED;
		return -ENOEXEC;
	}

	shell_print(sh, "SPP connect channel:%d", channel);
	return 0;
}

static int cmd_spp_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	const char *type;

	if (argc < 3) {
		return -ENOEXEC;
	}

	if (default_conn == NULL) {
		shell_error(sh, "please connect bt first");
		return -ENOEXEC;
	}

	type = argv[1];

	if (!strcmp(type, "channel")) {
		err = spp_channel_connect(sh, argc, argv);
	} else if (!strcmp(type, "uuid")) {
		err = spp_uuid_connect(sh, argc, argv);
	} else {
		shell_print(sh, "invalid type:%s", type);
		return -ENOEXEC;
	}

	return err;
}

static int cmd_spp_send(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t data[DATA_MTU];
	int len = DATA_MTU;
	int err;

	memset(data, 0xff, sizeof(data));

	if (spp_state != SPP_CONNECTED) {
		shell_error(sh, "SPP is not connected");
		return -ENOEXEC;
	}

	tx_buf = bt_rfcomm_create_pdu(&tx_pool);
	if (tx_buf == NULL) {
		shell_error(sh, "SPP tx_buf is NULL");
		return -ENOEXEC;
	}

	if (argc > 1) {
		len = strtoul(argv[1], NULL, 10);
	}

	len = MIN(DATA_MTU, len);
	shell_print(sh, "Send data len:%d", len);

	net_buf_add_mem(tx_buf, data, len);

	err = bt_spp_send(&default_spp, tx_buf);
	if (err < 0) {
		shell_error(sh, "fail to send data, err:%d", err);
		net_buf_unref(tx_buf);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_spp_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (spp_state != SPP_CONNECTED) {
		shell_error(sh, "SPP is not connected");
		return -ENOEXEC;
	}

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
	SHELL_CMD_ARG(register, NULL, "register server  <type: channel or uuid> <value>",
		      cmd_register_server, 3, 0),
	SHELL_CMD_ARG(connect, NULL, "connect <type: channel or uuid> <value>", cmd_spp_connect, 3,
		      0),
	SHELL_CMD_ARG(send, NULL, "send [length of packet(s)]", cmd_spp_send, 2, 1),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_spp_disconnect, 1, 0), SHELL_SUBCMD_SET_END);

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
