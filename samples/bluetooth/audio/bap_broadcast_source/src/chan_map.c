/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Channel map update from the Zephyr vendor-specific Channel Metrics event.
 *
 * The controller reports, per data channel, the reception success percentage of the
 * connections and of the scanning since its previous report, filtered into a per channel
 * score. Periodically, the channels scoring at or above the threshold, topped up with the best
 * scoring channels to a minimum count, are set as the Host Channel Classification, used by the
 * extended advertising, the periodic advertising and the Broadcast ISO.
 *
 * Every reset interval all channel scores are restored, so that the full channel map is used
 * again until the metrics report a channel as bad.
 *
 * The central decides the connection channel map. Channels it excludes, at connection or by
 * a Channel Map Update procedure, are reported as bad: their score is cleared, they are not
 * restored and they are not used to reach the minimum channel count. When the central uses
 * a channel again, its score is restored.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "chan_map.h"

#define CHAN_COUNT          BT_HCI_VS_CHAN_METRICS_CHAN_COUNT
#define CHAN_MAP_LEN        5U
#define SCORE_SHIFT         8U
#define SCORE_MAX           (100U << SCORE_SHIFT)
#define SCORE_THRESHOLD     ((uint32_t)CONFIG_CHAN_MAP_UPDATE_THRESHOLD << SCORE_SHIFT)
/* A report moves the score by 1/4 of its difference to the reported percentage */
#define SCORE_FILTER_SHIFT  2U

static struct k_spinlock lock;
static uint8_t pending_success[CHAN_COUNT];
static uint8_t pending_peer_map[CHAN_MAP_LEN];
static bool pending;

static uint32_t score[CHAN_COUNT];
static uint8_t peer_map[CHAN_MAP_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};

static bool chan_is_set(const uint8_t *map, uint8_t chan)
{
	return (map[chan >> 3] & BIT(chan & 0x07U)) != 0U;
}

static void chan_map_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(chan_map_work, chan_map_work_handler);

static void metrics_work_handler(struct k_work *work)
{
	uint8_t success[CHAN_COUNT];
	uint8_t map[CHAN_MAP_LEN];
	k_spinlock_key_t key;

	key = k_spin_lock(&lock);
	if (!pending) {
		k_spin_unlock(&lock, key);

		return;
	}
	(void)memcpy(success, pending_success, sizeof(success));
	(void)memcpy(map, pending_peer_map, sizeof(map));
	(void)memset(pending_success, BT_HCI_VS_CHAN_METRICS_NO_SAMPLES, sizeof(pending_success));
	pending = false;
	k_spin_unlock(&lock, key);

	if (memcmp(map, peer_map, sizeof(map)) != 0) {
		printk("Peer channel map 0x%010llx\n", (unsigned long long)sys_get_le40(map));

		/* Set Host Channel Classification calls are at least one second apart */
		(void)k_work_reschedule(&chan_map_work, K_SECONDS(1));
	}

	for (uint8_t i = 0U; i < CHAN_COUNT; i++) {
		uint32_t sample;

		if (!chan_is_set(map, i)) {
			score[i] = 0U;

			continue;
		}

		if (!chan_is_set(peer_map, i)) {
			score[i] = SCORE_MAX;
		}

		if (success[i] == BT_HCI_VS_CHAN_METRICS_NO_SAMPLES) {
			continue;
		}

		sample = (uint32_t)MIN(success[i], 100U) << SCORE_SHIFT;
		if (sample >= score[i]) {
			score[i] += (sample - score[i]) >> SCORE_FILTER_SHIFT;
		} else {
			score[i] -= (score[i] - sample) >> SCORE_FILTER_SHIFT;
		}
	}

	(void)memcpy(peer_map, map, sizeof(peer_map));
}

static K_WORK_DEFINE(metrics_work, metrics_work_handler);

bool chan_map_vnd_evt(struct net_buf_simple *buf)
{
	const struct bt_hci_evt_vs_chan_metrics *evt;
	const struct bt_hci_evt_vs *vs;
	k_spinlock_key_t key;

	if (buf->len < (sizeof(*vs) + sizeof(*evt))) {
		return false;
	}

	vs = net_buf_simple_pull_mem(buf, sizeof(*vs));
	if (vs->subevent != BT_HCI_EVT_VS_CHAN_METRICS) {
		return false;
	}

	evt = net_buf_simple_pull_mem(buf, sizeof(*evt));

	/* Keep a not yet filtered report of a channel that has no samples in this report */
	key = k_spin_lock(&lock);
	for (uint8_t i = 0U; i < CHAN_COUNT; i++) {
		if (evt->success[i] != BT_HCI_VS_CHAN_METRICS_NO_SAMPLES) {
			pending_success[i] = evt->success[i];
		}
	}
	(void)memcpy(pending_peer_map, evt->chan_map, sizeof(pending_peer_map));
	pending = true;
	k_spin_unlock(&lock, key);

	(void)k_work_submit(&metrics_work);

	return true;
}

static void chan_map_work_handler(struct k_work *work)
{
	static uint8_t map_curr[CHAN_MAP_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
	static int64_t reset_uptime;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	uint8_t map[CHAN_MAP_LEN] = {0U};
	uint8_t count = 0U;
	int err;

	if ((k_uptime_get() - reset_uptime) >= (CONFIG_CHAN_MAP_RESET_INTERVAL * MSEC_PER_SEC)) {
		reset_uptime = k_uptime_get();

		for (uint8_t i = 0U; i < CHAN_COUNT; i++) {
			if (chan_is_set(peer_map, i)) {
				score[i] = SCORE_MAX;
			}
		}
	}

	for (uint8_t i = 0U; i < CHAN_COUNT; i++) {
		if (chan_is_set(peer_map, i) && (score[i] >= SCORE_THRESHOLD)) {
			map[i >> 3] |= BIT(i & 0x07U);
			count++;
		}
	}

	/* A peer Central uses at least 2 channels, channels it excludes are not added */
	while (count < CONFIG_CHAN_MAP_UPDATE_CHAN_MIN) {
		bool found = false;
		uint8_t best = 0U;

		for (uint8_t i = 0U; i < CHAN_COUNT; i++) {
			if (chan_is_set(peer_map, i) && !chan_is_set(map, i) &&
			    (!found || (score[i] > score[best]))) {
				best = i;
				found = true;
			}
		}

		if (!found) {
			break;
		}

		map[best >> 3] |= BIT(best & 0x07U);
		count++;
	}

	if (memcmp(map, map_curr, sizeof(map)) != 0) {
		err = bt_le_set_chan_map(map);
		if (err == 0) {
			(void)memcpy(map_curr, map, sizeof(map_curr));
			printk("Channel map 0x%010llx, %u channels\n",
			       (unsigned long long)sys_get_le40(map), count);
		} else {
			/* Retried next interval, e.g. while a BIG channel map update is pending */
			printk("Failed to set channel map (err %d)\n", err);
		}
	}

	(void)k_work_reschedule(dwork, K_SECONDS(CONFIG_CHAN_MAP_UPDATE_INTERVAL));
}

void chan_map_init(void)
{
	for (uint8_t i = 0U; i < CHAN_COUNT; i++) {
		score[i] = SCORE_MAX;
	}
	(void)memset(pending_success, BT_HCI_VS_CHAN_METRICS_NO_SAMPLES, sizeof(pending_success));
	(void)memcpy(pending_peer_map, peer_map, sizeof(pending_peer_map));

	(void)k_work_schedule(&chan_map_work, K_SECONDS(CONFIG_CHAN_MAP_UPDATE_INTERVAL));
}
