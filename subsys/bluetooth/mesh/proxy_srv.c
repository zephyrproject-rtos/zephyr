/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>

#include "common/bt_str.h"

#include "mesh.h"
#include "net.h"
#include "rpl.h"
#include "transport.h"
#include "prov.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"
#include "proxy_msg.h"
#include "crypto.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROXY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_gatt);

#define PROXY_SVC_INIT_TIMEOUT K_MSEC(10)
#define PROXY_SVC_REG_ATTEMPTS 5

/* Interval to update random value in (10 minutes).
 *
 * Defined in the Bluetooth Mesh Specification v1.1, Section 7.2.2.2.4.
 */
#define PROXY_RANDOM_UPDATE_INTERVAL (10 * 60 * MSEC_PER_SEC)

#define ADV_OPT_ADDR(private) (IS_ENABLED(CONFIG_BT_MESH_DEBUG_USE_ID_ADDR) ?                      \
			       BT_LE_ADV_OPT_USE_IDENTITY : (private) ? BT_LE_ADV_OPT_USE_NRPA : 0)

#define ADV_OPT_PROXY(private)                                                                     \
	(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_SCANNABLE | ADV_OPT_ADDR(private))

static void proxy_send_beacons(struct k_work *work);
static int proxy_send(struct bt_conn *conn,
		      const void *data, uint16_t len,
		      bt_gatt_complete_func_t end, void *user_data);

static struct bt_mesh_proxy_client {
	struct bt_mesh_proxy_role *cli;
	uint16_t filter[CONFIG_BT_MESH_PROXY_FILTER_SIZE];
	enum __packed {
		NONE,
		ACCEPT,
		REJECT,
	} filter_type;
	struct k_work send_beacons;
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	bool privacy;
#endif
} clients[CONFIG_BT_MAX_CONN] = {
	[0 ... (CONFIG_BT_MAX_CONN - 1)] = {
		.send_beacons = Z_WORK_INITIALIZER(proxy_send_beacons),
	},
};

static bool service_registered;

static struct bt_mesh_proxy_client *find_client(struct bt_conn *conn)
{
	return &clients[bt_conn_index(conn)];
}

static ssize_t gatt_recv(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr, const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;

	if (len < 1) {
		LOG_WRN("Too small Proxy PDU");
		return -EINVAL;
	}

	if (PDU_TYPE(data) == BT_MESH_PROXY_PROV) {
		LOG_WRN("Proxy PDU type doesn't match GATT service");
		return -EINVAL;
	}

	return bt_mesh_proxy_msg_recv(conn, buf, len);
}

/* Next subnet in queue to be advertised */
static struct bt_mesh_subnet *beacon_sub;

static int filter_set(struct bt_mesh_proxy_client *client,
		      struct net_buf_simple *buf)
{
	uint8_t type;

	if (buf->len < 1) {
		LOG_WRN("Too short Filter Set message");
		return -EINVAL;
	}

	type = net_buf_simple_pull_u8(buf);
	LOG_DBG("type 0x%02x", type);

	switch (type) {
	case 0x00:
		(void)memset(client->filter, 0, sizeof(client->filter));
		client->filter_type = ACCEPT;
		break;
	case 0x01:
		(void)memset(client->filter, 0, sizeof(client->filter));
		client->filter_type = REJECT;
		break;
	default:
		LOG_WRN("Prohibited Filter Type 0x%02x", type);
		return -EINVAL;
	}

	return 0;
}

static void filter_add(struct bt_mesh_proxy_client *client, uint16_t addr)
{
	int i;

	LOG_DBG("addr 0x%04x", addr);

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

static void filter_remove(struct bt_mesh_proxy_client *client, uint16_t addr)
{
	int i;

	LOG_DBG("addr 0x%04x", addr);

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
	uint16_t filter_size;
	int i, err;

	/* Configuration messages always have dst unassigned */
	tx.ctx->addr = BT_MESH_ADDR_UNASSIGNED;

	net_buf_simple_reset(buf);
	net_buf_simple_reserve(buf, 10);

	net_buf_simple_add_u8(buf, CFG_FILTER_STATUS);

	if (client->filter_type == ACCEPT) {
		net_buf_simple_add_u8(buf, 0x00);
	} else {
		net_buf_simple_add_u8(buf, 0x01);
	}

	for (filter_size = 0U, i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] != BT_MESH_ADDR_UNASSIGNED) {
			filter_size++;
		}
	}

	net_buf_simple_add_be16(buf, filter_size);

	LOG_DBG("%u bytes: %s", buf->len, bt_hex(buf->data, buf->len));

	err = bt_mesh_net_encode(&tx, buf, BT_MESH_NONCE_PROXY);
	if (err) {
		LOG_ERR("Encoding Proxy cfg message failed (err %d)", err);
		return;
	}

	err = bt_mesh_proxy_msg_send(client->cli->conn, BT_MESH_PROXY_CONFIG,
				     buf, NULL, NULL);
	if (err) {
		LOG_ERR("Failed to send proxy cfg message (err %d)", err);
	}
}

static void proxy_filter_recv(struct bt_conn *conn,
			      struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_proxy_client *client;
	uint8_t opcode;

	client = find_client(conn);

	opcode = net_buf_simple_pull_u8(buf);
	switch (opcode) {
	case CFG_FILTER_SET:
		filter_set(client, buf);
		send_filter_status(client, rx, buf);
		break;
	case CFG_FILTER_ADD:
		while (buf->len >= 2) {
			uint16_t addr;

			addr = net_buf_simple_pull_be16(buf);
			filter_add(client, addr);
		}
		send_filter_status(client, rx, buf);
		break;
	case CFG_FILTER_REMOVE:
		while (buf->len >= 2) {
			uint16_t addr;

			addr = net_buf_simple_pull_be16(buf);
			filter_remove(client, addr);
		}
		send_filter_status(client, rx, buf);
		break;
	default:
		LOG_WRN("Unhandled configuration OpCode 0x%02x", opcode);
		break;
	}
}

static void proxy_cfg(struct bt_mesh_proxy_role *role)
{
	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_NET_MAX_PDU_LEN);
	struct bt_mesh_net_rx rx;
	int err;

	err = bt_mesh_net_decode(&role->buf, BT_MESH_NET_IF_PROXY_CFG,
				 &rx, &buf);
	if (err) {
		LOG_ERR("Failed to decode Proxy Configuration (err %d)", err);
		return;
	}

	rx.local_match = 1U;

	if (bt_mesh_rpl_check(&rx, NULL, false)) {
		LOG_WRN("Replay: src 0x%04x dst 0x%04x seq 0x%06x", rx.ctx.addr, rx.ctx.recv_dst,
			rx.seq);
		return;
	}

	/* Remove network headers */
	net_buf_simple_pull(&buf, BT_MESH_NET_HDR_LEN);

	LOG_DBG("%u bytes: %s", buf.len, bt_hex(buf.data, buf.len));

	if (buf.len < 1) {
		LOG_WRN("Too short proxy configuration PDU");
		return;
	}

	proxy_filter_recv(role->conn, &rx, &buf);
}

static void proxy_msg_recv(struct bt_mesh_proxy_role *role)
{
	switch (role->msg_type) {
	case BT_MESH_PROXY_NET_PDU:
		LOG_DBG("Mesh Network PDU");
		bt_mesh_net_recv(&role->buf, 0, BT_MESH_NET_IF_PROXY);
		break;
	case BT_MESH_PROXY_BEACON:
		LOG_DBG("Mesh Beacon PDU");
		bt_mesh_beacon_recv(&role->buf);
		break;
	case BT_MESH_PROXY_CONFIG:
		LOG_DBG("Mesh Configuration PDU");
		proxy_cfg(role);
		break;
	default:
		LOG_WRN("Unhandled Message Type 0x%02x", role->msg_type);
		break;
	}
}

static int beacon_send(struct bt_mesh_proxy_client *client,
		       struct bt_mesh_subnet *sub)
{
	int err;

	NET_BUF_SIMPLE_DEFINE(buf, 28);

	net_buf_simple_reserve(&buf, 1);

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	err = bt_mesh_beacon_create(sub, &buf, client->privacy);
#else
	err = bt_mesh_beacon_create(sub, &buf, false);
#endif
	if (err) {
		return err;
	}

	return bt_mesh_proxy_msg_send(client->cli->conn, BT_MESH_PROXY_BEACON,
				      &buf, NULL, NULL);
}

static bool send_beacon_cb(struct bt_mesh_subnet *sub, void *cb_data)
{
	struct bt_mesh_proxy_client *client = cb_data;

	return beacon_send(client, sub) != 0;
}

static void proxy_send_beacons(struct k_work *work)
{
	struct bt_mesh_proxy_client *client;

	client = CONTAINER_OF(work, struct bt_mesh_proxy_client, send_beacons);

	(void)bt_mesh_subnet_find(send_beacon_cb, client);
}

void bt_mesh_proxy_beacon_send(struct bt_mesh_subnet *sub)
{
	int i;

	if (!sub) {
		/* NULL means we send on all subnets */
		bt_mesh_subnet_foreach(bt_mesh_proxy_beacon_send);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].cli) {
			beacon_send(&clients[i], sub);
		}
	}
}

static void identity_enabled(struct bt_mesh_subnet *sub)
{
	sub->node_id = BT_MESH_NODE_IDENTITY_RUNNING;
	sub->node_id_start = k_uptime_get_32();

	STRUCT_SECTION_FOREACH(bt_mesh_proxy_cb, cb) {
		if (cb->identity_enabled) {
			cb->identity_enabled(sub->net_idx);
		}
	}
}

static void node_id_start(struct bt_mesh_subnet *sub)
{
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	sub->priv_beacon_ctx.node_id = false;
#endif

	identity_enabled(sub);
}

static void private_node_id_start(struct bt_mesh_subnet *sub)
{
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	sub->priv_beacon_ctx.node_id = true;
#endif

	identity_enabled(sub);
}

void bt_mesh_proxy_identity_start(struct bt_mesh_subnet *sub, bool private)
{
	if (private) {
		private_node_id_start(sub);
	} else {
		node_id_start(sub);
	}

	/* Prioritize the recently enabled subnet */
	beacon_sub = sub;
}

void bt_mesh_proxy_identity_stop(struct bt_mesh_subnet *sub)
{
	sub->node_id = BT_MESH_NODE_IDENTITY_STOPPED;
	sub->node_id_start = 0U;

	STRUCT_SECTION_FOREACH(bt_mesh_proxy_cb, cb) {
		if (cb->identity_disabled) {
			cb->identity_disabled(sub->net_idx);
		}
	}
}

int bt_mesh_proxy_identity_enable(void)
{
	LOG_DBG("");

	if (!bt_mesh_is_provisioned()) {
		return -EAGAIN;
	}

	if (bt_mesh_subnet_foreach(node_id_start)) {
		bt_mesh_adv_gatt_update();
	}

	return 0;
}

int bt_mesh_proxy_private_identity_enable(void)
{
	LOG_DBG("");

	if (!IS_ENABLED(CONFIG_BT_MESH_PRIV_BEACONS)) {
		return -ENOTSUP;
	}

	if (!bt_mesh_is_provisioned()) {
		return -EAGAIN;
	}

	if (bt_mesh_subnet_foreach(private_node_id_start)) {
		bt_mesh_adv_gatt_update();
	}

	return 0;
}

#define ENC_ID_LEN  19
#define NET_ID_LEN   11

#define NODE_ID_TIMEOUT (CONFIG_BT_MESH_NODE_ID_TIMEOUT * MSEC_PER_SEC)

static uint8_t proxy_svc_data[ENC_ID_LEN] = {
	BT_UUID_16_ENCODE(BT_UUID_MESH_PROXY_VAL),
};

static const struct bt_data enc_id_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_MESH_PROXY_VAL)),
	BT_DATA(BT_DATA_SVC_DATA16, proxy_svc_data, ENC_ID_LEN),
};

static const struct bt_data net_id_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_MESH_PROXY_VAL)),
	BT_DATA(BT_DATA_SVC_DATA16, proxy_svc_data, NET_ID_LEN),
};

static const struct bt_data sd[] = {
#if defined(CONFIG_BT_MESH_PROXY_USE_DEVICE_NAME)
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
#endif
};

static int randomize_bt_addr(void)
{
	/* TODO: There appears to be no way to force an RPA/NRPA refresh. */
	return 0;
}

static int enc_id_adv(struct bt_mesh_subnet *sub, uint8_t type,
		      uint8_t hash[16], int32_t duration)
{
	struct bt_le_adv_param slow_adv_param = {
		.id = BT_ID_DEFAULT,
		.options = ADV_OPT_PROXY(type == BT_MESH_ID_TYPE_PRIV_NET ||
					 type == BT_MESH_ID_TYPE_PRIV_NODE),
		ADV_SLOW_INT,
	};
	struct bt_le_adv_param fast_adv_param = {
		.id = BT_ID_DEFAULT,
		.options = ADV_OPT_PROXY(type == BT_MESH_ID_TYPE_PRIV_NET ||
					 type == BT_MESH_ID_TYPE_PRIV_NODE),
		ADV_FAST_INT,
	};
	int err;

	err = bt_mesh_encrypt(&sub->keys[SUBNET_KEY_TX_IDX(sub)].identity, hash, hash);
	if (err) {
		return err;
	}

	/* MshPRTv1.1: 7.2.2.2.4: The AdvA field shall be regenerated whenever the Random field is
	 * regenerated.
	 */
	err = randomize_bt_addr();
	if (err) {
		LOG_ERR("AdvA refresh failed: %d", err);
		return err;
	}

	proxy_svc_data[2] = type;
	memcpy(&proxy_svc_data[3], &hash[8], 8);

	err = bt_mesh_adv_gatt_start(
		type == BT_MESH_ID_TYPE_PRIV_NET ? &slow_adv_param : &fast_adv_param,
		duration, enc_id_ad, ARRAY_SIZE(enc_id_ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_WRN("Failed to advertise using type 0x%02x (err %d)", type, err);
		return err;
	}

	return 0;
}

static int node_id_adv(struct bt_mesh_subnet *sub, int32_t duration)
{
	uint8_t *random = &proxy_svc_data[11];
	uint8_t tmp[16];
	int err;

	LOG_DBG("0x%03x", sub->net_idx);

	err = bt_rand(random, 8);
	if (err) {
		return err;
	}

	memset(&tmp[0], 0x00, 6);
	memcpy(&tmp[6], random, 8);
	sys_put_be16(bt_mesh_primary_addr(), &tmp[14]);

	return enc_id_adv(sub, BT_MESH_ID_TYPE_NODE, tmp, duration);
}

static int priv_node_id_adv(struct bt_mesh_subnet *sub, int32_t duration)
{
	uint8_t *random = &proxy_svc_data[11];
	uint8_t tmp[16];
	int err;

	LOG_DBG("0x%03x", sub->net_idx);

	err = bt_rand(random, 8);
	if (err) {
		return err;
	}

	memset(&tmp[0], 0x00, 5);
	tmp[5] = 0x03;
	memcpy(&tmp[6], random, 8);
	sys_put_be16(bt_mesh_primary_addr(), &tmp[14]);

	return enc_id_adv(sub, BT_MESH_ID_TYPE_PRIV_NODE, tmp, duration);
}

static int priv_net_id_adv(struct bt_mesh_subnet *sub, int32_t duration)
{
	uint8_t *random = &proxy_svc_data[11];
	uint8_t tmp[16];
	int err;

	LOG_DBG("0x%03x", sub->net_idx);

	err = bt_rand(random, 8);
	if (err) {
		return err;
	}

	memcpy(&tmp[0], sub->keys[SUBNET_KEY_TX_IDX(sub)].net_id, 8);
	memcpy(&tmp[8], random, 8);

	return enc_id_adv(sub, BT_MESH_ID_TYPE_PRIV_NET, tmp, duration);
}

static int net_id_adv(struct bt_mesh_subnet *sub, int32_t duration)
{
	struct bt_le_adv_param slow_adv_param = {
		.id = BT_ID_DEFAULT,
		.options = ADV_OPT_PROXY(false),
		ADV_SLOW_INT,
	};
	int err;

	proxy_svc_data[2] = BT_MESH_ID_TYPE_NET;

	LOG_DBG("Advertising with NetId %s", bt_hex(sub->keys[SUBNET_KEY_TX_IDX(sub)].net_id, 8));

	memcpy(proxy_svc_data + 3, sub->keys[SUBNET_KEY_TX_IDX(sub)].net_id, 8);

	err = bt_mesh_adv_gatt_start(&slow_adv_param, duration, net_id_ad,
				     ARRAY_SIZE(net_id_ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_WRN("Failed to advertise using Network ID (err %d)", err);
		return err;
	}

	return 0;
}

static bool is_sub_proxy_active(struct bt_mesh_subnet *sub)
{
	if (sub->net_idx == BT_MESH_KEY_UNUSED) {
		return false;
	}

	return (sub->node_id == BT_MESH_NODE_IDENTITY_RUNNING ||
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
		(bt_mesh_od_priv_proxy_get() > 0 && sub->solicited) ||
#endif
		bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED ||
		bt_mesh_priv_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED);
}

static bool active_proxy_sub_cnt_cb(struct bt_mesh_subnet *sub, void *cb_data)
{
	int *cnt = cb_data;

	if (is_sub_proxy_active(sub)) {
		(*cnt)++;
	}

	/* Don't stop until we've visited all subnets.
	 * We're only using the "find" variant of the subnet iteration to get a context parameter.
	 */
	return false;
}

static int active_proxy_sub_cnt_get(void)
{
	int cnt = 0;

	(void)bt_mesh_subnet_find(active_proxy_sub_cnt_cb, &cnt);

	return cnt;
}

static void proxy_adv_timeout_eval(struct bt_mesh_subnet *sub)
{
	int32_t time_passed;

	if (sub->node_id == BT_MESH_NODE_IDENTITY_RUNNING) {
		time_passed = k_uptime_get_32() - sub->node_id_start;
		if (time_passed > (NODE_ID_TIMEOUT - MSEC_PER_SEC)) {
			bt_mesh_proxy_identity_stop(sub);
			LOG_DBG("Node ID stopped for subnet %d after %dms", sub->net_idx,
				time_passed);
		}
	}

#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
	if (bt_mesh_od_priv_proxy_get() > 0 && sub->solicited && sub->priv_net_id_sent) {
		time_passed = k_uptime_get_32() - sub->priv_net_id_sent;
		if (time_passed > ((MSEC_PER_SEC * bt_mesh_od_priv_proxy_get()) - MSEC_PER_SEC)) {
			sub->priv_net_id_sent = 0;
			sub->solicited = false;
			LOG_DBG("Private Network ID stopped for subnet %d after %dms on "
				"solicitation",
				sub->net_idx, time_passed);
		}
	}
#endif
}

enum proxy_adv_evt {
	NET_ID,
	PRIV_NET_ID,
	NODE_ID,
	PRIV_NODE_ID,
	OD_PRIV_NET_ID,
};

struct proxy_adv_request {
	int32_t duration;
	enum proxy_adv_evt evt;
};

static bool proxy_adv_request_get(struct bt_mesh_subnet *sub, struct proxy_adv_request *request)
{
	if (!sub) {
		return false;
	}

	if (sub->net_idx == BT_MESH_KEY_UNUSED) {
		return false;
	}

	/** The priority for proxy adv is first solicitation, then Node Identity,
	 *  and lastly Network ID. Network ID is prioritized last since, in many
	 *  cases, another device can fulfill the same demand. Solicitation is
	 *  prioritized first since legacy devices are dependent on this to
	 *  connect to the network.
	 */

#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
	if (bt_mesh_od_priv_proxy_get() > 0 && sub->solicited) {
		int32_t timeout = MSEC_PER_SEC * (int32_t)bt_mesh_od_priv_proxy_get();

		request->evt = OD_PRIV_NET_ID;
		request->duration = !sub->priv_net_id_sent
					    ? timeout
					    : timeout - (k_uptime_get_32() - sub->priv_net_id_sent);
		return true;
	}
#endif

	if (sub->node_id == BT_MESH_NODE_IDENTITY_RUNNING) {
		request->duration = NODE_ID_TIMEOUT - (k_uptime_get_32() - sub->node_id_start);
		request->evt =
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
			sub->priv_beacon_ctx.node_id ? PRIV_NODE_ID :
#endif
				NODE_ID;

		return true;
	}

	if (bt_mesh_priv_gatt_proxy_get() == BT_MESH_FEATURE_ENABLED) {
		request->evt = PRIV_NET_ID;
		request->duration = PROXY_RANDOM_UPDATE_INTERVAL;
		return true;
	}

	if (bt_mesh_gatt_proxy_get() == BT_MESH_FEATURE_ENABLED) {
		request->evt = NET_ID;
		request->duration = SYS_FOREVER_MS;
		return true;
	}

	return false;
}

static struct bt_mesh_subnet *adv_sub_get_next(struct bt_mesh_subnet *sub_start,
					       struct proxy_adv_request *request)
{
	struct bt_mesh_subnet *sub_temp = bt_mesh_subnet_next(sub_start);

	do {
		if (proxy_adv_request_get(sub_temp, request)) {
			return sub_temp;
		}

		sub_temp = bt_mesh_subnet_next(sub_temp);
	} while (sub_temp != sub_start);

	return NULL;
}

static struct {
	int32_t start;
	struct bt_mesh_subnet *sub;
	struct proxy_adv_request request;
} sub_adv;

static int gatt_proxy_advertise(void)
{
	int err;

	int32_t max_adv_duration = 0;
	int cnt;
	struct bt_mesh_subnet *sub;
	struct proxy_adv_request request;

	LOG_DBG("");

	/* Close proxy activity that has timed out on all subnets */
	bt_mesh_subnet_foreach(proxy_adv_timeout_eval);

	if (!bt_mesh_proxy_has_avail_conn()) {
		LOG_DBG("Connectable advertising deferred (max connections)");
		return -ENOMEM;
	}

	cnt = active_proxy_sub_cnt_get();
	if (!cnt) {
		LOG_DBG("No subnets to advertise proxy on");
		return -ENOENT;
	} else if (cnt > 1) {
		/** There is more than one subnet that requires proxy adv,
		 *  and the adv resources must be shared.
		 */

		/* We use NODE_ID_TIMEOUT as a starting point since it may
		 * be less than 60 seconds. Divide this period into at least
		 * 6 slices, but make sure that a slice is more than one
		 * second long (to avoid excessive rotation).
		 */
		max_adv_duration = NODE_ID_TIMEOUT / MAX(cnt, 6);
		max_adv_duration = MAX(max_adv_duration, MSEC_PER_SEC + 20);

		/* Check if the previous subnet finished its allocated timeslot */
		if ((sub_adv.request.duration != SYS_FOREVER_MS) &&
		    proxy_adv_request_get(sub_adv.sub, &request) &&
		    (sub_adv.request.evt == request.evt)) {
			int32_t time_passed = k_uptime_get_32() - sub_adv.start;

			if (time_passed < sub_adv.request.duration &&
			    ((sub_adv.request.duration - time_passed) >= MSEC_PER_SEC)) {
				sub = sub_adv.sub;
				request.duration = sub_adv.request.duration - time_passed;
				goto end;
			}
		}
	}

	sub = adv_sub_get_next(sub_adv.sub, &request);
	if (!sub) {
		LOG_ERR("Could not find subnet to advertise");
		return -ENOENT;
	}
end:
	if (cnt > 1) {
		request.duration = (request.duration == SYS_FOREVER_MS)
					   ? max_adv_duration
					   : MIN(request.duration, max_adv_duration);
	}

	/* Save current state for next iteration */
	sub_adv.start = k_uptime_get_32();
	sub_adv.sub = sub;
	sub_adv.request = request;

	switch (request.evt) {
	case NET_ID:
		err = net_id_adv(sub, request.duration);
		break;
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
	case OD_PRIV_NET_ID:
		if (!sub->priv_net_id_sent) {
			sub->priv_net_id_sent = k_uptime_get();
		}
		/* Fall through */
#endif
	case PRIV_NET_ID:
		err = priv_net_id_adv(sub, request.duration);
		break;
	case NODE_ID:
		err = node_id_adv(sub, request.duration);
		break;
	case PRIV_NODE_ID:
		err = priv_node_id_adv(sub, request.duration);
		break;
	default:
		LOG_ERR("Unexpected proxy adv evt: %d", request.evt);
		return -ENODEV;
	}

	if (err) {
		LOG_ERR("Advertising proxy failed (err: %d)", err);
		return err;
	}

	LOG_DBG("Advertising %d ms for net_idx 0x%04x", request.duration, sub->net_idx);
	return err;
}

static void subnet_evt(struct bt_mesh_subnet *sub, enum bt_mesh_key_evt evt)
{
	if (evt == BT_MESH_KEY_DELETED) {
		if (sub == beacon_sub) {
			beacon_sub = NULL;
		}
	} else {
		bt_mesh_proxy_beacon_send(sub);
		bt_mesh_adv_gatt_update();
	}
}

BT_MESH_SUBNET_CB_DEFINE(gatt_services) = {
	.evt_handler = subnet_evt,
};

static void proxy_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t proxy_ccc_write(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, uint16_t value)
{
	struct bt_mesh_proxy_client *client;

	LOG_DBG("value: 0x%04x", value);

	if (value != BT_GATT_CCC_NOTIFY) {
		LOG_WRN("Client wrote 0x%04x instead enabling notify", value);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	client = find_client(conn);
	if (client->filter_type == NONE) {
		client->filter_type = ACCEPT;
		bt_mesh_wq_submit(&client->send_beacons);
	}

	return sizeof(value);
}

/* Mesh Proxy Service Declaration */
static struct _bt_gatt_ccc proxy_ccc =
	BT_GATT_CCC_INITIALIZER(proxy_ccc_changed, proxy_ccc_write, NULL);

static struct bt_gatt_attr proxy_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_MESH_PROXY),

	BT_GATT_CHARACTERISTIC(BT_UUID_MESH_PROXY_DATA_IN,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE,
			       NULL, gatt_recv, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_MESH_PROXY_DATA_OUT,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE,
			       NULL, NULL, NULL),
	BT_GATT_CCC_MANAGED(&proxy_ccc,
			    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static struct bt_gatt_service proxy_svc = BT_GATT_SERVICE(proxy_attrs);
static void svc_reg_work_handler(struct k_work *work);
static struct k_work_delayable svc_reg_work = Z_WORK_DELAYABLE_INITIALIZER(svc_reg_work_handler);
static uint32_t svc_reg_attempts;

static void svc_reg_work_handler(struct k_work *work)
{
	int err;

	err = bt_gatt_service_register(&proxy_svc);
	if ((err == -EINVAL) && ((--svc_reg_attempts) > 0)) {
		/* settings_load() didn't finish yet. Try again. */
		(void)k_work_schedule(&svc_reg_work, PROXY_SVC_INIT_TIMEOUT);
		return;
	} else if (err) {
		LOG_ERR("Unable to register Mesh Proxy Service (err %d)", err);
		return;
	}

	service_registered = true;

	for (int i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].cli) {
			clients[i].filter_type = ACCEPT;
		}
	}

	bt_mesh_adv_gatt_update();
}

int bt_mesh_proxy_gatt_enable(void)
{
	int err;

	LOG_DBG("");

	if (!bt_mesh_is_provisioned()) {
		return -ENOTSUP;
	}

	if (service_registered) {
		return -EBUSY;
	}

	svc_reg_attempts = PROXY_SVC_REG_ATTEMPTS;
	err = k_work_schedule(&svc_reg_work, PROXY_SVC_INIT_TIMEOUT);
	if (err < 0) {
		LOG_ERR("Enabling GATT proxy failed (err %d)", err);
		return err;
	}

	return 0;
}

void bt_mesh_proxy_gatt_disconnect(void)
{
	int i;

	LOG_DBG("");

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];

		if (client->cli && (client->filter_type == ACCEPT ||
				     client->filter_type == REJECT)) {
			client->filter_type = NONE;
			bt_conn_disconnect(client->cli->conn,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
	}
}

int bt_mesh_proxy_gatt_disable(void)
{
	LOG_DBG("");

	if (!service_registered) {
		return -EALREADY;
	}

	bt_mesh_proxy_gatt_disconnect();

	bt_gatt_service_unregister(&proxy_svc);
	service_registered = false;

	return 0;
}

void bt_mesh_proxy_addr_add(struct net_buf_simple *buf, uint16_t addr)
{
	struct bt_mesh_proxy_client *client;
	struct bt_mesh_proxy_role *cli =
		CONTAINER_OF(buf, struct bt_mesh_proxy_role, buf);

	client = find_client(cli->conn);

	LOG_DBG("filter_type %u addr 0x%04x", client->filter_type, addr);

	if (client->filter_type == ACCEPT) {
		filter_add(client, addr);
	} else if (client->filter_type == REJECT) {
		filter_remove(client, addr);
	}
}

static bool client_filter_match(struct bt_mesh_proxy_client *client,
				uint16_t addr)
{
	int i;

	LOG_DBG("filter_type %u addr 0x%04x", client->filter_type, addr);

	if (client->filter_type == REJECT) {
		for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
			if (client->filter[i] == addr) {
				return false;
			}
		}

		return true;
	}

	if (addr == BT_MESH_ADDR_ALL_NODES) {
		return true;
	}

	if (client->filter_type == ACCEPT) {
		for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
			if (client->filter[i] == addr) {
				return true;
			}
		}
	}

	return false;
}

bool bt_mesh_proxy_relay(struct bt_mesh_adv *adv, uint16_t dst)
{
	bool relayed = false;
	int i;

	LOG_DBG("%u bytes to dst 0x%04x", adv->b.len, dst);

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];

		if (!client->cli) {
			continue;
		}

		if (!client_filter_match(client, dst)) {
			continue;
		}

		if (bt_mesh_proxy_relay_send(client->cli->conn, adv)) {
			continue;
		}

		relayed = true;
	}

	return relayed;
}

static void solicitation_reset(struct bt_mesh_subnet *sub)
{
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
	sub->solicited = false;
	sub->priv_net_id_sent = 0;
#endif
}

static void gatt_connected(struct bt_conn *conn, uint8_t conn_err)
{
	struct bt_mesh_proxy_client *client;
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err || info.role != BT_CONN_ROLE_PERIPHERAL || !service_registered ||
	    info.id != BT_ID_DEFAULT) {
		return;
	}

	LOG_DBG("conn %p err 0x%02x", (void *)conn, conn_err);

	client = find_client(conn);

	client->filter_type = NONE;
	(void)memset(client->filter, 0, sizeof(client->filter));
	client->cli = bt_mesh_proxy_role_setup(conn, proxy_send,
					       proxy_msg_recv);

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	/* Binding from MshPRTv1.1: 7.2.2.2.6. */
	enum bt_mesh_subnets_node_id_state cur_node_id = bt_mesh_subnets_node_id_state_get();

	if (bt_mesh_gatt_proxy_get() == BT_MESH_FEATURE_ENABLED ||
	    cur_node_id == BT_MESH_SUBNETS_NODE_ID_STATE_ENABLED) {
		client->privacy = false;
	} else {
		client->privacy = (bt_mesh_priv_gatt_proxy_get() == BT_MESH_FEATURE_ENABLED) ||
				  (cur_node_id == BT_MESH_SUBNETS_NODE_ID_STATE_ENABLED_PRIVATE);
	}

	LOG_DBG("privacy: %d", client->privacy);
#endif

	/* If connection was formed after Proxy Solicitation we need to stop future
	 * Private Network ID advertisements
	 */
	bt_mesh_subnet_foreach(solicitation_reset);

	/* Try to re-enable advertising in case it's possible */
	if (bt_mesh_proxy_has_avail_conn()) {
		bt_mesh_adv_gatt_update();
	}
}

static void gatt_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info info;
	struct bt_mesh_proxy_client *client;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err || info.role != BT_CONN_ROLE_PERIPHERAL || info.id != BT_ID_DEFAULT) {
		return;
	}

	if (!service_registered && bt_mesh_is_provisioned()) {
		(void)bt_mesh_proxy_gatt_enable();
		return;
	}

	client = find_client(conn);
	if (client->cli) {
		bt_mesh_proxy_role_cleanup(client->cli);
		client->cli = NULL;
	}
}

static int proxy_send(struct bt_conn *conn,
		      const void *data, uint16_t len,
		      bt_gatt_complete_func_t end, void *user_data)
{
	LOG_DBG("%u bytes: %s", len, bt_hex(data, len));

	struct bt_gatt_notify_params params = {
		.data = data,
		.len = len,
		.attr = &proxy_attrs[3],
		.user_data = user_data,
		.func = end,
	};

	return bt_gatt_notify_cb(conn, &params);
}

int bt_mesh_proxy_adv_start(void)
{
	LOG_DBG("");

	if (!service_registered || !bt_mesh_is_provisioned()) {
		return -ENOTSUP;
	}

	return gatt_proxy_advertise();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = gatt_connected,
	.disconnected = gatt_disconnected,
};

uint8_t bt_mesh_proxy_srv_connected_cnt(void)
{
	uint8_t cnt = 0;

	for (int i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].cli) {
			cnt++;
		}
	}

	return cnt;
}
