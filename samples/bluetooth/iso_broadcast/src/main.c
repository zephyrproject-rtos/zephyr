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

	bool is_first_iso_data_send = true;
	uint32_t ts_ctlr = 0U;
	uint32_t ts_app = 0U;
	int64_t delta_ts = 0;
	int32_t delta_sn = 0;

	while (true) {
		struct bt_iso_tx_info tx_info;

		for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT; chan++) {
			struct net_buf *buf;
			int ret;

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

			/* Send ISO data with timestamp, use sequence number 0 for first ISO data on
			 * any stream.
			 */
			if (!is_first_iso_data_send) {
				ret = bt_iso_chan_send_ts(&bis_iso_chan[chan], buf, seq_num,
							  ts_ctlr);
			} else {
				ret = bt_iso_chan_send(&bis_iso_chan[chan], buf, 0U);
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

			/* Determine the delta sn and synchronise ts */
			if (is_first_iso_data_send) {
				if (chan == 0U) {
					ts_ctlr = tx_info.ts;
					delta_sn = tx_info.seq_num;
				}

				/* Timestamp delta with the Controller */
				delta_ts = (int64_t)tx_info.ts - ts_app;
			}

			/* Use one channel timestamp to accumulate relative drift */
			if (chan == 0U) {
				/* NOTE: Change in delta (drift) for a duration of SDU intervals can
				 *       be used to synchronize the clock with the Controller.
				 */
			}

			/* Permit a reasonable clock jitter (at least Bluetooth active clock
			 * jitter), in the verification of timestamp.
			 */
			uint32_t jitter_us = 2U;

			/* Validate ISO data being enqueued at the required timestamp.
			 * Ignore checking first ISO data sent.
			 */
			if (!is_first_iso_data_send &&
			    (((seq_num + delta_sn) != tx_info.seq_num) ||
			     !IN_RANGE(tx_info.ts, (ts_app + delta_ts - jitter_us),
				       (ts_app + delta_ts + jitter_us)))) {
				printk("Mismatch in tx sync timestamp! (%u, %llu != %u, %u)\n",
				       (seq_num + delta_sn), (ts_app + delta_ts),
				       tx_info.seq_num, tx_info.ts);
				return 0;
			}
		}

		is_first_iso_data_send = false;

		if ((iso_send_count % CONFIG_ISO_PRINT_INTERVAL) == 0) {
			printk("Sending value %u\n", iso_send_count);
		}

		iso_send_count++;
		seq_num++;
		ts_ctlr += BIG_SDU_INTERVAL_US;
		ts_app += BIG_SDU_INTERVAL_US;

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

			/* Reset as we start over with new BIG created */
			ts_ctlr = 0U;
			ts_app = 0U;
			delta_ts = 0;
			delta_sn = 0;
			is_first_iso_data_send = true;
		}
	}
}
