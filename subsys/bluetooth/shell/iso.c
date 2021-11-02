/** @file
 *  @brief Bluetooth Audio shell
 *
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <ctype.h>
#include <zephyr.h>
#include <shell/shell.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/iso.h>

#include "bt.h"

static void iso_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		struct net_buf *buf)
{
	shell_print(ctx_shell, "Incoming data channel %p len %u, seq: %d, ts: %d",
		    chan, buf->len, info->sn, info->ts);
}

static void iso_connected(struct bt_iso_chan *chan)
{
	shell_print(ctx_shell, "ISO Channel %p connected", chan);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	shell_print(ctx_shell, "ISO Channel %p disconnected with reason 0x%02x",
		    chan, reason);
}

static struct bt_iso_chan_ops iso_ops = {
	.recv		= iso_recv,
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
};

#define DEFAULT_IO_QOS \
{ \
	.sdu		= 40u, \
	.phy		= BT_GAP_LE_PHY_2M, \
	.rtn		= 2u, \
}

static struct bt_iso_chan_io_qos iso_tx_qos = DEFAULT_IO_QOS;
static struct bt_iso_chan_io_qos iso_rx_qos = DEFAULT_IO_QOS;

static struct bt_iso_chan_qos iso_qos = {
	.tx		= &iso_tx_qos,
	.rx		= &iso_rx_qos,
};

#if defined(CONFIG_BT_ISO_UNICAST)
#define CIS_ISO_CHAN_COUNT 1

struct bt_iso_chan iso_chan = {
	.ops = &iso_ops,
	.qos = &iso_qos,
};

static struct bt_iso_cig *cig;

NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  NULL);

static int iso_accept(const struct bt_iso_accept_info *info,
		      struct bt_iso_chan **chan)
{
	shell_print(ctx_shell, "Incoming request from %p with CIG ID 0x%02X and CIS ID 0x%02X",
		    info->acl, info->cig_id, info->cis_id);

	if (iso_chan.iso) {
		shell_print(ctx_shell, "No channels available");
		return -ENOMEM;
	}

	*chan = &iso_chan;

	return 0;
}

struct bt_iso_server iso_server = {
	.sec_level = BT_SECURITY_L1,
	.accept = iso_accept,
};

static int cmd_listen(const struct shell *sh, size_t argc, char *argv[])
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
		shell_error(sh, "Invalid argument - use tx, rx or txrx");
		return -ENOEXEC;
	}

	if (argc > 2) {
		iso_server.sec_level = *argv[2] - '0';
	}

	err = bt_iso_server_register(&iso_server);
	if (err) {
		shell_error(sh, "Unable to register ISO cap (err %d)",
			    err);
		return err;
	}

	/* Setup peripheral iso data direction only if register is success */
	iso_chan.qos->tx = tx_qos;
	iso_chan.qos->rx = rx_qos;
	return err;
}

static int cmd_cig_create(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_iso_cig_create_param param;
	static struct bt_iso_chan *chans[CIS_ISO_CHAN_COUNT];

	if (cig != NULL) {
		shell_error(sh, "Already created");
		return -ENOEXEC;
	}

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
		param.interval = strtol(argv[2], NULL, 0);
	} else {
		param.interval = 10000;
	}

	if (argc > 3) {
		param.packing = strtol(argv[3], NULL, 0);
	} else {
		param.packing = 0;
	}

	if (argc > 4) {
		param.framing = strtol(argv[4], NULL, 0);
	} else {
		param.framing = 0;
	}

	if (argc > 5) {
		param.latency = strtol(argv[5], NULL, 0);
	} else {
		param.latency = 10;
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

	param.sca = BT_GAP_SCA_UNKNOWN;
	param.cis_channels = chans;
	param.num_cis = ARRAY_SIZE(chans);

	err = bt_iso_cig_create(&param, &cig);
	if (err) {
		shell_error(sh, "Unable to create CIG (err %d)", err);
		return 0;
	}

	shell_print(sh, "CIG created");

	return 0;
}

static int cmd_cig_term(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (cig == NULL) {
		shell_error(sh, "CIG not created");
		return -ENOEXEC;
	}

	err = bt_iso_cig_terminate(cig);
	if (err) {
		shell_error(sh, "Unable to terminate CIG (err %d)", err);
		return 0;
	}

	shell_print(sh, "CIG terminated");
	cig = NULL;

	return 0;
}

static int cmd_connect(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_iso_connect_param connect_param = {
		.acl = default_conn,
		.iso_chan = &iso_chan
	};
	int err;

	if (iso_chan.iso == NULL) {
		shell_error(sh, "ISO channel not initialized in a CIG");
		return 0;
	}

	err = bt_iso_chan_connect(&connect_param, 1);
	if (err) {
		shell_error(sh, "Unable to connect (err %d)", err);
		return 0;
	}

	shell_print(sh, "ISO Connect pending...");

	return 0;
}


static int cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	static uint8_t buf_data[CONFIG_BT_ISO_TX_MTU] = {
		[0 ... (CONFIG_BT_ISO_TX_MTU - 1)] = 0xff
	};
	int ret, len, count = 1;
	struct net_buf *buf;

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
	}

	if (!iso_chan.iso) {
		shell_error(sh, "Not bound");
		return 0;
	}

	if (!iso_chan.qos->tx) {
		shell_error(sh, "Transmission QoS disabled");
		return -ENOEXEC;
	}

	len = MIN(iso_chan.qos->tx->sdu, CONFIG_BT_ISO_TX_MTU);

	while (count--) {
		buf = net_buf_alloc(&tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

		net_buf_add_mem(buf, buf_data, len);
		shell_info(sh, "send: %d bytes of data", len);
		ret = bt_iso_chan_send(&iso_chan, buf);
		if (ret < 0) {
			shell_print(sh, "Unable to send: %d", -ret);
			net_buf_unref(buf);
			return -ENOEXEC;
		}
	}

	shell_print(sh, "ISO sending...");

	return 0;
}

static int cmd_disconnect(const struct shell *sh, size_t argc,
			      char *argv[])
{
	int err;

	err = bt_iso_chan_disconnect(&iso_chan);
	if (err) {
		shell_error(sh, "Unable to disconnect (err %d)", err);
		return 0;
	}

	shell_print(sh, "ISO Disconnect pending...");

	return 0;
}
#endif /* CONFIG_BT_ISO_UNICAST */

#if defined(CONFIG_BT_ISO_BROADCAST)
#define BIS_ISO_CHAN_COUNT 1
static struct bt_iso_big *big;

static struct bt_iso_chan_qos bis_iso_qos;

static struct bt_iso_chan bis_iso_chan = {
	.ops = &iso_ops,
	.qos = &bis_iso_qos,
};

static struct bt_iso_chan *bis_channels[BIS_ISO_CHAN_COUNT] = { &bis_iso_chan };

#if defined(CONFIG_BT_ISO_BROADCASTER)
NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, BIS_ISO_CHAN_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), NULL);

static int cmd_broadcast(const struct shell *sh, size_t argc, char *argv[])
{
	static uint8_t buf_data[CONFIG_BT_ISO_TX_MTU] = {
		[0 ... (CONFIG_BT_ISO_TX_MTU - 1)] = 0xff
	};
	int ret, len, count = 1;
	struct net_buf *buf;

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
	}

	if (!bis_iso_chan.iso) {
		shell_error(sh, "BIG not created");
		return -ENOEXEC;
	}

	if (!bis_iso_qos.tx) {
		shell_error(sh, "BIG not setup as broadcaster");
		return -ENOEXEC;
	}

	len = MIN(iso_chan.qos->tx->sdu, CONFIG_BT_ISO_TX_MTU);

	while (count--) {
		for (int i = 0; i < BIS_ISO_CHAN_COUNT; i++) {
			buf = net_buf_alloc(&bis_tx_pool, K_FOREVER);
			net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

			net_buf_add_mem(buf, buf_data, len);
			ret = bt_iso_chan_send(&bis_iso_chan, buf);
			if (ret < 0) {
				shell_print(sh, "[%i]: Unable to broadcast: %d", i, -ret);
				net_buf_unref(buf);
				return -ENOEXEC;
			}
		}
	}

	shell_print(sh, "ISO broadcasting...");

	return 0;
}

static int cmd_big_create(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_iso_big_create_param param;
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];

	if (!adv) {
		shell_error(sh, "No (periodic) advertising set selected");
		return -ENOEXEC;
	}

	/* TODO: Allow setting QOS from shell */
	bis_iso_qos.tx = &iso_tx_qos;
	bis_iso_qos.tx->phy = BT_GAP_LE_PHY_2M; /* 2 MBit */
	bis_iso_qos.tx->rtn = 2;
	bis_iso_qos.tx->sdu = CONFIG_BT_ISO_TX_MTU;

	param.interval = 10000;      /* us */
	param.latency = 20;          /* ms */
	param.bis_channels = bis_channels;
	param.num_bis = BIS_ISO_CHAN_COUNT;
	param.encryption = false;
	param.packing = BT_ISO_PACKING_SEQUENTIAL;
	param.framing = BT_ISO_FRAMING_UNFRAMED;

	if (argc > 1) {
		if (!strcmp(argv[1], "enc")) {
			uint8_t bcode_len = hex2bin(argv[1], strlen(argv[1]), param.bcode,
						    sizeof(param.bcode));
			if (!bcode_len || bcode_len != sizeof(param.bcode)) {
				shell_error(sh, "Invalid Broadcast Code Length");
				return -ENOEXEC;
			}
			param.encryption = true;
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	} else {
		memset(param.bcode, 0, sizeof(param.bcode));
	}

	err = bt_iso_big_create(adv, &param, &big);
	if (err) {
		shell_error(sh, "Unable to create BIG (err %d)", err);
		return 0;
	}

	shell_print(sh, "BIG created");

	return 0;
}
#endif /* CONFIG_BT_ISO_BROADCASTER */

#if defined(CONFIG_BT_ISO_SYNC_RECEIVER)
static int cmd_big_sync(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	/* TODO: Add support to select which PA sync to BIG sync to */
	struct bt_le_per_adv_sync *pa_sync = per_adv_syncs[0];
	struct bt_iso_big_sync_param param;

	if (!pa_sync) {
		shell_error(sh, "No PA sync selected");
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
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			bcode_len = hex2bin(argv[i], strlen(argv[i]), param.bcode,
					    sizeof(param.bcode));

			if (!bcode_len || bcode_len != sizeof(param.bcode)) {
				shell_error(sh, "Invalid Broadcast Code Length");
				return -ENOEXEC;
			}
			param.encryption = true;
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	err = bt_iso_big_sync(pa_sync, &param, &big);
	if (err) {
		shell_error(sh, "Unable to sync to BIG (err %d)", err);
		return 0;
	}

	shell_print(sh, "BIG syncing");

	return 0;
}
#endif /* CONFIG_BT_ISO_SYNC_RECEIVER */

static int cmd_big_term(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_iso_big_terminate(big);
	if (err) {
		shell_error(sh, "Unable to terminate BIG (err %d)", err);
		return 0;
	}

	shell_print(sh, "BIG terminated");

	return 0;
}
#endif /* CONFIG_BT_ISO_BROADCAST*/

SHELL_STATIC_SUBCMD_SET_CREATE(iso_cmds,
#if defined(CONFIG_BT_ISO_UNICAST)
	SHELL_CMD_ARG(cig_create, NULL, "[dir=tx,rx,txrx] [interval] [packing] [framing] "
		      "[latency] [sdu] [phy] [rtn]", cmd_cig_create, 1, 8),
	SHELL_CMD_ARG(cig_term, NULL, "Terminate the CIG", cmd_cig_term, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "Connect ISO Channel", cmd_connect, 1, 0),
	SHELL_CMD_ARG(listen, NULL, "<dir=tx,rx,txrx> [security level]", cmd_listen, 2, 1),
	SHELL_CMD_ARG(send, NULL, "Send to ISO Channel [count]",
		      cmd_send, 1, 1),
	SHELL_CMD_ARG(disconnect, NULL, "Disconnect ISO Channel",
		      cmd_disconnect, 1, 0),
#endif /* CONFIG_BT_ISO_UNICAST */
#if defined(CONFIG_BT_ISO_BROADCASTER)
	SHELL_CMD_ARG(create-big, NULL, "Create a BIG as a broadcaster [enc <broadcast code>]",
		      cmd_big_create, 1, 2),
	SHELL_CMD_ARG(broadcast, NULL, "Broadcast on ISO channels", cmd_broadcast, 1, 1),
#endif /* CONFIG_BT_ISO_BROADCASTER */
#if defined(CONFIG_BT_ISO_SYNC_RECEIVER)
	SHELL_CMD_ARG(sync-big, NULL, "Synchronize to a BIG as a receiver <BIS bitfield> [mse] "
		      "[timeout] [enc <broadcast code>]", cmd_big_sync, 2, 4),
#endif /* CONFIG_BT_ISO_SYNC_RECEIVER */
#if defined(CONFIG_BT_ISO_BROADCAST)
	SHELL_CMD_ARG(term-big, NULL, "Terminate a BIG", cmd_big_term, 1, 0),
#endif /* CONFIG_BT_ISO_BROADCAST */
	SHELL_SUBCMD_SET_END
);

static int cmd_iso(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(iso, &iso_cmds, "Bluetooth ISO shell commands",
		       cmd_iso, 1, 1);
