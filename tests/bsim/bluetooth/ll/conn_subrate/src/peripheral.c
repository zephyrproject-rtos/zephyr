/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Subrating peripheral. Advertise, accept a connection, then initiate a
 * Connection Subrate Request (Core Spec 5.1.20). The Central observes the
 * resulting event skipping and drives the assertions; the peripheral mostly
 * negotiates the requested parameters and stays alive.
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
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/hrs.h>

#include "conn_subrate.h"

#define WAIT_TIME 30 /* seconds */

extern enum bst_result_t bst_result;

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
static volatile bool connected_flag;
/* In the collision scenario both sides re-negotiate simultaneously; one request
 * may be rejected (a valid LLCP collision outcome) and the survival check, not
 * the subrate_changed callback, decides pass/fail.
 */
static volatile bool collision_mode;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

/* Write-without-response characteristic for the central->peripheral
 * continuation test; each write the Central sends increments the counter.
 */
static volatile int cwrite_count;

static ssize_t cwrite_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(buf);
	ARG_UNUSED(offset);
	ARG_UNUSED(flags);
	cwrite_count++;
	return len;
}

BT_GATT_SERVICE_DEFINE(cwrite_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_128(CWRITE_SVC_UUID)),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(CWRITE_CHR_UUID),
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP, BT_GATT_PERM_WRITE,
			       NULL, cwrite_cb, NULL));

static void subrate_changed(struct bt_conn *conn,
			    const struct bt_conn_le_subrate_changed *params)
{
	printk("Peripheral subrate changed: status %u factor %u cont %u "
	       "latency %u timeout %u\n", params->status, params->factor,
	       params->continuation_number, params->peripheral_latency,
	       params->supervision_timeout);

	if (params->status != BT_HCI_ERR_SUCCESS) {
		if (collision_mode) {
			/* A rejected request is an acceptable collision outcome. */
			return;
		}
		FAIL("Peripheral subrate change failed (status 0x%02x)\n",
		     params->status);
		return;
	}

	/* Negotiation succeeded; the Central validates the resulting skipping. */
	if (!collision_mode && params->factor >= 2U) {
		PASS("Peripheral subrating active (factor %u)\n", params->factor);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		FAIL("Peripheral failed to connect (err 0x%02x)\n", err);
		return;
	}

	default_conn = bt_conn_ref(conn);
	connected_flag = true;
	printk("Peripheral connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Peripheral disconnected (reason 0x%02x)\n", reason);

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
	connected_flag = false;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.subrate_changed = subrate_changed,
};

static int peripheral_setup(void)
{
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	printk("Peripheral Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return err;
	}

	printk("Peripheral advertising started\n");
	return 0;
}

static int periph_request_subrate(uint16_t subrate_min, uint16_t subrate_max,
				  uint16_t max_latency, uint16_t cont_number)
{
	struct bt_conn_le_subrate_param param = {
		.subrate_min = subrate_min,
		.subrate_max = subrate_max,
		.max_latency = max_latency,
		.continuation_number = cont_number,
		.supervision_timeout = SUBRATE_REQ_TIMEOUT,
	};
	int err = bt_conn_le_subrate_request(default_conn, &param);

	if (err) {
		FAIL("Peripheral subrate request failed (err %d)\n", err);
		return err;
	}
	printk("Peripheral requested subrating (min %u max %u lat %u cn %u)\n",
	       subrate_min, subrate_max, max_latency, cont_number);
	return 0;
}

/* Advertise, once connected and settled request subrating with the given
 * parameters, then stay alive responding to whatever the Central drives next.
 */
static void peripheral_run(uint16_t subrate_min, uint16_t subrate_max,
			   uint16_t max_latency, uint16_t cont_number)
{
	bool requested = false;

	if (peripheral_setup()) {
		return;
	}

	while (true) {
		k_sleep(K_MSEC(SETTLE_DELAY_MS));

		if (connected_flag && !requested && default_conn) {
			requested = true;
			if (periph_request_subrate(subrate_min, subrate_max,
						   max_latency, cont_number)) {
				return;
			}
		}
	}
}

static void test_peripheral_main(void)
{
	peripheral_run(SUBRATE_REQ_MIN, SUBRATE_REQ_MAX, 0U, 0U);
}

static void test_peripheral_main_transitions(void)
{
	/* Negotiate exactly factor M; the Central re-negotiates to N afterwards. */
	peripheral_run(SUBRATE_MTON_M, SUBRATE_MTON_M, 0U, 0U);
}

static void test_peripheral_main_continuation(void)
{
	peripheral_run(SUBRATE_CONT_FACTOR, SUBRATE_CONT_FACTOR, 0U, SUBRATE_CONT_CN);
}

static void test_peripheral_main_oddfactor(void)
{
	peripheral_run(SUBRATE_ODD_FACTOR, SUBRATE_ODD_FACTOR, 0U, 0U);
}

static void test_peripheral_main_supervision(void)
{
	peripheral_run(SUBRATE_SUPERVISION_FACTOR, SUBRATE_SUPERVISION_FACTOR, 0U, 0U);
}

static void test_peripheral_main_plain(void)
{
	if (peripheral_setup()) {
		return;
	}
	/* Full-rate peer for the multi-connection test: connect and idle, never
	 * request subrating. Mark pass once connected (the central drives and
	 * validates the cadence); unlike the subrating peers there is no
	 * subrate_changed to pass on. Then stay alive for the rest of the sim.
	 */
	while (!connected_flag) {
		k_sleep(K_MSEC(50));
	}
	PASS("Peripheral (plain) connected; full-rate peer\n");
	while (true) {
		k_sleep(K_MSEC(SETTLE_DELAY_MS));
	}
}

static void test_peripheral_main_cwrite(void)
{
	int before;

	if (peripheral_setup()) {
		return;
	}

	while (!connected_flag) {
		k_sleep(K_MSEC(50));
		if (bst_result == Failed) {
			return;
		}
	}
	k_sleep(K_MSEC(SETTLE_DELAY_MS));

	/* Subrate with continuation enabled, then count the Central's bursts. */
	if (periph_request_subrate(SUBRATE_CONT_FACTOR, SUBRATE_CONT_FACTOR, 0U,
				   SUBRATE_CONT_CN)) {
		return;
	}

	/* Let the Central discover the characteristic and start writing. */
	k_sleep(K_SECONDS(2));
	before = cwrite_count;
	k_sleep(K_SECONDS(3));
	printk("Peripheral received %d central writes under subrating\n",
	       cwrite_count - before);

	/* With the Central's own Tx opening the continuation window the burst
	 * arrives near the event rate (~100 in 3 s); throttled to the subrate
	 * cadence it would be ~factor times fewer (~12). Require well above that.
	 */
	if ((cwrite_count - before) < 30) {
		FAIL("Too few central writes under continuation: %d\n",
		     cwrite_count - before);
		return;
	}
	PASS("Peripheral C->P continuation validated (%d writes in 3 s)\n",
	     cwrite_count - before);
}

static void test_peripheral_main_latency(void)
{
	peripheral_run(SUBRATE_LAT_FACTOR, SUBRATE_LAT_FACTOR, SUBRATE_LAT_LATENCY, 0U);
}

static void test_peripheral_main_collision(void)
{
	if (peripheral_setup()) {
		return;
	}

	while (!connected_flag) {
		k_sleep(K_MSEC(50));
		if (bst_result == Failed) {
			return;
		}
	}
	k_sleep(K_MSEC(SETTLE_DELAY_MS));

	/* Establish subrating first. */
	if (periph_request_subrate(SUBRATE_COLL_INITIAL, SUBRATE_COLL_INITIAL, 0U, 0U)) {
		return;
	}

	/* Both sides re-negotiate at the same uptime to force an LLCP collision. */
	while (k_uptime_get() < COLLISION_TIME_MS) {
		k_sleep(K_MSEC(20));
	}
	collision_mode = true;
	(void)periph_request_subrate(SUBRATE_COLL_PERIPH, SUBRATE_COLL_PERIPH, 0U, 0U);

	/* Survive the collision: still connected a while later. Check before the
	 * central's end-of-test disconnect (it sleeps longer post-collision).
	 */
	k_sleep(K_MSEC(2000));
	if (!connected_flag) {
		FAIL("Peripheral lost connection after subrate collision\n");
		return;
	}
	PASS("Peripheral survived subrate collision\n");
}

static void test_peripheral_main_notify(void)
{
	uint16_t heartrate = 90U;
	bool requested = false;

	if (peripheral_setup()) {
		return;
	}

	/* Request subrating once connected+settled, then stream HRS notifications.
	 * Under subrating the controller can only send them on subrated/continuation
	 * events; the Central verifies they still flow.
	 */
	while (true) {
		k_sleep(K_MSEC(SUBRATE_NOTIFY_PERIOD_MS));

		if (connected_flag && !requested && default_conn) {
			requested = true;
			if (periph_request_subrate(SUBRATE_REQ_MIN, SUBRATE_REQ_MAX, 0U, 0U)) {
				return;
			}
		}

		if (connected_flag) {
			(void)bt_hrs_notify(heartrate);
			if (++heartrate > 160U) {
				heartrate = 90U;
			}
		}
	}
}

static void test_peripheral_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME * 1e6);
	bst_result = In_progress;
}

static void test_peripheral_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("peripheral failed (not passed after %i seconds)\n", WAIT_TIME);
	}
}

static const struct bst_test_instance test_peripheral[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral: requests subrating (factor > 1).",
		.test_pre_init_f = test_peripheral_init,
		.test_tick_f = test_peripheral_tick,
		.test_main_f = test_peripheral_main,
	},
	{
		.test_id = "peripheral_transitions",
		.test_descr = "Peripheral: negotiates factor M; Central re-negotiates "
			      "to N and changes the interval.",
		.test_pre_init_f = test_peripheral_init,
		.test_tick_f = test_peripheral_tick,
		.test_main_f = test_peripheral_main_transitions,
	},
	{
		.test_id = "peripheral_continuation",
		.test_descr = "Peripheral: subrating with continuation_number > 0.",
		.test_pre_init_f = test_peripheral_init,
		.test_tick_f = test_peripheral_tick,
		.test_main_f = test_peripheral_main_continuation,
	},
	{
		.test_id = "peripheral_latency",
		.test_descr = "Peripheral: subrating with peripheral latency > 0.",
		.test_pre_init_f = test_peripheral_init,
		.test_tick_f = test_peripheral_tick,
		.test_main_f = test_peripheral_main_latency,
	},
	{
		.test_id = "peripheral_collision",
		.test_descr = "Peripheral: re-negotiates subrating simultaneously with "
			      "the Central (LLCP collision); link must survive.",
		.test_pre_init_f = test_peripheral_init,
		.test_tick_f = test_peripheral_tick,
		.test_main_f = test_peripheral_main_collision,
	},
	{
		.test_id = "peripheral_notify",
		.test_descr = "Peripheral: streams HRS notifications while subrated.",
		.test_pre_init_f = test_peripheral_init,
		.test_tick_f = test_peripheral_tick,
		.test_main_f = test_peripheral_main_notify,
	},
	{
		.test_id = "peripheral_oddfactor",
		.test_descr = "Peripheral: negotiates a non-power-of-2 factor (5).",
		.test_pre_init_f = test_peripheral_init,
		.test_tick_f = test_peripheral_tick,
		.test_main_f = test_peripheral_main_oddfactor,
	},
	{
		.test_id = "peripheral_supervision",
		.test_descr = "Peripheral: large factor near the supervision-timeout bound.",
		.test_pre_init_f = test_peripheral_init,
		.test_tick_f = test_peripheral_tick,
		.test_main_f = test_peripheral_main_supervision,
	},
	{
		.test_id = "peripheral_cwrite",
		.test_descr = "Peripheral: counts the Central's write-without-response "
			      "burst under continuation.",
		.test_pre_init_f = test_peripheral_init,
		.test_tick_f = test_peripheral_tick,
		.test_main_f = test_peripheral_main_cwrite,
	},
	{
		.test_id = "peripheral_plain",
		.test_descr = "Peripheral: connects and idles, never requests subrating "
			      "(full-rate peer for the multi-connection test).",
		.test_pre_init_f = test_peripheral_init,
		.test_tick_f = test_peripheral_tick,
		.test_main_f = test_peripheral_main_plain,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_peripheral_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_peripheral);
}
