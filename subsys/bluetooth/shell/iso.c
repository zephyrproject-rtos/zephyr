/** @file
 *  @brief Bluetooth Audio shell
 *
 */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <ctype.h>
#include <zephyr.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/iso.h>

#include "bt.h"

#define DATA_MTU CONFIG_BT_ISO_TX_MTU

static void iso_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		struct net_buf *buf)
{
	printk("Incoming data channel %p len %u, seq: %d, ts: %d\n", chan, buf->len,
			info->sn, info->ts);
}

static void iso_connected(struct bt_iso_chan *chan)
{
	printk("ISO Channel %p connected\n", chan);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	printk("ISO Channel %p disconnected with reason 0x%02x\n", chan, reason);
}

static struct bt_iso_chan_ops iso_ops = {
	.recv		= iso_recv,
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
};

#define DEFAULT_IO_QOS \
{ \
	.interval	= 10000u, \
	.latency	= 10u, \
	.sdu		= 40u, \
	.phy		= BT_GAP_LE_PHY_2M, \
	.rtn		= 2u, \
}

static struct bt_iso_chan_io_qos iso_tx_qos = DEFAULT_IO_QOS;
static struct bt_iso_chan_io_qos iso_rx_qos = DEFAULT_IO_QOS;

static struct bt_iso_chan_qos iso_qos = {
	.sca		= BT_GAP_SCA_UNKNOWN,
	.tx		= &iso_tx_qos,
	.rx		= &iso_rx_qos,
};

struct bt_iso_chan iso_chan = {
	.ops = &iso_ops,
	.qos = &iso_qos,
};

#if defined(CONFIG_BT_ISO_UNICAST)
NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, DATA_MTU, NULL);

static int iso_accept(struct bt_conn *conn, struct bt_iso_chan **chan)
{
	printk("Incoming conn %p\n", conn);

	if (iso_chan.conn) {
		printk("No channels available\n");
		return -ENOMEM;
	}

	*chan = &iso_chan;

	return 0;
}

struct bt_iso_server iso_server = {
	.sec_level = BT_SECURITY_L1,
	.accept = iso_accept,
};

static int cmd_listen(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	static struct bt_iso_chan_io_qos *tx_qos, *rx_qos;

	if (!strcmp("tx", argv[1])) {
		tx_qos = &iso_tx_qos;
		rx_qos = NULL;
	} else if (!strcmp("rx", argv[1])) {
		tx_qos = NULL;
		rx_qos = &iso_rx_qos;
	} else if (!strcmp("txrx", argv[1])) {
		tx_qos = &iso_tx_qos;
		rx_qos = &iso_rx_qos;
	} else {
		shell_error(shell, "Invalid argument - use tx, rx or txrx");
		return -ENOEXEC;
	}

	if (argc > 2) {
		iso_server.sec_level = *argv[2] - '0';
	}

	err = bt_iso_server_register(&iso_server);
	if (err) {
		shell_error(shell, "Unable to register ISO cap (err %d)",
			    err);
		return err;
	}

	/* Setup peripheral iso data direction only if register is success */
	iso_chan.qos->tx = tx_qos;
	iso_chan.qos->rx = rx_qos;
	return err;
}

static int cmd_bind(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conns[1];
	struct bt_iso_chan *chans[1];
	int err;

	if (!default_conn) {
		shell_error(shell, "Not connected");
		return 0;
	}

	if (iso_chan.conn) {
		shell_error(shell, "Already bound");
		return 0;
	}

	conns[0] = default_conn;
	chans[0] = &iso_chan;

	if (argc > 1) {
		if (!strcmp("tx", argv[1])) {
			chans[0]->qos->tx = &iso_tx_qos;
			chans[0]->qos->rx = NULL;
		} else if (!strcmp("rx", argv[1])) {
			chans[0]->qos->tx = NULL;
			chans[0]->qos->rx = &iso_rx_qos;
		} else if (!strcmp("txrx", argv[1])) {
			chans[0]->qos->tx = &iso_tx_qos;
			chans[0]->qos->rx = &iso_rx_qos;
		}
	}

	if (argc > 2) {
		if (chans[0]->qos->tx) {
			chans[0]->qos->tx->interval = strtol(argv[2], NULL, 0);
		}

		if (chans[0]->qos->rx) {
			chans[0]->qos->rx->interval = strtol(argv[2], NULL, 0);
		}
	}

	if (argc > 3) {
		chans[0]->qos->packing = strtol(argv[3], NULL, 0);
	}

	if (argc > 4) {
		chans[0]->qos->framing = strtol(argv[4], NULL, 0);
	}

	if (argc > 5) {
		if (chans[0]->qos->tx) {
			chans[0]->qos->tx->latency = strtol(argv[5], NULL, 0);
		}

		if (chans[0]->qos->rx) {
			chans[0]->qos->rx->latency = strtol(argv[5], NULL, 0);
		}
	}

	if (argc > 6) {
		if (chans[0]->qos->tx) {
			chans[0]->qos->tx->sdu = strtol(argv[6], NULL, 0);
		}

		if (chans[0]->qos->rx) {
			chans[0]->qos->rx->sdu = strtol(argv[6], NULL, 0);
		}
	}

	if (argc > 7) {
		if (chans[0]->qos->tx) {
			chans[0]->qos->tx->phy = strtol(argv[7], NULL, 0);
		}

		if (chans[0]->qos->rx) {
			chans[0]->qos->rx->phy = strtol(argv[7], NULL, 0);
		}
	}

	if (argc > 8) {
		if (chans[0]->qos->tx) {
			chans[0]->qos->tx->rtn = strtol(argv[8], NULL, 0);
		}

		if (chans[0]->qos->rx) {
			chans[0]->qos->rx->rtn = strtol(argv[8], NULL, 0);
		}
	}

	err = bt_iso_chan_bind(conns, 1, chans);
	if (err) {
		shell_error(shell, "Unable to bind (err %d)", err);
		return 0;
	}

	shell_print(shell, "ISO Channel bound");

	return 0;
}

static int cmd_connect(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_iso_chan *chans[1];
	int err;

	if (!iso_chan.conn) {
		shell_error(shell, "Not bound");
		return 0;
	}

	chans[0] = &iso_chan;

	err = bt_iso_chan_connect(chans, 1);
	if (err) {
		shell_error(shell, "Unable to connect (err %d)", err);
		return 0;
	}

	shell_print(shell, "ISO Connect pending...");

	return 0;
}


static int cmd_send(const struct shell *shell, size_t argc, char *argv[])
{
	static uint8_t buf_data[DATA_MTU] = { [0 ... (DATA_MTU - 1)] = 0xff };
	int ret, len, count = 1;
	struct net_buf *buf;

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
	}

	if (!iso_chan.conn) {
		shell_error(shell, "Not bound");
		return 0;
	}

	if (!iso_chan.qos->tx) {
		shell_error(shell, "Transmission QoS disabled");
		return -ENOEXEC;
	}

	len = MIN(iso_chan.qos->tx->sdu, DATA_MTU - BT_ISO_CHAN_SEND_RESERVE);

	while (count--) {
		buf = net_buf_alloc(&tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

		net_buf_add_mem(buf, buf_data, len);
		shell_info(shell, "send: %d bytes of data", len);
		ret = bt_iso_chan_send(&iso_chan, buf);
		if (ret < 0) {
			shell_print(shell, "Unable to send: %d", -ret);
			net_buf_unref(buf);
			return -ENOEXEC;
		}
	}

	shell_print(shell, "ISO sending...");

	return 0;
}

static int cmd_disconnect(const struct shell *shell, size_t argc,
			      char *argv[])
{
	int err;

	err = bt_iso_chan_disconnect(&iso_chan);
	if (err) {
		shell_error(shell, "Unable to disconnect (err %d)", err);
		return 0;
	}

	shell_print(shell, "ISO Disconnect pending...");

	return 0;
}
#endif /* CONFIG_BT_ISO_UNICAST */

#if defined(CONFIG_BT_ISO_BROADCAST)
#define BIS_ISO_CHAN_COUNT 1

static struct bt_iso_chan_qos bis_iso_qos;

static struct bt_iso_chan bis_iso_chan = {
	.ops = &iso_ops,
	.qos = &bis_iso_qos,
};

static struct bt_iso_chan *bis_channels[BIS_ISO_CHAN_COUNT] = { &bis_iso_chan };

static struct bt_iso_big *big;

NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, BIS_ISO_CHAN_COUNT, DATA_MTU, NULL);

static int cmd_broadcast(const struct shell *shell, size_t argc, char *argv[])
{
	static uint8_t buf_data[DATA_MTU] = { [0 ... (DATA_MTU - 1)] = 0xff };
	int ret, len, count = 1;
	struct net_buf *buf;

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
	}

	if (!bis_iso_chan.conn) {
		shell_error(shell, "BIG not created");
		return -ENOEXEC;
	}

	if (!bis_iso_qos.tx) {
		shell_error(shell, "BIG not setup as broadcaster");
		return -ENOEXEC;
	}

	len = MIN(iso_chan.qos->tx->sdu, DATA_MTU - BT_ISO_CHAN_SEND_RESERVE);

	while (count--) {
		for (int i = 0; i < BIS_ISO_CHAN_COUNT; i++) {
			buf = net_buf_alloc(&bis_tx_pool, K_FOREVER);
			net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

			net_buf_add_mem(buf, buf_data, len);
			ret = bt_iso_chan_send(&bis_iso_chan, buf);
			if (ret < 0) {
				shell_print(shell, "[%i]: Unable to broadcast: %d", i, -ret);
				net_buf_unref(buf);
				return -ENOEXEC;
			}
		}
	}

	shell_print(shell, "ISO broadcasting...");

	return 0;
}

static int cmd_big_create(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	struct bt_iso_big_create_param param;
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];

	if (!adv) {
		shell_error(shell, "No (periodic) advertising set selected");
		return -ENOEXEC;
	}

	/* TODO: Allow setting QOS from shell */
	bis_iso_qos.tx = &iso_tx_qos;
	bis_iso_qos.tx->interval = 10000;      /* us */
	bis_iso_qos.tx->latency = 20;          /* ms */
	bis_iso_qos.tx->phy = BT_GAP_LE_PHY_2M; /* 2 MBit */
	bis_iso_qos.tx->rtn = 2;
	bis_iso_qos.tx->sdu = CONFIG_BT_ISO_TX_MTU;

	param.bis_channels = bis_channels;
	param.num_bis = BIS_ISO_CHAN_COUNT;
	param.encryption = false;

	if (argc > 1) {
		if (!strcmp(argv[1], "enc")) {
			uint8_t bcode_len = hex2bin(argv[1], strlen(argv[1]), param.bcode,
						    sizeof(param.bcode));
			if (!bcode_len || bcode_len != sizeof(param.bcode)) {
				shell_error(shell, "Invalid Broadcast Code Length");
				return -ENOEXEC;
			}
			param.encryption = true;
		} else {
			shell_help(shell);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	err = bt_iso_big_create(adv, &param, &big);
	if (err) {
		shell_error(shell, "Unable to create BIG (err %d)", err);
		return 0;
	}

	shell_print(shell, "BIG created");

	return 0;
}

static int cmd_big_sync(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	/* TODO: Add support to select which PA sync to BIG sync to */
	struct bt_le_per_adv_sync *pa_sync = per_adv_syncs[0];
	struct bt_iso_big_sync_param param;

	if (!pa_sync) {
		shell_error(shell, "No PA sync selected");
		return -ENOEXEC;
	}

	bis_iso_qos.tx = NULL;

	param.bis_channels = bis_channels;
	param.num_bis = BIS_ISO_CHAN_COUNT;
	param.encryption = false;
	param.bis_bitfield = strtoul(argv[1], NULL, 16);
	param.mse = 0;
	param.sync_timeout = 0xFF;

	for (int i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "mse")) {
			param.mse = strtoul(argv[i], NULL, 16);
		} else if (!strcmp(argv[i], "timeout")) {
			param.sync_timeout = strtoul(argv[i], NULL, 16);
		} else if (!strcmp(argv[i], "enc")) {
			uint8_t bcode_len;

			i++;
			if (i == argc) {
				shell_help(shell);
				return SHELL_CMD_HELP_PRINTED;
			}

			bcode_len = hex2bin(argv[i], strlen(argv[i]), param.bcode,
					    sizeof(param.bcode));

			if (!bcode_len || bcode_len != sizeof(param.bcode)) {
				shell_error(shell, "Invalid Broadcast Code Length");
				return -ENOEXEC;
			}
			param.encryption = true;
		} else {
			shell_help(shell);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	err = bt_iso_big_sync(pa_sync, &param, &big);
	if (err) {
		shell_error(shell, "Unable to sync to BIG (err %d)", err);
		return 0;
	}

	shell_print(shell, "BIG syncing");

	return 0;
}

static int cmd_big_term(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	err = bt_iso_big_terminate(big);
	if (err) {
		shell_error(shell, "Unable to terminate BIG", err);
		return 0;
	}

	shell_print(shell, "BIG terminated");

	return 0;
}
#endif /* CONFIG_BT_ISO_BROADCAST */

SHELL_STATIC_SUBCMD_SET_CREATE(iso_cmds,
#if defined(CONFIG_BT_ISO_UNICAST)
	SHELL_CMD_ARG(bind, NULL, "[dir=tx,rx,txrx] [interval] [packing] [framing] "
		      "[latency] [sdu] [phy] [rtn]", cmd_bind, 1, 8),
	SHELL_CMD_ARG(connect, NULL, "Connect ISO Channel", cmd_connect, 1, 0),
	SHELL_CMD_ARG(listen, NULL, "<dir=tx,rx,txrx> [security level]", cmd_listen, 2, 1),
	SHELL_CMD_ARG(send, NULL, "Send to ISO Channel [count]",
		      cmd_send, 1, 1),
	SHELL_CMD_ARG(disconnect, NULL, "Disconnect ISO Channel",
		      cmd_disconnect, 1, 0),
#endif /* CONFIG_BT_ISO_UNICAST */
#if defined(CONFIG_BT_ISO_BROADCAST)
	SHELL_CMD_ARG(create-big, NULL, "Create a BIG as a broadcaster [enc <broadcast code>]",
		      cmd_big_create, 1, 2),
	SHELL_CMD_ARG(sync-big, NULL, "Synchronize to a BIG as a receiver <BIS bitfield> [mse] "
		      "[timeout] [enc <broadcast code>]", cmd_big_sync, 2, 4),
	SHELL_CMD_ARG(term-big, NULL, "Terminate a BIG", cmd_big_term, 1, 0),
	SHELL_CMD_ARG(broadcast, NULL, "Broadcast on ISO channels", cmd_broadcast, 1, 1),
#endif /* CONFIG_BT_ISO_BROADCAST */
	SHELL_SUBCMD_SET_END
);

static int cmd_iso(const struct shell *shell, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(shell, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(shell, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(iso, &iso_cmds, "Bluetooth ISO shell commands",
		       cmd_iso, 1, 1);
