/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Registration lifecycle.
 *
 * What these cases assert, and how portable it is
 * ------------------------------------------------------------------
 * i3c_target_register() and i3c_target_unregister() bind a callback
 * table to a driver instance in host software.  That has no
 * representation on the wire, so MIPI I3C Basic v1.0 says nothing about
 * it and cannot: the specification governs the address phase, the
 * T-Bit, and CCCs, not a software callback registry.  The Zephyr
 * doxygen is also silent on a duplicate registration, documenting only
 * 0, -EINVAL, -EIO and -ENOSYS for both calls.
 *
 * So the oracle here is the driver under test, not the API.  i3c_dw.c
 * documents "@retval -EBUSY If another config is already registered"
 * and implements it, and test_register_duplicate_deterministic asserts
 * exactly that: -EBUSY, with the first registration still serving
 * traffic.  Every other in-tree I3C target driver returns 0
 * unconditionally from both calls, so that assertion is specific to
 * this driver rather than a conformance check any driver should pass.
 *
 * test_unregister_stops_responding asserts only that no bytes are delivered,
 * not that the target NACKs: the DW IP cannot be taken off the bus recoverably.
 * See the case comment for the measurements behind that.
 */

#include "test_common.h"

#include <zephyr/drivers/i3c/target_device.h>
#include <zephyr/sys/atomic.h>

/* Bytes delivered to the second, distinguishable callback table. */
static atomic_t dup_write_bytes;

static int dup_write_received_cb(struct i3c_target_config *config, uint8_t val)
{
	ARG_UNUSED(config);
	ARG_UNUSED(val);

	atomic_inc(&dup_write_bytes);
	return 0;
}

/**
 * @brief Round-trip a verified payload in both directions.
 *
 * Proves service by moving data, not by a call returning 0.  @p seed
 * distinguishes one caller's traffic from another's in a failure message.
 */
static void assert_serves_traffic(uint8_t seed)
{
	const size_t len = 8U;
	uint8_t pattern[TARGET_BUF_MAX];
	uint8_t staged[TARGET_BUF_MAX];
	uint8_t got[TARGET_BUF_MAX];
	size_t n;
	int rc;
	int a_rc = -1;

	event_reset();

	fill_pattern(pattern, len, seed);
	rc = ctrl_write(pattern, len, &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "seed 0x%02x: write to a registered target failed (%d)", seed, a_rc);

	rc = event_wait_count(EVENT_WRITE_BYTE, len, CB_SETTLE);
	zassert_ok(rc, "seed 0x%02x: only %zu of %zu bytes reached the application", seed,
		   event_count(EVENT_WRITE_BYTE), len);

	n = event_payload(got, sizeof(got));
	zassert_equal(n, len, "seed 0x%02x: received %zu bytes, expected %zu", seed, n, len);
	for (size_t i = 0; i < len; i++) {
		zassert_equal(got[i], pattern[i], "seed 0x%02x byte %zu: got 0x%02x want 0x%02x",
			      seed, i, got[i], pattern[i]);
	}

	fill_pattern(staged, len, (uint8_t)(seed + 0x40U));
	rc = target_stage_read(staged, len);
	zassert_ok(rc, "seed 0x%02x: staging failed (%d)", seed, rc);

	memset(got, 0, sizeof(got));
	rc = ctrl_read(len, got, sizeof(got), &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "seed 0x%02x: read from a registered target failed (%d)", seed, a_rc);

	for (size_t i = 0; i < len; i++) {
		zassert_equal(got[i], staged[i],
			      "seed 0x%02x read byte %zu: got 0x%02x want 0x%02x",
			      seed, i, got[i], staged[i]);
	}
}

/*
 * Put the suite's own registration back, so one case cannot strand the
 * rest of the suite.
 *
 * Deliberately generates no bus traffic: a probe transfer here would
 * race the target ISR and leak a byte into whichever case runs next.
 * The unregister first is what makes this valid, since a duplicate
 * registration is refused with -EBUSY.
 */
static void restore_registration(void)
{
	(void)i3c_target_unregister(i3c_dev, &target_cfg);
	(void)i3c_target_register(i3c_dev, &target_cfg);
}

/*
 * A registered target serves transactions addressed to it.
 *
 * Per target_device.h: "Enable I3C target mode for the dev I3C bus
 * driver using the provided config struct (cfg) containing the functions
 * and parameters to send bus events."
 *
 * Baseline for the group, verified cross-board in both directions.
 */
ZTEST(dual_board_loopback_target, test_register_serves_traffic)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	assert_serves_traffic(0x31);
}

/*
 * A second registration has one defined outcome.
 *
 * Neither the spec nor target_device.h defines this; the value asserted
 * here is the one i3c_dw.c documents and returns.  See the file header.
 *
 * The duplicate table is installed and then driven with real traffic, so
 * the case reports which table actually served the bytes rather than
 * inferring it from the return code.  The negative control is the
 * duplicate's own counter: it must stay at zero.
 */
ZTEST(dual_board_loopback_target, test_register_duplicate_deterministic)
{
	static const struct i3c_target_callbacks dup_callbacks = {
		.write_received_cb = dup_write_received_cb,
	};
	static struct i3c_target_config dup_cfg = {
		.callbacks = &dup_callbacks,
	};
	const size_t len = 4U;
	uint8_t pattern[TARGET_BUF_MAX];
	size_t first_table_bytes;
	uint32_t second_table_bytes;
	int reg_rc;
	int rc;
	int a_rc = -1;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	atomic_clear(&dup_write_bytes);
	fill_pattern(pattern, len, 0x4c);

	reg_rc = i3c_target_register(i3c_dev, &dup_cfg);

	rc = ctrl_write(pattern, len, &a_rc);
	(void)event_wait_count(EVENT_WRITE_BYTE, len, CB_SETTLE);

	first_table_bytes = event_count(EVENT_WRITE_BYTE);
	second_table_bytes = (uint32_t)atomic_get(&dup_write_bytes);

	/* Restore before asserting, so a failure cannot leave the suite
	 * bound to the duplicate table.
	 */
	if (reg_rc == 0) {
		(void)i3c_target_unregister(i3c_dev, &dup_cfg);
		restore_registration();
	}

	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_equal(reg_rc, -EBUSY, "duplicate i3c_target_register() returned %d, want -EBUSY",
		      reg_rc);
	zassert_ok(a_rc, "write during duplicate registration failed (%d)", a_rc);
	zassert_equal(second_table_bytes, 0U,
		      "the duplicate table received %u bytes; the first registration must keep "
		      "the binding",
		      second_table_bytes);
	zassert_equal(first_table_bytes, len, "the first table received %zu of %zu bytes",
		      first_table_bytes, len);
}

/*
 * After unregister no data reaches the application.
 *
 * Per target_device.h: "This routine disables I3C target mode for the
 * dev I3C bus driver using the provided config struct (cfg)."  Taken
 * literally that means Board A's write should fail, and i3c_stm32.c does
 * exactly that by calling LL_I3C_Disable().  The DW IP measured here
 * cannot: clearing the DEVICE_ADDR valid bits leaves it ACKing, and the
 * DEV_CTRL_ENABLE disable that does stop the ACK loses the assigned
 * dynamic address, after which a re-register cannot restore service.
 *
 * So this asserts only the part that is achievable and that actually
 * matters for safety: no bytes are delivered to a config that is no
 * longer registered.
 *
 * The paired positive control immediately before proves the write path
 * was working, so a pass cannot come from a bus that was already broken.
 */
ZTEST(dual_board_loopback_target, test_unregister_stops_responding)
{
	const size_t len = 4U;
	uint8_t pattern[TARGET_BUF_MAX];
	size_t bytes_after;
	int unreg_rc;
	int rc;
	int a_rc_before = -1;
	int a_rc_after = -1;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	fill_pattern(pattern, len, 0x5d);

	/* Positive control: service is working before we tear it down. */
	rc = ctrl_write(pattern, len, &a_rc_before);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc_before, "baseline write failed (%d)", a_rc_before);
	rc = event_wait_count(EVENT_WRITE_BYTE, len, CB_SETTLE);
	zassert_ok(rc, "baseline: only %zu of %zu bytes arrived", event_count(EVENT_WRITE_BYTE),
		   len);

	event_reset();
	unreg_rc = i3c_target_unregister(i3c_dev, &target_cfg);

	rc = ctrl_write(pattern, len, &a_rc_after);
	(void)event_wait_count(EVENT_WRITE_BYTE, len, CB_SETTLE);
	bytes_after = event_count(EVENT_WRITE_BYTE);

	restore_registration();

	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(unreg_rc, "i3c_target_unregister() failed (%d)", unreg_rc);
	zassert_equal(bytes_after, 0U, "%zu bytes still reached the application after unregister",
		      bytes_after);
}

/*
 * Re-registering restores service.
 *
 * The target block is torn down and reconfigured the way an application
 * would after re-initialising it, then service is proven again by a full
 * round trip rather than by the register call returning 0.
 *
 * The check while unregistered is the half that gives "restores" meaning:
 * without it the case would pass on a driver whose unregister did nothing,
 * because service would never have stopped.
 *
 * Unregister rather than SOFT_RST keeps this about the registration
 * lifecycle; recovery from a hardware reset is not covered by this suite.
 */
ZTEST(dual_board_loopback_target, test_reregister_restores_service)
{
	const size_t len = 8U;
	uint8_t pattern[TARGET_BUF_MAX];
	size_t bytes_while_down;
	int restore_rc;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	/* Prove service, tear it down, restore it, prove it again with a
	 * different pattern so a stale buffer cannot fake the result.
	 */
	assert_serves_traffic(0x6e);

	rc = i3c_target_unregister(i3c_dev, &target_cfg);
	zassert_ok(rc, "i3c_target_unregister() failed (%d)", rc);

	event_reset();
	fill_pattern(pattern, len, 0x1d);
	rc = ctrl_write(pattern, len, &a_rc);
	(void)event_wait_count(EVENT_WRITE_BYTE, len, CB_SETTLE);
	bytes_while_down = event_count(EVENT_WRITE_BYTE);

	/* Restore before asserting, so a failure above cannot leave the suite
	 * unregistered.  Board A's own return code is not the subject: the IP
	 * keeps ACKing its address while unbound, so the write may well
	 * succeed on the bus.
	 */
	restore_rc = target_configure();

	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_equal(bytes_while_down, 0U,
		      "%zu bytes reached the application while unregistered, so service never "
		      "stopped and the restore below proves nothing",
		      bytes_while_down);
	zassert_ok(restore_rc, "re-configure and re-register failed (%d)", restore_rc);

	assert_serves_traffic(0x27);
}
