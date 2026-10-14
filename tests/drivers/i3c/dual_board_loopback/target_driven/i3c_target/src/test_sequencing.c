/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Sequencing and lifecycle, asserted from the target.
 *
 * A single controller serialises every frame, so nothing here overlaps on
 * the bus.  What these cases cover is the order operations are served in
 * and which binding is in force when each one arrives.  The genuine
 * concurrency case, unregistering mid-transfer, is not writable on this IP.
 *
 * Every read staged here uses a length that is a multiple of 4, so these
 * cases exercise the binding rather than the staged-read length edge that
 * test_read_non_dword_length covers.
 */

#include "test_common.h"

#define PROBE_LEN 8U
#define MWL_TEST  16U

/* Counters for the alternate table, so a callback firing can be attributed to
 * one binding rather than to "a callback fired".
 */
static size_t alt_writes;
static size_t alt_stops;

static struct i3c_target_config alt_cfg;

static int alt_write_received(struct i3c_target_config *cfg, uint8_t val)
{
	ARG_UNUSED(cfg);
	ARG_UNUSED(val);
	alt_writes++;
	return 0;
}

static int alt_stop(struct i3c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	alt_stops++;
	return 0;
}

static const struct i3c_target_callbacks alt_callbacks = {
	.write_received_cb = alt_write_received,
	.stop_cb = alt_stop,
};

/**
 * @brief Swap the live binding for the alternate table.
 *
 * i3c_target_register() refuses a second registration with -EBUSY
 * (test_register_duplicate_deterministic), so the swap has to go through
 * unregister first.
 */
static void bind_alt(void)
{
	int rc;

	alt_writes = 0;
	alt_stops = 0;

	alt_cfg = target_cfg;
	alt_cfg.callbacks = &alt_callbacks;

	rc = target_bind(&alt_cfg);
	zassert_ok(rc, "registering the alternate table failed (%d)", rc);
}

/**
 * @brief Put the default table back, whatever the case did.
 */
static void bind_default(void)
{
	int rc = target_bind(&target_cfg);

	zassert_ok(rc, "restoring the default table failed (%d)", rc);
}

/*
 * Per target_device.h, i3c_target_register() binds a config and
 * i3c_target_unregister() "disables I3C target mode" for it.  Which table a
 * transfer is delivered to therefore follows the binding in force when it
 * arrives, not the one in force when the application last looked.
 *
 * Re-registering between transactions swaps tables cleanly.
 *
 * The negative control is the half that matters: asserting only that the new
 * table fired would also pass on a driver that called every table it had ever
 * seen.  The old table must be silent.
 */
ZTEST(dual_board_loopback_target, test_reregister_midstream)
{
	uint8_t pattern[TARGET_BUF_MAX];
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	/* Positive control: the default binding is serving before the swap. */
	rc = ctrl_round_trip(0x51U, PROBE_LEN);
	zassert_ok(rc, "baseline round trip failed (%d)", rc);

	bind_alt();

	event_reset();
	fill_pattern(pattern, PROBE_LEN, 0x62U);

	rc = ctrl_write(pattern, PROBE_LEN, &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "the write after re-registering failed (%d)", a_rc);

	k_msleep(50);

	zassert_equal(alt_writes, PROBE_LEN,
		      "the re-registered table received %zu of %u bytes",
		      alt_writes, PROBE_LEN);
	zassert_equal(alt_stops, 1U, "the re-registered table saw %zu completions, want 1",
		      alt_stops);
	zassert_equal(event_count(EVENT_WRITE_BYTE), 0U,
		      "the unregistered table still received %zu bytes; the swap left the old "
		      "binding live",
		      event_count(EVENT_WRITE_BYTE));

	bind_default();

	/* The swap must be reversible, or the suite is degraded from here on. */
	rc = ctrl_round_trip(0x73U, PROBE_LEN);
	zassert_ok(rc, "the default table did not serve traffic after being restored (%d)", rc);
}

/*
 * Per target_device.h stop_cb, after a transaction completes "the controller
 * shall enter a state where it is ready to react to new start conditions".  A
 * CCC is a new start condition carrying a command code rather than a private
 * payload, so one arriving straight after a private transfer must be decoded
 * as a CCC.
 *
 * A CCC immediately following a private transfer is not mis-parsed.
 *
 * The failure this catches is the block still treating the bus as mid-private
 * transfer and swallowing the command code as data.  Asserting only that the
 * CCC returned rc == 0 would not catch it -- the value has to land.
 */
ZTEST(dual_board_loopback_target, test_ccc_after_private_transfer)
{
	struct i3c_config_target cfg = {0};
	uint8_t pattern[TARGET_BUF_MAX];
	char cmd[32];
	unsigned int val = 0;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	/* The CCC must change something, or a driver that dropped it entirely
	 * would pass.
	 */
	rc = i3c_config_get(i3c_dev, I3C_CONFIG_TARGET, &cfg);
	zassert_ok(rc, "reading the target config failed (%d)", rc);
	zassert_not_equal(cfg.max_write_len, MWL_TEST,
			  "the target already holds MWL == %u, so this case cannot show the "
			  "CCC landed",
			  MWL_TEST);

	event_reset();
	fill_pattern(pattern, PROBE_LEN, 0x84U);

	rc = ctrl_write(pattern, PROBE_LEN, &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "the private write failed (%d)", a_rc);

	/* No settle delay: the point of the case is the CCC arriving while the
	 * target has only just finished the private transfer.
	 */
	snprintk(cmd, sizeof(cmd), "SETMWL %u", MWL_TEST);
	rc = ctrl_ccc(cmd, &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A for SETMWL (%d)", rc);
	zassert_ok(a_rc, "SETMWL immediately after a private write failed (%d)", a_rc);

	/* The private half must have survived too: a CCC that trampled the
	 * transfer would show up as missing bytes.
	 */
	rc = event_wait_count(EVENT_WRITE_BYTE, PROBE_LEN, CB_SETTLE);
	zassert_ok(rc, "only %zu of %u bytes of the private write arrived alongside the CCC",
		   event_count(EVENT_WRITE_BYTE), PROBE_LEN);

	rc = i3c_config_get(i3c_dev, I3C_CONFIG_TARGET, &cfg);
	zassert_ok(rc, "reading the target config back failed (%d)", rc);
	zassert_equal(cfg.max_write_len, MWL_TEST,
		      "MWL is %u, want %u; the CCC that followed a private transfer was not "
		      "applied",
		      cfg.max_write_len, MWL_TEST);

	/* Confirm from the controller's side as well, then restore. */
	rc = ctrl_ccc("GETMWL", &a_rc, &val);
	zassert_ok(rc, "harness: no reply from Board A for GETMWL (%d)", rc);
	zassert_ok(a_rc, "GETMWL failed (%d)", a_rc);
	zassert_equal(val, MWL_TEST, "GETMWL reports %u, want %u", val, MWL_TEST);

	snprintk(cmd, sizeof(cmd), "SETMWL %u", (unsigned int)TARGET_BUF_MAX);
	rc = ctrl_ccc(cmd, &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A restoring MWL (%d)", rc);
	zassert_ok(a_rc, "restoring MWL failed (%d)", a_rc);
}

/*
 * Staged reads accumulate in the TX FIFO and are served in order.
 *
 * target_device.h describes i3c_target_tx_write() as a write to "the target's
 * TX FIFO" returning "the total number of bytes written", with the application
 * driving any remainder through the callbacks.  So staging twice without an
 * intervening read queues both, and the reads take them in order.
 */
ZTEST(dual_board_loopback_target, test_staged_reads_queue_in_order)
{
	uint8_t first[PROBE_LEN];
	uint8_t second[PROBE_LEN];
	uint8_t got[TARGET_BUF_MAX];
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = ctrl_round_trip(0x95U, PROBE_LEN);
	zassert_ok(rc, "baseline round trip failed (%d)", rc);

	fill_pattern(first, sizeof(first), 0xA6U);
	fill_pattern(second, sizeof(second), 0xB7U);

	rc = target_stage_read(first, sizeof(first));
	zassert_ok(rc, "staging the first buffer failed (%d)", rc);
	rc = target_stage_read(second, sizeof(second));
	zassert_ok(rc, "staging the second buffer failed (%d)", rc);

	memset(got, 0, sizeof(got));
	rc = ctrl_read(sizeof(first), got, sizeof(got), &a_rc);
	zassert_ok(rc, "harness: no reply from Board A for the first read (%d)", rc);
	zassert_ok(a_rc, "the first read failed (%d)", a_rc);
	zassert_mem_equal(got, first, sizeof(first),
			  "the first read was not served the first staged buffer");

	memset(got, 0, sizeof(got));
	rc = ctrl_read(sizeof(second), got, sizeof(got), &a_rc);
	zassert_ok(rc, "harness: no reply from Board A for the second read (%d)", rc);
	zassert_ok(a_rc, "the second read failed (%d)", a_rc);
	zassert_mem_equal(got, second, sizeof(second),
			  "the second read was not served the second staged buffer");
}
