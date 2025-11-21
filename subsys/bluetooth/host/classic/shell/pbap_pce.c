/** @file
 * @brief Bluetooth PBAP shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */
/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stdlib.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/pbap.h>
#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

struct bt_pbap_pce_app {
	struct bt_pbap_pce pbap_pce;
	struct net_buf *tx_buf;
	struct bt_conn *conn;
};
#define APPL_PBAP_PCE_MAX_COUNT 1U
struct bt_pbap_pce_app g_pbap_pce[APPL_PBAP_PCE_MAX_COUNT];

static struct bt_pbap_pce_app *pbap_pce_app;

#define PBAP_APPL_PARAM_MAX_COUNT     10U
#define PBAP_APPL_PARAM_DATA_MAX_SIZE 10U
struct bt_obex_tlv gpbap_appl_param[PBAP_APPL_PARAM_MAX_COUNT];
uint8_t gpbap_appl_param_data[PBAP_APPL_PARAM_MAX_COUNT][PBAP_APPL_PARAM_DATA_MAX_SIZE];
uint8_t appl_param_count = 0U;

#define PBAP_MOPL CONFIG_BT_GOEP_RFCOMM_MTU

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_RFCOMM_BUF_SIZE(PBAP_MOPL),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#define APP_PBAP_PWD_MAX_LENGTH 50U
uint8_t pwd[APP_PBAP_PWD_MAX_LENGTH];

struct pbap_hdr {
	const uint8_t *value;
	uint16_t length;
};

static int stringTonum_64(char *str, uint64_t *value, uint8_t base)
{
	char *p = str;
	uint64_t total = 0;
	uint8_t chartoint = 0;
	if (base == 10) {
		while (p && *p != '\0') {
			if (*p >= '0' && *p <= '9') {
				total = total * 10 + (*p - '0');
			} else {
				return -EINVAL;
			}
			p++;
		}
	} else if (base == 16) {
		while (p && *p != '\0') {
			if (*p >= '0' && *p <= '9') {
				chartoint = *p - '0';
			} else if (*p >= 'a' && *p <= 'f') {
				chartoint = *p - 'a' + 10;
			} else if (*p >= 'A' && *p <= 'F') {
				chartoint = *p - 'A' + 10;
			} else {
				return -EINVAL;
			}
			total = 16 * total + chartoint;
			p++;
		}
	}
	*value = total;
	return 0;
}

static struct bt_pbap_pce_app *g_pbap_pce_allocate(struct bt_conn *conn)
{
	for (uint8_t i = 0; i < APPL_PBAP_PCE_MAX_COUNT; i++) {
		if (!g_pbap_pce[i].conn) {
			g_pbap_pce[i].conn = default_conn;
			return &g_pbap_pce[i];
		}
	}
	return NULL;
}

static void pbap_connected(struct bt_pbap_pce *pbap, uint16_t mpl)
{
	bt_shell_print("pbap connect success %d", mpl);
}

static void pbap_disconnected(struct bt_pbap_pce *pbap, uint8_t rsp_code)
{
	if (rsp_code == BT_PBAP_RSP_CODE_OK) {
		bt_shell_print("pbap disconnect success %d", rsp_code);
	} else {
		bt_shell_print("pbap disconnect fail %d", rsp_code);
	}
	pbap_pce_app->conn = NULL;
}

void pbap_pull_phonebook(struct bt_pbap_pce *pbap, uint8_t rsp_code, struct net_buf *buf)
{
	int err;
	struct pbap_hdr body;

	bt_shell_print("pbap_pull_phonebook");
	if (rsp_code == BT_PBAP_RSP_CODE_CONTINUE) {
		err = bt_pbap_pce_get_body(buf, &body.length, &body.value);
		if (err) {
			bt_shell_print("Fail to get body or no body %d", err);
		}

		// pbap_pce_app->tx_buf  = bt_pbap_create_pdu(&pbap_pce_app->pbap_pce, &tx_pool);
		// if (!pbap_pce_app->tx_buf){
		// 	bt_shell_print("Fail to allocate tx buf");
		// 	return;
		// }

		// err = bt_pbap_pce_pull_phonebook_create_cmd(&pbap_pce_app->pbap_pce,
		// pbap_pce_app->tx_buf, NULL, false); if (err){
		// 	net_buf_unref(pbap_pce_app->tx_buf);
		// 	bt_shell_print("Fail create pull phonebook cmd  %d", err);
		// 	return;
		// }

		// err = bt_pbap_pce_send_cmd(&pbap_pce_app->pbap_pce, pbap_pce_app->tx_buf);
		// if (err){
		// 	net_buf_unref(pbap_pce_app->tx_buf);
		// 	bt_shell_print("Fail to send command %d",err);
		// }

	} else if (rsp_code == BT_PBAP_RSP_CODE_SUCCESS) {
		err = bt_pbap_pce_get_end_body(buf, &body.length, &body.value);
		if (err) {
			bt_shell_print("Fail to get body or no body %d", err);
		}
	}
	if (body.length && body.value) {

		bt_shell_print("\n=========body=========\n");
		bt_shell_print("%s\n", body.value);
		bt_shell_print("=========body=========\n");
	}
	return;
}

void pbap_pull_vcardlisting(struct bt_pbap_pce *pbap, uint8_t rsp_code, struct net_buf *buf)
{
	int err;
	struct pbap_hdr body;
	bt_shell_print("pbap_pull_vcardlisting callback");

	if (rsp_code == BT_PBAP_RSP_CODE_CONTINUE) {
		err = bt_pbap_pce_get_body(buf, &body.length, &body.value);
		if (err) {
			bt_shell_print("Fail to get body or no body %d", err);
		}

		// pbap_pce_app->tx_buf  = bt_pbap_create_pdu(&pbap_pce_app->pbap_pce, &tx_pool);
		// if (!pbap_pce_app->tx_buf){
		// 	bt_shell_print("Fail to allocate tx buf");
		// 	return;
		// }
		// err = bt_pbap_pce_pull_vcardlisting_create_cmd(&pbap_pce_app->pbap_pce,
		// pbap_pce_app->tx_buf, NULL, false); if (err){
		// 	net_buf_unref(pbap_pce_app->tx_buf);
		// 	bt_shell_print("Fail create pull vcardlisting cmd  %d", err);
		// 	return;
		// }

		// err = bt_pbap_pce_send_cmd(&pbap_pce_app->pbap_pce, pbap_pce_app->tx_buf);
		// if (err){
		// 	net_buf_unref(pbap_pce_app->tx_buf);
		// 	bt_shell_print("Fail to send command %d",err);
		// }

	} else if (rsp_code == BT_PBAP_RSP_CODE_SUCCESS) {
		err = bt_pbap_pce_get_end_body(buf, &body.length, &body.value);
		if (err) {
			bt_shell_print("Fail to get body or no body %d", err);
		}
	}
	// if (body.length && body.value){

	// 	bt_shell_print("\n=========body=========\n");
	// 	// bt_shell_print("%s\n", body.value);
	// 	bt_shell_print("=========body=========\n");
	// }
	return;
}

void pbap_pull_vcardentry(struct bt_pbap_pce *pbap, uint8_t rsp_code, struct net_buf *buf)
{
	int err;
	struct pbap_hdr body;
	bt_shell_print("pbap_pull_vcardentry callback");
	if (rsp_code == BT_PBAP_RSP_CODE_CONTINUE) {
		err = bt_pbap_pce_get_body(buf, &body.length, &body.value);
		if (err) {
			bt_shell_print("Fail to get body or no body %d", err);
		}

		// pbap_pce_app->tx_buf  = bt_pbap_create_pdu(&pbap_pce_app->pbap_pce, &tx_pool);
		// if (!pbap_pce_app->tx_buf){
		// 	bt_shell_print("Fail to allocate tx buf");
		// 	return;
		// }

		// err = bt_pbap_pce_pull_vcardentry_create_cmd(&pbap_pce_app->pbap_pce,
		// pbap_pce_app->tx_buf, NULL, false); if (err){
		// 	net_buf_unref(pbap_pce_app->tx_buf);
		// 	bt_shell_print("Fail create pull vcardentry cmd  %d", err);
		// 	return;
		// }

		// err = bt_pbap_pce_send_cmd(&pbap_pce_app->pbap_pce, pbap_pce_app->tx_buf);
		// if (err){
		// 	net_buf_unref(pbap_pce_app->tx_buf);
		// 	bt_shell_print("Fail to send command %d",err);
		// }

	} else if (rsp_code == BT_PBAP_RSP_CODE_SUCCESS) {
		err = bt_pbap_pce_get_end_body(buf, &body.length, &body.value);
		if (err) {
			bt_shell_print("Fail to get body or no body %d", err);
		}
	}
	// if (body.length && body.value){

	// 	bt_shell_print("\n=========body=========\n");
	// 	// bt_shell_print("%s\n", body.value);
	// 	bt_shell_print("=========body=========\n");
	// }
}

void pbap_set_path(struct bt_pbap_pce *pbap, uint8_t rsp_code)
{
	if (rsp_code == BT_PBAP_RSP_CODE_SUCCESS) {
		bt_shell_print("set path success.");
	} else {
		bt_shell_print("set path fail.");
	}
}

void pbap_get_auth_info(struct bt_pbap_pce *pbap)
{
	memcpy(pwd, "0000", 4U);
	pbap->pwd = pwd;
}

struct bt_pbap_pce_cb cb = {
	.connect = pbap_connected,
	.disconnect = pbap_disconnected,
	.pull_phonebook = pbap_pull_phonebook,
	.pull_vcardlisting = pbap_pull_vcardlisting,
	.pull_vcardentry = pbap_pull_vcardentry,
	.set_path = pbap_set_path,
	.get_auth_info = pbap_get_auth_info,
};

static int cmd_register(const struct shell *sh, size_t argc, char *argv[])
{

	return bt_pbap_pce_register(&cb);
}

static int cmd_connect_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t channel = (uint8_t)strtoul(argv[1], NULL, 10);
	pbap_pce_app = g_pbap_pce_allocate(default_conn);
	if (!pbap_pce_app) {
		bt_shell_print("No available pbap");
		return -EINVAL;
	}
	pbap_pce_app->pbap_pce.mpl = 600;
	pbap_pce_app->pbap_pce.peer_feature = 0x3FF;
	pbap_pce_app->pbap_pce.pwd = NULL;
	if (argc > 2) {
		memcpy(pwd, argv[2], strlen(argv[2]));
		pbap_pce_app->pbap_pce.pwd = pwd;
	}
	return bt_pbap_pce_rfcomm_connect(default_conn, channel, &pbap_pce_app->pbap_pce);
}

static int cmd_connect_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t psm = (uint16_t)strtoul(argv[1], NULL, 10);
	pbap_pce_app = g_pbap_pce_allocate(default_conn);
	if (!pbap_pce_app) {
		bt_shell_print("No available pbap");
		return -EINVAL;
	}
	pbap_pce_app->pbap_pce.mpl = 600;
	pbap_pce_app->pbap_pce.peer_feature = 0x3FF;
	pbap_pce_app->pbap_pce.pwd = NULL;
	if (argc > 2) {
		memcpy(pwd, argv[2], strlen(argv[2]));
		pbap_pce_app->pbap_pce.pwd = pwd;
	}
	return bt_pbap_pce_l2cap_connect(default_conn, psm, &pbap_pce_app->pbap_pce);
}

static int cmd_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t enforce = (uint16_t)strtoul(argv[1], NULL, 10);
	return bt_pbap_pce_disconnect(&pbap_pce_app->pbap_pce, enforce);
}

static int cmd_pull_pb(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	char *name = argv[1];

	pbap_pce_app->tx_buf = bt_pbap_create_pdu(&pbap_pce_app->pbap_pce, &tx_pool);
	appl_param_count = 0U;
	if (!pbap_pce_app->tx_buf) {
		bt_shell_print("Fail to allocate tx buf");
		return -EINVAL;
	}

	err = bt_pbap_pce_pull_phonebook_create_cmd(&pbap_pce_app->pbap_pce, pbap_pce_app->tx_buf,
						    name, false);
	if (err) {
		bt_shell_print("Fail to create pull phonebook cmd %d", err);
		net_buf_unref(pbap_pce_app->tx_buf);
	}
	return err;
}

static int cmd_pull_vcardlisting(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	char *name = argv[1];

	pbap_pce_app->tx_buf = bt_pbap_create_pdu(&pbap_pce_app->pbap_pce, &tx_pool);
	appl_param_count = 0U;
	if (!pbap_pce_app->tx_buf) {
		bt_shell_print("Fail to allocate tx buf");
		return -EINVAL;
	}

	err = bt_pbap_pce_pull_vcardlisting_create_cmd(&pbap_pce_app->pbap_pce,
						       pbap_pce_app->tx_buf, name, false);
	if (err) {
		bt_shell_print("Fail to create pull vcardlisting cmd %d", err);
		net_buf_unref(pbap_pce_app->tx_buf);
	}
	return err;
}

static int cmd_pull_vcardentry(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	char *name = argv[1];

	pbap_pce_app->tx_buf = bt_pbap_create_pdu(&pbap_pce_app->pbap_pce, &tx_pool);
	appl_param_count = 0U;
	if (!pbap_pce_app->tx_buf) {
		bt_shell_print("Fail to allocate tx buf");
		return -EINVAL;
	}

	err = bt_pbap_pce_pull_vcardentry_create_cmd(&pbap_pce_app->pbap_pce, pbap_pce_app->tx_buf,
						     name, false);
	if (err) {
		bt_shell_print("Fail to create pull vcardlistentry cmd %d", err);
		net_buf_unref(pbap_pce_app->tx_buf);
	}
	return err;
}

static int cmd_set_path(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	char *name = argv[1];

	pbap_pce_app->tx_buf = bt_pbap_create_pdu(&pbap_pce_app->pbap_pce, &tx_pool);
	if (!pbap_pce_app->tx_buf) {
		bt_shell_print("Fail to allocate tx buf");
		return -EINVAL;
	}

	err = bt_pbap_pce_set_path(&pbap_pce_app->pbap_pce, pbap_pce_app->tx_buf, name);
	if (err) {
		bt_shell_print("Fail to send set path cmd %d", err);
		net_buf_unref(pbap_pce_app->tx_buf);
	}
	return err;
}

static int cmd_cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (appl_param_count > 0U) {
		err = bt_pbap_pce_add_app_param(pbap_pce_app->tx_buf, (size_t)appl_param_count,
						gpbap_appl_param);
		appl_param_count = 0U;
		if (err) {
			shell_error(sh, "Fail to add header app_param");
			net_buf_unref(pbap_pce_app->tx_buf);
			return err;
		}
	}

	err = bt_pbap_pce_send_cmd(&pbap_pce_app->pbap_pce, pbap_pce_app->tx_buf);
	if (err) {
		net_buf_unref(pbap_pce_app->tx_buf);
		bt_shell_print("Fail to send command %d", err);
	}
	return err;
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

static int cmd_add_appl_param(const struct shell *sh, size_t argc, char **argv)
{
	uint64_t value = 0;
	if (argc < 2) {
		shell_help(sh);
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (appl_param_count > PBAP_APPL_PARAM_MAX_COUNT) {
		shell_error(sh, "No space of TLV array, add app_param and clear tlvs");
		return -EAGAIN;
	}

	if (!pbap_pce_app->tx_buf) {
		bt_shell_print("No available tx buf");
		return -EINVAL;
	}

	uint8_t *tag = argv[0];

	if (!strcmp(tag, "ps")) {
		gpbap_appl_param[appl_param_count].type =
			BT_PBAP_APPL_PARAM_TAG_ID_PROPERTY_SELECTOR;
		gpbap_appl_param[appl_param_count].data_len = 8U;
		stringTonum_64(argv[1U], &value, 10);
		sys_put_be64(value, &gpbap_appl_param_data[appl_param_count][0]);
		gpbap_appl_param[appl_param_count].data =
			&gpbap_appl_param_data[appl_param_count][0];
	} else if (!strcmp(tag, "f")) {
		gpbap_appl_param[appl_param_count].type = BT_PBAP_APPL_PARAM_TAG_ID_FORMAT;
		gpbap_appl_param[appl_param_count].data_len = 1U;
		gpbap_appl_param_data[appl_param_count][0] = strtoul(argv[1U], NULL, 10);
		gpbap_appl_param[appl_param_count].data =
			&gpbap_appl_param_data[appl_param_count][0];
	} else if (!strcmp(tag, "mlc")) {
		gpbap_appl_param[appl_param_count].type = BT_PBAP_APPL_PARAM_TAG_ID_MAX_LIST_COUNT;
		gpbap_appl_param[appl_param_count].data_len = 2U;
		sys_put_be16((uint16_t)strtoul(argv[1U], NULL, 10),
			     &gpbap_appl_param_data[appl_param_count][0]);
		gpbap_appl_param[appl_param_count].data =
			&gpbap_appl_param_data[appl_param_count][0];
	} else if (!strcmp(tag, "lso")) {
		gpbap_appl_param[appl_param_count].type =
			BT_PBAP_APPL_PARAM_TAG_ID_LIST_START_OFFSET;
		gpbap_appl_param[appl_param_count].data_len = 2U;
		sys_put_be16((uint16_t)strtoul(argv[1U], NULL, 10),
			     &gpbap_appl_param_data[appl_param_count][0]);
		gpbap_appl_param[appl_param_count].data =
			&gpbap_appl_param_data[appl_param_count][0];
	} else if (!strcmp(tag, "rnmc")) {
		gpbap_appl_param[appl_param_count].type =
			BT_PBAP_APPL_PARAM_TAG_ID_RESET_NEW_MISSED_CALLS;
		gpbap_appl_param[appl_param_count].data_len = 1U;
		gpbap_appl_param_data[appl_param_count][0] = strtoul(argv[1U], NULL, 10);
		gpbap_appl_param[appl_param_count].data =
			&gpbap_appl_param_data[appl_param_count][0];
	} else if (!strcmp(tag, "vcs")) {
		gpbap_appl_param[appl_param_count].type = BT_PBAP_APPL_PARAM_TAG_ID_VCARD_SELECTOR;
		gpbap_appl_param[appl_param_count].data_len = 8U;
		stringTonum_64(argv[1U], &value, 10);
		sys_put_be64(value, &gpbap_appl_param_data[appl_param_count][0]);
		gpbap_appl_param[appl_param_count].data =
			&gpbap_appl_param_data[appl_param_count][0];
	} else if (!strcmp(tag, "vcso")) {
		gpbap_appl_param[appl_param_count].type =
			BT_PBAP_APPL_PARAM_TAG_ID_VCARD_SELECTOR_OPERATOR;
		gpbap_appl_param[appl_param_count].data_len = 1U;
		gpbap_appl_param_data[appl_param_count][0] = strtoul(argv[1U], NULL, 10);
		gpbap_appl_param[appl_param_count].data =
			&gpbap_appl_param_data[appl_param_count][0];
	} else if (!strcmp(tag, "o")) {
		gpbap_appl_param[appl_param_count].type = BT_PBAP_APPL_PARAM_TAG_ID_ORDER;
		gpbap_appl_param[appl_param_count].data_len = 1U;
		gpbap_appl_param_data[appl_param_count][0] = strtoul(argv[1U], NULL, 10);
		gpbap_appl_param[appl_param_count].data =
			&gpbap_appl_param_data[appl_param_count][0];
	} else if (!strcmp(tag, "sv")) {
		gpbap_appl_param[appl_param_count].type = BT_PBAP_APPL_PARAM_TAG_ID_SEARCH_VALUE;
		gpbap_appl_param[appl_param_count].data_len = strlen(argv[1]);
		memcpy(&gpbap_appl_param_data[appl_param_count][0], argv[1], strlen(argv[1]));
		gpbap_appl_param[appl_param_count].data =
			&gpbap_appl_param_data[appl_param_count][0];
	} else if (!strcmp(tag, "sp")) {
		gpbap_appl_param[appl_param_count].type = BT_PBAP_APPL_PARAM_TAG_ID_SEARCH_PROPERTY;
		gpbap_appl_param[appl_param_count].data_len = 1U;
		gpbap_appl_param_data[appl_param_count][0] = strtoul(argv[1U], NULL, 10);
		gpbap_appl_param[appl_param_count].data =
			&gpbap_appl_param_data[appl_param_count][0];
	} else {
		shell_error(sh, "No available appl param");
		return -EINVAL;
	}
	appl_param_count++;
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	pbap_add_appl_params,
	SHELL_CMD_ARG(ps, NULL, "PropertySelector : 8bytes", cmd_add_appl_param, 2, 0),
	SHELL_CMD_ARG(f, NULL, "Format : 1byte", cmd_add_appl_param, 2, 0),
	SHELL_CMD_ARG(mlc, NULL, "MaxListCount : 2bytes", cmd_add_appl_param, 2, 0),
	SHELL_CMD_ARG(lso, NULL, "ListStartOffset : 2bytes", cmd_add_appl_param, 2, 0),
	SHELL_CMD_ARG(rnmc, NULL, "ResetNewMissedCalls : 1byte", cmd_add_appl_param, 2, 0),
	SHELL_CMD_ARG(vcs, NULL, "vCardSelector : 8bytes", cmd_add_appl_param, 2, 0),
	SHELL_CMD_ARG(vcso, NULL, "vCardSelectorOperator : 1byte", cmd_add_appl_param, 2, 0),
	SHELL_CMD_ARG(o, NULL, "Order : 1byte", cmd_add_appl_param, 2, 0),
	SHELL_CMD_ARG(sv, NULL, "SearchValue : string", cmd_add_appl_param, 2, 0),
	SHELL_CMD_ARG(sp, NULL, "SearchProperty : 1byte", cmd_add_appl_param, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	pbap_cmds, SHELL_CMD_ARG(register, NULL, "", cmd_register, 1, 0),
	SHELL_CMD_ARG(connect_rfcomm, NULL, "<channel> <password(option)>", cmd_connect_rfcomm, 2,
		      1),
	SHELL_CMD_ARG(connect_l2cap, NULL, "<channel> <password(option)>", cmd_connect_l2cap, 2, 1),
	SHELL_CMD_ARG(disconnect, NULL, "<channel>", cmd_disconnect, 2, 0),
	SHELL_CMD_ARG(pull_pb_create, NULL, "<name>  <srmp>", cmd_pull_pb, 2, 0),
	SHELL_CMD_ARG(pull_vcardlisting_create, NULL, "<name>  <srmp>", cmd_pull_vcardlisting, 1,
		      2),
	SHELL_CMD_ARG(pull_vcardentry_create, NULL, "<name>  <srmp>", cmd_pull_vcardentry, 2, 1),
	SHELL_CMD_ARG(setpath, NULL, "<name>", cmd_set_path, 2, 0),
	SHELL_CMD_ARG(cmd_send, NULL, "<NULL>", cmd_cmd_send, 1, 0),
	SHELL_CMD_ARG(add_appl_param, &pbap_add_appl_params, "Adding appl params", cmd_common, 1,
		      0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(pbap, &pbap_cmds, "Bluetooth pbap shell commands", cmd_common, 1, 1);
