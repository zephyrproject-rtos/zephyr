/* sco.c - Bluetooth sco handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include "common/bt_str.h"

#include "host/addr_internal.h"
#include "host/hci_core.h"
#include "br.h"
#include "host/conn_internal.h"
#include "sco_internal.h"

#define LOG_LEVEL CONFIG_BT_CONN_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_sco);

struct bt_sco_server *sco_server;

#define SCO_CHAN(_sco) ((_sco)->sco.chan);

int bt_sco_server_register(struct bt_sco_server *server)
{
	CHECKIF(!server) {
		LOG_DBG("Invalid parameter: server %p", server);
		return -EINVAL;
	}

	if (sco_server) {
		return -EADDRINUSE;
	}

	if (!server->accept) {
		return -EINVAL;
	}

	if (server->sec_level > BT_SECURITY_L3) {
		return -EINVAL;
	}

	LOG_DBG("%p", server);

	sco_server = server;

	return 0;
}

int bt_sco_server_unregister(struct bt_sco_server *server)
{
	CHECKIF(!server) {
		LOG_DBG("Invalid parameter: server %p", server);
		return -EINVAL;
	}

	if (sco_server != server) {
		return -EINVAL;
	}

	sco_server = NULL;

	return 0;
}

void bt_sco_connected(struct bt_conn *sco)
{
	struct bt_sco_chan *chan;

	if (sco == NULL || sco->type != BT_CONN_TYPE_SCO) {
		LOG_ERR("Invalid parameters: sco %p sco->type %u", sco, sco ? sco->type : 0);
		return;
	}

	LOG_DBG("%p", sco);

	chan = SCO_CHAN(sco);

	if (chan == NULL) {
		LOG_ERR("Could not lookup chan from connected SCO");
		return;
	}

	bt_sco_chan_set_state(chan, BT_SCO_STATE_CONNECTED);

	if (chan->ops && chan->ops->connected) {
		chan->ops->connected(chan);
	}
}

void bt_sco_disconnected(struct bt_conn *sco)
{
	struct bt_sco_chan *chan;

	if (sco == NULL || sco->type != BT_CONN_TYPE_SCO) {
		LOG_ERR("Invalid parameters: sco %p sco->type %u", sco, sco ? sco->type : 0);
		return;
	}
	LOG_DBG("%p", sco);

	chan = SCO_CHAN(sco);
	if (chan == NULL) {
		LOG_ERR("Could not lookup chan from connected SCO");
		return;
	}

	bt_sco_chan_set_state(chan, BT_SCO_STATE_DISCONNECTED);

	bt_sco_cleanup_acl(sco);

	chan->sco = NULL;

	if (chan->ops && chan->ops->disconnected) {
		chan->ops->disconnected(chan, sco->err);
	}
}

static uint8_t sco_server_check_security(struct bt_conn *conn)
{
	if (IS_ENABLED(CONFIG_BT_CONN_DISABLE_SECURITY)) {
		return BT_HCI_ERR_SUCCESS;
	}

	if (conn->sec_level >= sco_server->sec_level) {
		return BT_HCI_ERR_SUCCESS;
	}

	return BT_HCI_ERR_INSUFFICIENT_SECURITY;
}

#if defined(CONFIG_BT_CONN_LOG_LEVEL_DBG)
const char *bt_sco_chan_state_str(uint8_t state)
{
	switch (state) {
	case BT_SCO_STATE_DISCONNECTED:
		return "disconnected";
	case BT_SCO_STATE_CONNECTING:
		return "connecting";
	case BT_SCO_STATE_ENCRYPT_PENDING:
		return "encryption pending";
	case BT_SCO_STATE_CONNECTED:
		return "connected";
	case BT_SCO_STATE_DISCONNECTING:
		return "disconnecting";
	default:
		return "unknown";
	}
}

void bt_sco_chan_set_state_debug(struct bt_sco_chan *chan,
				 enum bt_sco_state state,
				 const char *func, int line)
{
	LOG_DBG("chan %p sco %p %s -> %s", chan, chan->sco, bt_sco_chan_state_str(chan->state),
		bt_sco_chan_state_str(state));

	/* check transitions validness */
	switch (state) {
	case BT_SCO_STATE_DISCONNECTED:
		/* regardless of old state always allows this states */
		break;
	case BT_SCO_STATE_ENCRYPT_PENDING:
		__fallthrough;
	case BT_SCO_STATE_CONNECTING:
		if (chan->state != BT_SCO_STATE_DISCONNECTED) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_SCO_STATE_CONNECTED:
		if (chan->state != BT_SCO_STATE_CONNECTING) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_SCO_STATE_DISCONNECTING:
		if (chan->state != BT_SCO_STATE_CONNECTING &&
			chan->state != BT_SCO_STATE_CONNECTED) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	default:
		LOG_ERR("%s()%d: unknown (%u) state was set", func, line, state);
		return;
	}

	chan->state = state;
}
#else
void bt_sco_chan_set_state(struct bt_sco_chan *chan, enum bt_sco_state state)
{
	chan->state = state;
}
#endif /* CONFIG_BT_CONN_LOG_LEVEL_DBG */


static void bt_sco_chan_add(struct bt_conn *sco, struct bt_sco_chan *chan)
{
	/* Attach SCO channel to the connection */
	chan->sco = sco;
	sco->sco.chan = chan;

	LOG_DBG("sco %p chan %p", sco, chan);
}

static int sco_accept(struct bt_conn *acl, struct bt_conn *sco)
{
	struct bt_sco_accept_info accept_info;
	struct bt_sco_chan *chan;
	int err;

	CHECKIF(!sco || sco->type != BT_CONN_TYPE_SCO) {
		LOG_ERR("Invalid parameters: sco %p sco->type %u", sco, sco ? sco->type : 0);
		return -EINVAL;
	}

	LOG_DBG("%p", sco);

	accept_info.acl = acl;
	memcpy(accept_info.dev_class, sco->sco.dev_class, sizeof(accept_info.dev_class));
	accept_info.link_type = sco->sco.link_type;

	err = sco_server->accept(&accept_info, &chan);
	if (err < 0) {
		LOG_ERR("Server failed to accept: %d", err);
		return err;
	}

	if (chan->ops == NULL) {
		LOG_ERR("invalid parameter: chan %p chan->ops %p", chan, chan->ops);
		return -EINVAL;
	}

	bt_sco_chan_add(sco, chan);
	bt_sco_chan_set_state(chan, BT_SCO_STATE_CONNECTING);

	return 0;
}

static int accept_sco_conn(const bt_addr_t *bdaddr, struct bt_conn *sco_conn)
{
	struct bt_hci_cp_accept_sync_conn_req *cp;
	struct net_buf *buf;
	int err;

	err = sco_accept(sco_conn->sco.acl, sco_conn);
	if (err) {
		return err;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_ACCEPT_SYNC_CONN_REQ, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	cp->pkt_type = sco_conn->sco.pkt_type;
	cp->tx_bandwidth = 0x00001f40;
	cp->rx_bandwidth = 0x00001f40;
	cp->max_latency = 0x0007;
	cp->retrans_effort = 0x01;
	cp->content_format = BT_VOICE_CVSD_16BIT;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_ACCEPT_SYNC_CONN_REQ, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

uint8_t bt_esco_conn_req(struct bt_hci_evt_conn_request *evt)
{
	struct bt_conn *sco_conn;
	uint8_t sec_err;

	if (sco_server == NULL) {
		LOG_ERR("No SCO server registered");
		return BT_HCI_ERR_UNSPECIFIED;
	}

	sco_conn = bt_conn_add_sco(&evt->bdaddr, evt->link_type);
	if (!sco_conn) {
		return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
	}

	sec_err = sco_server_check_security(sco_conn->sco.acl);
	if (BT_HCI_ERR_SUCCESS != sec_err) {
		LOG_DBG("Insufficient security %u", sec_err);
		bt_sco_cleanup(sco_conn);
		return sec_err;
	}

	memcpy(sco_conn->sco.dev_class, evt->dev_class, sizeof(sco_conn->sco.dev_class));
	sco_conn->sco.link_type = evt->link_type;

	if (accept_sco_conn(&evt->bdaddr, sco_conn)) {
		LOG_ERR("Error accepting connection from %s", bt_addr_str(&evt->bdaddr));
		bt_sco_cleanup(sco_conn);
		return BT_HCI_ERR_UNSPECIFIED;
	}

	sco_conn->role = BT_HCI_ROLE_PERIPHERAL;
	bt_conn_set_state(sco_conn, BT_CONN_CONNECTING);
	bt_conn_unref(sco_conn);

	return BT_HCI_ERR_SUCCESS;
}

void bt_sco_cleanup_acl(struct bt_conn *sco)
{
	LOG_DBG("%p", sco);

	if (sco->sco.acl) {
		bt_conn_unref(sco->sco.acl);
		sco->sco.acl = NULL;
	}
}

static int sco_setup_sync_conn(struct bt_conn *sco_conn)
{
	struct net_buf *buf;
	struct bt_hci_cp_setup_sync_conn *cp;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_SETUP_SYNC_CONN, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	(void)memset(cp, 0, sizeof(*cp));

	LOG_DBG("handle : %x", sco_conn->sco.acl->handle);

	cp->handle = sco_conn->sco.acl->handle;
	cp->pkt_type = sco_conn->sco.pkt_type;
	cp->tx_bandwidth = 0x00001f40;
	cp->rx_bandwidth = 0x00001f40;
	cp->max_latency = 0x0007;
	cp->retrans_effort = 0x01;
	cp->content_format = BT_VOICE_CVSD_16BIT;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_SETUP_SYNC_CONN, buf, NULL);
	if (err < 0) {
		return err;
	}
	return 0;
}

struct bt_conn *bt_conn_create_sco(const bt_addr_t *peer, struct bt_sco_chan *chan)
{
	struct bt_conn *sco_conn;
	int link_type;

	sco_conn = bt_conn_lookup_addr_sco(peer);
	if (sco_conn) {
		switch (sco_conn->state) {
		case BT_CONN_CONNECTING:
		case BT_CONN_CONNECTED:
			return sco_conn;
		default:
			bt_conn_unref(sco_conn);
			return NULL;
		}
	}

	if (BT_FEAT_LMP_ESCO_CAPABLE(bt_dev.features)) {
		link_type = BT_HCI_ESCO;
	} else {
		link_type = BT_HCI_SCO;
	}

	sco_conn = bt_conn_add_sco(peer, link_type);
	if (!sco_conn) {
		return NULL;
	}

	sco_conn->sco.link_type = link_type;

	bt_sco_chan_add(sco_conn, chan);
	bt_conn_set_state(chan->sco, BT_CONN_CONNECTING);
	bt_sco_chan_set_state(chan, BT_SCO_STATE_CONNECTING);

	if (sco_setup_sync_conn(sco_conn) < 0) {
		bt_conn_set_state(chan->sco, BT_CONN_DISCONNECTED);
		bt_sco_chan_set_state(chan, BT_SCO_STATE_DISCONNECTED);
		bt_sco_cleanup(sco_conn);
		return NULL;
	}

	return sco_conn;
}
