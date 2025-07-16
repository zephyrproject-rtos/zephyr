/* smp_br_general.c - Bluetooth classic SMP general smoke test */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/l2cap.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define DATA_BREDR_MTU          48
#define SDP_CLIENT_USER_BUF_LEN 4096

NET_BUF_POOL_FIXED_DEFINE(data_tx_pool, 1, BT_L2CAP_SDU_BUF_SIZE(DATA_BREDR_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
NET_BUF_POOL_FIXED_DEFINE(data_rx_pool, 1, DATA_BREDR_MTU, 8, NULL);
NET_BUF_POOL_FIXED_DEFINE(sdp_client_pool, 1, SDP_CLIENT_USER_BUF_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_sdp_discover_params sdp_discover;
static union {
	struct bt_uuid_16 u16;
	struct bt_uuid_32 u32;
	struct bt_uuid_128 u128;
} sdp_discover_uuid;

struct l2cap_br_chan {
	bool active;
	struct bt_l2cap_br_chan chan;
};

const uint16_t svclass_list[] = {
	BT_SDP_SDP_SERVER_SVCLASS,
	BT_SDP_BROWSE_GRP_DESC_SVCLASS,
	BT_SDP_PUBLIC_BROWSE_GROUP,
	BT_SDP_SERIAL_PORT_SVCLASS,
	BT_SDP_LAN_ACCESS_SVCLASS,
	BT_SDP_DIALUP_NET_SVCLASS,
	BT_SDP_IRMC_SYNC_SVCLASS,
	BT_SDP_OBEX_OBJPUSH_SVCLASS,
	BT_SDP_OBEX_FILETRANS_SVCLASS,
	BT_SDP_IRMC_SYNC_CMD_SVCLASS,
	BT_SDP_HEADSET_SVCLASS,
	BT_SDP_CORDLESS_TELEPHONY_SVCLASS,
	BT_SDP_AUDIO_SOURCE_SVCLASS,
	BT_SDP_AUDIO_SINK_SVCLASS,
	BT_SDP_AV_REMOTE_TARGET_SVCLASS,
	BT_SDP_ADVANCED_AUDIO_SVCLASS,
	BT_SDP_AV_REMOTE_SVCLASS,
	BT_SDP_AV_REMOTE_CONTROLLER_SVCLASS,
	BT_SDP_INTERCOM_SVCLASS,
	BT_SDP_FAX_SVCLASS,
	BT_SDP_HEADSET_AGW_SVCLASS,
	BT_SDP_WAP_SVCLASS,
	BT_SDP_WAP_CLIENT_SVCLASS,
	BT_SDP_PANU_SVCLASS,
	BT_SDP_NAP_SVCLASS,
	BT_SDP_GN_SVCLASS,
	BT_SDP_DIRECT_PRINTING_SVCLASS,
	BT_SDP_REFERENCE_PRINTING_SVCLASS,
	BT_SDP_IMAGING_SVCLASS,
	BT_SDP_IMAGING_RESPONDER_SVCLASS,
	BT_SDP_IMAGING_ARCHIVE_SVCLASS,
	BT_SDP_IMAGING_REFOBJS_SVCLASS,
	BT_SDP_HANDSFREE_SVCLASS,
	BT_SDP_HANDSFREE_AGW_SVCLASS,
	BT_SDP_DIRECT_PRT_REFOBJS_SVCLASS,
	BT_SDP_REFLECTED_UI_SVCLASS,
	BT_SDP_BASIC_PRINTING_SVCLASS,
	BT_SDP_PRINTING_STATUS_SVCLASS,
	BT_SDP_HID_SVCLASS,
	BT_SDP_HCR_SVCLASS,
	BT_SDP_HCR_PRINT_SVCLASS,
	BT_SDP_HCR_SCAN_SVCLASS,
	BT_SDP_CIP_SVCLASS,
	BT_SDP_VIDEO_CONF_GW_SVCLASS,
	BT_SDP_UDI_MT_SVCLASS,
	BT_SDP_UDI_TA_SVCLASS,
	BT_SDP_AV_SVCLASS,
	BT_SDP_SAP_SVCLASS,
	BT_SDP_PBAP_PCE_SVCLASS,
	BT_SDP_PBAP_PSE_SVCLASS,
	BT_SDP_PBAP_SVCLASS,
	BT_SDP_MAP_MSE_SVCLASS,
	BT_SDP_MAP_MCE_SVCLASS,
	BT_SDP_MAP_SVCLASS,
	BT_SDP_GNSS_SVCLASS,
	BT_SDP_GNSS_SERVER_SVCLASS,
	BT_SDP_MPS_SC_SVCLASS,
	BT_SDP_MPS_SVCLASS,
	BT_SDP_PNP_INFO_SVCLASS,
	BT_SDP_GENERIC_NETWORKING_SVCLASS,
	BT_SDP_GENERIC_FILETRANS_SVCLASS,
	BT_SDP_GENERIC_AUDIO_SVCLASS,
	BT_SDP_GENERIC_TELEPHONY_SVCLASS,
	BT_SDP_UPNP_SVCLASS,
	BT_SDP_UPNP_IP_SVCLASS,
	BT_SDP_UPNP_PAN_SVCLASS,
	BT_SDP_UPNP_LAP_SVCLASS,
	BT_SDP_UPNP_L2CAP_SVCLASS,
	BT_SDP_VIDEO_SOURCE_SVCLASS,
	BT_SDP_VIDEO_SINK_SVCLASS,
	BT_SDP_VIDEO_DISTRIBUTION_SVCLASS,
	BT_SDP_HDP_SVCLASS,
	BT_SDP_HDP_SOURCE_SVCLASS,
	BT_SDP_HDP_SINK_SVCLASS,
	BT_SDP_GENERIC_ACCESS_SVCLASS,
	BT_SDP_GENERIC_ATTRIB_SVCLASS,
	BT_SDP_APPLE_AGENT_SVCLASS,
};

static bool sdp_record_found;

#define APPL_L2CAP_CONNECTION_MAX_COUNT 3
static struct l2cap_br_chan l2cap_chans[APPL_L2CAP_CONNECTION_MAX_COUNT];
static struct bt_l2cap_server l2cap_servers[APPL_L2CAP_CONNECTION_MAX_COUNT];

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan), struct l2cap_br_chan, chan);

	bt_shell_print("Incoming data channel %d len %u", ARRAY_INDEX(l2cap_chans, br_chan),
		       buf->len);

	if (buf->len) {
		bt_shell_hexdump(buf->data, buf->len);
	}

	return 0;
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan), struct l2cap_br_chan, chan);

	bt_shell_print("Channel %d connected", ARRAY_INDEX(l2cap_chans, br_chan));
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan), struct l2cap_br_chan, chan);

	br_chan->active = false;
	bt_shell_print("Channel %d disconnected", ARRAY_INDEX(l2cap_chans, br_chan));
}

static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan)
{
	bt_shell_print("Channel %p requires buffer", chan);

	return net_buf_alloc(&data_rx_pool, K_NO_WAIT);
}

static const struct bt_l2cap_chan_ops l2cap_ops = {
	.alloc_buf = l2cap_alloc_buf,
	.recv = l2cap_recv,
	.connected = l2cap_connected,
	.disconnected = l2cap_disconnected,
};

static struct l2cap_br_chan *l2cap_alloc_chan(void)
{
	ARRAY_FOR_EACH(l2cap_chans, index) {
		if (l2cap_chans[index].active == false) {
			l2cap_chans[index].active = true;
			l2cap_chans[index].chan.chan.ops = &l2cap_ops;
			l2cap_chans[index].chan.rx.mtu = DATA_BREDR_MTU;
			return &l2cap_chans[index];
		}
	}
	return NULL;
}

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			struct bt_l2cap_chan **chan)
{
	struct l2cap_br_chan *br_chan;

	bt_shell_print("Incoming BR/EDR conn %p", conn);

	br_chan = l2cap_alloc_chan();
	if (br_chan == NULL) {
		bt_shell_error("No channels available");
		return -ENOMEM;
	}

	*chan = &br_chan->chan.chan;

	return 0;
}

static struct bt_l2cap_server *l2cap_alloc_server(uint16_t psm)
{
	ARRAY_FOR_EACH(l2cap_servers, index) {
		if (l2cap_servers[index].psm == 0) {
			l2cap_servers[index].psm = psm;
			l2cap_servers[index].accept = l2cap_accept;
			return &l2cap_servers[index];
		}
	}
	return NULL;
}

static int cmd_l2cap_register(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t psm = strtoul(argv[1], NULL, 16);
	struct bt_l2cap_server *br_server;

	ARRAY_FOR_EACH(l2cap_servers, index) {
		if (l2cap_servers[index].psm == psm) {
			shell_print(sh, "Already registered");
			return -ENOEXEC;
		}
	}

	br_server = l2cap_alloc_server(psm);
	if (br_server == NULL) {
		shell_error(sh, "No servers available");
		return -ENOMEM;
	}

	if ((argc > 3) && (strcmp(argv[2], "sec") == 0)) {
		br_server->sec_level = strtoul(argv[3], NULL, 16);
	} else {
		br_server->sec_level = BT_SECURITY_L1;
	}

	if (bt_l2cap_br_server_register(br_server) < 0) {
		br_server->psm = 0U;
		shell_error(sh, "Unable to register psm");
		return -ENOEXEC;
	}

	shell_print(sh, "L2CAP psm %u registered", br_server->psm);

	return 0;
}

static int cmd_l2cap_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_conn_info info;
	struct l2cap_br_chan *br_chan;
	uint16_t psm;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	br_chan = l2cap_alloc_chan();
	if (br_chan == NULL) {
		shell_error(sh, "No channels available");
		br_chan->active = false;
		return -ENOMEM;
	}

	err = bt_conn_get_info(default_conn, &info);
	if ((err < 0) || (info.type != BT_CONN_TYPE_BR)) {
		shell_error(sh, "Invalid conn type");
		br_chan->active = false;
		return -ENOEXEC;
	}

	psm = strtoul(argv[1], NULL, 16);

	if ((argc > 3) && (strcmp(argv[2], "sec") == 0)) {
		br_chan->chan.required_sec_level = strtoul(argv[3], NULL, 16);
	} else {
		br_chan->chan.required_sec_level = BT_SECURITY_L1;
	}

	err = bt_l2cap_chan_connect(default_conn, &br_chan->chan.chan, psm);
	if (err < 0) {
		br_chan->active = false;
		shell_error(sh, "Unable to connect to psm %u (err %d)", psm, err);
	} else {
		shell_print(sh, "L2CAP connection pending");
	}

	return err;
}

static int cmd_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t id;

	id = strtoul(argv[1], NULL, 16);
	if ((id >= ARRAY_SIZE(l2cap_chans)) || (!l2cap_chans[id].active)) {
		shell_print(sh, "channel %d not connected", id);
		return -ENOEXEC;
	}

	err = bt_l2cap_chan_disconnect(&l2cap_chans[id].chan.chan);
	if (err) {
		shell_error(sh, "Unable to disconnect: %u", -err);
		return err;
	}

	return 0;
}

static int cmd_l2cap_send(const struct shell *sh, size_t argc, char *argv[])
{
	static uint8_t buf_data[DATA_BREDR_MTU];
	int err, len = DATA_BREDR_MTU, count = 1;
	uint8_t id;
	struct net_buf *buf;

	id = strtoul(argv[1], NULL, 16);
	if ((id >= ARRAY_SIZE(l2cap_chans)) || (!l2cap_chans[id].active)) {
		shell_print(sh, "channel %d not connected", id);
		return -ENOEXEC;
	}

	if (argc > 2) {
		count = strtoul(argv[2], NULL, 10);
	}

	if (argc > 3) {
		len = strtoul(argv[3], NULL, 10);
		if (len > DATA_BREDR_MTU) {
			shell_error(sh, "Length exceeds TX MTU for the channel");
			return -ENOEXEC;
		}
	}

	len = MIN(l2cap_chans[id].chan.tx.mtu, len);
	while (count--) {
		shell_print(sh, "Rem %d", count);
		buf = net_buf_alloc(&data_tx_pool, K_SECONDS(2));
		if (!buf) {
			if (l2cap_chans[id].chan.state != BT_L2CAP_CONNECTED) {
				shell_error(sh, "Channel disconnected, stopping TX");

				return -EAGAIN;
			}
			shell_error(sh, "Allocation timeout, stopping TX");

			return -EAGAIN;
		}
		net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
		memset(buf_data, count, sizeof(buf_data));

		net_buf_add_mem(buf, buf_data, len);
		err = bt_l2cap_chan_send(&l2cap_chans[id].chan.chan, buf);
		if (err < 0) {
			shell_error(sh, "Unable to send: %d", -err);
			net_buf_unref(buf);
			return -ENOEXEC;
		}
	}

	return 0;
}

static int cmd_set_security(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t psm = strtoul(argv[1], NULL, 16);
	uint8_t sec = strtoul(argv[2], NULL, 16);

	if (sec > BT_SECURITY_L4) {
		shell_error(sh, "Invalid security level: %d", sec);
		return -ENOEXEC;
	}

	ARRAY_FOR_EACH(l2cap_servers, index) {
		if (l2cap_servers[index].psm == psm) {
			l2cap_servers[index].sec_level = sec;
			shell_print(sh, "L2CAP psm %u security level %u", psm, sec);
			return 0;
		}
	}

	shell_error(sh, "L2CAP psm %u not registered", psm);
	return -ENOEXEC;
}

static int cmd_reboot(const struct shell *sh, size_t argc, char *argv[])
{
	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}

static uint8_t sdp_discover_func(struct bt_conn *conn, struct bt_sdp_client_result *result,
				 const struct bt_sdp_discover_params *params)
{
	int err;
	uint16_t param;

	if ((result == NULL) || (result->resp_buf == NULL) || (result->resp_buf->len == 0)) {
		if (sdp_record_found) {
			sdp_record_found = false;
			printk("SDP Discovery Done\n");
		} else {
			printk("No SDP Record\n");
		}
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	sdp_record_found = true;

	printk("SDP Rsp Data:\n");
	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_L2CAP, &param);
	if (!err) {
		printk("    PROTOCOL: L2CAP: %d\n", param);
	}
	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &param);
	if (!err) {
		printk("    PROTOCOL: RFCOMM: %d\n", param);
	}
	for (size_t i = 0; i < ARRAY_SIZE(svclass_list); i++) {
		err = bt_sdp_get_profile_version(result->resp_buf, svclass_list[i], &param);
		if (!err) {
			printk("    VERSION: %04X: %d\n", svclass_list[i], param);
		}
	}
	err = bt_sdp_get_features(result->resp_buf, &param);
	if (!err) {
		printk("    FEATURE: %04X\n", param);
	}
	printk("    RAW:");
	for (uint16_t i = 0; i < result->resp_buf->len; i++) {
		printk("%02X", result->resp_buf->data[i]);
	}
	printk("\n");

	if (!result->next_record_hint) {
		sdp_record_found = false;
		printk("SDP Discovery Done\n");
	}

	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static int cmd_ssa_discovery(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	size_t len;

	len = strlen(argv[1]);

	if (len == (BT_UUID_SIZE_16 * 2)) {
		uint16_t val;

		sdp_discover_uuid.u16.uuid.type = BT_UUID_TYPE_16;
		hex2bin(argv[1], len, (uint8_t *)&val, sizeof(val));
		sdp_discover_uuid.u16.val = sys_be16_to_cpu(val);
		sdp_discover.uuid = &sdp_discover_uuid.u16.uuid;
	} else if (len == (BT_UUID_SIZE_32 * 2)) {
		uint32_t val;

		sdp_discover_uuid.u32.uuid.type = BT_UUID_TYPE_32;
		hex2bin(argv[1], len, (uint8_t *)&val, sizeof(val));
		sdp_discover_uuid.u32.val = sys_be32_to_cpu(val);
		sdp_discover.uuid = &sdp_discover_uuid.u32.uuid;
	} else if (len == (BT_UUID_SIZE_128 * 2)) {
		sdp_discover_uuid.u128.uuid.type = BT_UUID_TYPE_128;
		hex2bin(argv[1], len, &sdp_discover_uuid.u128.val[0],
			sizeof(sdp_discover_uuid.u128.val));
		sdp_discover.uuid = &sdp_discover_uuid.u128.uuid;
	} else {
		shell_error(sh, "Invalid UUID");
		return -ENOEXEC;
	}

	sdp_discover.func = sdp_discover_func;
	sdp_discover.pool = &sdp_client_pool;
	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;

	err = bt_sdp_discover(default_conn, &sdp_discover);
	if (err) {
		shell_error(sh, "Fail to start SDP Discovery (err %d)", err);
		return err;
	}
	return 0;
}

static int cmd_get_security_info(const struct shell *sh, size_t argc, char *argv[])
{
	char addr_str[BT_ADDR_STR_LEN];
	struct bt_conn_info info;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}
	err = bt_conn_get_info(default_conn, &info);
	if (err) {
		shell_print(sh, "Failed to get info");
		return -ENOEXEC;
	}

	bt_addr_to_str(info.br.dst, addr_str, sizeof(addr_str));
	shell_print(sh, "Peer address %s", addr_str);
	shell_print(sh, "Encryption key size: %d", info.security.enc_key_size);
	shell_print(sh, "Security level: %d", info.security.level);

	return 0;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	l2cap_br_cmds,
	SHELL_CMD_ARG(register, NULL, "<psm> [sec] [sec: 0 - 4]", cmd_l2cap_register, 2, 2),
	SHELL_CMD_ARG(connect, NULL, "<psm> [sec] [sec: 0 - 4]", cmd_l2cap_connect, 2, 2),
	SHELL_CMD_ARG(disconnect, NULL, "<id>", cmd_l2cap_disconnect, 2, 0),
	SHELL_CMD_ARG(send, NULL, "<id> [number of packets] [length of packet(s)]", cmd_l2cap_send,
		      2, 2),
	SHELL_CMD_ARG(security, NULL, "<psm> <security level: 0 - 4>", cmd_set_security, 3, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sdp_client_cmds,
	SHELL_CMD_ARG(ssa_discovery, NULL, "<UUID>", cmd_ssa_discovery, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	test_smp_cmds,
	SHELL_CMD_ARG(reboot, NULL, HELP_NONE, cmd_reboot, 1, 0),
	SHELL_CMD_ARG(security_info, NULL, HELP_NONE, cmd_get_security_info, 1, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_default_handler(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(l2cap_br, &l2cap_br_cmds, "Bluetooth classic l2cap shell commands",
		   cmd_default_handler);

SHELL_CMD_REGISTER(sdp_client, &sdp_client_cmds, "Bluetooth classic SDP client shell commands",
		   cmd_default_handler);

SHELL_CMD_REGISTER(test_smp, &test_smp_cmds, "Bluetooth classic SMP shell commands",
		   cmd_default_handler);
