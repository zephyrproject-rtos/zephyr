/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Subrating central. Connects at a fixed interval, lets the peripheral
 * negotiate subrating, then probes the link with timed Read-By-UUID reads. A
 * subrated peer only listens on subrated events, so a read issued while it
 * sleeps is answered only after the next subrated event: large read latency
 * proves skipping, small latency proves no skipping / continuation.
 *
 * Scenarios: basic skip, M->N transition + conn-update reset, continuation
 * events, peripheral-latency stacking, and a simultaneous-request collision.
 */
#include <zephyr/kernel.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "conn_subrate.h"

#define WAIT_TIME 30 /* seconds */

extern enum bst_result_t bst_result;

#if defined(CONFIG_BT_CTLR_TEST_CONN_EVENT_COUNT)
/* Controller (ull_conn.c) per-connection count of events the device was present
 * for. Same binary in bsim, so this links directly. Used by central_lat_count.
 */
extern volatile uint32_t ll_test_conn_event_count[];
#endif

#define FAIL(...)					\
	do {						\
		bst_result = Failed;			\
		bs_trace_error_time_line(__VA_ARGS__);	\
	} while (0)

#define PASS(...)					\
	do {						\
		bst_result = Passed;			\
		bs_trace_info_time(1, __VA_ARGS__);	\
	} while (0)

static struct bt_conn *default_conn;
static struct bt_gatt_read_params read_params;

static volatile uint16_t subrate_factor;
static volatile uint16_t conn_interval;
static volatile int read_err;
static volatile bool collision_mode;
static volatile bool phy_updated;
static volatile bool central_connected;
static K_SEM_DEFINE(read_done, 0, 1);

static uint8_t read_func(struct bt_conn *conn, uint8_t err,
			 struct bt_gatt_read_params *params,
			 const void *data, uint16_t length)
{
	read_err = err;
	k_sem_give(&read_done);
	return BT_GATT_ITER_STOP;
}

#if defined(CONFIG_BT_CTLR_TEST_CONN_EVENT_COUNT)
/* Multi-connection test state: the Central holds two links - one that
 * negotiates subrating and one that does not - to verify the subrated link
 * keeps its cadence while the second (full-rate) link is active, i.e. the
 * scheduler juggling both does not break the subrate skip. The per-conn event
 * counter (ll_test_conn_event_count[]) measures each link independently.
 */
static struct bt_conn *m_conn[2];
static volatile int m_created;
static volatile int m_nconns;
static volatile uint16_t m_subrated_handle = 0xFFFFU;
static volatile bool multi_mode;

static void device_found_multi(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			       struct net_buf_simple *ad)
{
	struct bt_le_conn_param *param;
	int err;

	if ((m_created >= 2) || (type != BT_GAP_ADV_TYPE_ADV_IND &&
				 type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND)) {
		return;
	}

	err = bt_le_scan_stop();
	if (err) {
		FAIL("Multi: stop scan failed (err %d)\n", err);
		return;
	}

	param = BT_LE_CONN_PARAM(CONN_INTERVAL_UNITS, CONN_INTERVAL_UNITS,
				 0, CONN_TIMEOUT_UNITS);
	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &m_conn[m_created]);
	if (err) {
		FAIL("Multi: create connection %d failed (err %d)\n", m_created, err);
		return;
	}
	m_created++;
}
#endif /* CONFIG_BT_CTLR_TEST_CONN_EVENT_COUNT */

static void subrate_changed(struct bt_conn *conn,
			    const struct bt_conn_le_subrate_changed *params)
{
	printk("Central subrate changed: status %u factor %u cont %u "
	       "latency %u timeout %u\n", params->status, params->factor,
	       params->continuation_number, params->peripheral_latency,
	       params->supervision_timeout);

	if (params->status != BT_HCI_ERR_SUCCESS) {
		if (collision_mode) {
			return; /* a rejected request is a valid collision outcome */
		}
		FAIL("Central subrate change failed (status 0x%02x)\n",
		     params->status);
		return;
	}

	subrate_factor = params->factor;

#if defined(CONFIG_BT_CTLR_TEST_CONN_EVENT_COUNT)
	if (multi_mode && params->factor > 1U) {
		uint16_t h;

		if (!bt_hci_get_conn_handle(conn, &h)) {
			m_subrated_handle = h;
		}
	}
#endif
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	printk("Central conn params updated: interval %u latency %u timeout %u\n",
	       interval, latency, timeout);
	conn_interval = interval;
}

static void le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *info)
{
	printk("Central PHY updated: tx %u rx %u\n", info->tx_phy, info->rx_phy);
	phy_updated = true;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	if (conn_err) {
		FAIL("Central failed to connect (err 0x%02x)\n", conn_err);
		return;
	}

	central_connected = true;
	printk("Central connected\n");

#if defined(CONFIG_BT_CTLR_TEST_CONN_EVENT_COUNT)
	/* Just record completion; the test thread drives the scan for the next
	 * link (never issue a blocking HCI command from a connection callback).
	 */
	if (multi_mode) {
		m_nconns++;
	}
#endif
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Central disconnected (reason 0x%02x)\n", reason);

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
	central_connected = false;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.subrate_changed = subrate_changed,
	.le_param_updated = le_param_updated,
	.le_phy_updated = le_phy_updated,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	struct bt_le_conn_param *param;
	char dev[BT_ADDR_LE_STR_LEN];
	int err;

	if (default_conn) {
		return; /* ignore further reports once connecting */
	}
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	printk("Central found device %s, connecting\n", dev);

	err = bt_le_scan_stop();
	if (err) {
		FAIL("Stop scan failed (err %d)\n", err);
		return;
	}

	/* Fixed interval so the skip cadence is deterministic. */
	param = BT_LE_CONN_PARAM(CONN_INTERVAL_UNITS, CONN_INTERVAL_UNITS,
				 0, CONN_TIMEOUT_UNITS);
	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &default_conn);
	if (err) {
		FAIL("Create connection failed (err %d)\n", err);
	}
}

/* Single Read-By-UUID of the GAP Device Name; returns latency in ms or -1. */
static int64_t probe_read_latency(void)
{
	int64_t t0;
	int err;

	read_params.func = read_func;
	read_params.handle_count = 0;
	read_params.by_uuid.uuid = BT_UUID_GAP_DEVICE_NAME;
	read_params.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	read_params.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	read_err = 0;

	t0 = k_uptime_get();
	err = bt_gatt_read(default_conn, &read_params);
	if (err) {
		FAIL("Central read failed (err %d)\n", err);
		return -1;
	}
	if (k_sem_take(&read_done, K_SECONDS(5)) != 0) {
		FAIL("Central read timed out\n");
		return -1;
	}
	if (read_err) {
		FAIL("Central read returned ATT err 0x%02x\n", read_err);
		return -1;
	}

	return k_uptime_get() - t0;
}

/* Run `count` probes spaced by `gap_ms`, reporting min and max latency.
 * Returns 0, or -1 on failure.
 */
static int probe_minmax(int count, int gap_ms, int64_t *out_min, int64_t *out_max)
{
	int64_t mn = -1;
	int64_t mx = 0;

	for (int i = 0; i < count; i++) {
		int64_t latency = probe_read_latency();

		if (latency < 0) {
			return -1;
		}
		printk("Central read %d latency %lld ms\n", i, latency);
		if (mn < 0 || latency < mn) {
			mn = latency;
		}
		if (latency > mx) {
			mx = latency;
		}
		if (i < count - 1) {
			k_sleep(K_MSEC(gap_ms));
		}
	}

	*out_min = mn;
	*out_max = mx;
	return 0;
}

static int central_start(void)
{
	/* Generous acceptable params so any reasonable peripheral request is
	 * granted as-is (negotiated = requested, clamped to these ceilings).
	 */
	struct bt_conn_le_subrate_param defaults = {
		.subrate_min = 1U,
		.subrate_max = SUBRATE_ACC_MAX,
		.max_latency = 10U,
		.continuation_number = 0U,
		.supervision_timeout = CONN_TIMEOUT_UNITS,
	};
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return err;
	}
	printk("Central Bluetooth initialized\n");

	err = bt_conn_le_subrate_set_defaults(&defaults);
	if (err) {
		FAIL("Set default subrate failed (err %d)\n", err);
		return err;
	}

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return err;
	}

	return 0;
}

#define SUBRATE_WAIT(_cond)						\
	do {								\
		while (!(_cond)) {					\
			k_sleep(K_MSEC(100));				\
			if (bst_result == Failed) {			\
				return;					\
			}						\
		}							\
	} while (0)

static uint32_t interval_to_ms(uint16_t units)
{
	return (units * 5U) / 4U; /* 1.25 ms units -> ms */
}

static void test_central_main(void)
{
	int64_t mn, mx;
	uint32_t interval_ms, threshold_ms;

	if (central_start()) {
		return;
	}
	SUBRATE_WAIT(default_conn && subrate_factor >= 2U);

	interval_ms = interval_to_ms(CONN_INTERVAL_UNITS);
	threshold_ms = (subrate_factor * interval_ms) / 2U;
	printk("Central probing: factor %u, interval %u ms, threshold %u ms\n",
	       subrate_factor, interval_ms, threshold_ms);

	if (probe_minmax(NUM_READS, READ_GAP_MS, &mn, &mx)) {
		return;
	}
	printk("Central max read latency %lld ms (threshold %u ms)\n", mx, threshold_ms);
	if (mx < threshold_ms) {
		FAIL("Peripheral did not skip events: max latency %lld ms < %u ms\n",
		     mx, threshold_ms);
		return;
	}
	/* ...but not far beyond the negotiated cadence: a Central that over-skips
	 * (wrong factor/phase) would push the latency well past factor*interval.
	 */
	if (mx > 2U * subrate_factor * interval_ms) {
		FAIL("Skip exceeds negotiated factor %u cadence: max %lld ms > %u ms\n",
		     subrate_factor, mx, 2U * subrate_factor * interval_ms);
		return;
	}

	(void)bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	PASS("Central observed subrating skip (factor %u, max latency %lld ms)\n",
	     subrate_factor, mx);
	bs_trace_silent_exit(0);
}

static void test_central_main_transitions(void)
{
	struct bt_conn_le_subrate_param to_n = {
		.subrate_min = 1U,
		.subrate_max = SUBRATE_MTON_N,
		.max_latency = 0U,
		.continuation_number = 0U,
		.supervision_timeout = CONN_TIMEOUT_UNITS,
	};
	struct bt_le_conn_param *upd;
	uint32_t interval_ms;
	int64_t mn, mx;
	int err;

	if (central_start()) {
		return;
	}

	SUBRATE_WAIT(default_conn && subrate_factor == SUBRATE_MTON_M);
	printk("Central: peripheral negotiated M=%u\n", subrate_factor);

	/* Out-of-range parameters must be rejected (subrate_max > 0x01F4). */
	{
		struct bt_conn_le_subrate_param bad = {
			.subrate_min = 1U,
			.subrate_max = 600U,
			.max_latency = 0U,
			.continuation_number = 0U,
			.supervision_timeout = CONN_TIMEOUT_UNITS,
		};

		if (bt_conn_le_subrate_request(default_conn, &bad) == 0) {
			FAIL("Out-of-range subrate request was accepted\n");
			return;
		}
		printk("Central: out-of-range subrate request rejected\n");
	}

	err = bt_conn_le_subrate_request(default_conn, &to_n);
	if (err) {
		FAIL("Central M->N subrate request failed (err %d)\n", err);
		return;
	}
	SUBRATE_WAIT(subrate_factor == SUBRATE_MTON_N);
	printk("Central: transitioned to N=%u\n", subrate_factor);

	interval_ms = interval_to_ms(CONN_INTERVAL_UNITS);
	if (probe_minmax(NUM_READS, READ_GAP_MS, &mn, &mx)) {
		return;
	}
	printk("Central post-M->N max latency %lld ms\n", mx);
	if (mx < (SUBRATE_MTON_N * interval_ms) / 2U) {
		FAIL("No skip after M->N: max latency %lld ms\n", mx);
		return;
	}

	/* conn-update while subrated -> resets subrating to factor 1. */
	conn_interval = 0U;
	upd = BT_LE_CONN_PARAM(CONN_UPDATE_INTERVAL_UNITS, CONN_UPDATE_INTERVAL_UNITS,
			       0, CONN_TIMEOUT_UNITS);
	err = bt_conn_le_param_update(default_conn, upd);
	if (err) {
		FAIL("Central conn param update failed (err %d)\n", err);
		return;
	}
	SUBRATE_WAIT(conn_interval == CONN_UPDATE_INTERVAL_UNITS);
	printk("Central: interval updated to %u units while subrated\n", conn_interval);

	if (probe_minmax(NUM_READS / 2, READ_GAP_MS, &mn, &mx)) {
		return;
	}
	printk("Central post-update max latency %lld ms\n", mx);
	if (mx >= (SUBRATE_MTON_N * interval_to_ms(CONN_UPDATE_INTERVAL_UNITS)) / 2U) {
		FAIL("Subrating not reset after interval change: max latency %lld ms\n", mx);
		return;
	}

	(void)bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	PASS("Central M->N + conn-update validated\n");
	bs_trace_silent_exit(0);
}

static void test_central_main_continuation(void)
{
	uint32_t interval_ms;
	int64_t idle_min, idle_max, cont_min, cont_max;

	if (central_start()) {
		return;
	}
	SUBRATE_WAIT(default_conn && subrate_factor == SUBRATE_CONT_FACTOR);

	interval_ms = interval_to_ms(CONN_INTERVAL_UNITS);

	/* Idle, widely-spaced reads: the peripheral re-skips between them, so at
	 * least one read waits ~the skip period (continuation window expired).
	 */
	k_sleep(K_MSEC(CONT_IDLE_MS));
	if (probe_minmax(NUM_READS, READ_GAP_MS, &idle_min, &idle_max)) {
		return;
	}
	printk("Central continuation idle max %lld ms\n", idle_max);
	if (idle_max < (SUBRATE_CONT_FACTOR * interval_ms) / 2U) {
		FAIL("Idle reads did not show skip: max %lld ms\n", idle_max);
		return;
	}

	/* Close-spaced reads stay within the continuation window: each read's data
	 * reloads the window, so reads are answered immediately. Count reads that
	 * fall outside the window - tolerate one boundary slip but catch a window
	 * that is systematically broken (asserting only the min would hide that).
	 */
	int slow = 0;

	cont_min = -1;
	cont_max = 0;
	for (int i = 0; i < NUM_READS; i++) {
		int64_t lat = probe_read_latency();

		if (lat < 0) {
			return;
		}
		printk("Central continuation burst read %d latency %lld ms\n", i, lat);
		if (cont_min < 0 || lat < cont_min) {
			cont_min = lat;
		}
		if (lat > cont_max) {
			cont_max = lat;
		}
		if (lat > 2 * (int64_t)interval_ms) {
			slow++;
		}
		if (i < NUM_READS - 1) {
			k_sleep(K_MSEC(CONT_READ_GAP_MS));
		}
	}
	printk("Central continuation burst: min %lld max %lld, %d slow of %d\n",
	       cont_min, cont_max, slow, NUM_READS);
	if (cont_min > 2 * interval_ms) {
		FAIL("Continuation did not keep link awake: min %lld ms\n", cont_min);
		return;
	}
	if (slow > 2) {
		FAIL("Continuation window broke repeatedly: %d slow reads of %d\n",
		     slow, NUM_READS);
		return;
	}

	(void)bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	PASS("Central continuation validated (idle %lld ms, burst min %lld max %lld)\n",
	     idle_max, cont_min, cont_max);
	bs_trace_silent_exit(0);
}

static void test_central_main_latency(void)
{
	uint32_t interval_ms, effective;
	int64_t mn, mx;

	if (central_start()) {
		return;
	}
	SUBRATE_WAIT(default_conn && subrate_factor == SUBRATE_LAT_FACTOR);

	interval_ms = interval_to_ms(CONN_INTERVAL_UNITS);
	/* Peripheral skips factor*(latency+1) events; a probe waits up to that. */
	effective = SUBRATE_LAT_FACTOR * (SUBRATE_LAT_LATENCY + 1U);
	printk("Central latency probing: factor %u, effective skip %u events\n",
	       subrate_factor, effective);

	if (probe_minmax(NUM_READS, READ_GAP_MS, &mn, &mx)) {
		return;
	}
	printk("Central latency max read %lld ms\n", mx);
	/* Must reach into the stacked cadence (factor*(latency+1)); a factor-only
	 * skip would top out around SUBRATE_LAT_FACTOR*interval.
	 */
	if (mx < (effective * interval_ms) / 2U) {
		FAIL("Peripheral latency not stacked: max %lld ms < %u ms\n",
		     mx, (effective * interval_ms) / 2U);
		return;
	}
	/* ...and every read, not just the worst sample, must reach into the stacked
	 * cadence - a peripheral that ignored max_latency would top out near
	 * factor*interval on every read.
	 */
	if (mn < (effective * interval_ms) / 2U) {
		FAIL("Peripheral latency not consistently stacked: min %lld ms < %u ms\n",
		     mn, (effective * interval_ms) / 2U);
		return;
	}

	(void)bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	PASS("Central peripheral-latency validated (min %lld max %lld ms)\n", mn, mx);
	bs_trace_silent_exit(0);
}

#if defined(CONFIG_BT_CTLR_TEST_CONN_EVENT_COUNT)
/* Regression lock for the central-burst bug: under peripheral max_latency
 * stacking the central must stay on its subrate cadence (present every `factor`
 * events) and must NOT break latency to ~full rate on the peripheral's expected
 * skipped events. Reuses the peripheral_latency peer (factor 4, max_latency 2,
 * cn 0, then idle) and counts the central's own on-air events over an idle
 * window - the only way to see the burst, which has no peripheral-observable
 * effect.
 */
static void test_central_main_lat_count(void)
{
	uint32_t interval_ms, expected, threshold, wakes;
	uint32_t before;

	if (central_start()) {
		return;
	}
	SUBRATE_WAIT(default_conn && subrate_factor == SUBRATE_LAT_FACTOR);

	/* Let the subrate negotiation and any feature/PHY exchange settle so the
	 * steady-state count is clean.
	 */
	k_sleep(K_MSEC(SETTLE_DELAY_MS));

	interval_ms = interval_to_ms(CONN_INTERVAL_UNITS);

	before = ll_test_conn_event_count[0];
	k_sleep(K_MSEC(LAT_COUNT_WINDOW_MS));
	wakes = ll_test_conn_event_count[0] - before;

	/* Subrate cadence: the central is present once per `factor` events,
	 * regardless of the peripheral coasting further on max_latency.
	 */
	expected = LAT_COUNT_WINDOW_MS / (SUBRATE_LAT_FACTOR * interval_ms);
	threshold = expected + (expected / 2U) + 2U; /* 1.5x + slack */
	printk("Central present %u events in %u ms (subrate cadence ~%u, "
	       "threshold %u, factor %u)\n",
	       wakes, LAT_COUNT_WINDOW_MS, expected, threshold, subrate_factor);

	if (wakes > threshold) {
		FAIL("Central over-present: %u > %u events - subrate skip broken by "
		     "peripheral latency (collapsed toward full rate)\n", wakes, threshold);
		return;
	}
	/* Sanity: still present at roughly the cadence (not stalled/disconnected). */
	if (wakes < (expected / 2U)) {
		FAIL("Central barely present: %u < %u events - link stalled?\n",
		     wakes, expected / 2U);
		return;
	}

	(void)bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	PASS("Central held subrate cadence under peripheral latency (%u events "
	     "in %u ms, factor %u)\n", wakes, LAT_COUNT_WINDOW_MS, subrate_factor);
	bs_trace_silent_exit(0);
}

/* Exact central skip-cadence lock at the negotiated factor (no peripheral
 * latency stacking, so central and peripheral are aligned). The central must be
 * present ~once per `factor` events. Run against peripherals negotiating
 * different factors -- including non-power-of-2 -- to catch over-skip,
 * under-skip/burst and subrated-event phase-math errors, the dimension the
 * read-latency probes can't measure precisely.
 */
static void test_central_main_cadence(void)
{
	uint32_t interval_ms, expected, hi, lo, wakes, before, factor;

	if (central_start()) {
		return;
	}
	SUBRATE_WAIT(default_conn && subrate_factor >= 2U);
	factor = subrate_factor;

	/* Let the subrate negotiation and any feature/PHY exchange settle. */
	k_sleep(K_MSEC(SETTLE_DELAY_MS));
	interval_ms = interval_to_ms(CONN_INTERVAL_UNITS);

	before = ll_test_conn_event_count[0];
	k_sleep(K_MSEC(LAT_COUNT_WINDOW_MS));
	wakes = ll_test_conn_event_count[0] - before;

	expected = LAT_COUNT_WINDOW_MS / (factor * interval_ms);
	hi = expected + (expected / 2U) + 2U;
	lo = (expected > 2U) ? (expected / 2U) : 1U;
	printk("Central present %u events in %u ms (factor %u, cadence ~%u, [%u..%u])\n",
	       wakes, LAT_COUNT_WINDOW_MS, factor, expected, lo, hi);

	if (wakes > hi) {
		FAIL("Central over-present: %u > %u events - skip cadence broken "
		     "(factor %u)\n", wakes, hi, factor);
		return;
	}
	if (wakes < lo) {
		FAIL("Central under-present: %u < %u events - over-skip/phase error "
		     "(factor %u)\n", wakes, lo, factor);
		return;
	}

	(void)bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	PASS("Central skip cadence correct (%u events in %u ms, factor %u)\n",
	     wakes, LAT_COUNT_WINDOW_MS, factor);
	bs_trace_silent_exit(0);
}

static int central_init_multi(void)
{
	struct bt_conn_le_subrate_param defaults = {
		.subrate_min = 1U,
		.subrate_max = SUBRATE_ACC_MAX,
		.max_latency = 10U,
		.continuation_number = 0U,
		.supervision_timeout = CONN_TIMEOUT_UNITS,
	};
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	err = bt_conn_le_subrate_set_defaults(&defaults);
	if (err) {
		FAIL("Set default subrate failed (err %d)\n", err);
		return err;
	}

	return 0;
}

/* Multi-connection: the Central holds two links - one peer requests subrating,
 * the other never does (full rate). The subrated link must hold its skip
 * cadence while the full-rate link is active every event; a scheduler that
 * lets the busy link drag the subrated one back to full rate is caught here.
 * This mirrors the real split topology (subrated split link + non-subrated host
 * link on one Central).
 */
static void test_central_main_multi(void)
{
	uint32_t interval_ms, expected_sub, hi_sub, wakes_sub, wakes_plain;
	uint32_t before_sub, before_plain;
	uint16_t h0, h1, plain_handle, factor;
	int i;

	multi_mode = true;
	if (central_init_multi()) {
		return;
	}
	/* Connect to both peers one at a time, driving the scan from this thread.
	 * device_found_multi stops the scan and creates each link; the connected
	 * callback only bumps m_nconns (no HCI calls from callback context).
	 */
	for (i = 0; i < 2; i++) {
		int err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found_multi);

		if (err) {
			FAIL("Multi: scan start for link %d failed (err %d)\n", i, err);
			return;
		}
		SUBRATE_WAIT(m_nconns > i);
	}
	/* One peer requests subrating; wait for it to be negotiated. */
	SUBRATE_WAIT(m_subrated_handle != 0xFFFFU);
	factor = subrate_factor;

	if (bt_hci_get_conn_handle(m_conn[0], &h0) ||
	    bt_hci_get_conn_handle(m_conn[1], &h1)) {
		FAIL("Multi: could not read conn handles\n");
		return;
	}
	plain_handle = (h0 == m_subrated_handle) ? h1 : h0;
	printk("Multi: subrated handle %u (factor %u), plain handle %u\n",
	       m_subrated_handle, factor, plain_handle);

	/* Let both links settle (negotiation, feature/PHY exchange). */
	k_sleep(K_MSEC(SETTLE_DELAY_MS));
	interval_ms = interval_to_ms(CONN_INTERVAL_UNITS);

	before_sub = ll_test_conn_event_count[m_subrated_handle];
	before_plain = ll_test_conn_event_count[plain_handle];
	k_sleep(K_MSEC(LAT_COUNT_WINDOW_MS));
	wakes_sub = ll_test_conn_event_count[m_subrated_handle] - before_sub;
	wakes_plain = ll_test_conn_event_count[plain_handle] - before_plain;

	expected_sub = LAT_COUNT_WINDOW_MS / (factor * interval_ms);
	hi_sub = expected_sub + (expected_sub / 2U) + 2U;
	printk("Multi: subrated link %u events (cadence ~%u, <=%u), plain link %u events "
	       "in %u ms\n", wakes_sub, expected_sub, hi_sub, wakes_plain, LAT_COUNT_WINDOW_MS);

	/* The subrated link keeps its cadence despite the concurrent full-rate link. */
	if (wakes_sub > hi_sub) {
		FAIL("Multi: subrated link over-present: %u > %u events - cadence "
		     "broken by the concurrent link\n", wakes_sub, hi_sub);
		return;
	}
	if (wakes_sub < (expected_sub / 2U)) {
		FAIL("Multi: subrated link stalled: %u events\n", wakes_sub);
		return;
	}
	/* Sanity: the other link really is full rate (clearly above the subrated
	 * cadence), confirming we measured two distinct links.
	 */
	if (wakes_plain < (2U * hi_sub)) {
		FAIL("Multi: plain link not full-rate: %u events (expected >> %u)\n",
		     wakes_plain, hi_sub);
		return;
	}

	for (i = 0; i < 2; i++) {
		(void)bt_conn_disconnect(m_conn[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
	PASS("Multi: subrated link held cadence (%u events) alongside full-rate link "
	     "(%u events)\n", wakes_sub, wakes_plain);
	bs_trace_silent_exit(0);
}
#endif /* CONFIG_BT_CTLR_TEST_CONN_EVENT_COUNT */

static void test_central_main_collision(void)
{
	struct bt_conn_le_subrate_param to_central = {
		.subrate_min = 1U,
		.subrate_max = SUBRATE_COLL_CENTRAL,
		.max_latency = 0U,
		.continuation_number = 0U,
		.supervision_timeout = CONN_TIMEOUT_UNITS,
	};
	int64_t cmn, cmx;
	uint32_t civ;

	if (central_start()) {
		return;
	}
	SUBRATE_WAIT(default_conn && subrate_factor == SUBRATE_COLL_INITIAL);
	printk("Central: subrating established at factor %u\n", subrate_factor);

	/* Re-negotiate at the same uptime the peripheral does, forcing a collision. */
	while (k_uptime_get() < COLLISION_TIME_MS) {
		k_sleep(K_MSEC(20));
		if (bst_result == Failed) {
			return;
		}
	}
	collision_mode = true;
	(void)bt_conn_le_subrate_request(default_conn, &to_central);

	/* Let the collision resolve, then confirm the link is still functional and
	 * both sides converged: probe several times (a desync would drop the link
	 * or stall a read) and, if still subrated, check the skip cadence matches
	 * the Central's final factor. Subrate collisions have no spec-defined
	 * winner (no instant), so only agreement - not a particular factor - holds.
	 */
	k_sleep(K_MSEC(3000));
	if (!default_conn) {
		FAIL("Central lost connection after subrate collision\n");
		return;
	}
	civ = interval_to_ms(CONN_INTERVAL_UNITS);
	if (probe_minmax(NUM_READS / 2, READ_GAP_MS, &cmn, &cmx)) {
		return;
	}
	if (subrate_factor >= 2U && cmx < (subrate_factor * civ) / 2U) {
		FAIL("Post-collision skip inconsistent with factor %u: max %lld ms\n",
		     subrate_factor, cmx);
		return;
	}

	(void)bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	PASS("Central survived subrate collision (final factor %u, max %lld ms)\n",
	     subrate_factor, cmx);
	bs_trace_silent_exit(0);
}

static void test_central_main_phy(void)
{
	uint32_t interval_ms;
	int64_t mn, mx;
	int err;

	if (central_start()) {
		return;
	}
	SUBRATE_WAIT(default_conn && subrate_factor >= 2U);
	interval_ms = interval_to_ms(CONN_INTERVAL_UNITS);

	if (probe_minmax(NUM_READS / 2, READ_GAP_MS, &mn, &mx)) {
		return;
	}
	if (mx < (subrate_factor * interval_ms) / 2U) {
		FAIL("No skip before PHY update: max %lld ms\n", mx);
		return;
	}

	/* A PHY update is an instant-based procedure: the steady-state gate must
	 * keep both present every event so it completes, and subrating must
	 * survive it (the interval is unchanged).
	 */
	phy_updated = false;
	err = bt_conn_le_phy_update(default_conn, BT_CONN_LE_PHY_PARAM_2M);
	if (err) {
		FAIL("Central PHY update request failed (err %d)\n", err);
		return;
	}
	SUBRATE_WAIT(phy_updated);
	printk("Central: PHY updated while subrated\n");

	if (probe_minmax(NUM_READS / 2, READ_GAP_MS, &mn, &mx)) {
		return;
	}
	if (mx < (subrate_factor * interval_ms) / 2U) {
		FAIL("Subrating lost after PHY update: max %lld ms\n", mx);
		return;
	}

	(void)bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	PASS("Central PHY-update-while-subrated validated\n");
	bs_trace_silent_exit(0);
}

static void test_central_main_disable(void)
{
	struct bt_conn_le_subrate_param to_one = {
		.subrate_min = 1U,
		.subrate_max = 1U,
		.max_latency = 0U,
		.continuation_number = 0U,
		.supervision_timeout = CONN_TIMEOUT_UNITS,
	};
	uint32_t interval_ms;
	int64_t mn, mx;
	int err;

	if (central_start()) {
		return;
	}
	SUBRATE_WAIT(default_conn && subrate_factor >= 2U);
	interval_ms = interval_to_ms(CONN_INTERVAL_UNITS);

	if (probe_minmax(NUM_READS / 2, READ_GAP_MS, &mn, &mx)) {
		return;
	}
	if (mx < (subrate_factor * interval_ms) / 2U) {
		FAIL("No skip before disable: max %lld ms\n", mx);
		return;
	}

	/* Explicitly disable subrating (factor 1); skipping must stop. */
	err = bt_conn_le_subrate_request(default_conn, &to_one);
	if (err) {
		FAIL("Central N->1 subrate request failed (err %d)\n", err);
		return;
	}
	SUBRATE_WAIT(subrate_factor == 1U);
	printk("Central: subrating disabled (factor 1)\n");

	if (probe_minmax(NUM_READS / 2, READ_GAP_MS, &mn, &mx)) {
		return;
	}
	if (mx >= 3U * interval_ms) {
		FAIL("Still skipping after N->1 disable: max %lld ms\n", mx);
		return;
	}

	(void)bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	PASS("Central N->1 disable validated (max latency %lld ms)\n", mx);
	bs_trace_silent_exit(0);
}

/* Notifications-under-subrating: discover HRS, subscribe, and count the
 * notifications that arrive while subrated (the peripheral can only send them
 * on subrated/continuation events, so this checks the data-TX path is alive).
 */
static struct bt_uuid_16 nfy_uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params nfy_disc;
static struct bt_gatt_subscribe_params nfy_sub;
static volatile int notify_count;
static volatile bool subscribed;

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			 const void *data, uint16_t length)
{
	if (!data) {
		return BT_GATT_ITER_STOP;
	}
	notify_count++;
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t notify_disc_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(nfy_disc.uuid, BT_UUID_HRS)) {
		memcpy(&nfy_uuid, BT_UUID_HRS_MEASUREMENT, sizeof(nfy_uuid));
		nfy_disc.uuid = &nfy_uuid.uuid;
		nfy_disc.start_handle = attr->handle + 1;
		nfy_disc.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		(void)bt_gatt_discover(conn, &nfy_disc);
	} else if (!bt_uuid_cmp(nfy_disc.uuid, BT_UUID_HRS_MEASUREMENT)) {
		memcpy(&nfy_uuid, BT_UUID_GATT_CCC, sizeof(nfy_uuid));
		nfy_disc.uuid = &nfy_uuid.uuid;
		nfy_disc.start_handle = attr->handle + 2;
		nfy_disc.type = BT_GATT_DISCOVER_DESCRIPTOR;
		nfy_sub.value_handle = attr->handle + 1;
		(void)bt_gatt_discover(conn, &nfy_disc);
	} else {
		nfy_sub.notify = notify_cb;
		nfy_sub.value = BT_GATT_CCC_NOTIFY;
		nfy_sub.ccc_handle = attr->handle;
		err = bt_gatt_subscribe(conn, &nfy_sub);
		if (err && err != -EALREADY) {
			FAIL("Central subscribe failed (err %d)\n", err);
		} else {
			subscribed = true;
		}
	}

	return BT_GATT_ITER_STOP;
}

static void test_central_main_notify(void)
{
	int before;
	int err;

	if (central_start()) {
		return;
	}
	SUBRATE_WAIT(central_connected);

	memcpy(&nfy_uuid, BT_UUID_HRS, sizeof(nfy_uuid));
	nfy_disc.uuid = &nfy_uuid.uuid;
	nfy_disc.func = notify_disc_cb;
	nfy_disc.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	nfy_disc.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	nfy_disc.type = BT_GATT_DISCOVER_PRIMARY;
	err = bt_gatt_discover(default_conn, &nfy_disc);
	if (err) {
		FAIL("Central discover failed (err %d)\n", err);
		return;
	}

	SUBRATE_WAIT(subscribed && subrate_factor >= 2U);
	printk("Central: subscribed, subrating factor %u\n", subrate_factor);

	before = notify_count;
	k_sleep(K_SECONDS(4));
	printk("Central: %d notifications under subrating\n", notify_count - before);
	/* The peripheral notifies every SUBRATE_NOTIFY_PERIOD_MS (> the skip period),
	 * so ~4000/period are expected; each is queued during a skip and must wake
	 * the peripheral on a subrated event. Require most of them to arrive.
	 */
	if ((notify_count - before) < (4000 / SUBRATE_NOTIFY_PERIOD_MS) - 3) {
		FAIL("Too few notifications under subrating: %d\n", notify_count - before);
		return;
	}

	(void)bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	PASS("Central notifications-under-subrating validated (%d in 4 s)\n",
	     notify_count - before);
	bs_trace_silent_exit(0);
}

static void test_central_main_supervision(void)
{
	struct bt_conn_le_subrate_param hi = {
		.subrate_min = 1U,
		.subrate_max = SUBRATE_SUPERVISION_FACTOR,
		.max_latency = 0U,
		.continuation_number = 0U,
		.supervision_timeout = CONN_TIMEOUT_UNITS,
	};
	int64_t lat;

	if (central_start()) {
		return;
	}
	/* Raise the acceptable factor ceiling so the large request is granted. */
	(void)bt_conn_le_subrate_set_defaults(&hi);

	SUBRATE_WAIT(default_conn && subrate_factor == SUBRATE_SUPERVISION_FACTOR);
	printk("Central: factor %u, skip ~%u ms, supervision %u ms\n", subrate_factor,
	       subrate_factor * interval_to_ms(CONN_INTERVAL_UNITS), CONN_TIMEOUT_UNITS * 10U);

	/* Idle for many skip cycles with no data: the link survives only if the
	 * peripheral keeps listening on subrated events within the supervision
	 * timeout - a too-long skip would drop the link.
	 */
	k_sleep(K_SECONDS(10));
	if (!default_conn) {
		FAIL("Link dropped while idle-subrated near the supervision bound\n");
		return;
	}
	/* And it is still functional. */
	lat = probe_read_latency();
	if (lat < 0) {
		return;
	}

	(void)bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	PASS("Central supervision-boundary validated (factor %u survived 10 s idle)\n",
	     subrate_factor);
	bs_trace_silent_exit(0);
}

/* Central->Peripheral continuation: discover the write-without-response
 * characteristic and burst data to it (one write per connection event).
 */
static struct bt_uuid_128 cw_uuid;
static struct bt_gatt_discover_params cw_disc;
static volatile uint16_t cwrite_handle;

static uint8_t cwrite_disc_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      struct bt_gatt_discover_params *params)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	if (!attr) {
		return BT_GATT_ITER_STOP;
	}
	cwrite_handle = bt_gatt_attr_value_handle(attr);
	return BT_GATT_ITER_STOP;
}

static void test_central_main_cwrite(void)
{
	uint8_t val = 0;
	uint32_t period_ms;
	int64_t end;
	int err;

	if (central_start()) {
		return;
	}
	SUBRATE_WAIT(central_connected && subrate_factor == SUBRATE_CONT_FACTOR);

	memcpy(&cw_uuid, BT_UUID_DECLARE_128(CWRITE_CHR_UUID), sizeof(cw_uuid));
	cw_disc.uuid = &cw_uuid.uuid;
	cw_disc.func = cwrite_disc_cb;
	cw_disc.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	cw_disc.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	cw_disc.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	err = bt_gatt_discover(default_conn, &cw_disc);
	if (err) {
		FAIL("Central discover (cwrite) failed (err %d)\n", err);
		return;
	}
	SUBRATE_WAIT(cwrite_handle != 0U);
	printk("Central: bursting writes to handle %u\n", cwrite_handle);

	/* One write per connection event for several seconds. If the Central's own
	 * Tx opens the continuation window the writes go out at the event rate;
	 * otherwise they are throttled to the subrate cadence.
	 */
	period_ms = interval_to_ms(CONN_INTERVAL_UNITS);
	end = k_uptime_get() + 6000;
	while (k_uptime_get() < end) {
		err = bt_gatt_write_without_response(default_conn, cwrite_handle, &val,
						     sizeof(val), false);
		if (err == -ENOMEM) {
			k_sleep(K_MSEC(period_ms));
			continue;
		}
		if (err) {
			FAIL("Central write-without-response failed (err %d)\n", err);
			return;
		}
		val++;
		k_sleep(K_MSEC(period_ms));
	}

	(void)bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	PASS("Central C->P continuation burst sent\n");
	bs_trace_silent_exit(0);
}

static void test_central_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME * 1e6);
	bst_result = In_progress;
}

static void test_central_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("central failed (not passed after %i seconds)\n", WAIT_TIME);
	}
}

static const struct bst_test_instance test_central[] = {
	{
		.test_id = "central",
		.test_descr = "Central: verify the peripheral skips events.",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main,
	},
	{
		.test_id = "central_transitions",
		.test_descr = "Central: M->N transition then conn-update reset.",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main_transitions,
	},
	{
		.test_id = "central_continuation",
		.test_descr = "Central: continuation events keep the link awake "
			      "after data, idle reads still show the skip.",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main_continuation,
	},
	{
		.test_id = "central_latency",
		.test_descr = "Central: peripheral latency stacks on the factor; "
			      "they still rendezvous and the latency is higher.",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main_latency,
	},
#if defined(CONFIG_BT_CTLR_TEST_CONN_EVENT_COUNT)
	{
		.test_id = "central_lat_count",
		.test_descr = "Central: under peripheral max_latency stacking the "
			      "central stays on its subrate cadence and does not break "
			      "latency to full rate on the peripheral's skipped events.",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main_lat_count,
	},
	{
		.test_id = "central_cadence",
		.test_descr = "Central: exact on-air skip cadence at the negotiated "
			      "factor (catches over/under-skip and phase errors; run "
			      "at several factors incl non-power-of-2).",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main_cadence,
	},
	{
		.test_id = "central_multi",
		.test_descr = "Central: two links (one subrated, one full-rate); the "
			      "subrated link holds its cadence while the other is busy.",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main_multi,
	},
#endif
	{
		.test_id = "central_collision",
		.test_descr = "Central: re-negotiates subrating simultaneously with "
			      "the peripheral (LLCP collision); link must survive.",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main_collision,
	},
	{
		.test_id = "central_phy",
		.test_descr = "Central: PHY update while subrated; subrating survives "
			      "the instant-based procedure.",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main_phy,
	},
	{
		.test_id = "central_disable",
		.test_descr = "Central: explicitly disable subrating (N->1); skipping "
			      "stops.",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main_disable,
	},
	{
		.test_id = "central_notify",
		.test_descr = "Central: subscribe to HRS and verify notifications flow "
			      "while subrated.",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main_notify,
	},
	{
		.test_id = "central_supervision",
		.test_descr = "Central: large factor near the supervision-timeout bound; "
			      "idle link must survive.",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main_supervision,
	},
	{
		.test_id = "central_cwrite",
		.test_descr = "Central: bursts write-without-response to the peripheral "
			      "under continuation.",
		.test_pre_init_f = test_central_init,
		.test_tick_f = test_central_tick,
		.test_main_f = test_central_main_cwrite,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_central_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_central);
}
