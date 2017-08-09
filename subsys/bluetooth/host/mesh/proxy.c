/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/byteorder.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_PROXY)
#include "common/log.h"

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "prov.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"

#define PDU_TYPE(data)     (data[0] & BIT_MASK(6))
#define PDU_SAR(data)      (data[0] >> 6)

#define SAR_COMPLETE       0x00
#define SAR_FIRST          0x01
#define SAR_CONT           0x02
#define SAR_LAST           0x03

#define CFG_FILTER_SET     0x00
#define CFG_FILTER_ADD     0x01
#define CFG_FILTER_REMOVE  0x02
#define CFG_FILTER_STATUS  0x03

#define PDU_HDR(sar, type) (sar << 6 | (type & BIT_MASK(6)))

#define CLIENT_BUF_SIZE 68

static const struct bt_le_adv_param slow_adv_param = {
	.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME),
	.interval_min = BT_GAP_ADV_SLOW_INT_MIN,
	.interval_max = BT_GAP_ADV_SLOW_INT_MAX,
};

static const struct bt_le_adv_param fast_adv_param = {
	.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME),
	.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
	.interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
};

static const struct bt_le_adv_param *proxy_adv_param = &fast_adv_param;

static bool proxy_adv_enabled;

#if defined(CONFIG_BT_MESH_GATT_PROXY)
static void proxy_send_beacons(struct k_work *work);
#endif

static struct bt_mesh_proxy_client {
	struct bt_conn *conn;
	u16_t filter[CONFIG_BT_MESH_PROXY_FILTER_SIZE];
	enum __packed {
		NONE,
		WHITELIST,
		BLACKLIST,
		PROV,
	} filter_type;
	u8_t msg_type;
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	struct k_work send_beacons;
#endif
	struct net_buf_simple    buf;
	u8_t                     buf_data[CLIENT_BUF_SIZE];
} clients[CONFIG_BT_MAX_CONN] = {
	[0 ... (CONFIG_BT_MAX_CONN - 1)] = {
#if defined(CONFIG_BT_MESH_GATT_PROXY)
		.send_beacons = _K_WORK_INITIALIZER(proxy_send_beacons),
#endif
		.buf.size = CLIENT_BUF_SIZE,
	},
};

/* Track which service is enabled */
static enum {
	MESH_GATT_NONE,
	MESH_GATT_PROV,
	MESH_GATT_PROXY,
} gatt_svc = MESH_GATT_NONE;

static struct bt_mesh_proxy_client *find_client(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].conn == conn) {
			return &clients[i];
		}
	}

	return NULL;
}

#if defined(CONFIG_BT_MESH_GATT_PROXY)
static int proxy_segment_and_send(struct bt_conn *conn, u8_t type,
				  struct net_buf_simple *msg);

static int filter_set(struct bt_mesh_proxy_client *client,
		      struct net_buf_simple *buf)
{
	u8_t type;

	if (buf->len < 1) {
		BT_WARN("Too short Filter Set message");
		return -EINVAL;
	}

	type = net_buf_simple_pull_u8(buf);
	BT_DBG("type 0x%02x", type);

	switch (type) {
	case 0x00:
		memset(client->filter, 0, sizeof(client->filter));
		client->filter_type = WHITELIST;
		break;
	case 0x01:
		memset(client->filter, 0, sizeof(client->filter));
		client->filter_type = BLACKLIST;
		break;
	default:
		BT_WARN("Prohibited Filter Type 0x%02x", type);
		return -EINVAL;
	}

	return 0;
}

static void filter_add(struct bt_mesh_proxy_client *client, u16_t addr)
{
	int i;

	BT_DBG("addr 0x%02x", addr);

	if (addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] == addr) {
			return;
		}
	}

	for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] == BT_MESH_ADDR_UNASSIGNED) {
			client->filter[i] = addr;
			return;
		}
	}
}

static void filter_remove(struct bt_mesh_proxy_client *client, u16_t addr)
{
	int i;

	BT_DBG("addr 0x%02x", addr);

	if (addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] == addr) {
			client->filter[i] = BT_MESH_ADDR_UNASSIGNED;
			return;
		}
	}
}

static void send_filter_status(struct bt_mesh_proxy_client *client,
			       struct bt_mesh_net_rx *rx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_net_tx tx = {
		.sub = rx->sub,
		.ctx = &rx->ctx,
		.src = bt_mesh_primary_addr(),
	};
	u16_t filter_size;
	int i, err;

	/* Configuration messages always have dst unassigned */
	tx.ctx->addr = BT_MESH_ADDR_UNASSIGNED;

	net_buf_simple_init(buf, 10);

	net_buf_simple_add_u8(buf, CFG_FILTER_STATUS);

	if (client->filter_type == WHITELIST) {
		net_buf_simple_add_u8(buf, 0x00);
	} else {
		net_buf_simple_add_u8(buf, 0x01);
	}

	for (filter_size = 0, i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] != BT_MESH_ADDR_UNASSIGNED) {
			filter_size++;
		}
	}

	net_buf_simple_add_be16(buf, filter_size);

	BT_DBG("%u bytes: %s", buf->len, bt_hex(buf->data, buf->len));

	err = bt_mesh_net_encode(&tx, buf, true);
	if (err) {
		BT_ERR("Encoding Proxy cfg message failed (err %d)", err);
		return;
	}

	err = proxy_segment_and_send(client->conn, BT_MESH_PROXY_CONFIG, buf);
	if (err) {
		BT_ERR("Failed to send proxy cfg message (err %d)", err);
	}
}

static void proxy_cfg(struct bt_mesh_proxy_client *client)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(29);
	struct bt_mesh_net_rx rx;
	u8_t opcode;
	int err;

	err = bt_mesh_net_decode(&client->buf, BT_MESH_NET_IF_PROXY_CFG,
				 &rx, buf, NULL);
	if (err) {
		BT_ERR("Failed to decode Proxy Configuration (err %d)", err);
		return;
	}

	BT_DBG("%u bytes: %s", buf->len, bt_hex(buf->data, buf->len));

	if (buf->len < 1) {
		BT_WARN("Too short proxy configuration PDU");
		return;
	}

	opcode = net_buf_simple_pull_u8(buf);
	switch (opcode) {
	case CFG_FILTER_SET:
		filter_set(client, buf);
		send_filter_status(client, &rx, buf);
		break;
	case CFG_FILTER_ADD:
		while (buf->len >= 2) {
			u16_t addr;

			addr = net_buf_simple_pull_be16(buf);
			filter_add(client, addr);
		}
		send_filter_status(client, &rx, buf);
		break;
	case CFG_FILTER_REMOVE:
		while (buf->len >= 2) {
			u16_t addr;

			addr = net_buf_simple_pull_be16(buf);
			filter_remove(client, addr);
		}
		send_filter_status(client, &rx, buf);
		break;
	default:
		BT_WARN("Unhandled configuration OpCode 0x%02x", opcode);
		break;
	}
}

static int beacon_send(struct bt_conn *conn, struct bt_mesh_subnet *sub)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(23);

	net_buf_simple_init(buf, 1);
	bt_mesh_beacon_create(sub, buf);

	return proxy_segment_and_send(conn, BT_MESH_PROXY_BEACON, buf);
}

static void proxy_send_beacons(struct k_work *work)
{
	struct bt_mesh_proxy_client *client;
	int i;

	client = CONTAINER_OF(work, struct bt_mesh_proxy_client, send_beacons);

	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		struct bt_mesh_subnet *sub = &bt_mesh.sub[i];

		if (sub->net_idx != BT_MESH_KEY_UNUSED) {
			beacon_send(client->conn, sub);
		}
	}
}

void bt_mesh_proxy_beacon_send(struct bt_mesh_subnet *sub)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].conn) {
			beacon_send(clients[i].conn, sub);
		}
	}
}

int bt_mesh_proxy_identity_enable(void)
{
	/* FIXME: Add support for multiple subnets */
	struct bt_mesh_subnet *sub = &bt_mesh.sub[0];

	BT_DBG("");

	if (!bt_mesh_is_provisioned()) {
		return -EAGAIN;
	}

	if (sub->net_idx == BT_MESH_KEY_UNUSED) {
		return -ENOENT;
	}

	if (sub->node_id == BT_MESH_NODE_IDENTITY_NOT_SUPPORTED) {
		return -ENOTSUP;
	}

	if (sub->node_id == BT_MESH_NODE_IDENTITY_RUNNING) {
		return 0;
	}

	sub->node_id = BT_MESH_NODE_IDENTITY_RUNNING;
	bt_mesh_adv_update();

	return 0;
}

#endif /* GATT_PROXY */

static void proxy_complete_pdu(struct bt_mesh_proxy_client *client)
{
	switch (client->msg_type) {
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	case BT_MESH_PROXY_NET_PDU:
		BT_DBG("Mesh Network PDU");
		bt_mesh_net_recv(&client->buf, 0, BT_MESH_NET_IF_PROXY);
		break;
	case BT_MESH_PROXY_BEACON:
		BT_DBG("Mesh Beacon PDU");
		bt_mesh_beacon_recv(&client->buf);
		break;
	case BT_MESH_PROXY_CONFIG:
		BT_DBG("Mesh Configuration PDU");
		proxy_cfg(client);
		break;
#endif
#if defined(CONFIG_BT_MESH_PB_GATT)
	case BT_MESH_PROXY_PROV:
		BT_DBG("Mesh Provisioning PDU");
		bt_mesh_pb_gatt_recv(client->conn, &client->buf);
		break;
#endif
	default:
		BT_WARN("Unhandled Message Type 0x%02x", client->msg_type);
		break;
	}

	net_buf_simple_init(&client->buf, 0);
}

#define ATTR_IS_PROV(attr) (attr->user_data != NULL)

static ssize_t proxy_recv(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, const void *buf,
			  u16_t len, u16_t offset, u8_t flags)
{
	struct bt_mesh_proxy_client *client = find_client(conn);
	const u8_t *data = buf;

	if (!client) {
		return -ENOTCONN;
	}

	if (len < 1) {
		BT_WARN("Too small Proxy PDU");
		return -EINVAL;
	}

	if (ATTR_IS_PROV(attr) != (PDU_TYPE(data) == BT_MESH_PROXY_PROV)) {
		BT_WARN("Proxy PDU type doesn't match GATT service");
		return -EINVAL;
	}

	if (len - 1 > net_buf_simple_tailroom(&client->buf)) {
		BT_WARN("Too big proxy PDU");
		return -EINVAL;
	}

	switch (PDU_SAR(data)) {
	case SAR_COMPLETE:
		if (client->buf.len) {
			BT_WARN("Complete PDU while a pending incomplete one");
			return -EINVAL;
		}

		client->msg_type = PDU_TYPE(data);
		net_buf_simple_add_mem(&client->buf, data + 1, len - 1);
		proxy_complete_pdu(client);
		break;

	case SAR_FIRST:
		if (client->buf.len) {
			BT_WARN("First PDU while a pending incomplete one");
			return -EINVAL;
		}

		client->msg_type = PDU_TYPE(data);
		net_buf_simple_add_mem(&client->buf, data + 1, len - 1);
		break;

	case SAR_CONT:
		if (!client->buf.len) {
			BT_WARN("Continuation with no prior data");
			return -EINVAL;
		}

		if (client->msg_type != PDU_TYPE(data)) {
			BT_WARN("Unexpected message type in continuation");
			return -EINVAL;
		}

		net_buf_simple_add_mem(&client->buf, data + 1, len - 1);
		break;

	case SAR_LAST:
		if (!client->buf.len) {
			BT_WARN("Last SAR PDU with no prior data");
			return -EINVAL;
		}

		if (client->msg_type != PDU_TYPE(data)) {
			BT_WARN("Unexpected message type in last SAR PDU");
			return -EINVAL;
		}

		net_buf_simple_add_mem(&client->buf, data + 1, len - 1);
		proxy_complete_pdu(client);
		break;
	}

	return len;
}

static void proxy_connected(struct bt_conn *conn, u8_t err)
{
	struct bt_mesh_proxy_client *client;
	int i;

	BT_DBG("conn %p err 0x%02x", conn, err);

	/* Since we use ADV_OPT_ONE_TIME */
	proxy_adv_enabled = false;

#if CONFIG_BT_MAX_CONN > 1
	/* Try to re-enable advertising in case it's possible */
	bt_mesh_adv_update();
#endif

	for (client = NULL, i = 0; i < ARRAY_SIZE(clients); i++) {
		if (!clients[i].conn) {
			client = &clients[i];
			break;
		}
	}

	if (!client) {
		BT_ERR("No free Proxy Client objects");
		return;
	}

	client->conn = bt_conn_ref(conn);
	client->filter_type = NONE;
	memset(client->filter, 0, sizeof(client->filter));
	net_buf_simple_init(&client->buf, 0);
}

static void proxy_disconnected(struct bt_conn *conn, u8_t reason)
{
	int i;

	BT_DBG("conn %p reason 0x%02x", conn, reason);

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];

		if (client->conn == conn) {
			if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) &&
			    client->filter_type == PROV) {
				bt_mesh_pb_gatt_close(conn);
			}

			bt_conn_unref(client->conn);
			client->conn = NULL;
			break;
		}
	}

	bt_mesh_adv_update();
}

struct net_buf_simple *bt_mesh_proxy_get_buf(void)
{
	struct net_buf_simple *buf = &clients[0].buf;

	net_buf_simple_init(buf, 0);

	return buf;
}

#if defined(CONFIG_BT_MESH_PB_GATT)
static ssize_t prov_ccc_write(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, u16_t len,
			      u16_t offset, u8_t flags)
{
	struct bt_mesh_proxy_client *client;
	u16_t value;

	BT_DBG("len %u: %s", len, bt_hex(buf, len));

	if (len != sizeof(value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	value = sys_get_le16(buf);
	if (value != BT_GATT_CCC_NOTIFY) {
		BT_WARN("Client wrote 0x%04x instead enabling notify", value);
		return len;
	}

	/* If a connection exists there must be a client */
	client = find_client(conn);
	__ASSERT(client, "No client for connection");

	if (client->filter_type == NONE) {
		client->filter_type = PROV;
		bt_mesh_pb_gatt_open(conn);
	}

	return len;
}

/* Mesh Provisioning Service Declaration */
static struct bt_gatt_attr prov_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_MESH_PROV),

	BT_GATT_CHARACTERISTIC(BT_UUID_MESH_PROV_DATA_IN,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(BT_UUID_MESH_PROV_DATA_IN, BT_GATT_PERM_WRITE,
			   NULL, proxy_recv, (void *)1),

	BT_GATT_CHARACTERISTIC(BT_UUID_MESH_PROV_DATA_OUT,
			       BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_MESH_PROV_DATA_OUT, BT_GATT_PERM_NONE,
			   NULL, NULL, NULL),
	/* Add custom CCC as clients need to be tracked individually */
	BT_GATT_DESCRIPTOR(BT_UUID_GATT_CCC, BT_GATT_PERM_WRITE, NULL,
			   prov_ccc_write, NULL),
};

static struct bt_gatt_service prov_svc = BT_GATT_SERVICE(prov_attrs);

int bt_mesh_proxy_prov_enable(void)
{
	int i;

	BT_DBG("");

	bt_gatt_service_register(&prov_svc);
	gatt_svc = MESH_GATT_PROV;
	proxy_adv_param = &fast_adv_param;

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].conn) {
			clients[i].filter_type = PROV;
		}
	}


	return 0;
}

int bt_mesh_proxy_prov_disable(void)
{
	int i;

	BT_DBG("");

	bt_gatt_service_unregister(&prov_svc);
	gatt_svc = MESH_GATT_NONE;

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];

		if (clients->conn && client->filter_type == PROV) {
			bt_mesh_pb_gatt_close(client->conn);
			client->filter_type = NONE;
		}
	}

	return 0;
}

#endif /* CONFIG_BT_MESH_PB_GATT */



#if defined(CONFIG_BT_MESH_GATT_PROXY)
static ssize_t proxy_ccc_write(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       const void *buf, u16_t len,
			       u16_t offset, u8_t flags)
{
	struct bt_mesh_proxy_client *client;
	u16_t value;

	BT_DBG("len %u: %s", len, bt_hex(buf, len));

	if (len != sizeof(value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	value = sys_get_le16(buf);
	if (value != BT_GATT_CCC_NOTIFY) {
		BT_WARN("Client wrote 0x%04x instead enabling notify", value);
		return len;
	}

	/* If a connection exists there must be a client */
	client = find_client(conn);
	__ASSERT(client, "No client for connection");

	if (client->filter_type == NONE) {
		client->filter_type = WHITELIST;
		k_work_submit(&client->send_beacons);
	}

	return len;
}

/* Mesh Proxy Service Declaration */
static struct bt_gatt_attr proxy_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_MESH_PROXY),

	BT_GATT_CHARACTERISTIC(BT_UUID_MESH_PROXY_DATA_IN,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(BT_UUID_MESH_PROXY_DATA_IN, BT_GATT_PERM_WRITE,
			   NULL, proxy_recv, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_MESH_PROXY_DATA_OUT,
			       BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_MESH_PROXY_DATA_OUT, BT_GATT_PERM_NONE,
			   NULL, NULL, NULL),
	/* Add custom CCC as clients need to be tracked individually */
	BT_GATT_DESCRIPTOR(BT_UUID_GATT_CCC, BT_GATT_PERM_WRITE, NULL,
			   proxy_ccc_write, NULL),
};

static struct bt_gatt_service proxy_svc = BT_GATT_SERVICE(proxy_attrs);

int bt_mesh_proxy_gatt_enable(void)
{
	int i;

	BT_DBG("");

	bt_gatt_service_register(&proxy_svc);
	gatt_svc = MESH_GATT_PROXY;

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].conn) {
			clients[i].filter_type = WHITELIST;
		}
	}

	return 0;
}

int bt_mesh_proxy_gatt_disable(void)
{
	int i;

	BT_DBG("");

	bt_gatt_service_unregister(&proxy_svc);
	gatt_svc = MESH_GATT_NONE;

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];

		if (clients->conn && (client->filter_type == WHITELIST ||
				      client->filter_type == BLACKLIST)) {
			client->filter_type = NONE;
		}
	}

	return 0;
}

void bt_mesh_proxy_addr_add(struct net_buf_simple *buf, u16_t addr)
{
	struct bt_mesh_proxy_client *client =
		CONTAINER_OF(buf, struct bt_mesh_proxy_client, buf);

	BT_DBG("filter_type %u addr 0x%04x", client->filter_type, addr);

	if (client->filter_type == WHITELIST) {
		filter_add(client, addr);
	} else if (client->filter_type == BLACKLIST) {
		filter_remove(client, addr);
	}
}

static bool client_filter_match(struct bt_mesh_proxy_client *client,
				u16_t addr)
{
	int i;

	BT_DBG("filter_type %u addr 0x%04x", client->filter_type, addr);

	if (client->filter_type == WHITELIST) {
		for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
			if (client->filter[i] == addr) {
				return true;
			}
		}

		return false;
	}

	if (client->filter_type == BLACKLIST) {
		for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
			if (client->filter[i] == addr) {
				return false;
			}
		}

		return true;
	}

	return false;
}

bool bt_mesh_proxy_relay(struct net_buf_simple *buf, u16_t dst)
{
	bool relayed = false;
	int i;

	BT_DBG("%u bytes to dst 0x%04x", buf->len, dst);

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];
		struct net_buf_simple *msg = NET_BUF_SIMPLE(32);

		if (!client->conn) {
			continue;
		}

		if (!client_filter_match(client, dst)) {
			continue;
		}

		/* Proxy PDU sending modifies the original buffer,
		 * so we need to make a copy.
		 */
		net_buf_simple_init(msg, 1);
		net_buf_simple_add_mem(msg, buf->data, buf->len);

		bt_mesh_proxy_send(client->conn, BT_MESH_PROXY_NET_PDU, msg);
		relayed = true;
	}

	return relayed;
}

#endif /* CONFIG_BT_MESH_GATT_PROXY */

static int proxy_send(struct bt_conn *conn, const void *data, u16_t len)
{
	BT_DBG("%u bytes: %s", len, bt_hex(data, len));

#if defined(CONFIG_BT_MESH_GATT_PROXY)
	if (gatt_svc == MESH_GATT_PROXY) {
		return bt_gatt_notify(conn, &proxy_attrs[4], data, len);
	}
#endif

#if defined(CONFIG_BT_MESH_PB_GATT)
	if (gatt_svc == MESH_GATT_PROV) {
		return bt_gatt_notify(conn, &prov_attrs[4], data, len);
	}
#endif

	return 0;
}

static int proxy_segment_and_send(struct bt_conn *conn, u8_t type,
				  struct net_buf_simple *msg)
{
	u16_t mtu;

	BT_DBG("conn %p type 0x%02x len %u: %s", conn, type, msg->len,
	       bt_hex(msg->data, msg->len));

	/* ATT_MTU - OpCode (1 byte) - Handle (2 bytes) */
	mtu = bt_gatt_get_mtu(conn) - 3;
	if (mtu > msg->len) {
		net_buf_simple_push_u8(msg, PDU_HDR(SAR_COMPLETE, type));
		return proxy_send(conn, msg->data, msg->len);
	}

	net_buf_simple_push_u8(msg, PDU_HDR(SAR_FIRST, type));
	proxy_send(conn, msg->data, mtu);
	net_buf_simple_pull(msg, mtu);

	while (msg->len) {
		if (msg->len + 1 < mtu) {
			net_buf_simple_push_u8(msg, PDU_HDR(SAR_LAST, type));
			proxy_send(conn, msg->data, msg->len);
			break;
		}

		net_buf_simple_push_u8(msg, PDU_HDR(SAR_CONT, type));
		proxy_send(conn, msg->data, mtu);
		net_buf_simple_pull(msg, mtu);
	}

	return 0;
}

int bt_mesh_proxy_send(struct bt_conn *conn, u8_t type,
		       struct net_buf_simple *msg)
{
	struct bt_mesh_proxy_client *client = find_client(conn);

	if (!client) {
		BT_ERR("No Proxy Client found");
		return -ENOTCONN;
	}

	if ((client->filter_type == PROV) != (type == BT_MESH_PROXY_PROV)) {
		BT_ERR("Invalid PDU type for Proxy Client");
		return -EINVAL;
	}

	return proxy_segment_and_send(conn, type, msg);
}

#if defined(CONFIG_BT_MESH_PB_GATT)
static u8_t prov_svc_data[20] = { 0x27, 0x18, };

static const struct bt_data prov_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x27, 0x18),
	BT_DATA(BT_DATA_SVC_DATA16, prov_svc_data, sizeof(prov_svc_data)),
};

static const struct bt_data prov_sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		(sizeof(CONFIG_BT_DEVICE_NAME) - 1)),
};
#endif /* PB_GATT */

#if defined(CONFIG_BT_MESH_GATT_PROXY)
static s64_t node_id_start;

#define ID_TYPE_NET  0x00
#define ID_TYPE_NODE 0x01

#define NODE_ID_LEN  19
#define NET_ID_LEN   11

static u8_t proxy_svc_data[NODE_ID_LEN] = { 0x28, 0x18, };

static const struct bt_data node_id_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x28, 0x18),
	BT_DATA(BT_DATA_SVC_DATA16, proxy_svc_data, NODE_ID_LEN),
};

static const struct bt_data net_id_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x28, 0x18),
	BT_DATA(BT_DATA_SVC_DATA16, proxy_svc_data, NET_ID_LEN),
};

static int node_id_adv(struct bt_mesh_subnet *sub)
{
	u8_t tmp[16];
	int err;

	BT_DBG("");

	proxy_svc_data[2] = ID_TYPE_NODE;

	err = bt_rand(proxy_svc_data + 11, 8);
	if (err) {
		return err;
	}

	memset(tmp, 0, 6);
	memcpy(tmp + 6, proxy_svc_data + 11, 8);
	sys_put_be16(bt_mesh_primary_addr(), tmp + 14);

	err = bt_encrypt_be(sub->keys[sub->kr_flag].identity, tmp, tmp);
	if (err) {
		return err;
	}

	memcpy(proxy_svc_data + 3, tmp + 8, 8);

	err = bt_le_adv_start(proxy_adv_param, node_id_ad,
			      ARRAY_SIZE(node_id_ad), NULL, 0);
	if (err) {
		BT_ERR("Failed to advertise using Node ID (err %d)", err);
		return err;
	}

	proxy_adv_enabled = true;

	return 0;
}

static int net_id_adv(struct bt_mesh_subnet *sub)
{
	int err;

	BT_DBG("");

	proxy_svc_data[2] = ID_TYPE_NET;

	BT_DBG("Advertising with NetId %s",
	       bt_hex(sub->keys[sub->kr_flag].net_id, 8));

	memcpy(proxy_svc_data + 3, sub->keys[sub->kr_flag].net_id, 8);

	err = bt_le_adv_start(proxy_adv_param,
			      net_id_ad, ARRAY_SIZE(net_id_ad), NULL, 0);
	if (err) {
		BT_ERR("Failed to advertise using Network ID (err %d)", err);
		return err;
	}

	proxy_adv_enabled = true;

	return 0;
}

static s32_t gatt_proxy_advertise(void)
{
	/* TODO: Add support for multiple subnets */
	struct bt_mesh_subnet *sub = &bt_mesh.sub[0];
	s32_t remaining = K_FOREVER;

	BT_DBG("");

	if (sub->net_idx == BT_MESH_KEY_UNUSED) {
		BT_WARN("First subnet is not valid");
		return remaining;
	}

	if (node_id_start) {
		s64_t active = k_uptime_get() - node_id_start;

		BT_DBG("Node Id active for %lld ms", active);

		if (active < K_SECONDS(60)) {
			remaining = K_SECONDS(60) - active;
		} else {
			sub->node_id = BT_MESH_NODE_IDENTITY_STOPPED;
			node_id_start = 0;
		}
	}

	if (sub->node_id == BT_MESH_NODE_IDENTITY_RUNNING) {
		proxy_adv_param = &fast_adv_param;
		if (node_id_adv(sub) == 0 && !node_id_start) {
			node_id_start = k_uptime_get();
			remaining = K_SECONDS(60);
		}
	} else if (bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED) {
		proxy_adv_param = &slow_adv_param;
		net_id_adv(sub);
	}

	return remaining;
}
#endif /* GATT_PROXY */

s32_t bt_mesh_proxy_adv_start(void)
{
	BT_DBG("");

#if defined(CONFIG_BT_MESH_PB_GATT)
	if (!bt_mesh_is_provisioned()) {
		if (bt_le_adv_start(proxy_adv_param,
				    prov_ad, ARRAY_SIZE(prov_ad),
				    prov_sd, ARRAY_SIZE(prov_sd)) == 0) {
			proxy_adv_enabled = true;
			if (proxy_adv_param == &fast_adv_param) {
				proxy_adv_param = &slow_adv_param;
				return K_SECONDS(60);
			}
		}
	}
#endif /* PB_GATT */

#if defined(CONFIG_BT_MESH_GATT_PROXY)
	if (bt_mesh_is_provisioned()) {
		return gatt_proxy_advertise();
	}
#endif /* GATT_PROXY */

	return K_FOREVER;
}

void bt_mesh_proxy_adv_stop(void)
{
	int err;

	BT_DBG("adv_enabled %u", proxy_adv_enabled);

	if (!proxy_adv_enabled) {
		return;
	}

	err = bt_le_adv_stop();
	if (err) {
		BT_ERR("Failed to stop advertising (err %d)", err);
	} else {
		proxy_adv_enabled = false;
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = proxy_connected,
	.disconnected = proxy_disconnected,
};

int bt_mesh_proxy_init(void)
{
	bt_conn_cb_register(&conn_callbacks);

#if defined(CONFIG_BT_MESH_PB_GATT)
	memcpy(prov_svc_data + 2, bt_mesh_prov_get_uuid(), 16);
#endif

	return 0;
}
