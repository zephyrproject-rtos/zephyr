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
#include <misc/byteorder.h>
#include <zephyr.h>

#include <settings/settings.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>

#include <shell/shell.h>

#include "bt.h"

#if defined(CONFIG_BT_CONN)
/* Connection context for BR/EDR legacy pairing in sec mode 3 */
static struct bt_conn *pairing_conn;
#endif /* CONFIG_BT_CONN */

#define DATA_BREDR_MTU		48

NET_BUF_POOL_DEFINE(data_pool, 1, DATA_BREDR_MTU, BT_BUF_USER_DATA_MIN,
		    NULL);

#define SDP_CLIENT_USER_BUF_LEN		512
NET_BUF_POOL_DEFINE(sdp_client_pool, CONFIG_BT_MAX_CONN,
		    SDP_CLIENT_USER_BUF_LEN, BT_BUF_USER_DATA_MIN, NULL);

static void cmd_auth_pincode(const struct shell *shell,
			     size_t argc, char *argv[])
{
	struct bt_conn *conn;
	u8_t max = 16;

	if (default_conn) {
		conn = default_conn;
	} else if (pairing_conn) {
		conn = pairing_conn;
	} else {
		conn = NULL;
	}

	if (!conn) {
		print(shell, "Not connected\n");
		return;
	}

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	if (strlen(argv[1]) > max) {
		print(shell, "PIN code value invalid - enter max %u digits\n",
		      max);
		return;
	}

	print(shell, "PIN code \"%s\" applied\n", argv[1]);

	bt_conn_auth_pincode_entry(conn, argv[1]);
}

static void cmd_connect(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn;
	bt_addr_t addr;
	int err;

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	err = str2bt_addr(argv[1], &addr);
	if (err) {
		print(shell, "Invalid peer address (err %d)\n", err);
		return;
	}

	conn = bt_conn_create_br(&addr, BT_BR_CONN_PARAM_DEFAULT);
	if (!conn) {
		print(shell, "Connection failed\n");
	} else {

		print(shell, "Connection pending\n");

		/* unref connection obj in advance as app user */
		bt_conn_unref(conn);
	}
}

static void br_device_found(const bt_addr_t *addr, s8_t rssi,
				  const u8_t cod[3], const u8_t eir[240])
{
	char br_addr[BT_ADDR_STR_LEN];
	char name[239];
	int len = 240;

	(void)memset(name, 0, sizeof(name));

	while (len) {
		if (len < 2) {
			break;
		};

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

	print(NULL, "[DEVICE]: %s, RSSI %i %s\n", br_addr, rssi, name);
}

static struct bt_br_discovery_result br_discovery_results[5];

static void br_discovery_complete(struct bt_br_discovery_result *results,
				  size_t count)
{
	size_t i;

	print(NULL, "BR/EDR discovery complete\n");

	for (i = 0; i < count; i++) {
		br_device_found(&results[i].addr, results[i].rssi,
				results[i].cod, results[i].eir);
	}
}

static void cmd_discovery(const struct shell *shell, size_t argc, char *argv[])
{
	const char *action;

	if (!shell_cmd_precheck(shell, argc >= 2, NULL, 0)) {
		return;
	}

	action = argv[1];
	if (!strcmp(action, "on")) {
		struct bt_br_discovery_param param;

		param.limited = false;
		param.length = 8;

		if (argc > 2) {
			param.length = atoi(argv[2]);
		}

		if (argc > 3 && !strcmp(argv[3], "limited")) {
			param.limited = true;
		}

		if (bt_br_discovery_start(&param, br_discovery_results,
					  ARRAY_SIZE(br_discovery_results),
					  br_discovery_complete) < 0) {
			print(shell, "Failed to start discovery\n");
			return;
		}

		print(shell, "Discovery started\n");
	} else if (!strcmp(action, "off")) {
		if (bt_br_discovery_stop()) {
			print(shell, "Failed to stop discovery\n");
			return;
		}

		print(shell, "Discovery stopped\n");
	} else {
		shell_help_print(shell, NULL, 0);
		return;
	}
}

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	print(NULL, "Incoming data channel %p len %u\n", chan, buf->len);

	return 0;
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	print(NULL, "Channel %p connected\n", chan);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	print(NULL, "Channel %p disconnected\n", chan);
}

static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan)
{
	print(NULL, "Channel %p requires buffer\n", chan);

	return net_buf_alloc(&data_pool, K_FOREVER);
}

static struct bt_l2cap_chan_ops l2cap_ops = {
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

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	print(NULL, "Incoming BR/EDR conn %p\n", conn);

	if (l2cap_chan.chan.conn) {
		error(NULL, "No channels available");
		return -ENOMEM;
	}

	*chan = &l2cap_chan.chan;

	return 0;
}

static struct bt_l2cap_server br_server = {
	.accept = l2cap_accept,
};

static void cmd_l2cap_register(const struct shell *shell,
				    size_t argc, char *argv[])
{
	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	if (br_server.psm) {
		print(shell, "Already registered\n");
		return;
	}

	br_server.psm = strtoul(argv[1], NULL, 16);

	if (bt_l2cap_br_server_register(&br_server) < 0) {
		error(shell, "Unable to register psm\n");
		br_server.psm = 0;
	} else {
		print(shell, "L2CAP psm %u registered\n", br_server.psm);
	}
}

static void cmd_discoverable(const struct shell *shell,
			     size_t argc, char *argv[])
{
	int err;
	const char *action;

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	action = argv[1];

	if (!strcmp(action, "on")) {
		err = bt_br_set_discoverable(true);
	} else if (!strcmp(action, "off")) {
		err = bt_br_set_discoverable(false);
	} else {
		shell_help_print(shell, NULL, 0);
		return;
	}

	if (err) {
		print(shell, "BR/EDR set/reset discoverable failed (err %d)\n",
		      err);
	} else {
		print(shell, "BR/EDR set/reset discoverable done\n");
	}
}

static void cmd_connectable(const struct shell *shell,
			    size_t argc, char *argv[])
{
	int err;
	const char *action;

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	action = argv[1];

	if (!strcmp(action, "on")) {
		err = bt_br_set_connectable(true);
	} else if (!strcmp(action, "off")) {
		err = bt_br_set_connectable(false);
	} else {
		shell_help_print(shell, NULL, 0);
		return;
	}

	if (err) {
		print(shell, "BR/EDR set/rest connectable failed (err %d)\n",
		      err);
	} else {
		print(shell, "BR/EDR set/reset connectable done\n");
	}
}

static void cmd_oob(const struct shell *shell, size_t argc, char *argv[])
{
	char addr[BT_ADDR_STR_LEN];
	struct bt_br_oob oob;
	int err;

	err = bt_br_oob_get_local(&oob);
	if (err) {
		print(shell, "BR/EDR OOB data failed\n");
		return;
	}

	bt_addr_to_str(&oob.addr, addr, sizeof(addr));

	print(shell, "BR/EDR OOB data:\n");
	print(shell, "  addr %s\n", addr);
}

static u8_t sdp_hfp_ag_user(struct bt_conn *conn,
			       struct bt_sdp_client_result *result)
{
	char addr[BT_ADDR_STR_LEN];
	u16_t param, version;
	u16_t features;
	int res;

	conn_addr_str(conn, addr, sizeof(addr));

	if (result) {
		print(NULL, "SDP HFPAG data@%p (len %u) hint %u from remote %s"
		      "\n", result->resp_buf, result->resp_buf->len,
		      result->next_record_hint, addr);

		/*
		 * Focus to get BT_SDP_ATTR_PROTO_DESC_LIST attribute item to
		 * get HFPAG Server Channel Number operating on RFCOMM protocol.
		 */
		res = bt_sdp_get_proto_param(result->resp_buf,
					     BT_SDP_PROTO_RFCOMM, &param);
		if (res < 0) {
			error(NULL, "Error getting Server CN, err %d\n", res);
			goto done;
		}
		print(NULL, "HFPAG Server CN param 0x%04x\n", param);

		res = bt_sdp_get_profile_version(result->resp_buf,
						 BT_SDP_HANDSFREE_SVCLASS,
						 &version);
		if (res < 0) {
			error(NULL, "Error getting profile version, err %d\n",
			      res);
			goto done;
		}
		print(NULL, "HFP version param 0x%04x\n", version);

		/*
		 * Focus to get BT_SDP_ATTR_SUPPORTED_FEATURES attribute item to
		 * get profile Supported Features mask.
		 */
		res = bt_sdp_get_features(result->resp_buf, &features);
		if (res < 0) {
			error(NULL, "Error getting HFPAG Features, err %d\n",
			      res);
			goto done;
		}
		print(NULL, "HFPAG Supported Features param 0x%04x\n",
		      features);
	} else {
		print(NULL, "No SDP HFPAG data from remote %s\n", addr);
	}
done:
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static u8_t sdp_a2src_user(struct bt_conn *conn,
			   struct bt_sdp_client_result *result)
{
	char addr[BT_ADDR_STR_LEN];
	u16_t param, version;
	u16_t features;
	int res;

	conn_addr_str(conn, addr, sizeof(addr));

	if (result) {
		print(NULL, "SDP A2SRC data@%p (len %u) hint %u from remote %s"
		      "\n", result->resp_buf, result->resp_buf->len,
		      result->next_record_hint, addr);

		/*
		 * Focus to get BT_SDP_ATTR_PROTO_DESC_LIST attribute item to
		 * get A2SRC Server PSM Number.
		 */
		res = bt_sdp_get_proto_param(result->resp_buf,
					     BT_SDP_PROTO_L2CAP, &param);
		if (res < 0) {
			error(NULL, "A2SRC PSM Number not found, err %d\n",
			      res);
			goto done;
		}
		print(NULL, "A2SRC Server PSM Number param 0x%04x\n", param);

		/*
		 * Focus to get BT_SDP_ATTR_PROFILE_DESC_LIST attribute item to
		 * get profile version number.
		 */
		res = bt_sdp_get_profile_version(result->resp_buf,
						 BT_SDP_ADVANCED_AUDIO_SVCLASS,
						 &version);
		if (res < 0) {
			error(NULL, "A2SRC version not found, err %d\n", res);
			goto done;
		}
		print(NULL, "A2SRC version param 0x%04x\n", version);

		/*
		 * Focus to get BT_SDP_ATTR_SUPPORTED_FEATURES attribute item to
		 * get profile supported features mask.
		 */
		res = bt_sdp_get_features(result->resp_buf, &features);
		if (res < 0) {
			error(NULL, "A2SRC Features not found, err %d\n", res);
			goto done;
		}
		print(NULL, "A2SRC Supported Features param 0x%04x\n",
		      features);
	} else {
		print(NULL, "No SDP A2SRC data from remote %s\n", addr);
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

static void cmd_sdp_find_record(const struct shell *shell,
				size_t argc, char *argv[])
{
	int err = 0, res;
	const char *action;

	if (!default_conn) {
		print(shell, "Not connected\n");
		return;
	}

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	action = argv[1];

	if (!strcmp(action, "HFPAG")) {
		discov = discov_hfpag;
	} else if (!strcmp(action, "A2SRC")) {
		discov = discov_a2src;
	} else {
		shell_help_print(shell, NULL, 0);
		return;
	}

	if (err) {
		error(shell, "SDP UUID to resolve not valid (err %d)\n", err);
		print(shell, "Supported UUID are \'HFPAG\' \'A2SRC\' only\n");
		return;
	}

	print(shell, "SDP UUID \'%s\' gets applied\n", action);

	res = bt_sdp_discover(default_conn, &discov);
	if (res) {
		error(shell, "SDP discovery failed: result %d\n", res);
	} else {
		print(shell, "SDP discovery started\n");
	}
}

#define HELP_NONE "[none]"
#define HELP_ADDR_LE "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"

SHELL_CREATE_STATIC_SUBCMD_SET(br_cmds) {
	SHELL_CMD(auth-pincode, NULL, "<pincode>", cmd_auth_pincode),
	SHELL_CMD(connect, NULL, "<address>", cmd_connect),
	SHELL_CMD(discovery, NULL,
		  "<value: on, off> [length: 1-48] [mode: limited]",
		  cmd_discovery),
	SHELL_CMD(iscan, NULL, "<value: on, off>", cmd_discoverable),
	SHELL_CMD(l2cap-register, NULL, "<psm>", cmd_l2cap_register),
	SHELL_CMD(oob, NULL, NULL, cmd_oob),
	SHELL_CMD(pscan, NULL, "value: on, off", cmd_connectable),
	SHELL_CMD(sdp-find, NULL, "<HFPAG>", cmd_sdp_find_record),
	SHELL_SUBCMD_SET_END
};

static void cmd_br(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help_print(shell, NULL, 0);
		return;
	}

	if (!shell_cmd_precheck(shell, (argc == 2), NULL, 0)) {
		return;
	}

	shell_fprintf(shell, SHELL_ERROR, "%s:%s%s\r\n", argv[0],
		      "unknown parameter: ", argv[1]);
}

SHELL_CMD_REGISTER(br, &br_cmds, "Bluetooth BR/EDR shell commands", cmd_br);
