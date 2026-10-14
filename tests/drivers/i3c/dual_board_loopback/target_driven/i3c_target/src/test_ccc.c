/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The target side of CCCs that the controller suite only observes
 * from Board A.
 *
 * Board B has no public way to read back its own dynamic address or its
 * SLV_EVENT_STATUS, so the address cases assert the observable effect
 * instead: the address the target answers to, proven by a round trip,
 * with the previous address as the negative control.
 */

#include "test_common.h"
#include "test_identity.h"

#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/ztest.h>

/* Spare address the bench guarantees is free; see the Kconfig help. */
#define ALT_DA CONFIG_I3C_DUAL_BOARD_LOOPBACK_TARGET_ALT_ADDR

static int read_target_config(struct i3c_config_target *out)
{
	return i3c_config_get(i3c_dev, I3C_CONFIG_TARGET, out);
}

/* Drive a write from A and report how many bytes reached the application. */
static size_t write_bytes(uint8_t seed, size_t len, int *a_rc_out)
{
	uint8_t pattern[TARGET_BUF_MAX];
	int rc;

	event_reset();
	fill_pattern(pattern, len, seed);

	rc = ctrl_write(pattern, len, a_rc_out);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);

	(void)event_wait_count(EVENT_WRITE_BYTE, len, CB_SETTLE);
	return event_count(EVENT_WRITE_BYTE);
}

/*
 * SETMWL changes the target's effective max write length.
 *
 * Source: MIPI I3C Basic v1.0 5.1.9.3.5.
 *
 * The value has to differ from the one already in force, or a SETMWL that is
 * dropped on the floor still reads back what the test asked for.  The
 * baseline check makes that a failure rather than a silent pass.
 */
#define MWL_SET 16U

ZTEST(dual_board_loopback_target, test_setmwl_applied)
{
	struct i3c_config_target cfg = {0};
	unsigned int got = 0;
	char cmd[16];
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = read_target_config(&cfg);
	zassert_ok(rc, "i3c_config_get(TARGET) failed (%d)", rc);
	zassert_not_equal(cfg.max_write_len, MWL_SET,
			  "MWL is already %u, so SETMWL %u would prove nothing",
			  cfg.max_write_len, MWL_SET);

	snprintk(cmd, sizeof(cmd), "SETMWL %u", MWL_SET);
	rc = ctrl_ccc(cmd, &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "SETMWL failed (%d)", a_rc);

	rc = read_target_config(&cfg);
	zassert_ok(rc, "i3c_config_get(TARGET) failed (%d)", rc);
	zassert_equal(cfg.max_write_len, MWL_SET, "target MWL is %u, want %u",
		      cfg.max_write_len, MWL_SET);

	rc = ctrl_ccc("GETMWL", &a_rc, &got);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "GETMWL failed (%d)", a_rc);
	zassert_equal(got, MWL_SET, "GETMWL returned %u, want %u", got, MWL_SET);

	/* Put it back: later cases write TARGET_BUF_MAX bytes. */
	snprintk(cmd, sizeof(cmd), "SETMWL %u", (unsigned int)TARGET_BUF_MAX);
	(void)ctrl_ccc(cmd, &a_rc, NULL);
}

/*
 * SETMRL likewise for the read length.
 *
 * Source: MIPI I3C Basic v1.0 5.1.9.3.6.
 */
ZTEST(dual_board_loopback_target, test_setmrl_applied)
{
	struct i3c_config_target cfg = {0};
	unsigned int got = 0;
	char cmd[16];
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = ctrl_ccc("SETMRL 24", &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "SETMRL failed (%d)", a_rc);

	rc = read_target_config(&cfg);
	zassert_ok(rc, "i3c_config_get(TARGET) failed (%d)", rc);
	zassert_equal(cfg.max_read_len, 24U, "target MRL is %u, want 24", cfg.max_read_len);

	rc = ctrl_ccc("GETMRL", &a_rc, &got);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "GETMRL failed (%d)", a_rc);
	zassert_equal(got, 24U, "GETMRL returned %u, want 24", got);

	/* Put it back: later cases read TARGET_BUF_MAX bytes. */
	snprintk(cmd, sizeof(cmd), "SETMRL %u", (unsigned int)TARGET_BUF_MAX);
	(void)ctrl_ccc(cmd, &a_rc, NULL);
}

/*
 * The target answers to the new address and not the old one.
 *
 * Source: MIPI I3C Basic v1.0 5.1.9.3.11.  The old address is the
 * negative control: without it this would pass on a driver that ignored
 * the CCC, since Board A would simply keep using the address that still
 * worked.
 */
ZTEST(dual_board_loopback_target, test_setnewda_applied)
{
	uint8_t old_da = target_da;
	size_t bytes;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	zassert_not_equal(old_da, ALT_DA, "bench DA collides with the test address");

	rc = ctrl_ccc("SETNEWDA " STRINGIFY(ALT_DA), &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "SETNEWDA failed (%d)", a_rc);

	/* A's descriptor now holds the new DA, so an ordinary write proves
	 * the target moved with it.
	 */
	bytes = write_bytes(0x81, 8U, &a_rc);
	zassert_ok(a_rc, "write to the new DA failed (%d)", a_rc);
	zassert_equal(bytes, 8U, "only %zu of 8 bytes arrived at the new DA", bytes);

	/* Negative control: the old address must be dead. */
	event_reset();
	{
		const uint8_t probe[4] = {0xde, 0xad, 0xbe, 0xef};

		rc = ctrl_write_addr(old_da, probe, sizeof(probe), &a_rc);
		zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	}
	(void)event_wait_count(EVENT_WRITE_BYTE, 1U, CB_SETTLE);
	zassert_equal(event_count(EVENT_WRITE_BYTE), 0U,
		      "%zu bytes still reached us at the old DA 0x%02x",
		      event_count(EVENT_WRITE_BYTE), old_da);

	(void)ctrl_re_enum(&a_rc);
}

/*
 * After RSTDAA the target holds no valid dynamic address.
 *
 * Source: MIPI I3C Basic v1.0 5.1.9.3.3.
 *
 * Asserted on the target's own DEVICE_ADDR register because nothing in the
 * public API exposes DA validity: i3c_config_get(I3C_CONFIG_TARGET) reports
 * static_addr only.  An earlier version instead required a write to the old
 * address to fail, which is not decidable on this bench -- TARGET_STATIC_ADDR
 * equals the assigned DA, and the static address stays valid across RSTDAA, so
 * traffic to that address may legitimately still be answered.
 */
#define TGT_DEVICE_ADDR              0x4
#define TGT_DEVICE_ADDR_DA_VALID     BIT(31)
#define TGT_DEVICE_ADDR_DA_MASK      GENMASK(22, 16)

static uint32_t read_device_addr(void)
{
	return sys_read32(DT_REG_ADDR(DT_ALIAS(test_i3c)) + TGT_DEVICE_ADDR);
}

/* The dynamic address the target currently answers to. */
static uint8_t read_device_da(void)
{
	return (uint8_t)FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, read_device_addr());
}

ZTEST(dual_board_loopback_target, test_rstdaa_clears_da)
{
	uint32_t da_after_rstdaa;
	uint32_t da_after_restore;
	size_t bytes;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	/* Positive control: we are addressed and serving traffic. */
	bytes = write_bytes(0x92, 8U, &a_rc);
	zassert_ok(a_rc, "baseline write failed (%d)", a_rc);
	zassert_equal(bytes, 8U, "baseline: only %zu of 8 bytes arrived", bytes);
	zassert_not_equal(read_device_addr() & TGT_DEVICE_ADDR_DA_VALID, 0U,
			  "baseline: no valid dynamic address before RSTDAA");

	rc = ctrl_ccc("RSTDAA", &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "RSTDAA failed (%d)", a_rc);

	da_after_rstdaa = read_device_addr();

	(void)ctrl_re_enum(&a_rc);
	da_after_restore = read_device_addr();

	zassert_equal(da_after_rstdaa & TGT_DEVICE_ADDR_DA_VALID, 0U,
		      "DEVICE_ADDR=%08x still marks the dynamic address valid after RSTDAA",
		      da_after_rstdaa);

	/* Negative control: re-enumeration must give it back, so the check
	 * above cannot pass on a target that simply never has a valid DA.
	 */
	zassert_not_equal(da_after_restore & TGT_DEVICE_ADDR_DA_VALID, 0U,
			  "DEVICE_ADDR=%08x has no valid dynamic address after re-enumeration",
			  da_after_restore);
}

/*
 * After ENTDAA the target holds the DA the controller assigned.
 *
 * Source: MIPI I3C Basic v1.0 5.1.4.1.
 */
ZTEST(dual_board_loopback_target, test_entdaa_assigns_da)
{
	unsigned int assigned = 0;
	size_t bytes;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = ctrl_ccc("RSTDAA", &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "RSTDAA failed (%d)", a_rc);

	rc = ctrl_ccc("ENTDAA", &a_rc, &assigned);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "ENTDAA failed (%d)", a_rc);
	zassert_not_equal(assigned, 0U, "ENTDAA left the controller without a DA for us");

	/* The controller's descriptor and the target agree only if a
	 * transfer to that DA actually reaches the application.
	 */
	bytes = write_bytes(0xb4, 8U, &a_rc);
	zassert_ok(a_rc, "write after ENTDAA failed (%d)", a_rc);
	zassert_equal(bytes, 8U, "only %zu of 8 bytes arrived at DA 0x%02x", bytes, assigned);
}

/*
 * DISEC is accepted and leaves the target serving traffic.
 *
 * Source: MIPI I3C Basic v1.0 5.1.9.3.1.  The event-status readback is not
 * reachable from the application, so this case
 * covers acceptance and non-interference only.
 * test_tir_refused_when_disabled is what proves the suppression itself;
 * the name here deliberately does not claim it.
 */
ZTEST(dual_board_loopback_target, test_disec_accepted_and_traffic_survives)
{
	char cmd[24];
	size_t bytes;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	/* I3C_CCC_EVT_* are BIT() macros, so STRINGIFY() would send "(1UL << 0)". */
	snprintk(cmd, sizeof(cmd), "DISEC %u", (unsigned int)I3C_CCC_EVT_INTR);
	rc = ctrl_ccc(cmd, &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "DISEC failed (%d)", a_rc);

	bytes = write_bytes(0xc5, 8U, &a_rc);
	zassert_ok(a_rc, "write after DISEC failed (%d)", a_rc);
	zassert_equal(bytes, 8U, "only %zu of 8 bytes arrived after DISEC", bytes);

	snprintk(cmd, sizeof(cmd), "ENEC %u", (unsigned int)I3C_CCC_EVT_INTR);
	rc = ctrl_ccc(cmd, &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "ENEC failed (%d)", a_rc);
}

/*
 * A SETMWL value survives a GETMWL round trip.
 *
 * Source: MIPI I3C Basic v1.0 5.1.9.3.5.  The CCC constrains the
 * controller, not the target: the spec gives no target-side rejection
 * behaviour for an over-length write, so this is a compliance and
 * round-trip case rather than an enforcement one.
 */
ZTEST(dual_board_loopback_target, test_mwl_roundtrip_and_honoured)
{
	static const unsigned int values[] = {8U, 16U, 32U};
	char cmd[16];
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	for (size_t i = 0; i < ARRAY_SIZE(values); i++) {
		struct i3c_config_target cfg = {0};
		unsigned int got = 0;

		snprintk(cmd, sizeof(cmd), "SETMWL %u", values[i]);
		rc = ctrl_ccc(cmd, &a_rc, NULL);
		zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
		zassert_ok(a_rc, "SETMWL %u failed (%d)", values[i], a_rc);

		rc = ctrl_ccc("GETMWL", &a_rc, &got);
		zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
		zassert_ok(a_rc, "GETMWL failed (%d)", a_rc);
		zassert_equal(got, values[i], "GETMWL returned %u, want %u", got, values[i]);

		rc = read_target_config(&cfg);
		zassert_ok(rc, "i3c_config_get(TARGET) failed (%d)", rc);
		zassert_equal(cfg.max_write_len, values[i], "target MWL is %u, want %u",
			      cfg.max_write_len, values[i]);
	}

	/* Put it back rather than relying on the last value in the array. */
	snprintk(cmd, sizeof(cmd), "SETMWL %u", (unsigned int)TARGET_BUF_MAX);
	(void)ctrl_ccc(cmd, &a_rc, NULL);
}

/*
 * SETDASA assigns the dynamic address the controller names.
 *
 * Source: MIPI I3C Basic v1.0 5.1.9.3.10.
 *
 * The controller suite cannot see this: it addresses the target by the DAT
 * entry it just wrote, so a controller that got the address wrong still
 * talks to the target it meant to.  Reading the target's own DEVICE_ADDR is
 * the independent observer.  ALT_DA differs from the enumerated DA, so a
 * SETDASA that silently did nothing fails here.
 */
ZTEST(dual_board_loopback_target, test_setdasa_assigns_da)
{
	uint32_t addr_after_setdasa;
	unsigned int assigned = 0;
	char cmd[16];
	size_t bytes;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	/* SETDASA addresses the target by its static address, so the dynamic
	 * one has to be gone first.
	 */
	rc = ctrl_ccc("RSTDAA", &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "RSTDAA failed (%d)", a_rc);

	snprintk(cmd, sizeof(cmd), "SETDASA %u", (unsigned int)ALT_DA);
	rc = ctrl_ccc(cmd, &a_rc, &assigned);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "SETDASA failed (%d)", a_rc);

	addr_after_setdasa = read_device_addr();

	/* The controller now addresses us at ALT_DA; prove the target
	 * agrees before handing the bus back.
	 */
	bytes = write_bytes(0x5c, 8U, &a_rc);

	(void)ctrl_re_enum(&a_rc);

	zassert_equal(assigned, ALT_DA, "controller recorded DA 0x%02x, want 0x%02x", assigned,
		      ALT_DA);
	zassert_not_equal(addr_after_setdasa & TGT_DEVICE_ADDR_DA_VALID, 0U,
			  "DEVICE_ADDR=%08x marks no valid dynamic address after SETDASA",
			  addr_after_setdasa);
	zassert_equal(FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, addr_after_setdasa), ALT_DA,
		      "target answers to DA 0x%02lx, want 0x%02x (DEVICE_ADDR=%08x)",
		      FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, addr_after_setdasa), ALT_DA,
		      addr_after_setdasa);
	zassert_equal(bytes, 8U, "only %zu of 8 bytes arrived at DA 0x%02x", bytes, ALT_DA);
}

/*
 * The identity the controller captured during ENTDAA is ours.
 *
 * Source: MIPI I3C Basic v1.0 5.1.4.1.
 *
 * BCR and DCR carry the weight here: Board A's descriptor is hand-built and
 * leaves both zero, so only the ENTDAA payload can fill them and a capture
 * that silently failed still reads 0.
 *
 * view.pid against cfg.pid alone would be vacuous -- both are read back from
 * the same target registers, so they agree even if those registers were
 * programmed from the wrong half of the configured PID.  Anchoring cfg.pid to
 * the TARGET_PID constant closes that: it is the configured value, so the pair
 * of assertions proves the PID was carried from i3c_configure() onto the bus
 * intact.  TARGET_PID is asymmetric for this reason.
 */
ZTEST(dual_board_loopback_target, test_entdaa_reports_identity)
{
	struct i3c_config_target cfg = {0};
	struct ctrl_view view = {0};
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = read_target_config(&cfg);
	zassert_ok(rc, "i3c_config_get(TARGET) failed (%d)", rc);

	zassert_equal(cfg.pid, (uint64_t)TARGET_PID,
		      "configured PID %012llx did not survive the round trip, read back %012llx",
		      (uint64_t)TARGET_PID, cfg.pid);

	rc = ctrl_ccc("RSTDAA", &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "RSTDAA failed (%d)", a_rc);

	rc = ctrl_ccc("ENTDAA", &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "ENTDAA failed (%d)", a_rc);

	rc = ctrl_dump(&view);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);

	zassert_equal(view.pid, cfg.pid, "controller captured PID %012llx, target reports %012llx",
		      view.pid, cfg.pid);
	zassert_equal(view.bcr, cfg.bcr, "controller captured BCR 0x%02x, target reports 0x%02x",
		      view.bcr, cfg.bcr);
	zassert_equal(view.dcr, cfg.dcr, "controller captured DCR 0x%02x, target reports 0x%02x",
		      view.dcr, cfg.dcr);
	zassert_equal(view.da, read_device_da(),
		      "controller holds DA 0x%02x, target answers to 0x%02x", view.da,
		      read_device_da());
}

/*
 * GETMXDS reports this target's data-speed limits.
 *
 * Source: MIPI I3C Basic v1.0 5.1.9.3.7.
 *
 * The target-side API has no max-data-speed field, so the oracle is the
 * target's own MAX_DATA_SPEED register: read-only and fixed at synthesis,
 * which makes it an observer the CCC path cannot influence.
 *
 * This IP is synthesised with both speed codes at 0, so the register reads
 * 0x00000000 and comparing it against the reply would pass on a controller
 * that received nothing.  Board A therefore seeds its response buffer with
 * 0xff before issuing the CCC (its GETMXDS handler in
 * i3c_controller/src/main.c), which decodes to speed code 7 - both an
 * inequality and an undefined code, so an unanswered CCC fails here rather
 * than passing quietly.
 *
 * MAX_DATA_SPEED: DW databook 6.1.32, offset 0x84.
 */
#define MAX_DATA_SPEED          0x84
#define MXDS_MAX_WR_SPEED       GENMASK(2, 0)
#define MXDS_MAX_RD_SPEED       GENMASK(10, 8)

/* DW databook 6.1.32 defines 0..4 (12.5/8/6/4/2 MHz); 5..7 are undefined. */
#define MXDS_SPEED_CODE_MAX     4U

ZTEST(dual_board_loopback_target, test_getmxds_reports_limits)
{
	struct i3c_config_target cfg = {0};
	unsigned int packed = 0;
	uint32_t mxds;
	uint8_t want_wr;
	uint8_t want_rd;
	uint8_t got_wr;
	uint8_t got_rd;
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	rc = read_target_config(&cfg);
	zassert_ok(rc, "i3c_config_get(TARGET) failed (%d)", rc);

	/* BCR bit 0 is what makes GETMXDS mandatory rather than optional, so
	 * the CCC only has to be answered when the target advertises it.
	 */
	zassert_not_equal(cfg.bcr & I3C_BCR_MAX_DATA_SPEED_LIMIT, 0U,
			  "BCR 0x%02x claims no data-speed limit, so GETMXDS is not required",
			  cfg.bcr);

	mxds = sys_read32(DT_REG_ADDR(DT_ALIAS(test_i3c)) + MAX_DATA_SPEED);
	want_wr = (uint8_t)FIELD_GET(MXDS_MAX_WR_SPEED, mxds);
	want_rd = (uint8_t)FIELD_GET(MXDS_MAX_RD_SPEED, mxds);

	rc = ctrl_ccc("GETMXDS", &a_rc, &packed);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "GETMXDS failed (%d)", a_rc);

	got_wr = I3C_CCC_GETMXDS_MAXWR_MAX_SDR_FSCL((uint8_t)(packed & 0xffU));
	got_rd = I3C_CCC_GETMXDS_MAXRD_MAX_SDR_FSCL((uint8_t)((packed >> 8) & 0xffU));

	zassert_equal(got_wr, want_wr, "controller read maxWr code %u, target register says %u",
		      got_wr, want_wr);
	zassert_equal(got_rd, want_rd, "controller read maxRd code %u, target register says %u",
		      got_rd, want_rd);
	zassert_true(got_wr <= MXDS_SPEED_CODE_MAX, "maxWr code %u is not a defined speed",
		     got_wr);
	zassert_true(got_rd <= MXDS_SPEED_CODE_MAX, "maxRd code %u is not a defined speed",
		     got_rd);
}
