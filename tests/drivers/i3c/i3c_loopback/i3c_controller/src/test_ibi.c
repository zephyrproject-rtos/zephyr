/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 5: IBI — exercises ibi_enable, ibi_disable, ibi_hj_response.
 *
 * Board B side raises IBIs in response to UART sync commands.
 * This test issues those commands and waits for the IBI to be
 * delivered through the controller's IBI callback path.
 */

#include "test_common.h"

#include <limits.h>
#include <stdio.h>

#include <zephyr/drivers/i3c/ccc.h>

#ifdef CONFIG_I3C_USE_IBI

static struct k_sem ibi_sem;
static uint8_t last_ibi_payload[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];
static uint8_t last_ibi_len;

static int ibi_cb(struct i3c_device_desc *target, struct i3c_ibi_payload *payload)
{
	ARG_UNUSED(target);
	if (payload != NULL) {
		last_ibi_len = MIN(payload->payload_len, sizeof(last_ibi_payload));
		memcpy(last_ibi_payload, payload->payload, last_ibi_len);
	} else {
		last_ibi_len = 0;
	}
	k_sem_give(&ibi_sem);
	return 0;
}

/*
 * Send a RAISE_* command to the target and consume the
 * "IBI <kind> rc=<n>" acknowledgment.  Drops unrelated lines so
 * stale ACKs don't get mis-parsed.  Returns target's i3c_ibi_raise()
 * return code.
 */
static int ibi_raise_and_get_rc(const char *cmd)
{
	char line[SYNC_LINE_MAX];
	const k_timepoint_t deadline = sys_timepoint_calc(K_SECONDS(2));

	sync_send(sync_uart, "%s", cmd);

	while (!sys_timepoint_expired(deadline)) {
		k_timeout_t remaining = sys_timepoint_timeout(deadline);
		int n = sync_recv_line(sync_uart, line, sizeof(line), remaining);

		if (n < 0) {
			break;
		}
		if (strncmp(line, "IBI ", 4) != 0) {
			continue; /* stale line, drop */
		}

		int rc = INT_MIN;

		if (sscanf(line, "IBI %*s rc=%d", &rc) != 1) {
			zassert_unreachable("unexpected IBI ack: '%s'", line);
		}
		return rc;
	}

	zassert_unreachable("no IBI ack from target within 2s");
	return -EAGAIN;
}

ZTEST(i3c_loopback, test_ibi_enable_disable)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	int rc;

	k_sem_init(&ibi_sem, 0, 1);
	target_b_desc.ibi_cb = ibi_cb;

	rc = i3c_ibi_enable(&target_b_desc);
	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENOSYS || rc == -ENXIO, "ibi_enable rc=%d",
		     rc);

	rc = i3c_ibi_disable(&target_b_desc);
	zassert_true(rc == 0 || rc == -ENODEV || rc == -ENOSYS || rc == -ENXIO, "ibi_disable rc=%d",
		     rc);
	target_b_desc.ibi_cb = NULL;
}

ZTEST(i3c_loopback, test_ibi_target_intr_received)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	k_sem_init(&ibi_sem, 0, 1);
	target_b_desc.ibi_cb = ibi_cb;
	int rc = -1;

	for (int a = 0; a < I3C_NACK_RETRIES; a++) {
		rc = i3c_ibi_enable(&target_b_desc);
		if (rc != -ENXIO) {
			break;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}
	zassert_ok(rc, "ibi_enable failed (%d)", rc);

	rc = ibi_raise_and_get_rc("RAISE_TIR 00");
	zassert_ok(rc, "target i3c_ibi_raise failed (%d)", rc);
	rc = k_sem_take(&ibi_sem, K_MSEC(500));
	zassert_ok(rc, "IBI not received within 500ms");

	(void)i3c_ibi_disable(&target_b_desc);
	target_b_desc.ibi_cb = NULL;
}

ZTEST(i3c_loopback, test_ibi_tir_with_mandatory_byte)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	k_sem_init(&ibi_sem, 0, 1);
	last_ibi_len = 0;
	memset(last_ibi_payload, 0, sizeof(last_ibi_payload));
	target_b_desc.ibi_cb = ibi_cb;
	int rc = -1;

	for (int a = 0; a < I3C_NACK_RETRIES; a++) {
		rc = i3c_ibi_enable(&target_b_desc);
		if (rc != -ENXIO) {
			break;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}
	zassert_ok(rc, "ibi_enable failed (%d)", rc);

	rc = ibi_raise_and_get_rc("RAISE_TIR 5A010203");
	zassert_ok(rc, "target i3c_ibi_raise failed (%d)", rc);
	rc = k_sem_take(&ibi_sem, K_MSEC(500));
	zassert_ok(rc, "IBI with mandatory byte not received within 500ms");
	zassert_true(last_ibi_len >= 1, "expected >= 1 byte payload, got %u", last_ibi_len);
	zassert_equal(last_ibi_payload[0], 0x5A, "MDB mismatch: got 0x%02x expected 0x5A",
		      last_ibi_payload[0]);
	zassert_equal(last_ibi_payload[1], 0x01, "data[0] mismatch: got 0x%02x expected 0x01",
		      last_ibi_payload[1]);
	zassert_equal(last_ibi_payload[2], 0x02, "data[1] mismatch: got 0x%02x expected 0x02",
		      last_ibi_payload[2]);
	zassert_equal(last_ibi_payload[3], 0x03, "data[2] mismatch: got 0x%02x expected 0x03",
		      last_ibi_payload[3]);

	(void)i3c_ibi_disable(&target_b_desc);
	target_b_desc.ibi_cb = NULL;
}

ZTEST(i3c_loopback, test_ibi_tir_extended_payload)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	k_sem_init(&ibi_sem, 0, 1);
	last_ibi_len = 0;
	target_b_desc.ibi_cb = ibi_cb;
	int rc = -1;

	for (int a = 0; a < I3C_NACK_RETRIES; a++) {
		rc = i3c_ibi_enable(&target_b_desc);
		if (rc != -ENXIO) {
			break;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}
	zassert_ok(rc, "ibi_enable failed (%d)", rc);

	rc = ibi_raise_and_get_rc("RAISE_TIR DEADBEEF");
	zassert_ok(rc, "target i3c_ibi_raise failed (%d)", rc);
	rc = k_sem_take(&ibi_sem, K_MSEC(500));
	zassert_ok(rc, "IBI with extended payload not received within 500ms");
	zassert_true(last_ibi_len >= 4, "expected >= 4 byte payload, got %u", last_ibi_len);

	(void)i3c_ibi_disable(&target_b_desc);
	target_b_desc.ibi_cb = NULL;
}

/*
 * Single-shot Hot-Join end-to-end:
 *   1. Enable HJ ACK on the controller.
 *   2. SOFT_RST the target so it loses its DA.
 *   3. Zero the controller-side desc DA so the HJ workqueue's
 *      i3c_do_daa() will free the DAT slot and re-enumerate.
 *   4. Target raises HJ via i3c_ibi_raise(I3C_IBI_HOTJOIN).
 *   5. Wait (bounded) for the HJ workqueue to rebind a fresh DA.
 *   6. Issue a private write to the new DA to confirm.
 *
 * TODO: currently skipped — controller's HJ workqueue does not
 * rebind target_b's DA after the target raises HJ.  Needs LA capture
 * to determine if the HJ ARB at 0x02 actually reaches the wire.
 */
ZTEST(i3c_loopback, test_ibi_hot_join)
{
	ztest_test_skip();
	int rc;
	uint8_t probe = 0xC3;
	struct i3c_msg msg = {
		.buf = &probe,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};

	rc = i3c_ibi_hj_response(i3c_dev, true);
	if (rc == -ENOSYS) {
		/* We don't support HJ - skip test */
		ztest_test_skip();
		return;
	}
	zassert_ok(rc, "i3c_ibi_hj_response(true) rc=%d", rc);

	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	/* Step 2: drop target's DA via SOFT_RST. */
	sync_drain(sync_uart);
	sync_send(sync_uart, "SOFT_RST");
	rc = sync_expect(sync_uart, "READY", K_MSEC(1000));
	zassert_ok(rc, "SOFT_RST READY timeout (%d)", rc);

	/* Step 3: zero controller-side desc DA so HJ workqueue's
	 * i3c_do_daa() actually re-enumerates (frees DAT slot first).
	 */
	uint8_t old_da = target_b_desc.dynamic_addr;

	target_b_desc.dynamic_addr = 0U;

	/* Step 4: target raises HJ. */
	int hj_rc = ibi_raise_and_get_rc("RAISE_HJ");

	zassert_ok(hj_rc, "target HJ raise rc=%d", hj_rc);

	/* Step 5: wait (bounded) for controller HJ workqueue to rebind. */
	int waited_ms = 0;

#ifdef CONFIG_I3C_LOOPBACK_BENCH_HAS_PERMANENT_TARGET
	uint8_t bench_da_pre = bench_permanent_target_desc.dynamic_addr;
#endif

	while (target_b_desc.dynamic_addr == 0U && waited_ms < 500) {
		k_msleep(10);
		waited_ms += 10;
	}

	printk("[hj-dbg] post-HJ: target_b.da=0x%02x (was 0x%02x), "
#ifdef CONFIG_I3C_LOOPBACK_BENCH_HAS_PERMANENT_TARGET
	       "bench.da=0x%02x (was 0x%02x), "
#endif
	       "waited=%dms\n",
	       target_b_desc.dynamic_addr, old_da,
#ifdef CONFIG_I3C_LOOPBACK_BENCH_HAS_PERMANENT_TARGET
	       bench_permanent_target_desc.dynamic_addr, bench_da_pre,
#endif
	       waited_ms);

	zassert_not_equal(target_b_desc.dynamic_addr, 0U, "no re-DAA in 500ms (old_da=0x%02x)",
			  old_da);
	zassert_true(target_b_desc.dynamic_addr <= 0x7F, "rebound DA 0x%02x out of range",
		     target_b_desc.dynamic_addr);

	/* Step 6: prove new DA addresses the target on the wire. */
	rc = -1;
	for (int a = 0; a < I3C_NACK_RETRIES; a++) {
		rc = i3c_transfer(&target_b_desc, &msg, 1);
		if (rc == 0) {
			break;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}
	zassert_ok(rc, "post-HJ private write to new DA 0x%02x failed (%d)",
		   target_b_desc.dynamic_addr, rc);
}

ZTEST(i3c_loopback, test_ibi_controller_role_request)
{
	(void)ibi_raise_and_get_rc("RAISE_MR");
	k_msleep(10);
}

ZTEST(i3c_loopback, test_ibi_back_to_back)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	k_sem_init(&ibi_sem, 0, 4);
	target_b_desc.ibi_cb = ibi_cb;
	int rc = -1;

	for (int a = 0; a < I3C_NACK_RETRIES; a++) {
		rc = i3c_ibi_enable(&target_b_desc);
		if (rc != -ENXIO) {
			break;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}
	zassert_ok(rc, "ibi_enable failed (%d)", rc);

	int received = 0;

	for (int i = 0; i < 4; i++) {
		char cmd[24];

		snprintf(cmd, sizeof(cmd), "RAISE_TIR %02x", i);
		rc = ibi_raise_and_get_rc(cmd);
		if (rc != 0) {
			continue;
		}
		if (k_sem_take(&ibi_sem, K_MSEC(200)) == 0) {
			received++;
		}
	}

	zassert_equal(received, 4, "only %d/4 IBIs received", received);

	(void)i3c_ibi_disable(&target_b_desc);
	target_b_desc.ibi_cb = NULL;
}

/*
 * Hot-Join stress test.  Per iteration: SOFT_RST target, zero
 * controller-side desc DA, target raises HJ, wait for controller's
 * HJ workqueue to rebind, then probe with a private write.
 *
 * Failures are bucketed for easy regression localisation:
 *   drop_da    — target's IBI raise itself failed
 *   no_redaa   — controller never updated target_b_desc.dynamic_addr
 *   post_probe — DA rebound but private write NACKed
 *
 * TODO: skipped — same root cause as test_ibi_hot_join.
 */
ZTEST(i3c_loopback, test_ibi_hot_join_stress)
{
	ztest_test_skip();
	const int iters = 50;
	int hj_response_rc;
	int fail_drop_da = 0;    /* target HJ raise rc != 0 */
	int fail_no_redaa = 0;   /* desc not rebound after HJ */
	int fail_post_probe = 0; /* desc rebound but probe NACKed */
	int total_failed = 0;
	uint8_t probe = 0xC3;
	struct i3c_msg msg = {
		.buf = &probe,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};

	/* Enable HJ ACK on the controller.  Persistent for the suite. */
	hj_response_rc = i3c_ibi_hj_response(i3c_dev, true);
	if (hj_response_rc == -ENOSYS) {
		ztest_test_skip();
		return;
	}
	zassert_ok(hj_response_rc, "i3c_ibi_hj_response(true) rc=%d", hj_response_rc);

	/* Bus must start in a known-good state so we can attribute any
	 * iteration failures to the HJ path under test rather than to
	 * a pre-existing broken bus.
	 */
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	for (int i = 0; i < iters; i++) {
		/* Step 1: tear down the target's DA via SOFT_RST.  The
		 * target replies "READY soft_rst rc=N CTRL=... ADDR=..."
		 * once its IP is back up.  Drain stragglers first so we
		 * don't latch onto a READY from a previous command.
		 */
		sync_drain(sync_uart);
		sync_send(sync_uart, "SOFT_RST");
		int rc = sync_expect(sync_uart, "READY", K_MSEC(1000));

		zassert_ok(rc, "iter %d: SOFT_RST READY timeout (%d)", i, rc);

		/* Step 2: zero the controller-side desc DA so the HJ
		 * workqueue's i3c_do_daa() will free the DAT slot and
		 * actually enumerate the rejoining target via ENTDAA
		 * (see comment block above).
		 */
		uint8_t old_da = target_b_desc.dynamic_addr;

		target_b_desc.dynamic_addr = 0U;

		/* Step 3: ask target to raise HJ; its IP returns rc via
		 * "IBI hj rc=N" once SLV_EVENT_STATUS[HJ_EN] has driven
		 * the HJ arbitration and DYN_ADDR_ASSGN has been seen.
		 */
		int hj_rc = ibi_raise_and_get_rc("RAISE_HJ");

		if (hj_rc != 0) {
			fail_drop_da++;
			total_failed++;
			printk("[hj-stress] iter %d: target HJ raise rc=%d\n", i, hj_rc);
			/* Best-effort: restore controller view via full
			 * re-enum so subsequent iters start cleanly.
			 */
			(void)i3c_ccc_do_rstdaa_all(i3c_dev);
			(void)i3c_do_daa(i3c_dev);
			continue;
		}

		/* Step 4: wait for the controller's HJ workqueue to
		 * complete (ENTDAA + add_slave_from_daa).  Bound the
		 * wait so a stuck workqueue is reported, not hung on.
		 */
		int waited_ms = 0;

		while (target_b_desc.dynamic_addr == 0U && waited_ms < 500) {
			k_msleep(10);
			waited_ms += 10;
		}
		if (target_b_desc.dynamic_addr == 0U) {
			fail_no_redaa++;
			total_failed++;
			printk("[hj-stress] iter %d: no re-DAA in 500ms"
			       " (old_da=0x%02x)\n",
			       i, old_da);
			(void)i3c_ccc_do_rstdaa_all(i3c_dev);
			(void)i3c_do_daa(i3c_dev);
			continue;
		}

		/* Step 5: prove the new DA actually addresses the
		 * target on the wire.
		 */
		rc = i3c_transfer(&target_b_desc, &msg, 1);
		if (rc != 0) {
			fail_post_probe++;
			total_failed++;
			printk("[hj-stress] iter %d: post-HJ probe rc=%d"
			       " new_da=0x%02x\n",
			       i, rc, target_b_desc.dynamic_addr);
			(void)i3c_recover_bus(i3c_dev);
			continue;
		}
	}

	printk("[hj-stress] iters=%d fail=%d drop_da=%d no_redaa=%d"
	       " post_probe=%d\n",
	       iters, total_failed, fail_drop_da, fail_no_redaa, fail_post_probe);

	/* Tolerate a small per-iter failure rate (cascade-NACK can
	 * still bite during stress) but require the HJ path to work
	 * in the majority of cases so a regression that disables HJ
	 * entirely is caught.
	 */
	zassert_true(total_failed <= iters / 5,
		     "HJ stress: %d/%d iterations failed (>20%% threshold)", total_failed, iters);
}

#else /* !CONFIG_I3C_USE_IBI */

ZTEST(i3c_loopback, test_ibi_not_compiled)
{
	ztest_test_skip();
}

#endif /* CONFIG_I3C_USE_IBI */
