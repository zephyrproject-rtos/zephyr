/*  Bluetooth ISO */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/iso.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "iso_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_ISO)
#define LOG_MODULE_NAME bt_iso
#include "common/log.h"

NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  CONFIG_BT_ISO_TX_MTU, NULL);
NET_BUF_POOL_FIXED_DEFINE(iso_rx_pool, CONFIG_BT_ISO_RX_BUF_COUNT,
			  CONFIG_BT_ISO_RX_MTU, NULL);

#if CONFIG_BT_ISO_TX_FRAG_COUNT > 0
NET_BUF_POOL_FIXED_DEFINE(iso_frag_pool, CONFIG_BT_ISO_TX_FRAG_COUNT,
			  CONFIG_BT_ISO_TX_MTU, NULL);
#endif /* CONFIG_BT_ISO_TX_FRAG_COUNT > 0 */

/* TODO: Allow more than one server? */
static struct bt_iso_server *iso_server;
struct bt_conn iso_conns[CONFIG_BT_MAX_ISO_CONN];

/* Audio Data Path direction */
enum {
	BT_ISO_CHAN_HOST_TO_CTRL,
	BT_ISO_CHAN_CTRL_TO_HOST,
};

struct bt_iso_data_path {
	/* Data Path direction */
	uint8_t dir;
	/* Data Path ID */
	uint8_t pid;
	/* Data Path param reference */
	struct bt_iso_chan_path *path;
};


struct net_buf *bt_iso_get_rx(k_timeout_t timeout)
{
	struct net_buf *buf = net_buf_alloc(&iso_rx_pool, timeout);

	if (buf) {
		net_buf_reserve(buf, BT_BUF_RESERVE);
		bt_buf_set_type(buf, BT_BUF_ISO_IN);
	}

	return buf;
}

void hci_iso(struct net_buf *buf)
{
	struct bt_hci_iso_hdr *hdr;
	uint16_t handle, len;
	struct bt_conn *conn;
	uint8_t flags;

	BT_DBG("buf %p", buf);

	BT_ASSERT(buf->len >= sizeof(*hdr));

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	len = sys_le16_to_cpu(hdr->len);
	handle = sys_le16_to_cpu(hdr->handle);
	flags = bt_iso_flags(handle);

	iso(buf)->handle = bt_iso_handle(handle);
	iso(buf)->index = BT_CONN_INDEX_INVALID;

	BT_DBG("handle %u len %u flags %u", iso(buf)->handle, len, flags);

	if (buf->len != len) {
		BT_ERR("ISO data length mismatch (%u != %u)", buf->len, len);
		net_buf_unref(buf);
		return;
	}

	conn = bt_conn_lookup_handle(iso(buf)->handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", iso(buf)->handle);
		net_buf_unref(buf);
		return;
	}

	iso(buf)->index = bt_conn_index(conn);

	bt_conn_recv(conn, buf, flags);
	bt_conn_unref(conn);
}

void hci_le_cis_estabilished(struct net_buf *buf)
{
	struct bt_hci_evt_le_cis_established *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->conn_handle);
	struct bt_conn *conn;

	BT_DBG("status %u handle %u", evt->status, handle);

	/* ISO connection handles are already assigned at this point */
	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("No connection found for handle %u", handle);
		return;
	}

	__ASSERT(conn->type == BT_CONN_TYPE_ISO, "Invalid connection type");

	if (!evt->status) {
		/* TODO: Add CIG sync delay */
		bt_conn_set_state(conn, BT_CONN_CONNECTED);
		bt_conn_unref(conn);
		return;
	}

	conn->err = evt->status;
	bt_iso_disconnected(conn);
	bt_conn_unref(conn);
}

int hci_le_reject_cis(uint16_t handle, uint8_t reason)
{
	struct bt_hci_cp_le_reject_cis *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_REJECT_CIS, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->reason = reason;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_REJECT_CIS, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

int hci_le_accept_cis(uint16_t handle)
{
	struct bt_hci_cp_le_accept_cis *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_ACCEPT_CIS, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_ACCEPT_CIS, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

void hci_le_cis_req(struct net_buf *buf)
{
	struct bt_hci_evt_le_cis_req *evt = (void *)buf->data;
	uint16_t acl_handle = sys_le16_to_cpu(evt->acl_handle);
	uint16_t cis_handle = sys_le16_to_cpu(evt->cis_handle);
	struct bt_conn *conn, *iso;
	int err;

	BT_DBG("acl_handle %u cis_handle %u cig_id %u cis %u",
		acl_handle, cis_handle, evt->cig_id, evt->cis_id);

	/* Lookup existing connection with same handle */
	iso = bt_conn_lookup_handle(cis_handle);
	if (iso) {
		BT_ERR("Invalid ISO handle %d", cis_handle);
		hci_le_reject_cis(cis_handle, BT_HCI_ERR_CONN_LIMIT_EXCEEDED);
		bt_conn_unref(iso);
		return;
	}

	/* Lookup ACL connection to attach */
	conn = bt_conn_lookup_handle(acl_handle);
	if (!conn) {
		BT_ERR("Invalid ACL handle %d", acl_handle);
		hci_le_reject_cis(cis_handle, BT_HCI_ERR_UNKNOWN_CONN_ID);
		return;
	}

	/* Add ISO connection */
	iso = bt_conn_add_iso(conn);

	bt_conn_unref(conn);

	if (!iso) {
		hci_le_reject_cis(cis_handle,
				  BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}

	/* Request application to accept */
	err = bt_iso_accept(iso);
	if (err) {
		bt_iso_cleanup(iso);
		hci_le_reject_cis(cis_handle,
				  BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}

	iso->handle = cis_handle;
	iso->role = BT_HCI_ROLE_SLAVE;
	bt_conn_set_state(iso, BT_CONN_CONNECT);

	hci_le_accept_cis(cis_handle);
}

int hci_le_remove_cig(uint8_t cig_id)
{
	struct bt_hci_cp_le_remove_cig *req;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_REMOVE_CIG, sizeof(*req));
	if (!buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(buf, sizeof(*req));

	memset(req, 0, sizeof(*req));

	req->cig_id = cig_id;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_REMOVE_CIG, buf, NULL);
}

void bt_iso_cleanup(struct bt_conn *conn)
{
	int i;

	bt_conn_unref(conn->iso.acl);
	conn->iso.acl = NULL;

	/* Check if conn is last of CIG */
	for (i = 0; i < CONFIG_BT_MAX_ISO_CONN; i++) {
		if (conn == &iso_conns[i]) {
			continue;
		}

		if (atomic_get(&iso_conns[i].ref) &&
		    iso_conns[i].iso.cig_id == conn->iso.cig_id) {
			break;
		}
	}

	if (i == CONFIG_BT_MAX_ISO_CONN) {
		hci_le_remove_cig(conn->iso.cig_id);
	}

	bt_conn_unref(conn);

}

struct bt_conn *iso_new(void)
{
	return bt_conn_new(iso_conns, ARRAY_SIZE(iso_conns));
}

struct bt_conn *bt_conn_add_iso(struct bt_conn *acl)
{
	struct bt_conn *conn = iso_new();

	if (!conn) {
		return NULL;
	}

	conn->iso.acl = bt_conn_ref(acl);
	conn->type = BT_CONN_TYPE_ISO;
	sys_slist_init(&conn->channels);

	return conn;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *bt_iso_create_pdu_timeout_debug(struct net_buf_pool *pool,
						size_t reserve,
						k_timeout_t timeout,
						const char *func, int line)
#else
struct net_buf *bt_iso_create_pdu_timeout(struct net_buf_pool *pool,
					  size_t reserve, k_timeout_t timeout)
#endif
{
	if (!pool) {
		pool = &iso_tx_pool;
	}

	reserve += sizeof(struct bt_hci_iso_data_hdr);

#if defined(CONFIG_NET_BUF_LOG)
	return bt_conn_create_pdu_timeout_debug(pool, reserve, timeout, func,
						line);
#else
	return bt_conn_create_pdu_timeout(pool, reserve, timeout);
#endif
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *bt_iso_create_frag_timeout_debug(size_t reserve,
						 k_timeout_t timeout,
						 const char *func, int line)
#else
struct net_buf *bt_iso_create_frag_timeout(size_t reserve, k_timeout_t timeout)
#endif
{
	struct net_buf_pool *pool = NULL;

#if CONFIG_BT_L2CAP_TX_FRAG_COUNT > 0
	pool = &iso_frag_pool;
#endif

#if defined(CONFIG_NET_BUF_LOG)
	return bt_conn_create_pdu_timeout_debug(pool, reserve, timeout, func,
						line);
#else
	return bt_conn_create_pdu_timeout(pool, reserve, timeout);
#endif
}

static struct net_buf *hci_le_set_cig_params(struct bt_iso_create_param *param)
{
	struct bt_hci_cis_params *cis;
	struct bt_hci_cp_le_set_cig_params *req;
	struct net_buf *buf;
	struct net_buf *rsp;
	int i, err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_CIG_PARAMS,
				sizeof(*req) + sizeof(*cis) * param->num_conns);
	if (!buf) {
		return NULL;
	}

	req = net_buf_add(buf, sizeof(*req));

	memset(req, 0, sizeof(*req));

	req->cig_id = param->conns[0]->iso.cig_id;
	sys_put_le24(param->chans[0]->qos->interval, req->m_interval);
	sys_put_le24(param->chans[0]->qos->interval, req->s_interval);
	req->sca = param->chans[0]->qos->sca;
	req->packing = param->chans[0]->qos->packing;
	req->framing = param->chans[0]->qos->framing;
	req->m_latency = sys_cpu_to_le16(param->chans[0]->qos->latency);
	req->s_latency = sys_cpu_to_le16(param->chans[0]->qos->latency);
	req->num_cis = param->num_conns;

	/* Program the cis parameters */
	for (i = 0; i < param->num_conns; i++) {
		cis = net_buf_add(buf, sizeof(*cis));

		memset(cis, 0, sizeof(*cis));

		cis->cis_id = param->conns[i]->iso.cis_id;

		switch (param->chans[i]->qos->dir) {
		case BT_ISO_CHAN_QOS_IN:
			cis->m_sdu = param->chans[i]->qos->sdu;
			break;
		case BT_ISO_CHAN_QOS_OUT:
			cis->s_sdu = param->chans[i]->qos->sdu;
			break;
		case BT_ISO_CHAN_QOS_INOUT:
			cis->m_sdu = param->chans[i]->qos->sdu;
			cis->s_sdu = param->chans[i]->qos->sdu;
			break;
		}

		cis->m_phy = param->chans[i]->qos->phy;
		cis->s_phy = param->chans[i]->qos->phy;
		cis->m_rtn = param->chans[i]->qos->rtn;
		cis->s_rtn = param->chans[i]->qos->rtn;
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CIG_PARAMS, buf, &rsp);
	if (err) {
		return NULL;
	}

	return rsp;
}

int bt_conn_bind_iso(struct bt_iso_create_param *param)
{
	struct bt_conn *conn;
	struct net_buf *rsp;
	struct bt_hci_rp_le_set_cig_params *cig_rsp;
	int i, err;

	/* Check if controller is ISO capable */
	if (!BT_FEAT_LE_CIS_MASTER(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (!param->num_conns || param->num_conns > CONFIG_BT_MAX_ISO_CONN) {
		return -EINVAL;
	}

	/* Assign ISO connections to each LE connection */
	for (i = 0; i < param->num_conns; i++) {
		conn = param->conns[i];

		if (conn->type != BT_CONN_TYPE_LE) {
			err = -EINVAL;
			goto failed;
		}

		conn = bt_conn_add_iso(conn);
		if (!conn) {
			err = -ENOMEM;
			goto failed;
		}

		conn->iso.cig_id = param->id;
		conn->iso.cis_id = bt_conn_index(conn);

		param->conns[i] = conn;
	}

	rsp = hci_le_set_cig_params(param);
	if (!rsp) {
		err = -EIO;
		goto failed;
	}

	cig_rsp = (void *)rsp->data;

	if (rsp->len < sizeof(cig_rsp) ||
	    cig_rsp->num_handles != param->num_conns) {
		BT_WARN("Unexpected response to hci_le_set_cig_params");
		err = -EIO;
	}

	for (i = 0; i < cig_rsp->num_handles; i++) {
		/* Assign the connection handle */
		param->conns[i]->handle = cig_rsp->handle[i];
	}

	net_buf_unref(rsp);

	return 0;

failed:
	for (i = 0; i < param->num_conns; i++) {
		bt_iso_cleanup(param->conns[i]);
	}

	return err;
}

static int hci_le_create_cis(struct bt_conn **conn, uint8_t num_conns)
{
	struct bt_hci_cis *cis;
	struct bt_hci_cp_le_create_cis *req;
	struct net_buf *buf;
	int i;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CREATE_CIS,
				sizeof(*req) + sizeof(*cis) * num_conns);
	if (!buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(buf, sizeof(*req));

	memset(req, 0, sizeof(*req));

	req->num_cis = num_conns;

	/* Program the cis parameters */
	for (i = 0; i < num_conns; i++) {
		cis = net_buf_add(buf, sizeof(*cis));

		memset(cis, 0, sizeof(*cis));

		cis->cis_handle = sys_cpu_to_le16(conn[i]->handle);
		cis->acl_handle = sys_cpu_to_le16(conn[i]->iso.acl->handle);
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CREATE_CIS, buf, NULL);
}

int bt_conn_connect_iso(struct bt_conn **conns, uint8_t num_conns)
{
	int i, err;

	/* Check if controller is ISO capable */
	if (!BT_FEAT_LE_CIS_MASTER(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (num_conns > CONFIG_BT_MAX_ISO_CONN) {
		return -EINVAL;
	}

	for (i = 0; i < num_conns; i++) {
		if (conns[i]->type != BT_CONN_TYPE_ISO) {
			return -EINVAL;
		}
	}

	err = hci_le_create_cis(conns, num_conns);
	if (err) {
		return err;
	}

	/* Set connection state */
	for (i = 0; i < num_conns; i++) {
		bt_conn_set_state(conns[i], BT_CONN_CONNECT);
	}

	return 0;
}

static int hci_le_setup_iso_data_path(struct bt_conn *conn,
				      struct bt_iso_data_path *path)
{
	struct bt_hci_cp_le_setup_iso_path *cp;
	struct bt_hci_rp_le_setup_iso_path *rp;
	struct net_buf *buf, *rsp;
	uint8_t *cc;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SETUP_ISO_PATH, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->path_dir = path->dir;
	cp->path_id = path->pid;
	cp->coding_format = path->path->format;
	cp->company_id = sys_cpu_to_le16(path->path->cid);
	cp->vendor_id = sys_cpu_to_le16(path->path->vid);
	sys_put_le24(path->path->delay, cp->controller_delay);
	cp->codec_config_len = path->path->cc_len;
	cc = net_buf_add(buf, cp->codec_config_len);
	memcpy(cc, path->path->cc, cp->codec_config_len);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SETUP_ISO_PATH, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;
	if (rp->status || (rp->handle != conn->handle)) {
		err = -EIO;
	}

	net_buf_unref(rsp);

	return err;
}

static int hci_le_remove_iso_data_path(struct bt_conn *conn, uint8_t dir)
{
	struct bt_hci_cp_le_remove_iso_path *cp;
	struct bt_hci_rp_le_remove_iso_path *rp;
	struct net_buf *buf, *rsp;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_REMOVE_ISO_PATH, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = conn->handle;
	cp->path_dir = dir;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_REMOVE_ISO_PATH, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;
	if (rp->status || (rp->handle != conn->handle)) {
		err = -EIO;
	}

	net_buf_unref(rsp);

	return err;
}

static void bt_iso_chan_add(struct bt_conn *conn, struct bt_iso_chan *chan)
{
	/* Attach channel to the connection */
	sys_slist_append(&conn->channels, &chan->node);
	chan->conn = conn;

	BT_DBG("conn %p chan %p", conn, chan);
}

int bt_iso_accept(struct bt_conn *conn)
{
	struct bt_iso_chan *chan;
	int err;

	__ASSERT_NO_MSG(conn->type == BT_CONN_TYPE_ISO);

	BT_DBG("%p", conn);

	if (!iso_server) {
		return -ENOMEM;
	}

	err = iso_server->accept(conn, &chan);
	if (err < 0) {
		BT_ERR("err %d", err);
		return err;
	}

	bt_iso_chan_add(conn, chan);
	bt_iso_chan_set_state(chan, BT_ISO_BOUND);

	return 0;
}

static int bt_iso_setup_data_path(struct bt_conn *conn)
{
	int err;
	struct bt_iso_chan *chan;
	struct bt_iso_chan_path path = {};
	struct bt_iso_data_path out_path = { .dir = BT_ISO_CHAN_CTRL_TO_HOST };
	struct bt_iso_data_path in_path = { .dir = BT_ISO_CHAN_HOST_TO_CTRL };

	chan = SYS_SLIST_PEEK_HEAD_CONTAINER(&conn->channels, chan, node);
	if (!chan) {
		return -EINVAL;
	}

	in_path.path = chan->path ? chan->path : &path;
	out_path.path = chan->path ? chan->path : &path;

	switch (chan->qos->dir) {
	case BT_ISO_CHAN_QOS_IN:
		in_path.pid = in_path.path->pid;
		out_path.pid = BT_ISO_DATA_PATH_DISABLED;
		break;
	case BT_ISO_CHAN_QOS_OUT:
		in_path.pid = BT_ISO_DATA_PATH_DISABLED;
		out_path.pid = out_path.path->pid;
		break;
	case BT_ISO_CHAN_QOS_INOUT:
		in_path.pid = in_path.path->pid;
		out_path.pid = out_path.path->pid;
		break;
	default:
		return -EINVAL;
	}

	err = hci_le_setup_iso_data_path(conn, &in_path);
	if (err) {
		return err;
	}

	return hci_le_setup_iso_data_path(conn, &out_path);
}

void bt_iso_connected(struct bt_conn *conn)
{
	struct bt_iso_chan *chan;

	__ASSERT_NO_MSG(conn->type == BT_CONN_TYPE_ISO);

	BT_DBG("%p", conn);

	if (bt_iso_setup_data_path(conn)) {
		BT_ERR("Unable to setup data path");
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (chan->ops->connected) {
			chan->ops->connected(chan);
		}

		bt_iso_chan_set_state(chan, BT_ISO_CONNECTED);
	}
}

static void bt_iso_remove_data_path(struct bt_conn *conn)
{
	BT_DBG("%p", conn);

	/* Remove both directions */
	hci_le_remove_iso_data_path(conn, BT_ISO_CHAN_CTRL_TO_HOST);
	hci_le_remove_iso_data_path(conn, BT_ISO_CHAN_HOST_TO_CTRL);
}

void bt_iso_disconnected(struct bt_conn *conn)
{
	struct bt_iso_chan *chan, *next;

	__ASSERT_NO_MSG(conn->type == BT_CONN_TYPE_ISO);

	BT_DBG("%p", conn);

	if (sys_slist_is_empty(&conn->channels)) {
		return;
	}

	bt_iso_remove_data_path(conn);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&conn->channels, chan, next, node) {
		if (chan->ops->disconnected) {
			chan->ops->disconnected(chan);
		}

		chan->conn = NULL;
		bt_iso_chan_set_state(chan, BT_ISO_DISCONNECTED);
	}
}

int bt_iso_server_register(struct bt_iso_server *server)
{
	__ASSERT_NO_MSG(server);

	/* Check if controller is ISO capable */
	if (!BT_FEAT_LE_CIS_SLAVE(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (iso_server) {
		return -EADDRINUSE;
	}

	if (!server->accept) {
		return -EINVAL;
	}

	if (server->sec_level > BT_SECURITY_L3) {
		return -EINVAL;
	} else if (server->sec_level < BT_SECURITY_L1) {
		/* Level 0 is only applicable for BR/EDR */
		server->sec_level = BT_SECURITY_L1;
	}

	BT_DBG("%p", server);

	iso_server = server;

	return 0;
}

#if defined(CONFIG_BT_AUDIO_DEBUG_ISO)
const char *bt_iso_chan_state_str(uint8_t state)
{
	switch (state) {
	case BT_ISO_DISCONNECTED:
		return "disconnected";
	case BT_ISO_BOUND:
		return "bound";
	case BT_ISO_CONNECT:
		return "connect";
	case BT_ISO_CONNECTED:
		return "connected";
	case BT_ISO_DISCONNECT:
		return "disconnect";
	default:
		return "unknown";
	}
}

void bt_iso_chan_set_state_debug(struct bt_iso_chan *chan, uint8_t state,
				 const char *func, int line)
{
	BT_DBG("chan %p conn %p %s -> %s", chan, chan->conn,
	       bt_iso_chan_state_str(chan->state),
	       bt_iso_chan_state_str(state));

	/* check transitions validness */
	switch (state) {
	case BT_ISO_DISCONNECTED:
		/* regardless of old state always allows this state */
		break;
	case BT_ISO_BOUND:
		if (chan->state != BT_ISO_DISCONNECTED) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_ISO_CONNECT:
		if (chan->state != BT_ISO_BOUND) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_ISO_CONNECTED:
		if (chan->state != BT_ISO_BOUND &&
		    chan->state != BT_ISO_CONNECT) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_ISO_DISCONNECT:
		if (chan->state != BT_ISO_CONNECTED) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	default:
		BT_ERR("%s()%d: unknown (%u) state was set", func, line, state);
		return;
	}

	chan->state = state;
}
#else
void bt_iso_chan_set_state(struct bt_iso_chan *chan, uint8_t state)
{
	chan->state = state;
}
#endif /* CONFIG_BT_AUDIO_DEBUG_ISO */

void bt_iso_chan_remove(struct bt_conn *conn, struct bt_iso_chan *chan)
{
	struct bt_iso_chan *c;
	sys_snode_t *prev = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, c, node) {
		if (c == chan) {
			sys_slist_remove(&conn->channels, prev, &chan->node);
			return;
		}

		prev = &chan->node;
	}
}

int bt_iso_chan_bind(struct bt_conn **conns, uint8_t num_conns,
		     struct bt_iso_chan **chans)
{
	struct bt_iso_create_param param;
	int i, err;
	static uint8_t id;

	__ASSERT_NO_MSG(conns);
	__ASSERT_NO_MSG(num_conns);
	__ASSERT_NO_MSG(chans);

	memset(&param, 0, sizeof(param));

	param.id = id++;
	param.num_conns = num_conns;
	param.conns = conns;
	param.chans = chans;

	err = bt_conn_bind_iso(&param);
	if (err) {
		return err;
	}

	/* Bind respective connection to channel */
	for (i = 0; i < num_conns; i++) {
		bt_iso_chan_add(conns[i], chans[i]);
		bt_iso_chan_set_state(chans[i], BT_ISO_BOUND);
	}

	return 0;
}

int bt_iso_chan_connect(struct bt_iso_chan **chans, uint8_t num_chans)
{
	struct bt_conn *conns[CONFIG_BT_MAX_ISO_CONN];
	int i, err;

	__ASSERT_NO_MSG(chans);
	__ASSERT_NO_MSG(num_chans);

	for (i = 0; i < num_chans; i++) {
		if (!chans[i]->conn) {
			return -ENOTCONN;
		}

		conns[i] = chans[i]->conn;
	}

	err = bt_conn_connect_iso(conns, num_chans);
	if (err) {
		return err;
	}

	for (i = 0; i < num_chans; i++) {
		bt_iso_chan_set_state(chans[i], BT_ISO_CONNECT);
	}

	return 0;
}

int bt_iso_chan_disconnect(struct bt_iso_chan *chan)
{
	__ASSERT_NO_MSG(chan);

	if (!chan->conn) {
		return -ENOTCONN;
	}

	if (chan->state == BT_ISO_BOUND) {
		bt_iso_chan_set_state(chan, BT_ISO_DISCONNECTED);
		bt_conn_unref(chan->conn);
		chan->conn = NULL;
		return 0;
	}

	return bt_conn_disconnect(chan->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

void bt_iso_recv(struct bt_conn *conn, struct net_buf *buf, uint8_t flags)
{
	struct bt_hci_iso_data_hdr *hdr;
	struct bt_iso_chan *chan;
	uint8_t pb, ts;
	uint16_t len;

	pb = bt_iso_flags_pb(flags);
	ts = bt_iso_flags_ts(flags);

	BT_DBG("handle %u len %u flags 0x%02x pb 0x%02x ts 0x%02x",
	       conn->handle, buf->len, flags, pb, ts);

	/* When the PB_Flag does not equal 0b00, the fields Time_Stamp,
	 * Packet_Sequence_Number, Packet_Status_Flag and ISO_SDU_Length
	 * are omitted from the HCI ISO Data packet.
	 */
	switch (pb) {
	case BT_ISO_START:
	case BT_ISO_SINGLE:
		/* The ISO_Data_Load field contains either the first fragment
		 * of an SDU or a complete SDU.
		 */
		if (ts) {
			struct bt_hci_iso_ts_data_hdr *ts_hdr;

			ts_hdr = net_buf_pull_mem(buf, sizeof(*ts_hdr));
			iso(buf)->ts = sys_le32_to_cpu(ts_hdr->ts);

			hdr = &ts_hdr->data;
		} else {
			hdr = net_buf_pull_mem(buf, sizeof(*hdr));
			/* TODO: Generate a timestamp? */
			iso(buf)->ts = 0x00000000;
		}

		len = sys_le16_to_cpu(hdr->slen);
		flags = bt_iso_pkt_flags(len);
		len = bt_iso_pkt_len(len);

		/* TODO: Drop the packet if NOP? */

		BT_DBG("%s, len %u total %u flags 0x%02x timestamp %u",
		       pb == BT_ISO_START ? "Start" : "Single", buf->len, len,
		       flags, iso(buf)->ts);

		if (conn->rx_len) {
			BT_ERR("Unexpected ISO %s fragment",
			       pb == BT_ISO_START ? "Start" : "Single");
			bt_conn_reset_rx_state(conn);
		}

		conn->rx_len = len - buf->len;
		if (conn->rx_len) {
			if (pb == BT_ISO_SINGLE) {
				BT_ERR("Unexpected ISO single fragment");
				bt_conn_reset_rx_state(conn);
			}
			conn->rx = buf;
			return;
		}
		break;

	case BT_ISO_CONT:
		/* The ISO_Data_Load field contains a continuation fragment of
		 * an SDU.
		 */
		if (!conn->rx_len) {
			BT_ERR("Unexpected ISO continuation fragment");
			bt_conn_reset_rx_state(conn);
			net_buf_unref(buf);
			return;
		}

		BT_DBG("Cont, len %u rx_len %u", buf->len, conn->rx_len);

		if (buf->len > net_buf_tailroom(conn->rx)) {
			BT_ERR("Not enough buffer space for ISO data");
			bt_conn_reset_rx_state(conn);
			net_buf_unref(buf);
			return;
		}

		net_buf_add_mem(conn->rx, buf->data, buf->len);
		conn->rx_len -= buf->len;
		net_buf_unref(buf);
		return;

	case BT_ISO_END:
		/* The ISO_Data_Load field contains the last fragment of an
		 * SDU.
		 */
		BT_DBG("End, len %u rx_len %u", buf->len, conn->rx_len);

		if (buf->len > net_buf_tailroom(conn->rx)) {
			BT_ERR("Not enough buffer space for ISO data");
			bt_conn_reset_rx_state(conn);
			net_buf_unref(buf);
			return;
		}

		net_buf_add_mem(conn->rx, buf->data, buf->len);
		conn->rx_len -= buf->len;
		net_buf_unref(buf);

		if (conn->rx_len) {
			BT_ERR("Unexpected ISO end fragment");
			return;
		}

		buf = conn->rx;
		conn->rx = NULL;
		conn->rx_len = 0U;

		break;
	default:
		BT_ERR("Unexpected ISO pb flags (0x%02x)", pb);
		bt_conn_reset_rx_state(conn);
		net_buf_unref(buf);
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (chan->ops->recv) {
			chan->ops->recv(chan, buf);
		}
	}
}

int bt_iso_chan_send(struct bt_iso_chan *chan, struct net_buf *buf)
{
	struct bt_hci_iso_data_hdr *hdr;
	static uint16_t sn;

	__ASSERT_NO_MSG(chan);
	__ASSERT_NO_MSG(buf);

	BT_DBG("chan %p len %zu", chan, net_buf_frags_len(buf));

	if (!chan->conn) {
		return -ENOTCONN;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->sn = sys_cpu_to_le16(sn++);
	hdr->slen = sys_cpu_to_le16(bt_iso_pkt_len_pack(net_buf_frags_len(buf)
							- sizeof(*hdr),
							BT_ISO_DATA_VALID));

	return bt_conn_send(chan->conn, buf);
}
