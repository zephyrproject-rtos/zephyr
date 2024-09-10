/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>

#define BUF_ALLOC_TIMEOUT (20) /* milliseconds */
#define BIG_TERMINATE_TIMEOUT_US (60 * USEC_PER_SEC) /* microseconds */
#define BIG_SDU_INTERVAL_US (10000)

#define BIS_ISO_CHAN_COUNT 2
NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, BIS_ISO_CHAN_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static K_SEM_DEFINE(sem_big_cmplt, 0, BIS_ISO_CHAN_COUNT);
static K_SEM_DEFINE(sem_big_term, 0, BIS_ISO_CHAN_COUNT);
static K_SEM_DEFINE(sem_iso_data, CONFIG_BT_ISO_TX_BUF_COUNT,
				   CONFIG_BT_ISO_TX_BUF_COUNT);

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

static void iso_sent(struct bt_iso_chan *chan)
{
	k_sem_give(&sem_iso_data);
}

static struct bt_iso_chan_ops iso_ops = {
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
	.sent           = iso_sent,
};

static struct bt_iso_chan_io_qos iso_tx_qos = {
	.sdu = sizeof(uint32_t), /* bytes */
	.rtn = 1,
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

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

int main(void)
{
	uint32_t timeout_counter = INITIAL_TIMEOUT_COUNTER;
	struct bt_le_ext_adv *adv;
	struct bt_iso_big *big;
	int err;

	uint32_t iso_send_count = 0;
	uint8_t iso_data[sizeof(iso_send_count)] = { 0 };

	printk("Starting ISO Broadcast Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	/* Create a non-connectable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return 0;
	}

	/* Set advertising data to have complete local name set */
	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return 0;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters"
		       " (err %d)\n", err);
		return 0;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return 0;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return 0;
	}

	/* Create BIG */
	err = bt_iso_big_create(adv, &big_create_param, &big);
	if (err) {
		printk("Failed to create BIG (err %d)\n", err);
		return 0;
	}

	for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT; chan++) {
		printk("Waiting for BIG complete chan %u...\n", chan);
		err = k_sem_take(&sem_big_cmplt, K_FOREVER);
		if (err) {
			printk("failed (err %d)\n", err);
			return 0;
		}
		printk("BIG create complete chan %u.\n", chan);
	}

	while (true) {
		for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT; chan++) {
			struct net_buf *buf;
			int ret;

			buf = net_buf_alloc(&bis_tx_pool,
					    K_MSEC(BUF_ALLOC_TIMEOUT));
			if (!buf) {
				printk("Data buffer allocate timeout on channel"
				       " %u\n", chan);
				return 0;
			}

			ret = k_sem_take(&sem_iso_data,
					 K_MSEC(BUF_ALLOC_TIMEOUT));
			if (ret) {
				printk("k_sem_take for ISO data sent failed\n");
				net_buf_unref(buf);
				return 0;
			}

			net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
			sys_put_le32(iso_send_count, iso_data);
			net_buf_add_mem(buf, iso_data, sizeof(iso_data));
			ret = bt_iso_chan_send(&bis_iso_chan[chan], buf, seq_num);
			if (ret < 0) {
				printk("Unable to broadcast data on channel %u"
				       " : %d", chan, ret);
				net_buf_unref(buf);
				return 0;
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
				return 0;
			}
			printk("done.\n");

			for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT;
			     chan++) {
				printk("Waiting for BIG terminate complete"
				       " chan %u...\n", chan);
				err = k_sem_take(&sem_big_term, K_FOREVER);
				if (err) {
					printk("failed (err %d)\n", err);
					return 0;
				}
				printk("BIG terminate complete chan %u.\n",
				       chan);
			}

			printk("Create BIG...");
			err = bt_iso_big_create(adv, &big_create_param, &big);
			if (err) {
				printk("failed (err %d)\n", err);
				return 0;
			}
			printk("done.\n");

			for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT;
			     chan++) {
				printk("Waiting for BIG complete chan %u...\n",
				       chan);
				err = k_sem_take(&sem_big_cmplt, K_FOREVER);
				if (err) {
					printk("failed (err %d)\n", err);
					return 0;
				}
				printk("BIG create complete chan %u.\n", chan);
			}
		}
	}
}
