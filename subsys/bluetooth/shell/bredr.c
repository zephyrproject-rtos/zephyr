/** @file
 * @brief Bluetooth BR/EDR shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2018 Intel Corporation
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
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include <zephyr/shell/shell.h>

#include "bt.h"

#if defined(CONFIG_BT_CONN)
/* Connection context for BR/EDR legacy pairing in sec mode 3 */
static struct bt_conn *pairing_conn;
#endif /* CONFIG_BT_CONN */

#define DATA_BREDR_MTU		48

NET_BUF_POOL_FIXED_DEFINE(data_pool, 1, DATA_BREDR_MTU, 8, NULL);

#define SDP_CLIENT_USER_BUF_LEN		512
NET_BUF_POOL_FIXED_DEFINE(sdp_client_pool, CONFIG_BT_MAX_CONN,
			  SDP_CLIENT_USER_BUF_LEN, 8, NULL);

static int cmd_auth_pincode(const struct shell *sh,
			    size_t argc, char *argv[])
{
	struct bt_conn *conn;
	uint8_t max = 16U;

	if (default_conn) {
		conn = default_conn;
	} else if (pairing_conn) {
		conn = pairing_conn;
	} else {
		conn = NULL;
	}

	if (!conn) {
		shell_print(sh, "Not connected");
		return 0;
	}

	if (strlen(argv[1]) > max) {
		shell_print(sh, "PIN code value invalid - enter max %u "
			    "digits", max);
		return 0;
	}

	shell_print(sh, "PIN code \"%s\" applied", argv[1]);

	bt_conn_auth_pincode_entry(conn, argv[1]);

	return 0;
}

static int cmd_connect(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_conn *conn;
	bt_addr_t addr;
	int err;

	err = bt_addr_from_str(argv[1], &addr);
	if (err) {
		shell_print(sh, "Invalid peer address (err %d)", err);
		return -ENOEXEC;
	}

	conn = bt_conn_create_br(&addr, BT_BR_CONN_PARAM_DEFAULT);
	if (!conn) {
		shell_print(sh, "Connection failed");
	} else {

		shell_print(sh, "Connection pending");

		/* unref connection obj in advance as app user */
		bt_conn_unref(conn);
	}

	return 0;
}

static void br_device_found(const bt_addr_t *addr, int8_t rssi,
				  const uint8_t cod[3], const uint8_t eir[240])
{
	char br_addr[BT_ADDR_STR_LEN];
	char name[239];
	int len = 240;

	(void)memset(name, 0, sizeof(name));

	while (len) {
		if (len < 2) {
			break;
		}

		/* Look for early termination */
		if (!eir[0]) {
			break;
		}

		/* Check if field length is correct */
		if (eir[0] > len - 1) {
			break;
		}

		switch (eir[1]) {
		case BT_DATA_NAME_SHORTENED:
		case BT_DATA_NAME_COMPLETE:
			if (eir[0] > sizeof(name) - 1) {
				memcpy(name, &eir[2], sizeof(name) - 1);
			} else {
				memcpy(name, &eir[2], eir[0] - 1);
			}
			break;
		default:
			break;
		}

		/* Parse next AD Structure */
		len -= eir[0] + 1;
		eir += eir[0] + 1;
	}

	bt_addr_to_str(addr, br_addr, sizeof(br_addr));

	shell_print(ctx_shell, "[DEVICE]: %s, RSSI %i %s", br_addr, rssi, name);
}

static struct bt_br_discovery_result br_discovery_results[5];

static void br_discovery_complete(const struct bt_br_discovery_result *results,
				  size_t count)
{
	size_t i;

	shell_print(ctx_shell, "BR/EDR discovery complete");

	for (i = 0; i < count; i++) {
		br_device_found(&results[i].addr, results[i].rssi,
				results[i].cod, results[i].eir);
	}
}

static struct bt_br_discovery_cb discovery_cb = {
	.recv = NULL,
	.timeout = br_discovery_complete,
};

static int cmd_discovery(const struct shell *sh, size_t argc, char *argv[])
{
	const char *action;

	action = argv[1];
	if (!strcmp(action, "on")) {
		static bool reg_cb = true;
		struct bt_br_discovery_param param;

		param.limited = false;
		param.length = 8U;

		if (argc > 2) {
			param.length = atoi(argv[2]);
		}

		if (argc > 3 && !strcmp(argv[3], "limited")) {
			param.limited = true;
		}

		if (reg_cb) {
			reg_cb = false;
			bt_br_discovery_cb_register(&discovery_cb);
		}

		if (bt_br_discovery_start(&param, br_discovery_results,
					  ARRAY_SIZE(br_discovery_results)) < 0) {
			shell_print(sh, "Failed to start discovery");
			return 0;
		}

		shell_print(sh, "Discovery started");
	} else if (!strcmp(action, "off")) {
		if (bt_br_discovery_stop()) {
			shell_print(sh, "Failed to stop discovery");
			return 0;
		}

		shell_print(sh, "Discovery stopped");
	} else {
		shell_help(sh);
	}

	return 0;
}

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	shell_print(ctx_shell, "Incoming data channel %p len %u", chan,
		    buf->len);

	return 0;
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	shell_print(ctx_shell, "Channel %p connected", chan);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	shell_print(ctx_shell, "Channel %p disconnected", chan);
}

static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan)
{
	shell_print(ctx_shell, "Channel %p requires buffer", chan);

	return net_buf_alloc(&data_pool, K_FOREVER);
}

static const struct bt_l2cap_chan_ops l2cap_ops = {
	.alloc_buf	= l2cap_alloc_buf,
	.recv		= l2cap_recv,
	.connected	= l2cap_connected,
	.disconnected	= l2cap_disconnected,
};

static struct bt_l2cap_br_chan l2cap_chan = {
	.chan.ops	= &l2cap_ops,
	 /* Set for now min. MTU */
	.rx.mtu		= 48,
};

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			struct bt_l2cap_chan **chan)
{
	shell_print(ctx_shell, "Incoming BR/EDR conn %p", conn);

	if (l2cap_chan.chan.conn) {
		shell_error(ctx_shell, "No channels available");
		return -ENOMEM;
	}

	*chan = &l2cap_chan.chan;

	return 0;
}

static struct bt_l2cap_server br_server = {
	.accept = l2cap_accept,
};

static int cmd_l2cap_register(const struct shell *sh,
			      size_t argc, char *argv[])
{
	if (br_server.psm) {
		shell_print(sh, "Already registered");
		return 0;
	}

	br_server.psm = strtoul(argv[1], NULL, 16);

	if (bt_l2cap_br_server_register(&br_server) < 0) {
		shell_error(sh, "Unable to register psm");
		br_server.psm = 0U;
		return -ENOEXEC;
	} else {
		shell_print(sh, "L2CAP psm %u registered", br_server.psm);
	}

	return 0;
}

static int cmd_discoverable(const struct shell *sh,
			    size_t argc, char *argv[])
{
	int err;
	const char *action;

	action = argv[1];

	if (!strcmp(action, "on")) {
		err = bt_br_set_discoverable(true);
	} else if (!strcmp(action, "off")) {
		err = bt_br_set_discoverable(false);
	} else {
		shell_help(sh);
		return 0;
	}

	if (err) {
		shell_print(sh, "BR/EDR set/reset discoverable failed "
			    "(err %d)", err);
		return -ENOEXEC;
	} else {
		shell_print(sh, "BR/EDR set/reset discoverable done");
	}

	return 0;
}

static int cmd_connectable(const struct shell *sh,
			   size_t argc, char *argv[])
{
	int err;
	const char *action;

	action = argv[1];

	if (!strcmp(action, "on")) {
		err = bt_br_set_connectable(true);
	} else if (!strcmp(action, "off")) {
		err = bt_br_set_connectable(false);
	} else {
		shell_help(sh);
		return 0;
	}

	if (err) {
		shell_print(sh, "BR/EDR set/rest connectable failed "
			    "(err %d)", err);
		return -ENOEXEC;
	} else {
		shell_print(sh, "BR/EDR set/reset connectable done");
	}

	return 0;
}

static int cmd_oob(const struct shell *sh, size_t argc, char *argv[])
{
	char addr[BT_ADDR_STR_LEN];
	struct bt_br_oob oob;
	int err;

	err = bt_br_oob_get_local(&oob);
	if (err) {
		shell_print(sh, "BR/EDR OOB data failed");
		return -ENOEXEC;
	}

	bt_addr_to_str(&oob.addr, addr, sizeof(addr));

	shell_print(sh, "BR/EDR OOB data:");
	shell_print(sh, "  addr %s", addr);
	return 0;
}

static uint8_t sdp_hfp_ag_user(struct bt_conn *conn,
			       struct bt_sdp_client_result *result)
{
	char addr[BT_ADDR_STR_LEN];
	uint16_t param, version;
	uint16_t features;
	int res;

	conn_addr_str(conn, addr, sizeof(addr));

	if (result) {
		shell_print(ctx_shell, "SDP HFPAG data@%p (len %u) hint %u from"
			    " remote %s", result->resp_buf,
			    result->resp_buf->len, result->next_record_hint,
			    addr);

		/*
		 * Focus to get BT_SDP_ATTR_PROTO_DESC_LIST attribute item to
		 * get HFPAG Server Channel Number operating on RFCOMM protocol.
		 */
		res = bt_sdp_get_proto_param(result->resp_buf,
					     BT_SDP_PROTO_RFCOMM, &param);
		if (res < 0) {
			shell_error(ctx_shell, "Error getting Server CN, "
				    "err %d", res);
			goto done;
		}
		shell_print(ctx_shell, "HFPAG Server CN param 0x%04x", param);

		res = bt_sdp_get_profile_version(result->resp_buf,
						 BT_SDP_HANDSFREE_SVCLASS,
						 &version);
		if (res < 0) {
			shell_error(ctx_shell, "Error getting profile version, "
				    "err %d", res);
			goto done;
		}
		shell_print(ctx_shell, "HFP version param 0x%04x", version);

		/*
		 * Focus to get BT_SDP_ATTR_SUPPORTED_FEATURES attribute item to
		 * get profile Supported Features mask.
		 */
		res = bt_sdp_get_features(result->resp_buf, &features);
		if (res < 0) {
			shell_error(ctx_shell, "Error getting HFPAG Features, "
				    "err %d", res);
			goto done;
		}
		shell_print(ctx_shell, "HFPAG Supported Features param 0x%04x",
		      features);
	} else {
		shell_print(ctx_shell, "No SDP HFPAG data from remote %s",
			    addr);
	}
done:
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static uint8_t sdp_a2src_user(struct bt_conn *conn,
			   struct bt_sdp_client_result *result)
{
	char addr[BT_ADDR_STR_LEN];
	uint16_t param, version;
	uint16_t features;
	int res;

	conn_addr_str(conn, addr, sizeof(addr));

	if (result) {
		shell_print(ctx_shell, "SDP A2SRC data@%p (len %u) hint %u from"
			    " remote %s", result->resp_buf,
			    result->resp_buf->len, result->next_record_hint,
			    addr);

		/*
		 * Focus to get BT_SDP_ATTR_PROTO_DESC_LIST attribute item to
		 * get A2SRC Server PSM Number.
		 */
		res = bt_sdp_get_proto_param(result->resp_buf,
					     BT_SDP_PROTO_L2CAP, &param);
		if (res < 0) {
			shell_error(ctx_shell, "A2SRC PSM Number not found, "
				    "err %d", res);
			goto done;
		}

		shell_print(ctx_shell, "A2SRC Server PSM Number param 0x%04x",
			    param);

		/*
		 * Focus to get BT_SDP_ATTR_PROFILE_DESC_LIST attribute item to
		 * get profile version number.
		 */
		res = bt_sdp_get_profile_version(result->resp_buf,
						 BT_SDP_ADVANCED_AUDIO_SVCLASS,
						 &version);
		if (res < 0) {
			shell_error(ctx_shell, "A2SRC version not found, "
				    "err %d", res);
			goto done;
		}
		shell_print(ctx_shell, "A2SRC version param 0x%04x", version);

		/*
		 * Focus to get BT_SDP_ATTR_SUPPORTED_FEATURES attribute item to
		 * get profile supported features mask.
		 */
		res = bt_sdp_get_features(result->resp_buf, &features);
		if (res < 0) {
			shell_error(ctx_shell, "A2SRC Features not found, "
				    "err %d", res);
			goto done;
		}
		shell_print(ctx_shell, "A2SRC Supported Features param 0x%04x",
		      features);
	} else {
		shell_print(ctx_shell, "No SDP A2SRC data from remote %s",
			    addr);
	}
done:
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static struct bt_sdp_discover_params discov_hfpag = {
	.uuid = BT_UUID_DECLARE_16(BT_SDP_HANDSFREE_AGW_SVCLASS),
	.func = sdp_hfp_ag_user,
	.pool = &sdp_client_pool,
};

static struct bt_sdp_discover_params discov_a2src = {
	.uuid = BT_UUID_DECLARE_16(BT_SDP_AUDIO_SOURCE_SVCLASS),
	.func = sdp_a2src_user,
	.pool = &sdp_client_pool,
};

static struct bt_sdp_discover_params discov;

static int cmd_sdp_find_record(const struct shell *sh,
			       size_t argc, char *argv[])
{
	int res;
	const char *action;

	if (!default_conn) {
		shell_print(sh, "Not connected");
		return 0;
	}

	action = argv[1];

	if (!strcmp(action, "HFPAG")) {
		discov = discov_hfpag;
	} else if (!strcmp(action, "A2SRC")) {
		discov = discov_a2src;
	} else {
		shell_help(sh);
		return 0;
	}

	shell_print(sh, "SDP UUID \'%s\' gets applied", action);

	res = bt_sdp_discover(default_conn, &discov);
	if (res) {
		shell_error(sh, "SDP discovery failed: result %d", res);
		return -ENOEXEC;
	} else {
		shell_print(sh, "SDP discovery started");
	}

	return 0;
}

#define HELP_NONE "[none]"
#define HELP_ADDR_LE "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"

SHELL_STATIC_SUBCMD_SET_CREATE(br_cmds,
	SHELL_CMD_ARG(auth-pincode, NULL, "<pincode>", cmd_auth_pincode, 2, 0),
	SHELL_CMD_ARG(connect, NULL, "<address>", cmd_connect, 2, 0),
	SHELL_CMD_ARG(discovery, NULL,
		      "<value: on, off> [length: 1-48] [mode: limited]",
		      cmd_discovery, 2, 2),
	SHELL_CMD_ARG(iscan, NULL, "<value: on, off>", cmd_discoverable, 2, 0),
	SHELL_CMD_ARG(l2cap-register, NULL, "<psm>", cmd_l2cap_register, 2, 0),
	SHELL_CMD_ARG(oob, NULL, NULL, cmd_oob, 1, 0),
	SHELL_CMD_ARG(pscan, NULL, "<value: on, off>", cmd_connectable, 2, 0),
	SHELL_CMD_ARG(sdp-find, NULL, "<HFPAG>", cmd_sdp_find_record, 2, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_br(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* shell returns 1 when help is printed */
		return 1;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(br, &br_cmds, "Bluetooth BR/EDR shell commands", cmd_br,
		       1, 1);
