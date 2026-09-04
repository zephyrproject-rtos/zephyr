/*
 * Copyright 2026 NXP
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

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include <zephyr/shell/shell.h>

#include <host/shell/bt.h>
#include <common/bt_shell_private.h>
#include <common/bt_str.h>

struct bt_rfcomm_chan {
	struct bt_rfcomm_dlc dlc;
	struct k_fifo rx_fifo;
	bool hold_credit;
	bool used;
};

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN,
			  BT_RFCOMM_BUF_SIZE(CONFIG_BT_RFCOMM_L2CAP_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#define MAX_SPP_COUNT 10

static uint8_t credit_limit;
static bool hold_credit;
static struct bt_rfcomm_server server[MAX_SPP_COUNT];
static struct bt_rfcomm_chan chan[MAX_SPP_COUNT];

#define _SDP_ATTRS_DEFINE(index, sdp_attrs_name) \
static struct bt_sdp_attribute sdp_attrs_name##index[] = { \
	BT_SDP_NEW_SERVICE, \
	BT_SDP_LIST( \
		BT_SDP_ATTR_SVCLASS_ID_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS) \
		}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROTO_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) \
			}, \
			) \
		}, \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8), \
				&server[index].channel \
			}, \
			) \
		}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROFILE_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
				BT_SDP_ARRAY_16(0x0102) \
			}, \
			) \
		}, \
		) \
	), \
	BT_SDP_SERVICE_NAME("Serial Port"), \
}

#define SDP_ATTRS_DEFINE(index, ...) _SDP_ATTRS_DEFINE(index, ##__VA_ARGS__)

LISTIFY(MAX_SPP_COUNT, SDP_ATTRS_DEFINE, (;), rfcomm_attrs);

#define _SDP_REC_DEFINE(index, sdp_attrs_name) BT_SDP_RECORD(sdp_attrs_name##index)
#define SDP_REC_DEFINE(index, ...)             _SDP_REC_DEFINE(index, ##__VA_ARGS__)

static struct bt_sdp_record rfcomm_rec[MAX_SPP_COUNT] = {
	LISTIFY(MAX_SPP_COUNT, SDP_REC_DEFINE, (,), rfcomm_attrs),
};

static uint8_t discovered_server_channel[MAX_SPP_COUNT];
static uint8_t discovered_server_channel_count;

#define RFCOMM_SDP_DISCOVERING 1

static atomic_t flags;

static uint8_t rfcomm_sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
				      const struct bt_sdp_discover_params *params)
{
	int err;
	uint16_t param;

	if (result == NULL || result->resp_buf == NULL) {
		bt_shell_print("SDP discover done");
		atomic_clear_bit(&flags, RFCOMM_SDP_DISCOVERING);
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &param);
	if (err != 0) {
		bt_shell_error("Error getting Server channel (%d)", err);
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	bt_shell_print("Found rfcomm channel %u", param);
	if (discovered_server_channel_count < ARRAY_SIZE(discovered_server_channel)) {
		discovered_server_channel[discovered_server_channel_count] = param;
		discovered_server_channel_count++;
	}
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

#define RFCOMM_SDP_BUF_LEN 512U

NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, RFCOMM_SDP_BUF_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static void sdp_discover_work_handler(struct k_work *work)
{
	static struct bt_sdp_attribute_id_range id_range[] = {
		{ BT_SDP_ATTR_PROTO_DESC_LIST, BT_SDP_ATTR_PROTO_DESC_LIST },
		{ BT_SDP_ATTR_PROFILE_DESC_LIST, BT_SDP_ATTR_PROFILE_DESC_LIST },
	};
	static struct bt_sdp_attribute_id_list id_list = {
		.count = ARRAY_SIZE(id_range),
		.ranges = id_range,
	};
	static struct bt_sdp_discover_params sdp_param;
	static struct bt_uuid_16 uuid;
	int err;

	if (default_conn == NULL) {
		bt_shell_error("Invalid ACL conn");
		return;
	}

	if (!bt_conn_is_type(default_conn, BT_CONN_TYPE_BR)) {
		bt_shell_error("Invalid ACL conn type");
		return;
	}

	if (atomic_test_and_set_bit(&flags, RFCOMM_SDP_DISCOVERING)) {
		bt_shell_error("Discovery is ongoing");
		return;
	}

	discovered_server_channel_count = 0;
	memset(&discovered_server_channel, 0, sizeof(discovered_server_channel));

	uuid.uuid.type = BT_UUID_TYPE_16;
	uuid.val = BT_SDP_SERIAL_PORT_SVCLASS;

	sdp_param.func = rfcomm_sdp_discover_cb;
	sdp_param.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
	sdp_param.uuid = &uuid.uuid;
	sdp_param.pool = &sdp_pool;
	sdp_param.ids  = &id_list;

	err = bt_sdp_discover(default_conn, &sdp_param);
	if (err != 0) {
		bt_shell_error("SDP discovery failed (%d)", err);
		atomic_clear_bit(&flags, RFCOMM_SDP_DISCOVERING);
		return;
	}
}

static K_WORK_DEFINE(sdp_discover_work, sdp_discover_work_handler);

static void chan_connected(struct bt_rfcomm_dlc *dlc)
{
	struct bt_rfcomm_chan *c = CONTAINER_OF(dlc, struct bt_rfcomm_chan, dlc);
	bt_shell_print("DLC %zu connected", ARRAY_INDEX(chan, c));
}

static void chan_disconnected(struct bt_rfcomm_dlc *dlc)
{
	struct net_buf *buf;
	struct bt_rfcomm_chan *c = CONTAINER_OF(dlc, struct bt_rfcomm_chan, dlc);

	bt_shell_print("DLC %zu disconnected", ARRAY_INDEX(chan, c));

	buf = k_fifo_get(&c->rx_fifo, K_NO_WAIT);
	while (buf != NULL) {
		net_buf_unref(buf);
		buf = k_fifo_get(&c->rx_fifo, K_NO_WAIT);
	}
	c->used = false;
}

static int chan_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct bt_rfcomm_chan *c = CONTAINER_OF(dlc, struct bt_rfcomm_chan, dlc);

	bt_shell_print("DLC %zu RX len %u", ARRAY_INDEX(chan, c), buf->len);

	if (!c->hold_credit) {
		return 0;
	}

	k_fifo_put(&c->rx_fifo, buf);

	return -EINPROGRESS;
}

static struct bt_rfcomm_dlc_ops rfcomm_dlc_ops = {
	.connected = chan_connected,
	.disconnected = chan_disconnected,
	.recv = chan_recv,
};

static int accept(struct bt_conn *conn, struct bt_rfcomm_server *s, struct bt_rfcomm_dlc **dlc)
{
	struct bt_rfcomm_chan *c;

	if (ARRAY_INDEX(server, s) >= ARRAY_SIZE(chan)) {
		bt_shell_warn("No more rfcomm DLC");
		return -ENOMEM;
	}

	c = &chan[ARRAY_INDEX(server, s)];
	if (c->used) {
		bt_shell_warn("DLC %p is used", &c->dlc);
		return -EBUSY;
	}

	c->hold_credit = hold_credit;
	c->used = true;
	c->dlc.rx_credit_limit = credit_limit;
	c->dlc.ops = &rfcomm_dlc_ops;
	c->dlc.mtu = CONFIG_BT_RFCOMM_L2CAP_MTU;
	c->dlc.required_sec_level = BT_SECURITY_L2;
	k_fifo_init(&c->rx_fifo);
	*dlc = &c->dlc;
	return 0;
}

static int cmd_credit(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	unsigned long value = 0;

	value = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid value %s", argv[1]);
		return err;
	}
	if (value > UINT8_MAX) {
		shell_error(sh, "%s is out of range (0, 255)", argv[1]);
		return -EINVAL;
	}
	credit_limit = value;
	return 0;
}

static int cmd_hold_credit(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	bool value;

	value = shell_strtobool(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid value %s", argv[1]);
		return err;
	}
	hold_credit = value;
	return 0;
}

static int cmd_discover(const struct shell *sh, size_t argc, char *argv[])
{
	if (default_conn == NULL) {
		shell_error(sh, "Invalid ACL conn");
		return -ENOTCONN;
	}

	k_work_submit(&sdp_discover_work);
	return 0;
}

static int cmd_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	unsigned long start = 0;
	unsigned long end = MAX_SPP_COUNT - 1;

	if (argc > 1) {
		start = shell_strtoul(argv[1], 0, &err);
		if (err != 0) {
			shell_error(sh, "Invalid index %s", argv[1]);
			return err;
		}
		if (start >= MAX_SPP_COUNT) {
			shell_error(sh, "%s is out of index %u", argv[1], MAX_SPP_COUNT);
			return -EINVAL;
		}
		end = start;
	}

	for (unsigned long index = start; index <= end; index++) {
		if (server[index].channel != 0) {
			shell_error(sh, "Server already registered (%lu)", index);
			continue;
		}

		server[index].accept = accept;
		err = bt_rfcomm_server_register(&server[index]);
		if (err < 0) {
			shell_error(sh, "Failed to register server (%lu, %d)", index, err);
			continue;
		}

		err = bt_sdp_register_service(&rfcomm_rec[index]);
		if (err < 0) {
			bt_shell_error("Failed to register SDP record (%lu, %d)", index, err);
			bt_rfcomm_server_unregister(&server[index]);
			server[index].channel = 0;
			continue;
		}

		shell_print(sh, "Registered rfcomm channel %u", server[index].channel);
	}
	return 0;
}

static int cmd_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	unsigned long start = 0;
	unsigned long end = MAX_SPP_COUNT - 1;

	if (default_conn == NULL) {
		shell_error(sh, "No valid ACL conn");
		return -ENOEXEC;
	}

	if (argc > 1) {
		start = shell_strtoul(argv[1], 0, &err);
		if (err != 0) {
			shell_error(sh, "Invalid index %s", argv[1]);
			return err;
		}
		if (start >= MAX_SPP_COUNT) {
			shell_error(sh, "%s is out of index %u", argv[1], MAX_SPP_COUNT);
			return -EINVAL;
		}
		end = start;
	}

	if (discovered_server_channel_count == 0) {
		shell_error(sh, "No valid rfcomm channel, please use `rfcomm discover` to discover "
			        "supported rfcomm channel of peer device");
		return -EINVAL;
	}

	for (unsigned long index = start; index <= end; index++) {
		uint8_t channel;

		if (index >= discovered_server_channel_count ||
		    discovered_server_channel[index] == 0) {
			shell_error(sh, "Channel not discovered for index %lu", index);
			continue;
		}

		if (chan[index].used) {
			shell_error(sh, "Channel index %lu is used", index);
			continue;
		}

		channel = discovered_server_channel[index];
		chan[index].hold_credit = hold_credit;
		chan[index].used = true;
		chan[index].dlc.rx_credit_limit = credit_limit;
		chan[index].dlc.ops = &rfcomm_dlc_ops;
		chan[index].dlc.mtu = CONFIG_BT_RFCOMM_L2CAP_MTU;
		chan[index].dlc.required_sec_level = BT_SECURITY_L2;
		k_fifo_init(&chan[index].rx_fifo);

		err = bt_rfcomm_dlc_connect(default_conn, &chan[index].dlc, channel);
		if (err != 0) {
			shell_error(sh, "Failed to create DLC conn %u (%lu, %d)", channel, index,
				    err);
			chan[index].used = false;
			continue;
		}
	}

	return 0;
}

static int cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	unsigned long index = 0;
	unsigned long len = CONFIG_BT_RFCOMM_L2CAP_MTU;
	struct net_buf *buf;
	uint8_t *data;

	index = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid index %s", argv[1]);
		return err;
	}
	if (index >= MAX_SPP_COUNT) {
		shell_error(sh, "%s is out of index %u", argv[1], MAX_SPP_COUNT);
		return -EINVAL;
	}

	if (argc > 2) {
		len = shell_strtoul(argv[2], 0, &err);
		if (err != 0) {
			shell_error(sh, "Invalid len %s", argv[2]);
			return -EINVAL;
		}
		if (len > CONFIG_BT_RFCOMM_L2CAP_MTU) {
			shell_error(sh, "%s > %u", argv[1], CONFIG_BT_RFCOMM_L2CAP_MTU);
			return -EINVAL;
		}
	}

	if (!chan[index].used) {
		shell_error(sh, "Channel[%lu] is not used", index);
		return -ENOTCONN;
	}

	buf = bt_rfcomm_create_pdu(&tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Failed to allocate buffer");
		return -ENOEXEC;
	}

	len = MIN(net_buf_tailroom(buf), len);
	data = net_buf_add(buf, len);
	memset(data, 0xff, len);

	err = bt_rfcomm_dlc_send(&chan[index].dlc, buf);
	if (err != 0) {
		shell_error(sh, "Failed to send data %lu (%d)", len, err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	unsigned long start = 0;
	unsigned long end = MAX_SPP_COUNT - 1;

	if (default_conn == NULL) {
		shell_error(sh, "No valid ACL conn");
		return -ENOEXEC;
	}

	if (argc > 1) {
		start = shell_strtoul(argv[1], 0, &err);
		if (err != 0) {
			shell_error(sh, "Invalid index %s", argv[1]);
			return err;
		}
		if (start >= MAX_SPP_COUNT) {
			shell_error(sh, "%s is out of index %u", argv[1], MAX_SPP_COUNT);
			return -EINVAL;
		}
		end = start;
	}

	for (unsigned long index = start; index <= end; index++) {
		if (!chan[index].used) {
			shell_error(sh, "Channel[%lu] is not used", index);
			continue;
		}

		err = bt_rfcomm_dlc_disconnect(&chan[index].dlc);
		if (err != 0) {
			shell_error(sh, "Failed to disconn rfcomm channel (%lu, %d)", index, err);
			continue;
		}
	}

	return 0;
}

static int cmd_rls(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	unsigned long index = 0;
	uint8_t line_status = BT_RFCOMM_RLS_NO_ERR;

	index = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid index %s", argv[1]);
		return err;
	}
	if (index >= MAX_SPP_COUNT) {
		shell_error(sh, "%s is out of index %u", argv[1], MAX_SPP_COUNT);
		return -EINVAL;
	}

	if (argc > 2) {
		if (strcmp(argv[2], "overrun") == 0) {
			line_status = BT_RFCOMM_RLS_ERR(BT_RFCOMM_RLS_ERR_OVERRUN_ERROR);
		} else if (strcmp(argv[2], "parity") == 0) {
			line_status = BT_RFCOMM_RLS_ERR(BT_RFCOMM_RLS_ERR_PARITY_ERROR);
		} else if (strcmp(argv[2], "framing") == 0) {
			line_status = BT_RFCOMM_RLS_ERR(BT_RFCOMM_RLS_ERR_FRAMING_ERROR);
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	if (!chan[index].used) {
		shell_error(sh, "Channel[%lu] is not used", index);
		return -ENOTCONN;
	}

	err = bt_rfcomm_send_rls_cmd(&chan[index].dlc, line_status);
	if (err != 0) {
		shell_error(sh, "Failed to send RLS (%lu, %d)", index, err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_recv_complete(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	unsigned long start = 0;
	unsigned long end = MAX_SPP_COUNT - 1;
	struct net_buf *buf;

	if (default_conn == NULL) {
		shell_error(sh, "No valid ACL conn");
		return -ENOEXEC;
	}

	if (argc > 1) {
		start = shell_strtoul(argv[1], 0, &err);
		if (err != 0) {
			shell_error(sh, "Invalid index %s", argv[1]);
			return err;
		}
		if (start >= MAX_SPP_COUNT) {
			shell_error(sh, "%s is out of index %u", argv[1], MAX_SPP_COUNT);
			return -EINVAL;
		}
		end = start;
	}

	for (unsigned long index = start; index <= end; index++) {
		if (!chan[index].used) {
			shell_error(sh, "Channel[%lu] is not used", index);
			continue;
		}

		buf = k_fifo_get(&chan[index].rx_fifo, K_NO_WAIT);
		if (buf == NULL) {
			shell_error(sh, "No inprogress buffer");
			continue;
		}

		err = bt_rfcomm_dlc_recv_complete(&chan[index].dlc, buf);
		if (err != 0) {
			shell_error(sh, "Failed to call recv_complete (%lu, %d)", index, err);
			continue;
		}
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	spp_cmds,
	SHELL_CMD_ARG(credit, NULL, "<limited credit value>", cmd_credit, 2, 0),
	SHELL_CMD_ARG(hold_credit, NULL, "<on/off>", cmd_hold_credit, 2, 0),
	SHELL_CMD_ARG(discover, NULL, "", cmd_discover, 1, 0),
	SHELL_CMD_ARG(register, NULL, "[index]", cmd_register, 1, 1),
	SHELL_CMD_ARG(connect, NULL, "[index]", cmd_connect, 1, 1),
	SHELL_CMD_ARG(disconnect, NULL, "[index]", cmd_disconnect, 1, 1),
	SHELL_CMD_ARG(send, NULL, "<index> [length of data]", cmd_send, 2, 1),
	SHELL_CMD_ARG(rls, NULL, "<index> [overrun|parity|framing]", cmd_rls, 2, 1),
	SHELL_CMD_ARG(recv_complete, NULL, "[index]", cmd_recv_complete, 1, 1),
	SHELL_SUBCMD_SET_END
);

static int cmd_rfcomm(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "Unknown argument '%s'", argv[1]);
	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(rfcomm, &spp_cmds, "Bluetooth RFCOMM sh commands", cmd_rfcomm, 1, 1);
