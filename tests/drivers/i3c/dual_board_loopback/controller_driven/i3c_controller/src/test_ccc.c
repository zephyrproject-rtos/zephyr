/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 2: CCC sweep — exercises i3c_ccc_do_* helpers against Board B.
 */

#include "test_common.h"
#include "test_identity.h"

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/sys/util.h>

/* GETSTATUS format 1 fields (MIPI I3C): bit 5 protocol error, 15:8 reserved. */
#define CCC_GETSTATUS_PROTOCOL_ERR BIT(5)
#define CCC_GETSTATUS_RESERVED     GENMASK(15, 8)

/* Target-side SLV_EVENT_STATUS (I3C base + 0x38), reported as EVT= by DUMP.
 * Bits 5:4 carry the Activity State last requested by an ENTASx CCC and bit 0
 * the SIR enable last requested by ENEC/DISEC.  The driver names only the
 * event-enable bits, so the Activity State field is spelled out here.
 */
#define TGT_SLV_EVENT_SIR_EN        BIT(0)
#define TGT_SLV_EVENT_ACTIVITY_MASK GENMASK(5, 4)

ZTEST(dual_board_loopback, test_ccc_getbcr)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_getbcr bcr = {0};
	int rc = i3c_ccc_do_getbcr(&target_b_desc, &bcr);

	zassert_ok(rc, "GETBCR failed (%d)", rc);
	zassert_equal(bcr.bcr, TARGET_BCR, "GETBCR returned 0x%02x, target configured 0x%02x",
		      bcr.bcr, TARGET_BCR);
	/* desc.bcr is filled in by the driver at enumeration, so this catches a
	 * stale cache rather than restating the check above.
	 */
	zassert_equal(target_b_desc.bcr, bcr.bcr,
		      "cached BCR 0x%02x disagrees with wire 0x%02x", target_b_desc.bcr, bcr.bcr);
}

ZTEST(dual_board_loopback, test_ccc_getdcr)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_getdcr dcr = {0};
	int rc = i3c_ccc_do_getdcr(&target_b_desc, &dcr);

	zassert_ok(rc, "GETDCR failed (%d)", rc);
	zassert_equal(dcr.dcr, TARGET_DCR, "GETDCR returned 0x%02x, target configured 0x%02x",
		      dcr.dcr, TARGET_DCR);
	zassert_equal(target_b_desc.dcr, dcr.dcr,
		      "cached DCR 0x%02x disagrees with wire 0x%02x", target_b_desc.dcr, dcr.dcr);
}

ZTEST(dual_board_loopback, test_ccc_getpid)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_getpid pid = {0};
	int rc = i3c_ccc_do_getpid(&target_b_desc, &pid);
	uint64_t got = 0;

	zassert_ok(rc, "GETPID failed (%d)", rc);

	/* pid[] arrives MSB-first. */
	for (int i = 0; i < 6; i++) {
		got = (got << 8) | pid.pid[i];
	}

	zassert_equal(got, (uint64_t)TARGET_PID, "GETPID returned %08x%08x, expected %08x%08x",
		      (uint32_t)(got >> 32), (uint32_t)got,
		      (uint32_t)((uint64_t)TARGET_PID >> 32), (uint32_t)(uint64_t)TARGET_PID);
}

ZTEST(dual_board_loopback, test_ccc_getmrl)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_mrl mrl = {0};
	int rc = i3c_ccc_do_getmrl(&target_b_desc, &mrl);

	zassert_ok(rc, "GETMRL failed (%d)", rc);
	zassert_not_equal(mrl.len, 0U, "GETMRL reported a zero read length");
	/* i3c_device_basic_info_get() seeds these at enumeration, so a mismatch
	 * is a stale cache rather than a restatement of the wire read.
	 */
	zassert_equal(target_b_desc.data_length.mrl, mrl.len,
		      "cached MRL %u disagrees with wire %u",
		      target_b_desc.data_length.mrl, mrl.len);
	zassert_equal(target_b_desc.data_length.max_ibi, mrl.ibi_len,
		      "cached IBI length %u disagrees with wire %u",
		      target_b_desc.data_length.max_ibi, mrl.ibi_len);
}

ZTEST(dual_board_loopback, test_ccc_getmwl)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_mwl mwl = {0};
	int rc = i3c_ccc_do_getmwl(&target_b_desc, &mwl);

	zassert_ok(rc, "GETMWL failed (%d)", rc);
	zassert_not_equal(mwl.len, 0U, "GETMWL reported a zero write length");
	zassert_equal(target_b_desc.data_length.mwl, mwl.len,
		      "cached MWL %u disagrees with wire %u",
		      target_b_desc.data_length.mwl, mwl.len);
}

ZTEST(dual_board_loopback, test_ccc_setmrl_setmwl_roundtrip)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	struct i3c_ccc_mrl mrl_orig = {0};
	struct i3c_ccc_mwl mwl_orig = {0};
	struct i3c_ccc_mrl mrl_set = {.len = 64, .ibi_len = 4};
	struct i3c_ccc_mwl mwl_set = {.len = 64};
	struct i3c_ccc_mrl mrl_get = {0};
	struct i3c_ccc_mwl mwl_get = {0};
	struct i3c_ccc_mrl mrl_back = {0};
	struct i3c_ccc_mwl mwl_back = {0};
	int save_mrl_rc;
	int save_mwl_rc;
	int set_mrl_rc = -1;
	int set_mwl_rc = -1;
	int get_mrl_rc = -1;
	int get_mwl_rc = -1;
	int restore_mrl_rc = -1;
	int restore_mwl_rc = -1;
	int back_mrl_rc = -1;
	int back_mwl_rc = -1;

	/* Restore what the target was actually advertising: hard-coded values
	 * silently redefine MRL/MWL for every later test in the suite.
	 */
	save_mrl_rc = i3c_ccc_do_getmrl(&target_b_desc, &mrl_orig);
	save_mwl_rc = i3c_ccc_do_getmwl(&target_b_desc, &mwl_orig);

	if (save_mrl_rc == 0 && save_mwl_rc == 0) {
		set_mrl_rc = i3c_ccc_do_setmrl(&target_b_desc, &mrl_set);
		set_mwl_rc = i3c_ccc_do_setmwl(&target_b_desc, &mwl_set);
		get_mrl_rc = i3c_ccc_do_getmrl(&target_b_desc, &mrl_get);
		get_mwl_rc = i3c_ccc_do_getmwl(&target_b_desc, &mwl_get);

		restore_mrl_rc = i3c_ccc_do_setmrl(&target_b_desc, &mrl_orig);
		restore_mwl_rc = i3c_ccc_do_setmwl(&target_b_desc, &mwl_orig);
		back_mrl_rc = i3c_ccc_do_getmrl(&target_b_desc, &mrl_back);
		back_mwl_rc = i3c_ccc_do_getmwl(&target_b_desc, &mwl_back);
	}

	zassert_ok(save_mrl_rc, "GETMRL (save) failed (%d)", save_mrl_rc);
	zassert_ok(save_mwl_rc, "GETMWL (save) failed (%d)", save_mwl_rc);
	zassert_ok(set_mrl_rc, "SETMRL failed (%d)", set_mrl_rc);
	zassert_ok(set_mwl_rc, "SETMWL failed (%d)", set_mwl_rc);
	zassert_ok(get_mrl_rc, "GETMRL after SET failed (%d)", get_mrl_rc);
	zassert_equal(mrl_get.len, mrl_set.len, "MRL mismatch: got %u, set %u", mrl_get.len,
		      mrl_set.len);
	zassert_equal(mrl_get.ibi_len, mrl_set.ibi_len, "MRL IBI length mismatch: got %u, set %u",
		      mrl_get.ibi_len, mrl_set.ibi_len);
	zassert_ok(get_mwl_rc, "GETMWL after SET failed (%d)", get_mwl_rc);
	zassert_equal(mwl_get.len, mwl_set.len, "MWL mismatch: got %u, set %u", mwl_get.len,
		      mwl_set.len);

	zassert_ok(restore_mrl_rc, "MRL restore failed (%d)", restore_mrl_rc);
	zassert_ok(restore_mwl_rc, "MWL restore failed (%d)", restore_mwl_rc);
	zassert_ok(back_mrl_rc, "GETMRL after restore failed (%d)", back_mrl_rc);
	zassert_ok(back_mwl_rc, "GETMWL after restore failed (%d)", back_mwl_rc);
	zassert_equal(mrl_back.len, mrl_orig.len, "MRL left at %u, target booted with %u",
		      mrl_back.len, mrl_orig.len);
	zassert_equal(mrl_back.ibi_len, mrl_orig.ibi_len,
		      "MRL IBI length left at %u, target booted with %u", mrl_back.ibi_len,
		      mrl_orig.ibi_len);
	zassert_equal(mwl_back.len, mwl_orig.len, "MWL left at %u, target booted with %u",
		      mwl_back.len, mwl_orig.len);
}

ZTEST(dual_board_loopback, test_ccc_enec_disec)
{
	struct i3c_ccc_events ev = {.events = I3C_CCC_EVT_INTR};
	uint32_t evt_entry = 0;
	uint32_t evt[4] = {0};
	int read_rc[4];
	int ccc_rc[4];
	int entry_rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	entry_rc = target_dump_field_read("EVT=", &evt_entry);

	/* Directed then broadcast, each enable followed by its disable. */
	ccc_rc[0] = i3c_ccc_do_events_set(&target_b_desc, true, &ev);
	k_msleep(TGT_CCC_SETTLE_MS);
	read_rc[0] = target_dump_field_read("EVT=", &evt[0]);

	ccc_rc[1] = i3c_ccc_do_events_set(&target_b_desc, false, &ev);
	k_msleep(TGT_CCC_SETTLE_MS);
	read_rc[1] = target_dump_field_read("EVT=", &evt[1]);

	ccc_rc[2] = i3c_ccc_do_events_all_set(i3c_dev, true, &ev);
	k_msleep(TGT_CCC_SETTLE_MS);
	read_rc[2] = target_dump_field_read("EVT=", &evt[2]);

	ccc_rc[3] = i3c_ccc_do_events_all_set(i3c_dev, false, &ev);
	k_msleep(TGT_CCC_SETTLE_MS);
	read_rc[3] = target_dump_field_read("EVT=", &evt[3]);

	/* Hand the bus back in the state it was found in, whichever that was. */
	if (entry_rc == 0) {
		(void)i3c_ccc_do_events_set(&target_b_desc,
					    (evt_entry & TGT_SLV_EVENT_SIR_EN) != 0U, &ev);
	}

	zassert_ok(entry_rc, "EVT read before ENEC failed (%d)", entry_rc);
	zassert_ok(ccc_rc[0], "ENEC failed (%d)", ccc_rc[0]);
	zassert_ok(ccc_rc[1], "DISEC failed (%d)", ccc_rc[1]);
	zassert_ok(ccc_rc[2], "ENEC all failed (%d)", ccc_rc[2]);
	zassert_ok(ccc_rc[3], "DISEC all failed (%d)", ccc_rc[3]);

	for (size_t i = 0; i < ARRAY_SIZE(evt); i++) {
		zassert_ok(read_rc[i], "EVT read %u failed (%d)", (unsigned int)i, read_rc[i]);
	}

	zassert_not_equal(evt[0] & TGT_SLV_EVENT_SIR_EN, 0U,
			  "ENEC left the target with SIR disabled (EVT 0x%08x)", evt[0]);
	zassert_equal(evt[1] & TGT_SLV_EVENT_SIR_EN, 0U,
		      "DISEC left the target with SIR enabled (EVT 0x%08x)", evt[1]);
	zassert_not_equal(evt[2] & TGT_SLV_EVENT_SIR_EN, 0U,
			  "broadcast ENEC left the target with SIR disabled (EVT 0x%08x)", evt[2]);
	zassert_equal(evt[3] & TGT_SLV_EVENT_SIR_EN, 0U,
		      "broadcast DISEC left the target with SIR enabled (EVT 0x%08x)", evt[3]);
}

ZTEST(dual_board_loopback, test_ccc_getstatus)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	union i3c_ccc_getstatus st = {0};
	int rc = i3c_ccc_do_getstatus_fmt1(&target_b_desc, &st);

	zassert_ok(rc, "GETSTATUS failed (%d)", rc);
	zassert_equal(st.fmt1.status & CCC_GETSTATUS_PROTOCOL_ERR, 0U,
		      "target reports a protocol error (status 0x%04x)", st.fmt1.status);
	/* Reserved bits land in 15:8 only if the big-endian conversion is wrong. */
	zassert_equal(st.fmt1.status & CCC_GETSTATUS_RESERVED, 0U,
		      "reserved status bits set (status 0x%04x)", st.fmt1.status);
}

/* Walk the four Activity States in a random order so a target that simply
 * latched the previous value cannot be mistaken for one that decoded each
 * CCC.
 */
static const uint8_t entas_probe_order[] = {2U, 0U, 3U, 1U};

static int entas_send(uint8_t level)
{
	switch (level) {
	case 0U:
		return i3c_ccc_do_entas0(&target_b_desc);
	case 1U:
		return i3c_ccc_do_entas1(&target_b_desc);
	case 2U:
		return i3c_ccc_do_entas2(&target_b_desc);
	default:
		return i3c_ccc_do_entas3(&target_b_desc);
	}
}

ZTEST(dual_board_loopback, test_ccc_entas_levels)
{
	int entas_rc[ARRAY_SIZE(entas_probe_order)];
	int read_rc[ARRAY_SIZE(entas_probe_order)];
	uint32_t evt[ARRAY_SIZE(entas_probe_order)];
	uint32_t evt_all = 0;
	int all_rc;
	int all_read_rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	for (size_t i = 0; i < ARRAY_SIZE(entas_probe_order); i++) {
		evt[i] = 0;
		entas_rc[i] = entas_send(entas_probe_order[i]);
		k_msleep(TGT_CCC_SETTLE_MS);
		read_rc[i] = target_dump_field_read("EVT=", &evt[i]);
	}

	/* Broadcast form, and it doubles as the restore to the idle state. */
	all_rc = i3c_ccc_do_entas0_all(i3c_dev);
	k_msleep(TGT_CCC_SETTLE_MS);
	all_read_rc = target_dump_field_read("EVT=", &evt_all);

	for (size_t i = 0; i < ARRAY_SIZE(entas_probe_order); i++) {
		uint8_t level = entas_probe_order[i];
		uint8_t state;

		zassert_ok(entas_rc[i], "ENTAS%u failed (%d)", level, entas_rc[i]);
		zassert_ok(read_rc[i], "EVT read after ENTAS%u failed (%d)", level, read_rc[i]);

		state = (uint8_t)FIELD_GET(TGT_SLV_EVENT_ACTIVITY_MASK, evt[i]);
		zassert_equal(state, level, "ENTAS%u left the target in activity state %u", level,
			      state);
	}

	zassert_ok(all_rc, "ENTAS0 all failed (%d)", all_rc);
	zassert_ok(all_read_rc, "EVT read after broadcast ENTAS0 failed (%d)", all_read_rc);
	zassert_equal(FIELD_GET(TGT_SLV_EVENT_ACTIVITY_MASK, evt_all), 0U,
		      "broadcast ENTAS0 left the target in activity state %u",
		      (uint8_t)FIELD_GET(TGT_SLV_EVENT_ACTIVITY_MASK, evt_all));
}

ZTEST(dual_board_loopback, test_ccc_getmxds)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	/* MIPI I3C makes GETMXDS optional unless the target advertises BCR[0]. */
	if ((target_b_desc.bcr & I3C_BCR_MAX_DATA_SPEED_LIMIT) == 0U) {
		ztest_test_skip();
		return;
	}

	union i3c_ccc_getmxds fmt1 = {0};
	union i3c_ccc_getmxds fmt2 = {0};
	int fmt1_rc = i3c_ccc_do_getmxds_fmt1(&target_b_desc, &fmt1);
	int fmt2_rc = i3c_ccc_do_getmxds_fmt2(&target_b_desc, &fmt2);

	zassert_ok(fmt1_rc, "GETMXDS format 1 failed (%d)", fmt1_rc);
	zassert_ok(fmt2_rc, "GETMXDS format 2 failed (%d)", fmt2_rc);

	/* Format 2 is the same two bytes plus a turnaround time, so a length or
	 * endianness slip in either path shows up as a disagreement.
	 */
	zassert_equal(fmt1.fmt1.maxwr, fmt2.fmt2.maxwr,
		      "maxWr differs across formats (0x%02x/0x%02x)",
		      fmt1.fmt1.maxwr, fmt2.fmt2.maxwr);
	zassert_equal(fmt1.fmt1.maxrd, fmt2.fmt2.maxrd,
		      "maxRd differs across formats (0x%02x/0x%02x)",
		      fmt1.fmt1.maxrd, fmt2.fmt2.maxrd);

	/* Speed codes above 2MHz are reserved by MIPI I3C. */
	zassert_true(I3C_CCC_GETMXDS_MAXWR_MAX_SDR_FSCL(fmt1.fmt1.maxwr) <=
			     I3C_CCC_GETMXDS_MAX_SDR_FSCL_2MHZ,
		     "reserved maxWr speed code (maxWr 0x%02x)", fmt1.fmt1.maxwr);
	zassert_true(I3C_CCC_GETMXDS_MAXRD_MAX_SDR_FSCL(fmt1.fmt1.maxrd) <=
			     I3C_CCC_GETMXDS_MAX_SDR_FSCL_2MHZ,
		     "reserved maxRd speed code (maxRd 0x%02x)", fmt1.fmt1.maxrd);
}

ZTEST(dual_board_loopback, test_ccc_setnewda_round_trip)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)i3c_dev->data;
	uint8_t saved = target_b_desc.dynamic_addr;
	uint32_t moved_addr = 0;
	uint32_t back_addr = 0;
	uint8_t new_da;
	int setnewda_rc;
	int moved_rc = -1;
	int restore_rc = -1;
	int back_rc = -1;
	bool healed = false;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	/* A hard-coded DA could already belong to another device on the bus. */
	new_da = i3c_addr_slots_next_free_find(&data->attached_dev.addr_slots,
					       (uint8_t)(saved + 1U));
	zassert_not_equal(new_da, 0U, "no free DA available for SETNEWDA");

	struct i3c_ccc_address to_new = {.addr = CCC_ADDR_PAYLOAD(new_da)};

	setnewda_rc = i3c_ccc_do_setnewda(&target_b_desc, to_new);

	if (setnewda_rc == 0) {
		k_msleep(TGT_CCC_SETTLE_MS);
		moved_rc = target_device_addr_read(&moved_addr);

		/* The restore CCC is routed through the DAT, so the table has to
		 * follow the target before it is sent.
		 */
		target_b_desc.dynamic_addr = new_da;
		(void)i3c_reattach_i3c_device(&target_b_desc, saved);

		struct i3c_ccc_address to_old = {.addr = CCC_ADDR_PAYLOAD(saved)};

		restore_rc = i3c_ccc_do_setnewda(&target_b_desc, to_old);
		if (restore_rc == 0) {
			target_b_desc.dynamic_addr = saved;
			restore_rc = i3c_reattach_i3c_device(&target_b_desc, new_da);
			k_msleep(TGT_CCC_SETTLE_MS);
			back_rc = target_device_addr_read(&back_addr);
		}
	}

	if (setnewda_rc != 0 || restore_rc != 0) {
		healed = (dual_board_loopback_heal_target() == 0);
	}

	zassert_ok(setnewda_rc, "SETNEWDA to 0x%02x returned %d", new_da, setnewda_rc);
	zassert_ok(moved_rc, "DUMP after SETNEWDA failed (%d)", moved_rc);
	zassert_not_equal(moved_addr & TGT_DEVICE_ADDR_DA_VALID, 0U,
			  "SETNEWDA left the target with no valid DA (DEVICE_ADDR=0x%08x)",
			  moved_addr);
	zassert_equal(FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, moved_addr), new_da,
		      "SETNEWDA reported success but the target holds DA 0x%02x, expected 0x%02x",
		      (uint8_t)FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, moved_addr), new_da);

	zassert_ok(restore_rc, "restore to DA 0x%02x returned %d (healed=%d)", saved, restore_rc,
		   healed);
	zassert_ok(back_rc, "DUMP after restore failed (%d)", back_rc);
	zassert_equal(FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, back_addr), saved,
		      "round trip left the target at DA 0x%02x, expected 0x%02x",
		      (uint8_t)FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, back_addr), saved);
}

/*
 * The controller's descriptor cannot show whether SETDASA carried the right
 * address: the driver sets desc->dynamic_addr from its own bookkeeping
 * whatever went out on the wire.  Only the target's DEVICE_ADDR can.
 */
ZTEST(dual_board_loopback, test_ccc_setdasa_assigns_static_addr)
{
	struct i3c_ccc_address da = {.addr = CCC_ADDR_PAYLOAD(TARGET_STATIC_ADDR)};
	uint32_t device_addr = 0;
	int rstdaa_rc;
	int setdasa_rc = -1;
	int dump_rc = -1;
	bool healed = false;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	/*
	 * Without the RSTDAA the target keeps the DA it already holds and the
	 * check below passes without SETDASA having assigned anything.
	 */
	rstdaa_rc = i3c_ccc_do_rstdaa_all(i3c_dev);

	if (rstdaa_rc == 0) {
		target_b_desc.dynamic_addr = 0;
		setdasa_rc = i3c_ccc_do_setdasa(&target_b_desc, da);
	}

	if (setdasa_rc == 0) {
		target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
		k_msleep(TGT_CCC_SETTLE_MS);
		dump_rc = target_device_addr_read(&device_addr);
	} else {
		healed = (dual_board_loopback_heal_target() == 0);
	}

	printk("[setdasa] rstdaa=%d setdasa=%d DEVICE_ADDR 0x%08x\n", rstdaa_rc, setdasa_rc,
	       device_addr);

	zassert_ok(rstdaa_rc, "pre-SETDASA RSTDAA returned %d", rstdaa_rc);
	zassert_ok(setdasa_rc, "SETDASA returned %d (healed=%d)", setdasa_rc, healed);
	zassert_ok(dump_rc, "DUMP after SETDASA failed (%d)", dump_rc);
	zassert_not_equal(device_addr & TGT_DEVICE_ADDR_DA_VALID, 0U,
			  "SETDASA left the target with no valid DA (DEVICE_ADDR=0x%08x)",
			  device_addr);
	zassert_equal(FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, device_addr), TARGET_STATIC_ADDR,
		      "SETDASA gave the target DA 0x%02x, expected its static address 0x%02x",
		      (uint8_t)FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, device_addr), TARGET_STATIC_ADDR);
}

ZTEST(dual_board_loopback, test_ccc_setaasa_all)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)i3c_dev->data;
	uint32_t device_addr = 0;
	int rstdaa_rc;
	int setaasa_rc = -1;
	int dump_rc = -1;
	int reattach_rc = -1;
	bool healed = false;

	/*
	 * SETAASA only assigns SA as DA to a target that has no DA, so without
	 * the RSTDAA the target keeps the DA it already holds and every check
	 * below passes without SETAASA having done anything.
	 */
	rstdaa_rc = i3c_ccc_do_rstdaa_all(i3c_dev);

	if (rstdaa_rc == 0) {
		setaasa_rc = i3c_ccc_do_setaasa_all(i3c_dev);
	}

	if (setaasa_rc == 0) {
		k_msleep(TGT_CCC_SETTLE_MS);
		dump_rc = target_device_addr_read(&device_addr);

		/* The driver's post-RSTDAA cleanup freed this address slot, and
		 * a broadcast CCC cannot mark it taken again.  Re-attaching is
		 * what re-reserves it; assigning dynamic_addr alone leaves the
		 * slot free while the target is using it.
		 */
		(void)i3c_detach_i3c_device(&target_b_desc);
		target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
		reattach_rc = i3c_attach_i3c_device(&target_b_desc);
	}

	if (reattach_rc != 0) {
		healed = (dual_board_loopback_heal_target() == 0);
	}

	zassert_ok(rstdaa_rc, "pre-SETAASA RSTDAA returned %d", rstdaa_rc);
	zassert_ok(setaasa_rc, "SETAASA returned %d", setaasa_rc);
	zassert_ok(dump_rc, "DUMP after SETAASA failed (%d)", dump_rc);
	zassert_not_equal(device_addr & TGT_DEVICE_ADDR_DA_VALID, 0U,
			  "SETAASA left the target with no valid DA (DEVICE_ADDR=0x%08x)",
			  device_addr);
	zassert_equal(FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, device_addr), TARGET_STATIC_ADDR,
		      "SETAASA gave the target DA 0x%02x, expected its static address 0x%02x",
		      (uint8_t)FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, device_addr),
		      TARGET_STATIC_ADDR);
	zassert_ok(reattach_rc, "re-attach after SETAASA returned %d (healed=%d)", reattach_rc,
		   healed);
	zassert_false(i3c_addr_slots_is_free(&data->attached_dev.addr_slots, TARGET_STATIC_ADDR),
		      "address 0x%02x left free while the target is using it",
		      TARGET_STATIC_ADDR);
}

ZTEST(dual_board_loopback, test_ccc_rstdaa_all)
{
	uint32_t before = 0;
	uint32_t after = 0;
	uint32_t ctrl_before = 0;
	uint32_t ctrl_after = 0;
	int setdasa_rc;
	int dump_rc;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	dump_rc = target_device_addr_read(&before);
	zassert_ok(dump_rc, "DUMP before RSTDAA failed (%d)", dump_rc);
	zassert_not_equal(before & TGT_DEVICE_ADDR_DA_VALID, 0U,
			  "precondition: target holds no valid DA before RSTDAA "
			  "(DEVICE_ADDR=0x%08x)",
			  before);

	rc = i3c_ccc_do_rstdaa_all(i3c_dev);
	if (rc == 0) {
		k_msleep(TGT_CCC_SETTLE_MS);
		dump_rc = target_device_addr_read(&after);
	}

	/* Re-enumerate target_b via SETDASA (same path as suite_setup) BEFORE
	 * asserting the outcome: a failing zassert aborts the test, and leaving
	 * target_b without a DA cascades into every later test in the suite.
	 */
	target_b_desc.dynamic_addr = 0;

	struct i3c_ccc_address da = {.addr = CCC_ADDR_PAYLOAD(TARGET_STATIC_ADDR)};

	setdasa_rc = i3c_ccc_do_setdasa(&target_b_desc, da);
	if (setdasa_rc == 0) {
		target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
	}

	printk("[rstdaa] rc=%d DEVICE_ADDR 0x%08x -> 0x%08x setdasa=%d\n", rc, before, after,
	       setdasa_rc);

	/* RSTDAA is a mandatory broadcast CCC; tolerating -ENODEV/-ENOSYS here
	 * would let the DA-valid check below pass without ever running.
	 */
	zassert_ok(rc, "RSTDAA all returned %d", rc);
	zassert_ok(dump_rc, "DUMP after RSTDAA failed (%d)", dump_rc);
	zassert_equal(after & TGT_DEVICE_ADDR_DA_VALID, 0U,
		      "RSTDAA reported success but target still holds valid DA 0x%02x "
		      "(DEVICE_ADDR 0x%08x -> 0x%08x)",
		      (uint8_t)FIELD_GET(TGT_DEVICE_ADDR_DA_MASK, after), before, after);
	zassert_ok(setdasa_rc, "SETDASA re-enumeration after RSTDAA failed (%d)", setdasa_rc);

	/* Negative control: repeat the same observation with no RSTDAA in
	 * between.  Without it the check above cannot distinguish "RSTDAA
	 * cleared the DA" from "the settle delay or the DUMP itself clears it".
	 * Reads only, so the bus is left as the positive phase restored it.
	 */
	dump_rc = target_device_addr_read(&ctrl_before);
	zassert_ok(dump_rc, "control DUMP #1 failed (%d)", dump_rc);
	k_msleep(TGT_CCC_SETTLE_MS);
	dump_rc = target_device_addr_read(&ctrl_after);
	zassert_ok(dump_rc, "control DUMP #2 failed (%d)", dump_rc);

	printk("[rstdaa] control DEVICE_ADDR 0x%08x -> 0x%08x\n", ctrl_before, ctrl_after);

	zassert_not_equal(ctrl_before & TGT_DEVICE_ADDR_DA_VALID, 0U,
			  "control precondition: SETDASA left no valid DA (DEVICE_ADDR=0x%08x)",
			  ctrl_before);
	zassert_equal(ctrl_before, ctrl_after,
		      "control: DEVICE_ADDR changed with no CCC on the bus "
		      "(0x%08x -> 0x%08x)",
		      ctrl_before, ctrl_after);
}
