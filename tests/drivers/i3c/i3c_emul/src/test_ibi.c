/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * M3 IBI tests for the I3C emulator: enable/disable propagation,
 * synchronous and workqueue-based IBI delivery, and HJ/CRR ack gating.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/drivers/i3c/ibi.h>
#include <zephyr/drivers/i3c_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "test_target_emul.h"

#define I3C_BUS DT_NODELABEL(i3c0)
#define TARGET_A DT_NODELABEL(test_target_a)

#define TARGET_A_PID		TEST_TARGET_A_PID

static const struct device *bus = DEVICE_DT_GET(I3C_BUS);
static const struct emul *target_a = EMUL_DT_GET(TARGET_A);

static struct {
	atomic_t calls;
	uint8_t last_payload[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];
	uint8_t last_payload_len;
	struct i3c_device_desc *last_target;
	struct k_sem fired;
} g_ibi;

static int test_ibi_cb(struct i3c_device_desc *target, struct i3c_ibi_payload *payload)
{
	atomic_inc(&g_ibi.calls);
	g_ibi.last_target = target;
	if (payload != NULL) {
		g_ibi.last_payload_len = payload->payload_len;
		if (payload->payload_len <= sizeof(g_ibi.last_payload)) {
			memcpy(g_ibi.last_payload, payload->payload, payload->payload_len);
		}
	} else {
		g_ibi.last_payload_len = 0;
	}
	k_sem_give(&g_ibi.fired);
	return 0;
}

static struct i3c_device_desc *find_desc(uint64_t pid)
{
	struct i3c_device_id id = { .pid = pid };

	return i3c_device_find(bus, &id);
}

static void *i3c_emul_ibi_setup(void)
{
	struct i3c_device_desc *desc;
	int rc = test_target_bus_known_state(bus, TEST_TARGET_A_PID, TEST_TARGET_A_STATIC,
					     TEST_TARGET_B_PID, TEST_TARGET_B_INIT_DA);

	zassert_ok(rc, "test_target_bus_known_state: %d", rc);

	desc = find_desc(TARGET_A_PID);
	zassert_not_null(desc, "target A desc");
	desc->ibi_cb = test_ibi_cb;
	k_sem_init(&g_ibi.fired, 0, 1);
	return NULL;
}

static void i3c_emul_ibi_before(void *fixture)
{
	ARG_UNUSED(fixture);
	atomic_set(&g_ibi.calls, 0);
	g_ibi.last_payload_len = 0;
	g_ibi.last_target = NULL;
	memset(g_ibi.last_payload, 0, sizeof(g_ibi.last_payload));
	k_sem_reset(&g_ibi.fired);
	(void)i3c_ibi_hj_response(bus, false);

	/* Re-establish the canonical address state in case a prior test
	 * (RSTDAA, HJ-driven DAA, etc.) left things in a different shape.
	 */
	(void)test_target_bus_known_state(bus, TEST_TARGET_A_PID, TEST_TARGET_A_STATIC,
					  TEST_TARGET_B_PID, TEST_TARGET_B_INIT_DA);

	/* Re-establish the post-bus-init event-mask baseline (only HJ
	 * enabled); a prior test's DISEC/ENEC could have left it skewed.
	 */
	{
		struct i3c_ccc_events ev_all = { .events = I3C_CCC_EVT_ALL };
		struct i3c_ccc_events ev_hj = { .events = I3C_CCC_EVT_HJ };

		(void)i3c_ccc_do_events_all_set(bus, false, &ev_all);
		(void)i3c_ccc_do_events_all_set(bus, true, &ev_hj);
	}
}

ZTEST(i3c_emul_ibi, test_ibi_disabled_drops)
{
	uint8_t payload[] = {0xAA, 0xBB};
	int rc = test_target_trigger_ibi(target_a, payload, sizeof(payload));

	zassert_equal(rc, -ENOTCONN, "expected -ENOTCONN with IBI disabled, got %d", rc);
	zassert_equal(atomic_get(&g_ibi.calls), 0, "callback should not have fired");
}

ZTEST(i3c_emul_ibi, test_ibi_enabled_delivers_payload)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);
	uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
	int rc;

	rc = i3c_ibi_enable(desc);
	zassert_ok(rc, "i3c_ibi_enable: %d", rc);
	zassert_true(test_target_event_enabled(target_a, I3C_CCC_EVT_INTR),
		     "i3c_ibi_enable should ENEC(INTR) on the wire");

	rc = test_target_trigger_ibi(target_a, payload, sizeof(payload));
	zassert_ok(rc, "trigger_ibi: %d", rc);

	/*
	 * Sync mode: the callback fires inline before trigger_ibi returns,
	 * so the sem is already given. Workqueue mode: the IBI is enqueued
	 * and the callback runs from i3c_ibi_workq; pend on the sem until
	 * the callback signals it.
	 */
	zassert_ok(k_sem_take(&g_ibi.fired, K_MSEC(100)),
		   "ibi_cb did not fire within 100ms");

	zassert_equal(atomic_get(&g_ibi.calls), 1, "callback fired once");
	zassert_equal(g_ibi.last_target, desc, "callback target matches");
	zassert_equal(g_ibi.last_payload_len, sizeof(payload), "payload length");
	zassert_mem_equal(g_ibi.last_payload, payload, sizeof(payload), "payload bytes");

	(void)i3c_ibi_disable(desc);
}

ZTEST(i3c_emul_ibi, test_hj_nack)
{
	int rc;

	/*
	 * HJ is only legal when the target has no dynamic address. RSTDAA
	 * to clear it, then assert the controller's NACK preference blocks
	 * the HJ at the bus emulator (target_raise_hj returns -ENOTCONN).
	 * Per-test before-hook restores the canonical address state for
	 * subsequent tests.
	 */
	rc = i3c_bus_rstdaa_all(bus);
	zassert_ok(rc, "rstdaa: %d", rc);

	rc = i3c_ibi_hj_response(bus, false);
	zassert_ok(rc, "hj_response false: %d", rc);

	rc = test_target_trigger_hj(target_a);
	zassert_equal(rc, -ENOTCONN, "expected -ENOTCONN when HJ NACKed, got %d", rc);
	zassert_equal(test_target_get_dynamic_addr(target_a), 0,
		      "NACKed HJ must not give the target a dynamic address");
}

ZTEST(i3c_emul_ibi, test_hj_ack)
{
	int rc;

	/*
	 * Spec model: a target with no dynamic address raises HJ; the
	 * active controller has armed HJ ACK, so it ACKs and runs ENTDAA
	 * (i3c_do_daa) from the IBI workqueue handler. After the workq
	 * drains, the target should have a dynamic address. Per-test
	 * before-hook restores the canonical address state.
	 */
	rc = i3c_bus_rstdaa_all(bus);
	zassert_ok(rc, "rstdaa: %d", rc);
	zassert_equal(test_target_get_dynamic_addr(target_a), 0,
		      "precondition: target A should have no dynamic address");

	rc = i3c_ibi_hj_response(bus, true);
	zassert_ok(rc, "hj_response true: %d", rc);

	rc = test_target_trigger_hj(target_a);
	zassert_ok(rc, "trigger_hj after ACK: %d", rc);

	zassert_true(WAIT_FOR(test_target_get_dynamic_addr(target_a) != 0,
			      USEC_PER_MSEC * 100, k_msleep(1)),
		     "ACKed HJ must trigger DAA and give the target a dynamic address");

	(void)i3c_ibi_hj_response(bus, false);
}

ZTEST(i3c_emul_ibi, test_hj_rejected_when_target_has_da)
{
	int rc;

	/*
	 * Target A enters this test with a dynamic address from the suite
	 * setup. Per spec, HJ is invalid for a device that already has a
	 * DA — the bus emulator must reject it before any controller
	 * preference is even consulted.
	 */
	zassert_not_equal(test_target_get_dynamic_addr(target_a), 0,
			  "precondition: target A should have a dynamic address");

	rc = test_target_trigger_hj(target_a);
	zassert_equal(rc, -EACCES,
		      "HJ from a target with a DA must be rejected, got %d", rc);
}

ZTEST(i3c_emul_ibi, test_crr_nack)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);
	struct i3c_ccc_events events = { .events = I3C_CCC_EVT_CR };
	int rc;

	zassert_not_null(desc, "target A desc");

	rc = i3c_ccc_do_events_set(desc, true, &events);
	zassert_ok(rc, "ENEC(CR): %d", rc);

	rc = i3c_ibi_crr_response(desc, false);
	zassert_ok(rc, "crr_response false: %d", rc);

	rc = test_target_trigger_crr(target_a);
	zassert_equal(rc, -ENOTCONN, "expected -ENOTCONN when CRR NACKed, got %d", rc);
}

ZTEST(i3c_emul_ibi, test_crr_ack)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);
	struct i3c_ccc_events events = { .events = I3C_CCC_EVT_CR };
	int rc;

	zassert_not_null(desc, "target A desc");

	rc = i3c_ccc_do_events_set(desc, true, &events);
	zassert_ok(rc, "ENEC(CR): %d", rc);

	rc = i3c_ibi_crr_response(desc, true);
	zassert_ok(rc, "crr_response true: %d", rc);

	rc = test_target_trigger_crr(target_a);
	zassert_ok(rc, "trigger_crr after ACK: %d", rc);

	(void)i3c_ibi_crr_response(desc, false);
}

ZTEST(i3c_emul_ibi, test_disec_hj_blocks_target_from_raising_hj)
{
	struct i3c_ccc_events events = { .events = I3C_CCC_EVT_HJ };
	int rc;

	/*
	 * Bus init broadcast ENEC(HJ), so peripheral baseline is HJ-enabled.
	 * Confirm baseline, then DISEC(HJ) and verify the peripheral itself
	 * refuses to drive HJ even though the controller is armed to ACK.
	 */
	zassert_true(test_target_event_enabled(target_a, I3C_CCC_EVT_HJ),
		     "peripheral baseline: HJ enabled by bus-init ENEC(HJ)");

	rc = i3c_bus_rstdaa_all(bus);
	zassert_ok(rc, "rstdaa: %d", rc);
	rc = i3c_ibi_hj_response(bus, true);
	zassert_ok(rc, "hj_response true: %d", rc);

	rc = i3c_ccc_do_events_all_set(bus, false, &events);
	zassert_ok(rc, "DISEC(HJ): %d", rc);
	zassert_false(test_target_event_enabled(target_a, I3C_CCC_EVT_HJ),
		      "peripheral observed DISEC(HJ)");

	rc = test_target_trigger_hj(target_a);
	zassert_not_equal(rc, 0,
			  "peripheral with HJ disabled must refuse to raise HJ");

	(void)i3c_ibi_hj_response(bus, false);
}

ZTEST(i3c_emul_ibi, test_disec_cr_blocks_target_from_raising_crr)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);
	struct i3c_ccc_events events = { .events = I3C_CCC_EVT_CR };
	int rc;

	zassert_not_null(desc, "target A desc");

	rc = i3c_ibi_crr_response(desc, true);
	zassert_ok(rc, "crr_response true: %d", rc);

	rc = i3c_ccc_do_events_all_set(bus, false, &events);
	zassert_ok(rc, "DISEC(CR): %d", rc);
	zassert_false(test_target_event_enabled(target_a, I3C_CCC_EVT_CR),
		      "peripheral observed DISEC(CR)");

	rc = test_target_trigger_crr(target_a);
	zassert_not_equal(rc, 0,
			  "peripheral with CR disabled must refuse to raise CRR");

	(void)i3c_ibi_crr_response(desc, false);
}

ZTEST(i3c_emul_ibi, test_disec_intr_blocks_target_from_raising_ibi)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);
	struct i3c_ccc_events events = { .events = I3C_CCC_EVT_INTR };
	uint8_t payload[] = {0x11, 0x22};
	int rc;

	zassert_not_null(desc, "target A desc");

	rc = i3c_ibi_enable(desc);
	zassert_ok(rc, "i3c_ibi_enable: %d", rc);

	rc = i3c_ccc_do_events_all_set(bus, false, &events);
	zassert_ok(rc, "DISEC(INTR): %d", rc);
	zassert_false(test_target_event_enabled(target_a, I3C_CCC_EVT_INTR),
		      "peripheral observed DISEC(INTR)");

	rc = test_target_trigger_ibi(target_a, payload, sizeof(payload));
	zassert_not_equal(rc, 0,
			  "peripheral with INTR disabled must refuse to raise IBI");

	(void)i3c_ibi_disable(desc);
}

ZTEST(i3c_emul_ibi, test_enec_re_enables_blocked_event)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);
	struct i3c_ccc_events events = { .events = I3C_CCC_EVT_INTR };
	uint8_t payload[] = {0x33, 0x44};
	int rc;

	zassert_not_null(desc, "target A desc");

	/*
	 * DISEC(INTR), then ENEC(INTR), then a successful trigger proves
	 * the bit-toggle round-trips and the peripheral re-arms.
	 */
	rc = i3c_ibi_enable(desc);
	zassert_ok(rc, "i3c_ibi_enable: %d", rc);

	rc = i3c_ccc_do_events_all_set(bus, false, &events);
	zassert_ok(rc, "DISEC(INTR): %d", rc);
	zassert_false(test_target_event_enabled(target_a, I3C_CCC_EVT_INTR),
		      "INTR cleared by DISEC");

	rc = i3c_ccc_do_events_all_set(bus, true, &events);
	zassert_ok(rc, "ENEC(INTR): %d", rc);
	zassert_true(test_target_event_enabled(target_a, I3C_CCC_EVT_INTR),
		     "INTR re-set by ENEC");

	rc = test_target_trigger_ibi(target_a, payload, sizeof(payload));
	zassert_ok(rc, "trigger_ibi after re-enable: %d", rc);

	(void)i3c_ibi_disable(desc);
}

ZTEST_SUITE(i3c_emul_ibi, NULL, i3c_emul_ibi_setup, i3c_emul_ibi_before, NULL, NULL);
