/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>

#include "settings_priv.h"
#include <zephyr/ztest.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>

/* This is a test suite for performance testing of settings subsystem by writing
 * many small setting values repeatedly. Ideally, this should consume as small
 * amount of time as possible for best possible UX.
 */

static struct k_work_q settings_work_q;
static K_THREAD_STACK_DEFINE(settings_work_stack, 2024);
static struct k_work_delayable pending_store;

#define TEST_SETTINGS_COUNT      (128)
#define TEST_STORE_ITR           (5)
#define TEST_TIMEOUT_SEC         (60)
#define TEST_SETTINGS_WORKQ_PRIO (1)

static void bt_scan_cb([[maybe_unused]] const bt_addr_le_t *addr, [[maybe_unused]] int8_t rssi,
		       [[maybe_unused]] uint8_t adv_type, struct net_buf_simple *buf)
{
	printk("len %u\n", buf->len);
}

struct test_setting {
	uint32_t val;
};
struct test_setting test_settings[TEST_SETTINGS_COUNT];

K_SEM_DEFINE(waitfor_work, 0, 1);

static void store_pending(struct k_work *work)
{
	int err;
	char path[20];
	struct test_stats {
		uint32_t total_calculated;
		uint32_t total_measured;
		uint32_t single_entry_max;
		uint32_t single_entry_min;
	};
	struct test_stats stats = {0, 0, 0, UINT32_MAX};

	int64_t ts1 = k_uptime_get();

	/* benchmark storage performance */
	for (int j = 0; j < TEST_STORE_ITR; j++) {
		for (int i = 0; i < TEST_SETTINGS_COUNT; i++) {
			test_settings[i].val = TEST_SETTINGS_COUNT * j + i;

			int64_t ts2 = k_uptime_get();

			snprintk(path, sizeof(path), "ab/cdef/ghi/%04x", i);
			err = settings_save_one(path, &test_settings[i],
						sizeof(struct test_setting));
			zassert_equal(err, 0, "settings_save_one failed %d", err);

			int64_t delta2 = k_uptime_delta(&ts2);

			if (stats.single_entry_max < delta2) {
				stats.single_entry_max = delta2;
			}
			if (stats.single_entry_min > delta2) {
				stats.single_entry_min = delta2;
			}
			stats.total_calculated += delta2;
		}
	}

	int64_t delta1 = k_uptime_delta(&ts1);

	stats.total_measured = delta1;

	printk("*** storing of %u entries completed ***\n", ARRAY_SIZE(test_settings));
	printk("total calculated: %u, total measured: %u\n", stats.total_calculated,
	       stats.total_measured);
	printk("entry max: %u, entry min: %u\n", stats.single_entry_max, stats.single_entry_min);

	k_sem_give(&waitfor_work);
}

ZTEST_SUITE(settings_perf, NULL, NULL, NULL, NULL, NULL);

ZTEST(settings_perf, test_performance)
{
	int err;

	if (IS_ENABLED(CONFIG_NVS)) {
		printk("Testing with NVS\n");
	} else if (IS_ENABLED(CONFIG_ZMS)) {
		printk("Testing with ZMS\n");
	}

	k_work_queue_start(&settings_work_q, settings_work_stack,
			   K_THREAD_STACK_SIZEOF(settings_work_stack),
			   K_PRIO_COOP(TEST_SETTINGS_WORKQ_PRIO), NULL);
	k_thread_name_set(&settings_work_q.thread, "Settings workq");
	k_work_init_delayable(&pending_store, store_pending);

	if (IS_ENABLED(CONFIG_BT)) {
		/* enable one of the major subsystems, and start scanning. */
		err = bt_enable(NULL);
		zassert_equal(err, 0, "Bluetooth init failed (err %d)\n", err);

		err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, bt_scan_cb);
		zassert_equal(err, 0, "Scanning failed to start (err %d)\n", err);
	}

	err = settings_subsys_init();
	zassert_equal(err, 0, "settings_backend_init failed %d", err);

	/* fill with values */
	for (int i = 0; i < TEST_SETTINGS_COUNT; i++) {
		test_settings[i].val = i;
	}

	k_work_reschedule_for_queue(&settings_work_q, &pending_store, K_NO_WAIT);

	err = k_sem_take(&waitfor_work, K_SECONDS(TEST_TIMEOUT_SEC));
	zassert_equal(err, 0, "k_sem_take failed %d", err);

	if (IS_ENABLED(CONFIG_BT)) {
		err = bt_le_scan_stop();
		zassert_equal(err, 0, "Scanning failed to stop (err %d)\n", err);
	}
}
