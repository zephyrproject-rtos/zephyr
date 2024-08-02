/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/console/console.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(iso_broadcast_receiver, LOG_LEVEL_DBG);

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME))

#define PA_RETRY_COUNT 6
#define ISO_RETRY_COUNT 10

struct iso_recv_stats {
	uint32_t     iso_recv_count;
	uint32_t     iso_lost_count;
};

static bool         broadcaster_found;
static bool         per_adv_lost;
static bool         big_sync_lost;
static bool         biginfo_received;
static bt_addr_le_t per_addr;
static uint8_t      per_sid;
static uint32_t     per_interval_us;
static uint32_t     iso_interval_us;
static uint8_t      bis_count;
static uint32_t     last_received_counter;
static int64_t      big_sync_start_time;
static size_t       big_sync_count;

static struct iso_recv_stats stats_current_sync;
static struct iso_recv_stats stats_overall;

static K_SEM_DEFINE(sem_per_adv, 0, 1);
static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_per_sync_lost, 0, 1);
static K_SEM_DEFINE(sem_per_big_info, 0, 1);
static K_SEM_DEFINE(sem_big_sync, 0, 1);
static K_SEM_DEFINE(sem_big_sync_lost, 0, 1);

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

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, DEVICE_NAME_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[DEVICE_NAME_LEN];

	if (broadcaster_found) {
		return;
	}

	if (info->interval == 0U) {
		/* Not broadcast periodic advertising - Ignore */
		return;
	}

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	if (strncmp(DEVICE_NAME, name, strlen(DEVICE_NAME))) {
		return;
	}

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	LOG_INF("Found broadcaster with address %s (RSSI %i)",
		le_addr, info->rssi);

	broadcaster_found = true;

	per_sid = info->sid;
	per_interval_us = BT_CONN_INTERVAL_TO_US(info->interval);
	bt_addr_le_copy(&per_addr, info->addr);

	k_sem_give(&sem_per_adv);
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void sync_cb(struct bt_le_per_adv_sync *sync,
		    struct bt_le_per_adv_sync_synced_info *info)
{
	LOG_INF("Periodic advertisement synced");

	k_sem_give(&sem_per_sync);
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	LOG_INF("Periodic advertisement sync terminated");

	per_adv_lost = true;
	k_sem_give(&sem_per_sync_lost);
}

static void biginfo_cb(struct bt_le_per_adv_sync *sync,
		       const struct bt_iso_biginfo *biginfo)
{
	if (biginfo_received) {
		return;
	}

	LOG_INF("BIGinfo received: num_bis %u, nse %u, interval %u us, "
		"bn %u, pto %u, irc %u, max_pdu %u, sdu_interval %u us, "
		"max_sdu %u, phy %s, %s framing, %sencrypted",
		biginfo->num_bis, biginfo->sub_evt_count,
		BT_CONN_INTERVAL_TO_US(biginfo->iso_interval),
		biginfo->burst_number, biginfo->offset, biginfo->rep_count,
		biginfo->max_pdu, biginfo->sdu_interval, biginfo->max_sdu,
		phy2str(biginfo->phy), biginfo->framing ? "with" : "without",
		biginfo->encryption ? "" : "not ");

	iso_interval_us = BT_CONN_INTERVAL_TO_US(biginfo->iso_interval);
	bis_count = MIN(biginfo->num_bis, CONFIG_BT_ISO_MAX_CHAN);
	biginfo_received = true;
	k_sem_give(&sem_per_big_info);
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.term = term_cb,
	.biginfo = biginfo_cb,
};

static void print_stats(char *name, struct iso_recv_stats *stats)
{
	uint32_t total_packets;

	total_packets = stats->iso_recv_count + stats->iso_lost_count;

	LOG_INF("%s: Received %u/%u (%.2f%%) - Total packets lost %u",
		name, stats->iso_recv_count, total_packets,
		(double)((float)stats->iso_recv_count * 100 / total_packets),
		stats->iso_lost_count);

}

static void iso_recv(struct bt_iso_chan *chan,
		     const struct bt_iso_recv_info *info,
		     struct net_buf *buf)
{
	uint32_t total_packets;
	static bool stats_latest_arr[1000];
	static size_t stats_latest_arr_pos;

	/* NOTE: The packets received may be on different BISes */

	if (info->flags & BT_ISO_FLAGS_VALID) {
		stats_current_sync.iso_recv_count++;
		stats_overall.iso_recv_count++;
		stats_latest_arr[stats_latest_arr_pos++] = true;
	} else {
		stats_current_sync.iso_lost_count++;
		stats_overall.iso_lost_count++;
		stats_latest_arr[stats_latest_arr_pos++] = false;
	}

	if (stats_latest_arr_pos == sizeof(stats_latest_arr)) {
		stats_latest_arr_pos = 0;
	}

	total_packets = stats_overall.iso_recv_count + stats_overall.iso_lost_count;

	if ((total_packets % 100) == 0) {
		struct iso_recv_stats stats_latest = { 0 };

		for (int i = 0; i < ARRAY_SIZE(stats_latest_arr); i++) {
			/* If we have not yet received 1000 packets, break
			 * early
			 */
			if (i == total_packets) {
				break;
			}

			if (stats_latest_arr[i]) {
				stats_latest.iso_recv_count++;
			} else {
				stats_latest.iso_lost_count++;
			}
		}

		print_stats("Overall     ", &stats_overall);
		print_stats("Current Sync", &stats_current_sync);
		print_stats("Latest 1000 ", &stats_latest);
		LOG_INF(""); /* Empty line to separate the stats */
	}
}

static void iso_connected(struct bt_iso_chan *chan)
{
	LOG_INF("ISO Channel %p connected", chan);

	big_sync_start_time = k_uptime_get();

	k_sem_give(&sem_big_sync);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	/* Calculate cumulative moving average - Be aware that this may
	 * cause overflow at sufficiently large counts or durations
	 */
	static int64_t average_duration;
	uint64_t big_sync_duration = k_uptime_get() - big_sync_start_time;
	uint64_t total_duration = big_sync_duration + (big_sync_count - 1) * average_duration;

	average_duration = total_duration / big_sync_count;

	LOG_INF("ISO Channel %p disconnected with reason 0x%02x after "
		"%llu milliseconds (average duration %llu)",
		chan, reason, big_sync_duration, average_duration);

	big_sync_lost = true;

	k_sem_give(&sem_big_sync_lost);
}

static struct bt_iso_chan_ops iso_ops = {
	.recv		= iso_recv,
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
};

static struct bt_iso_chan_io_qos iso_rx_qos;

static struct bt_iso_chan_qos bis_iso_qos = {
	.rx = &iso_rx_qos,
};


static int start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		LOG_ERR("Scan start failed (err %d)", err);
		return err;
	}

	LOG_INF("Scan started");

	return 0;
}

static int stop_scan(void)
{
	int err;

	err = bt_le_scan_stop();
	if (err != 0) {
		LOG_ERR("Scan stop failed (err %d)", err);
		return err;
	}
	LOG_INF("Scan stopped");

	return 0;
}

static int create_pa_sync(struct bt_le_per_adv_sync **sync)
{
	struct bt_le_per_adv_sync_param sync_create_param;
	int err;
	uint32_t sem_timeout_us;

	LOG_INF("Creating Periodic Advertising Sync");
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options = 0;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	/* Multiple PA interval with retry count and convert to unit of 10 ms */
	sync_create_param.timeout = (per_interval_us * PA_RETRY_COUNT) / (10 * USEC_PER_MSEC);
	sem_timeout_us = per_interval_us * PA_RETRY_COUNT;
	err = bt_le_per_adv_sync_create(&sync_create_param, sync);
	if (err != 0) {
		LOG_ERR("Periodic advertisement sync create failed (err %d)",
		       err);
		return err;
	}

	LOG_INF("Waiting for periodic sync");
	err = k_sem_take(&sem_per_sync, K_USEC(sem_timeout_us));
	if (err != 0) {
		LOG_INF("failed to take sem_per_sync (err %d)", err);

		LOG_INF("Deleting Periodic Advertising Sync");
		err = bt_le_per_adv_sync_delete(*sync);
		if (err != 0) {
			LOG_ERR("Failed to delete Periodic advertisement sync (err %d)",
			       err);

			return err;
		}
	}
	LOG_INF("Periodic sync established");

	return 0;
}

static int create_big_sync(struct bt_iso_big **big, struct bt_le_per_adv_sync *sync)
{
	int err;
	uint32_t sem_timeout_us = per_interval_us * PA_RETRY_COUNT;
	uint32_t sync_timeout_us;
	static struct bt_iso_chan bis_iso_chan[CONFIG_BT_ISO_MAX_CHAN];
	struct bt_iso_chan *bis[CONFIG_BT_ISO_MAX_CHAN];
	struct bt_iso_big_sync_param big_sync_param = {
		.bis_channels = bis,
		.num_bis = 0,
		.bis_bitfield = 0,
		.mse = 0,
		.sync_timeout = 0
	};

	for (int i = 0; i < ARRAY_SIZE(bis_iso_chan); i++) {
		bis_iso_chan[i].ops = &iso_ops;
		bis_iso_chan[i].qos = &bis_iso_qos;
		bis[i] = &bis_iso_chan[i];
	}

	LOG_INF("Waiting for BIG info");
	err = k_sem_take(&sem_per_big_info, K_USEC(sem_timeout_us));
	if (err != 0) {
		LOG_ERR("failed to take sem_per_big_info (err %d)", err);
		return err;
	}

	sync_timeout_us = iso_interval_us * ISO_RETRY_COUNT;
	/* timeout is in 10 ms units */
	big_sync_param.sync_timeout = CLAMP(sync_timeout_us / (10 * USEC_PER_MSEC),
					    BT_ISO_SYNC_TIMEOUT_MIN,
					    BT_ISO_SYNC_TIMEOUT_MAX);
	big_sync_param.num_bis = bis_count;
	/* BIS indexes start from 0x01 */
	for (int i = 1; i <= big_sync_param.num_bis; i++) {
		big_sync_param.bis_bitfield |= BT_ISO_BIS_INDEX_BIT(i);
	}

	LOG_INF("Syncing to BIG");
	err = bt_iso_big_sync(sync, &big_sync_param, big);
	if (err != 0) {
		LOG_ERR("BIG sync failed (err %d)", err);
		return err;
	}

	LOG_INF("Waiting for BIG sync");
	err = k_sem_take(&sem_big_sync, K_USEC(sem_timeout_us));
	if (err != 0) {
		LOG_ERR("failed to take sem_big_sync (err %d)", err);
		return err;
	}
	LOG_INF("BIG sync established");

	big_sync_count++;

	return 0;
}

static int cleanup(struct bt_le_per_adv_sync *sync, struct bt_iso_big *big)
{
	int pa_err = 0;
	int big_err = 0;

	if (!per_adv_lost && sync) {
		LOG_INF("Deleting Periodic advertisement Sync");
		pa_err = bt_le_per_adv_sync_delete(sync);
		if (pa_err != 0) {
			LOG_ERR("Failed to delete Periodic advertisement sync (err %d)",
				pa_err);
		}
	}

	if (!big_sync_lost && big) {
		LOG_INF("Terminating BIG Sync");
		big_err = bt_iso_big_terminate(big);
		if (big_err != 0) {
			LOG_ERR("BIG terminate failed (err %d)", big_err);
		}
	}

	if (pa_err != 0 || big_err != 0) {
		LOG_ERR("Cleanup failed (%d), recommend restart application to "
		       "avoid any potential leftovers",
		       pa_err != 0 ? pa_err : big_err);
	}

	return pa_err != 0 ? pa_err : big_err;
}

static void reset_sems(void)
{
	(void)k_sem_reset(&sem_per_adv);
	(void)k_sem_reset(&sem_per_sync);
	(void)k_sem_reset(&sem_per_sync_lost);
	(void)k_sem_reset(&sem_per_big_info);
	(void)k_sem_reset(&sem_big_sync);
	(void)k_sem_reset(&sem_big_sync_lost);
}

int test_run_receiver(void)
{
	struct bt_le_per_adv_sync *sync = NULL;
	struct bt_iso_big *big = NULL;
	int err;
	static bool callbacks_registered;

	broadcaster_found = false;
	per_adv_lost = false;
	big_sync_lost = false;
	biginfo_received = false;
	reset_sems();

	if (!callbacks_registered) {
		bt_le_per_adv_sync_cb_register(&sync_callbacks);
		bt_le_scan_cb_register(&scan_callbacks);
		callbacks_registered = true;
	}

	err = start_scan();
	if (err != 0) {
		return err;
	}

	LOG_INF("Waiting for periodic advertiser");
	err = k_sem_take(&sem_per_adv, K_FOREVER);
	if (err != 0) {
		LOG_ERR("failed to take sem_per_adv (err %d)", err);
		return err;
	}
	LOG_INF("Periodic advertiser found");

	err = stop_scan();
	if (err != 0) {
		return err;
	}

	err = create_pa_sync(&sync);
	if (err != 0) {
		return err;
	}

	last_received_counter = 0;
	memset(&stats_current_sync, 0, sizeof(stats_current_sync));
	big_sync_start_time = 0;

	err = create_big_sync(&big, sync);
	if (err != 0) {
		(void)cleanup(sync, big);
		return err;
	}

	err = k_sem_take(&sem_big_sync_lost, K_FOREVER);
	if (err != 0) {
		LOG_ERR("failed to take sem_big_sync_lost (err %d)", err);
		return err;
	}
	LOG_INF("BIG sync lost, returning");
	/* TODO: Add support to cancel the test early */

	return cleanup(sync, big);
}
