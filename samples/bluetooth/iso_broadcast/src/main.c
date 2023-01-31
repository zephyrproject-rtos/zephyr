/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>

#define BUF_ALLOC_TIMEOUT   (10) /* milliseconds */
#define BIG_SDU_INTERVAL_US (5000)
#define BIG_SDU_LATENCY_MS  (5)

#define BIG_COUNT CONFIG_BT_ISO_MAX_BIG
#define BIS_ISO_CHAN_COUNT 1
NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);

static K_SEM_DEFINE(sem_big_cmplt, 0, BIS_ISO_CHAN_COUNT * BIG_COUNT);
static K_SEM_DEFINE(sem_big_term, 0, BIS_ISO_CHAN_COUNT * BIG_COUNT);

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

static struct bt_iso_chan_io_qos iso_tx_qos[] = {
	{
	.sdu = CONFIG_BT_ISO_TX_MTU, /* bytes */
	.rtn = 2,
	.phy = BT_GAP_LE_PHY_2M,
	},
	{
	.sdu = CONFIG_BT_ISO_TX_MTU, /* bytes */
	.rtn = 2,
	.phy = BT_GAP_LE_PHY_2M,
	},
	{
	.sdu = CONFIG_BT_ISO_TX_MTU, /* bytes */
	.rtn = 2,
	.phy = BT_GAP_LE_PHY_2M,
	},
	{
	.sdu = CONFIG_BT_ISO_TX_MTU, /* bytes */
	.rtn = 2,
	.phy = BT_GAP_LE_PHY_2M,
	},
};

static struct bt_iso_chan_qos bis_iso_qos[] = {
	{
	.tx = &iso_tx_qos[0],
	},
	{
	.tx = &iso_tx_qos[1],
	},
	{
	.tx = &iso_tx_qos[2],
	},
	{
	.tx = &iso_tx_qos[3],
	},
};

static struct bt_iso_chan bis_iso_chan[] = {
	{ .ops = &iso_ops, .qos = &bis_iso_qos[0], },
	{ .ops = &iso_ops, .qos = &bis_iso_qos[1], },
	{ .ops = &iso_ops, .qos = &bis_iso_qos[2], },
	{ .ops = &iso_ops, .qos = &bis_iso_qos[3], },
};

static struct bt_iso_chan *bis[][2] = {
	{
	&bis_iso_chan[0],
	&bis_iso_chan[1],
	},
	{
	&bis_iso_chan[2],
	&bis_iso_chan[3],
	},
};

static struct bt_iso_big_create_param big_create_param[] = {
	{
	.num_bis = BIS_ISO_CHAN_COUNT,
	.bis_channels = bis[0],
	.interval = BIG_SDU_INTERVAL_US, /* in microseconds */
	.latency = BIG_SDU_LATENCY_MS, /* in milliseconds */
	.packing = 0, /* 0 - sequential, 1 - interleaved */
	.framing = 0, /* 0 - unframed, 1 - framed */
	},
	{
	.num_bis = BIS_ISO_CHAN_COUNT,
	.bis_channels = bis[1],
	.interval = BIG_SDU_INTERVAL_US, /* in microseconds */
	.latency = BIG_SDU_LATENCY_MS, /* in milliseconds */
	.packing = 0, /* 0 - sequential, 1 - interleaved */
	.framing = 0, /* 0 - unframed, 1 - framed */
	},
};

void main(void)
{
	struct bt_le_ext_adv *adv[BIG_COUNT];
	struct bt_iso_big *big[BIG_COUNT];
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

	for (int i = 0; i < BIG_COUNT ; i++) {
		/* Create a non-connectable non-scannable advertising set */
		err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, &adv[i]);
		if (err) {
			printk("Failed to create advertising set (err %d)\n", err);
			return;
		}

		/* Set periodic advertising parameters */
		err = bt_le_per_adv_set_param(adv[i], BT_LE_PER_ADV_DEFAULT);
		if (err) {
			printk("Failed to set periodic advertising parameters"
			       " (err %d)\n", err);
			return;
		}

		/* Enable Periodic Advertising */
		err = bt_le_per_adv_start(adv[i]);
		if (err) {
			printk("Failed to enable periodic advertising (err %d)\n", err);
			return;
		}

		/* Start extended advertising */
		err = bt_le_ext_adv_start(adv[i], BT_LE_EXT_ADV_START_DEFAULT);
		if (err) {
			printk("Failed to start extended advertising (err %d)\n", err);
			return;
		}

		/* Create BIG */
		err = bt_iso_big_create(adv[i], &big_create_param[i], &big[i]);
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
	}

	while (true) {
		int ret;

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
	}
}
