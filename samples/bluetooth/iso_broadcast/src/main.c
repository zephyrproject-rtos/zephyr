/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>

#define BT_LE_EXT_ADV_CUSTOM BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
					     BT_LE_ADV_OPT_USE_NAME, \
					     0x0040, \
					     0x0040, \
					     NULL)

#define BT_LE_PER_ADV_CUSTOM BT_LE_PER_ADV_PARAM(0x0050, \
						 0x0050, \
						 BT_LE_PER_ADV_OPT_NONE)

#define BUF_ALLOC_TIMEOUT (10) /* milliseconds */
#define BIG_TERMINATE_TIMEOUT_US (60 * USEC_PER_SEC) /* microseconds */
#define BIG_SDU_INTERVAL_US (10000)

#define BIS_ISO_CHAN_COUNT 2
NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, BIS_ISO_CHAN_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);

static K_SEM_DEFINE(sem_big_cmplt, 0, BIS_ISO_CHAN_COUNT);
static K_SEM_DEFINE(sem_big_term, 0, BIS_ISO_CHAN_COUNT);

#define INITIAL_TIMEOUT_COUNTER (BIG_TERMINATE_TIMEOUT_US / BIG_SDU_INTERVAL_US)

static uint16_t seq_num;

static void iso_connected(struct bt_iso_chan *chan)
{
	printk("ISO Channel %p connected\n", chan);

	seq_num = 0U;

	k_sem_give(&sem_big_cmplt);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	printk("ISO Channel %p disconnected with reason 0x%02x\n",
	       chan, reason);
	k_sem_give(&sem_big_term);
}

static struct bt_iso_chan_ops iso_ops = {
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
};

static struct bt_iso_chan_io_qos iso_tx_qos = {
	.sdu = CONFIG_BT_ISO_TX_MTU, /* bytes */
	.rtn = 2U,
	.phy = BT_GAP_LE_PHY_2M,
};

static struct bt_iso_chan_qos bis_iso_qos = {
	.tx = &iso_tx_qos,
};

static struct bt_iso_chan bis_iso_chan[] = {
	{ .ops = &iso_ops, .qos = &bis_iso_qos, },
	{ .ops = &iso_ops, .qos = &bis_iso_qos, },
};

static struct bt_iso_chan *bis[] = {
	&bis_iso_chan[0],
	&bis_iso_chan[1],
};

static struct bt_iso_big_create_param big_create_param = {
	.num_bis = BIS_ISO_CHAN_COUNT,
	.bis_channels = bis,
	.interval = BIG_SDU_INTERVAL_US, /* in microseconds */
	.latency = 10, /* in milliseconds */
	.packing = 0, /* 0 - sequential, 1 - interleaved */
	.framing = 0, /* 0 - unframed, 1 - framed */
};

#if defined(CONFIG_BT_CENTRAL)
#define BT_LE_SCAN_CUSTOM \
	BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_PASSIVE, \
			 BT_LE_SCAN_OPT_NONE, \
			 0x0010, \
			 0x0010)

#define BT_CONN_LE_CREATE_CONN_CUSTOM  \
	BT_CONN_LE_CREATE_PARAM(BT_CONN_LE_OPT_NONE, \
				0x0010, \
				0x0010)

#define BT_LE_CONN_PARAM_CUSTOM \
	BT_LE_CONN_PARAM(0x0010, 0x0010, 0, 12)

#define NAME_LEN 30

static K_SEM_DEFINE(sem_peer_addr, 0, 1);
static K_SEM_DEFINE(sem_peer_conn, 0, 1);
static K_SEM_DEFINE(sem_peer_disc, 0, 2);
static bt_addr_le_t peer_addr;

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		memcpy(name, data->data, MIN(data->data_len, NAME_LEN - 1));
		return false;
	default:
		return true;
	}
}

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0: return "No packets";
	case BT_GAP_LE_PHY_1M: return "LE 1M";
	case BT_GAP_LE_PHY_2M: return "LE 2M";
	case BT_GAP_LE_PHY_CODED: return "LE Coded";
	default: return "Unknown";
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printk("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i %s "
	       "C:%u S:%u D:%u SR:%u E:%u Prim: %s, Secn: %s, "
	       "Interval: 0x%04x (%u ms), SID: %u\n",
	       le_addr, info->adv_type, info->tx_power, info->rssi, name,
	       (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
	       phy2str(info->primary_phy), phy2str(info->secondary_phy),
	       info->interval, info->interval * 5 / 4, info->sid);

	bt_addr_le_copy(&peer_addr, info->addr);
	k_sem_give(&sem_peer_addr);
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		struct bt_conn_info conn_info;

		printk("Failed to connect to %s (%u)\n", addr, err);

		err = bt_conn_get_info(conn, &conn_info);
		if (err) {
			printk("Failed to get connection info (%d).\n", err);
			return;
		}

		printk("%s: %s role %u\n", __func__, addr, conn_info.role);

		if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
			bt_conn_unref(conn);
		}

		return;
	}

	printk("Connected: %s\n", addr);

	k_sem_give(&sem_peer_conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info conn_info;
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		printk("Failed to get connection info (%d).\n", err);
		return;
	}

	printk("%s: %s role %u\n", __func__, addr, conn_info.role);

	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		bt_conn_unref(conn);
	}

	k_sem_give(&sem_peer_disc);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};
#endif /* CONFIG_BT_CENTRAL */

void main(void)
{
	uint32_t timeout_counter = INITIAL_TIMEOUT_COUNTER;
	struct bt_le_ext_adv *adv;
	struct bt_iso_big *big;
	int err;

	uint32_t iso_send_count = 0;
	uint8_t iso_data[CONFIG_BT_ISO_TX_MTU] = { 0 };

	printk("Starting ISO Broadcast Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

#if defined(CONFIG_BT_CENTRAL)
	printk("Scan callbacks register...");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("success.\n");
#endif /* CONFIG_BT_CENTRAL */

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CUSTOM, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_CUSTOM);
	if (err) {
		printk("Failed to set periodic advertising parameters"
		       " (err %d)\n", err);
		return;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return;
	}

	/* Create BIG */
	err = bt_iso_big_create(adv, &big_create_param, &big);
	if (err) {
		printk("Failed to create BIG (err %d)\n", err);
		return;
	}

	for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT; chan++) {
		printk("Waiting for BIG complete chan %u...\n", chan);
		err = k_sem_take(&sem_big_cmplt, K_FOREVER);
		if (err) {
			printk("failed (err %d)\n", err);
			return;
		}
		printk("BIG create complete chan %u.\n", chan);
	}

#if defined(CONFIG_BT_CENTRAL)
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct bt_conn *conn;

		printk("Start scanning...");
		err = bt_le_scan_start(BT_LE_SCAN_CUSTOM, NULL);
		if (err) {
			printk("Could not start scan: %d\n", err);
			return;
		}
		printk("success.\n");

		printk("Waiting for advertising report...\n");
		err = k_sem_take(&sem_peer_addr, K_FOREVER);
		if (err) {
			printk("failed (err %d)\n", err);
			return;
		}
		printk("Found peer advertising.\n");

		printk("Stop scanning... ");
		err = bt_le_scan_stop();
		if (err) {
			printk("Could not stop scan: %d\n", err);
			return;
		}
		printk("success.\n");

		printk("Create connection...");
		err = bt_conn_le_create(&peer_addr, BT_CONN_LE_CREATE_CONN_CUSTOM,
					BT_LE_CONN_PARAM_CUSTOM, &conn);
		if (err) {
			printk("Create connection failed (0x%x)\n", err);
		}
		printk("success.\n");

		printk("Waiting for connection %d...", i);
		err = k_sem_take(&sem_peer_conn, K_FOREVER);
		if (err) {
			printk("failed (err %d)\n", err);
			return;
		}
		printk("connected to peer device %d.\n", i);
	}
#endif /* CONFIG_BT_CENTRAL */

	printk("Start ISO data send...\n");
	while (true) {
		int ret;

		k_sleep(K_USEC(big_create_param.interval));

		for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT; chan++) {
			struct net_buf *buf;

			buf = net_buf_alloc(&bis_tx_pool,
					    K_MSEC(BUF_ALLOC_TIMEOUT));
			if (!buf) {
				printk("Data buffer allocate timeout on channel"
				       " %u\n", chan);
				return;
			}
			net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
			sys_put_le32(iso_send_count, iso_data);
			net_buf_add_mem(buf, iso_data, sizeof(iso_data));
			ret = bt_iso_chan_send(&bis_iso_chan[chan], buf,
					       seq_num, BT_ISO_TIMESTAMP_NONE);
			if (ret < 0) {
				printk("Unable to broadcast data on channel %u"
				       " : %d", chan, ret);
				net_buf_unref(buf);
				return;
			}

		}

		if ((iso_send_count % CONFIG_ISO_PRINT_INTERVAL) == 0) {
			printk("Sending value %u\n", iso_send_count);
		}

		iso_send_count++;
		seq_num++;

		timeout_counter--;
		if (!timeout_counter) {
			timeout_counter = INITIAL_TIMEOUT_COUNTER;

			printk("BIG Terminate...");
			err = bt_iso_big_terminate(big);
			if (err) {
				printk("failed (err %d)\n", err);
				return;
			}
			printk("done.\n");

			for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT;
			     chan++) {
				printk("Waiting for BIG terminate complete"
				       " chan %u...\n", chan);
				err = k_sem_take(&sem_big_term, K_FOREVER);
				if (err) {
					printk("failed (err %d)\n", err);
					return;
				}
				printk("BIG terminate complete chan %u.\n",
				       chan);
			}

			printk("Create BIG...");
			err = bt_iso_big_create(adv, &big_create_param, &big);
			if (err) {
				printk("failed (err %d)\n", err);
				return;
			}
			printk("done.\n");

			for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT;
			     chan++) {
				printk("Waiting for BIG complete chan %u...\n",
				       chan);
				err = k_sem_take(&sem_big_cmplt, K_FOREVER);
				if (err) {
					printk("failed (err %d)\n", err);
					return;
				}
				printk("BIG create complete chan %u.\n", chan);
			}
		}
	}
}
