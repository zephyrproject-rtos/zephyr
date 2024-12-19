/*
 * Copyright (c) 2021-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>

#define BIG_SDU_INTERVAL_US      (10000)
#define BUF_ALLOC_TIMEOUT_US     (BIG_SDU_INTERVAL_US * 2U) /* milliseconds */
#define BIG_TERMINATE_TIMEOUT_US (60 * USEC_PER_SEC) /* microseconds */

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
static uint32_t ts;
static int32_t  delta;

static void iso_connected(struct bt_iso_chan *chan)
{
	printk("ISO Channel %p connected\n", chan);

	seq_num = 0U;
	ts = 0U;
	delta = 0;

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
	/* Some controllers work best while Extended Advertising interval to be a multiple
	 * of the ISO Interval minus 10 ms (max. advertising random delay). This is
	 * required to place the AUX_ADV_IND PDUs in a non-overlapping interval with the
	 * Broadcast ISO radio events.
	 * For 10ms SDU interval a extended advertising interval of 60 - 10 = 50 is suitable
	 */
	const uint16_t adv_interval_ms = 60U;
	const uint16_t ext_adv_interval_ms = adv_interval_ms - 10U;
	const struct bt_le_adv_param *ext_adv_param = BT_LE_ADV_PARAM(
		BT_LE_ADV_OPT_EXT_ADV, BT_GAP_MS_TO_ADV_INTERVAL(ext_adv_interval_ms),
		BT_GAP_MS_TO_ADV_INTERVAL(ext_adv_interval_ms), NULL);
	const struct bt_le_per_adv_param *per_adv_param = BT_LE_PER_ADV_PARAM(
		BT_GAP_MS_TO_PER_ADV_INTERVAL(adv_interval_ms),
		BT_GAP_MS_TO_PER_ADV_INTERVAL(adv_interval_ms), BT_LE_PER_ADV_OPT_NONE);
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
	err = bt_le_ext_adv_create(ext_adv_param, NULL, &adv);
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
	err = bt_le_per_adv_set_param(adv, per_adv_param);
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

	uint32_t skip = 3U;

	while (true) {
		struct bt_iso_tx_info tx_info;

		for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT; chan++) {
			struct net_buf *buf;
			uint16_t sn;
			int ret;

			if (skip && (chan == 0U)) {
				skip--;

				continue;
			}

			buf = net_buf_alloc(&bis_tx_pool, K_USEC(BUF_ALLOC_TIMEOUT_US));
			if (!buf) {
				printk("Data buffer allocate timeout on channel"
				       " %u\n", chan);
				return 0;
			}

			ret = k_sem_take(&sem_iso_data, K_USEC(BUF_ALLOC_TIMEOUT_US));
			if (ret) {
				printk("k_sem_take for ISO data sent failed\n");
				net_buf_unref(buf);
				return 0;
			}

			net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
			sys_put_le32(iso_send_count, iso_data);
			net_buf_add_mem(buf, iso_data, sizeof(iso_data));

			/* Ensure that first ISO data on all streams are given with sequence number
			 * 0, so that the (Zephyr) Controller uses the same payload counter for
			 * a given sequence number on all streams at an SDU interval.
			 * Read Tx Sync interface returns error if no ISO data was given previously.
			 */
			ret = bt_iso_chan_get_tx_sync(&bis_iso_chan[chan], &tx_info);
			if (ret) {
				printk("Failed to get tx sync before send (%d)\n", ret);

				/* Use sequence number 0, to synchronize current stream with the
				 * current payload counter in the ISO group of streams.
				 */
				sn = 0U;

			} else {
				sn = seq_num;
			}

			/* Send ISO data with timestamp, use sequence number 0 for first ISO data on
			 * any stream.
			 */
			if (sn) {
				ret = bt_iso_chan_send_ts(&bis_iso_chan[chan], buf, sn, ts);
			} else {
				ret = bt_iso_chan_send(&bis_iso_chan[chan], buf, sn);
			}

			if (ret < 0) {
				printk("Unable to broadcast data on channel %u"
				       " : %d", chan, ret);
				net_buf_unref(buf);
				return 0;
			}

			/* Get Tx Sync, to verify that all streams have their ISO data at the same
			 * timestamp.
			 */
			ret = bt_iso_chan_get_tx_sync(&bis_iso_chan[chan], &tx_info);
			if (ret) {
				printk("Failed to get tx sync after send (%d)\n", ret);
				return 0;
			}

			/* Timestamp delta with the Controller */
			if (!delta) {
				delta = tx_info.ts - ts;
			} else {
				int32_t drift = ((delta - tx_info.ts + ts) / BIG_SDU_INTERVAL_US) /
						1000000U;

				/* NOTE: As the `ts` is just a increase in software in this sample,
				 *       we will not see any drift.
				 */
				if (drift) {
					printk("Drift: %d ppm.\n", drift);
				}

				/* NOTE: Change in delta (drift) for a duration of SDU intervals can
				 *       be used to synchronize the clock with the Controller.
				 */
			}

			printk("Tx chan %u: seq num %u -> %u ts %u -> %u offset %u\n", chan,
			       seq_num, tx_info.seq_num, ts, tx_info.ts, tx_info.offset);

			/* Permit a reasonable clock jitter (at least Bluetooth active clock
			 * jitter), in the verification of timestamp.
			 */
			uint32_t jitter_us = 2U;

			/* Validate ISO data being enqueued at the required timestamp.
			 * Ignore checking first ISO data sent.
			 */
			if (ts && sn &&
			    !IN_RANGE((ts + delta), (tx_info.ts - jitter_us),
				      (tx_info.ts + jitter_us))) {
				printk("Mismatch in tx sync timestamp! (%u != %u)\n", ts,
				       tx_info.ts);
				return 0;
			}
		}

		if ((iso_send_count % CONFIG_ISO_PRINT_INTERVAL) == 0) {
			printk("Sending value %u\n", iso_send_count);
		}

		iso_send_count++;
		seq_num++;
		ts += BIG_SDU_INTERVAL_US;

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
