/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Channel metrics histogram from the Zephyr vendor-specific Channel Metrics event.
 *
 * The controller reports, per data channel, the reception success percentage of the
 * connections and of the synchronized Broadcast Isochronous Group since its previous report,
 * filtered into a per channel score in the scale of 0 to 100. Periodically, the scores are
 * printed as a histogram.
 *
 * Channels excluded by the channel map in use are reported as bad: their score is cleared
 * and is restored when the channel is used again.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "chan_metrics.h"

#define CHAN_COUNT         BT_HCI_VS_CHAN_METRICS_CHAN_COUNT
#define CHAN_MAP_LEN       5U
#define SCORE_SHIFT        8U
#define SCORE_MAX          (100U << SCORE_SHIFT)
/* A report moves the score by 1/4 of its difference to the reported percentage */
#define SCORE_FILTER_SHIFT 2U
/* Prefix of each printed line, to filter the histogram from the rest of the log */
#define LINE_PREFIX        "CM: "
/* One line printed per interval, so that a histogram does not overflow the deferred log
 * buffer, also when shared with other frequent prints like the Controller ISR profiling.
 */
#define LINE_INTERVAL_MS   10U

static struct k_spinlock lock;
static uint8_t pending_success[CHAN_COUNT];
static uint8_t pending_chan_map[CHAN_MAP_LEN];
static uint32_t pending_count;
static bool pending;

static uint32_t score[CHAN_COUNT];
static uint8_t chan_map[CHAN_MAP_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
static uint32_t event_count;

/* Snapshot printed one line at a time */
static uint8_t print_score[CHAN_COUNT];
static uint8_t print_chan_map[CHAN_MAP_LEN];
static uint8_t print_chan;

static bool chan_is_set(const uint8_t *map, uint8_t chan)
{
	return (map[chan >> 3] & BIT(chan & 0x07U)) != 0U;
}

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
	(void)memcpy(map, pending_chan_map, sizeof(map));
	(void)memset(pending_success, BT_HCI_VS_CHAN_METRICS_NO_SAMPLES, sizeof(pending_success));
	event_count += pending_count;
	pending_count = 0U;
	pending = false;
	k_spin_unlock(&lock, key);

	for (uint8_t i = 0U; i < CHAN_COUNT; i++) {
		uint32_t sample;

		if (!chan_is_set(map, i)) {
			score[i] = 0U;

			continue;
		}

		if (!chan_is_set(chan_map, i)) {
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

	(void)memcpy(chan_map, map, sizeof(chan_map));
}

static K_WORK_DEFINE(metrics_work, metrics_work_handler);

bool chan_metrics_vnd_evt(struct net_buf_simple *buf)
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
	(void)memcpy(pending_chan_map, evt->chan_map, sizeof(pending_chan_map));
	pending_count++;
	pending = true;
	k_spin_unlock(&lock, key);

	(void)k_work_submit(&metrics_work);

	return true;
}

static void print_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	/* Static, as the system work queue stack is small */
	static char bar[100U + 1U];
	uint8_t cnt;
	uint8_t i;

	if (print_chan == 0U) {
		for (i = 0U; i < CHAN_COUNT; i++) {
			print_score[i] = (score[i] + BIT(SCORE_SHIFT - 1U)) >> SCORE_SHIFT;
		}
		(void)memcpy(print_chan_map, chan_map, sizeof(print_chan_map));

		printk(LINE_PREFIX "Channel metrics, %u events, channel map 0x%010llx\n",
		       event_count, (unsigned long long)sys_get_le40(print_chan_map));
		printk(LINE_PREFIX "chan #: score - histogram\n");
	}

	i = print_chan;
	if (!chan_is_set(print_chan_map, i)) {
		printk(LINE_PREFIX "%02u: --- - excluded\n", i);
	} else {
		cnt = MIN(print_score[i], sizeof(bar) - 1U);
		(void)memset(bar, '*', cnt);
		bar[cnt] = '\0';
		printk(LINE_PREFIX "%02u: %03u - %s\n", i, print_score[i], bar);
	}

	print_chan++;
	if (print_chan < CHAN_COUNT) {
		(void)k_work_reschedule(dwork, K_MSEC(LINE_INTERVAL_MS));

		return;
	}

	print_chan = 0U;
	(void)k_work_reschedule(dwork, K_SECONDS(CONFIG_CHAN_METRICS_PRINT_INTERVAL));
}

static K_WORK_DELAYABLE_DEFINE(print_work, print_work_handler);

void chan_metrics_init(void)
{
	for (uint8_t i = 0U; i < CHAN_COUNT; i++) {
		score[i] = SCORE_MAX;
	}
	(void)memset(pending_success, BT_HCI_VS_CHAN_METRICS_NO_SAMPLES, sizeof(pending_success));
	(void)memcpy(pending_chan_map, chan_map, sizeof(pending_chan_map));

	(void)k_work_schedule(&print_work, K_SECONDS(CONFIG_CHAN_METRICS_PRINT_INTERVAL));
}
