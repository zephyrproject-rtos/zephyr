/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 4: Attach / detach / reattach.  Exercises
 *   dw_i3c_attach_device, dw_i3c_detach_device, dw_i3c_reattach_device,
 *   i3c_addr_slots tracking.
 */

#include "test_common.h"
#include "test_identity.h"
#include <string.h>

#include <zephyr/drivers/i3c/ccc.h>

/*
 * i3c_attach_i3c_device() appends &desc->node to a driver-owned list, so
 * the driver keeps this pointer until detach.  A failing zassert aborts
 * the test before its detach runs, so these must not live on the stack.
 */
#define TEST_ATTACH_SCRATCH_DESCS 4

/*
 * Scratch PIDs never reach the bus - attach does not inspect the field and
 * these descriptors are never addressed.  They exist only to identify a
 * descriptor in a memory dump, so the sole requirement is that each value
 * differs from the others and from TARGET_PID.
 */
#define SCRATCH_PID_VENDOR 0xF0CACC1A
#define SCRATCH_PID(part)  ((((uint64_t)SCRATCH_PID_VENDOR) << 32) | (uint32_t)(part))

/* One part number per test, so a stray descriptor names its origin. */
#define SCRATCH_PART_NO_ADDRESS       0x0001U
#define SCRATCH_PART_ALREADY_ATTACHED 0x0002U
#define SCRATCH_PART_RESERVES_ADDR    0x0003U
#define SCRATCH_PART_REATTACH_TAKEN   0x0004U
#define SCRATCH_PART_DUPLICATE_ADDR   0x0005U
#define SCRATCH_PART_NOT_ATTACHED     0x0006U
#define SCRATCH_PART_MULTI_FIRST      0x0010U

/* Judged by whether the transfer completes, not by what the target got. */
#define TEST_XFER_BYTE 0x5aU

static struct i3c_device_desc scratch_desc[TEST_ATTACH_SCRATCH_DESCS];
static bool scratch_attached[TEST_ATTACH_SCRATCH_DESCS];

static int scratch_attach_at(int i, uint64_t pid, uint8_t static_addr)
{
	int rc;

	/* struct i3c_device_desc has const members, so no struct assignment. */
	memset(&scratch_desc[i], 0, sizeof(scratch_desc[i]));
	scratch_desc[i].bus = i3c_dev;
	scratch_desc[i].pid = pid;
	scratch_desc[i].static_addr = static_addr;

	rc = i3c_attach_i3c_device(&scratch_desc[i]);
	if (rc == 0) {
		scratch_attached[i] = true;
	}

	return rc;
}

static int scratch_attach(int i, uint64_t pid)
{
	return scratch_attach_at(i, pid, 0U);
}

static int scratch_detach(int i)
{
	int rc;

	if (!scratch_attached[i]) {
		return 0;
	}

	rc = i3c_detach_i3c_device(&scratch_desc[i]);
	scratch_attached[i] = false;

	return rc;
}

void dual_board_loopback_attach_cleanup(void)
{
	for (int i = 0; i < TEST_ATTACH_SCRATCH_DESCS; i++) {
		int rc = scratch_detach(i);

		/* Reached only after a test already failed, so report
		 * rather than assert and mask the original failure.
		 */
		if (rc != 0) {
			printk("attach cleanup: detach %d returned %d\n", i, rc);
		}
	}
}

static bool scratch_on_bus_list(int i)
{
	return i3c_is_i3c_device_attached(&scratch_desc[i]);
}

ZTEST(dual_board_loopback, test_attach_device_without_address)
{
	int rc;

	rc = scratch_attach(0, SCRATCH_PID(SCRATCH_PART_NO_ADDRESS));
	zassert_ok(rc, "attach returned %d", rc);
	zassert_true(scratch_on_bus_list(0), "descriptor missing from attached list");

	/* dw_i3c_attach_device() stores the allocated DAT slot here. */
	zassert_not_null(scratch_desc[0].controller_priv, "attach allocated no DAT slot");

	rc = scratch_detach(0);
	zassert_ok(rc, "detach returned %d", rc);
	zassert_false(scratch_on_bus_list(0), "descriptor still on attached list after detach");
	zassert_is_null(scratch_desc[0].controller_priv, "detach released no DAT slot");
}

ZTEST(dual_board_loopback, test_detach_rejects_xfer)
{
	uint8_t buf = TEST_XFER_BYTE;
	struct i3c_msg msg = {
		.buf = &buf,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	int xfer_rc;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	rc = i3c_detach_i3c_device(&target_b_desc);
	zassert_ok(rc, "detach returned %d", rc);

	xfer_rc = i3c_transfer(&target_b_desc, &msg, 1);

	/* Re-attach before asserting so a wrong xfer_rc cannot leave the
	 * shared target detached for every later test.
	 */
	rc = i3c_attach_i3c_device(&target_b_desc);
	zassert_ok(rc, "re-attach returned %d", rc);

	/* i3c_transfer() documents only 0/-EBUSY/-EIO and drivers disagree on
	 * which error a detached descriptor yields, so require only that it
	 * does not report success.
	 */
	zassert_not_equal(xfer_rc, 0, "transfer to a detached device reported success");
}

/*
 * Sibling of test_detach_rejects_xfer.  Detach removes the descriptor from the
 * driver's list; RSTDAA leaves it attached, holding its DAT slot, with no
 * dynamic address.  A private I3C transfer has no address to send in that
 * state, so it must not report success.
 */
ZTEST(dual_board_loopback, test_no_dynamic_addr_rejects_xfer)
{
	uint8_t buf = TEST_XFER_BYTE;
	struct i3c_msg msg = {
		.buf = &buf,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	struct i3c_ccc_address da = {.addr = CCC_ADDR_PAYLOAD(TARGET_STATIC_ADDR)};
	uint8_t da_after_rstdaa = 0xFFU;
	int xfer_rc = 0;
	int setdasa_rc;
	int reattach_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	rc = i3c_ccc_do_rstdaa_all(i3c_dev);
	if (rc == 0) {
		k_msleep(TGT_CCC_SETTLE_MS);
		da_after_rstdaa = target_b_desc.dynamic_addr;
		xfer_rc = i3c_transfer(&target_b_desc, &msg, 1);
	}

	/* Re-enumerate before asserting: a failing zassert aborts the test, and
	 * leaving target_b without a DA cascades into every later test.
	 */
	target_b_desc.dynamic_addr = 0;
	setdasa_rc = i3c_ccc_do_setdasa(&target_b_desc, da);
	if (setdasa_rc == 0) {
		/* Post-RSTDAA cleanup freed the address slot and a CCC cannot
		 * reserve it again; only re-attaching does.
		 */
		(void)i3c_detach_i3c_device(&target_b_desc);
		target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
		reattach_rc = i3c_attach_i3c_device(&target_b_desc);
	}

	printk("[nodaxfer] rstdaa=%d da_after=0x%02x xfer_rc=%d setdasa=%d reattach=%d\n", rc,
	       da_after_rstdaa, xfer_rc, setdasa_rc, reattach_rc);

	zassert_ok(rc, "RSTDAA all returned %d", rc);
	zassert_ok(setdasa_rc, "SETDASA re-enumeration after RSTDAA failed (%d)", setdasa_rc);
	zassert_ok(reattach_rc, "re-attach after SETDASA returned %d", reattach_rc);
	zassert_equal(da_after_rstdaa, 0U,
		      "precondition: RSTDAA left the descriptor holding DA 0x%02x",
		      da_after_rstdaa);
	zassert_not_equal(xfer_rc, 0,
			  "transfer to an attached target with no dynamic address "
			  "reported success");
}

ZTEST(dual_board_loopback, test_attach_already_attached)
{
	int rc;

	rc = scratch_attach(0, SCRATCH_PID(SCRATCH_PART_ALREADY_ATTACHED));
	zassert_ok(rc, "first attach returned %d", rc);

	/* Not scratch_attach(), which would clear the descriptor first. */
	rc = i3c_attach_i3c_device(&scratch_desc[0]);
	zassert_equal(rc, -EALREADY, "expected -EALREADY re-attaching, got %d", rc);

	zassert_true(scratch_on_bus_list(0), "rejected re-attach disturbed the attached list");

	rc = scratch_detach(0);
	zassert_ok(rc, "detach returned %d", rc);
}

ZTEST(dual_board_loopback, test_attach_reserves_address)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)i3c_dev->data;
	uint8_t addr_slot;
	int rc;

	addr_slot = i3c_addr_slots_next_free_find(&data->attached_dev.addr_slots, 0U);
	zassert_not_equal(addr_slot, 0U, "no free address available to attach at");

	rc = scratch_attach_at(0, SCRATCH_PID(SCRATCH_PART_RESERVES_ADDR), addr_slot);
	zassert_ok(rc, "attach at 0x%02x returned %d", addr_slot, rc);

	/* i3c.h states attach exists to reserve the address. */
	zassert_false(i3c_addr_slots_is_free(&data->attached_dev.addr_slots, addr_slot),
		      "address 0x%02x still free after attach", addr_slot);

	rc = scratch_detach(0);
	zassert_ok(rc, "detach returned %d", rc);
	zassert_true(i3c_addr_slots_is_free(&data->attached_dev.addr_slots, addr_slot),
		     "address 0x%02x not released by detach", addr_slot);
}

ZTEST(dual_board_loopback, test_reattach_rejects_taken_addr)
{
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	rc = scratch_attach(0, SCRATCH_PID(SCRATCH_PART_REATTACH_TAKEN));
	zassert_ok(rc, "attach returned %d", rc);

	scratch_desc[0].dynamic_addr = target_b_desc.dynamic_addr;
	rc = i3c_reattach_i3c_device(&scratch_desc[0], 0U);
	scratch_desc[0].dynamic_addr = 0U;

	/* i3c.h documents -EINVAL for this case; the implementation returns
	 * -EADDRNOTAVAIL, matching i3c_attach_i3c_device().
	 */
	zassert_equal(rc, -EADDRNOTAVAIL, "expected -EADDRNOTAVAIL reattaching onto 0x%02x, got %d",
		      target_b_desc.dynamic_addr, rc);

	rc = scratch_detach(0);
	zassert_ok(rc, "detach returned %d", rc);
}

ZTEST(dual_board_loopback, test_reattach_with_new_da)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)i3c_dev->data;
	uint8_t old_da = target_b_desc.dynamic_addr;
	uint8_t buf = TEST_XFER_BYTE;
	struct i3c_msg msg = {
		.buf = &buf,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	uint8_t new_da;
	int setnewda_rc;
	int reattach_rc = -1;
	int xfer_rc = -1;
	int restore_rc = 0;
	bool new_da_taken = false;
	bool old_da_freed = false;
	bool healed = false;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	new_da = i3c_addr_slots_next_free_find(&data->attached_dev.addr_slots,
					       (uint8_t)(old_da + 1U));
	zassert_not_equal(new_da, 0U, "no free DA available for reattach");

	/* SETNEWDA is addressed to old_da, so it has to precede the reattach. */
	struct i3c_ccc_address to_new = {.addr = CCC_ADDR_PAYLOAD(new_da)};

	setnewda_rc = i3c_ccc_do_setnewda(&target_b_desc, to_new);

	if (setnewda_rc == 0) {
		target_b_desc.dynamic_addr = new_da;
		reattach_rc = i3c_reattach_i3c_device(&target_b_desc, old_da);

		new_da_taken = !i3c_addr_slots_is_free(&data->attached_dev.addr_slots, new_da);
		old_da_freed = i3c_addr_slots_is_free(&data->attached_dev.addr_slots, old_da);

		/* Reaches the target only if the reattach rewrote the DAT. */
		xfer_rc = i3c_transfer_retry(&target_b_desc, &msg, 1);

		struct i3c_ccc_address to_old = {.addr = CCC_ADDR_PAYLOAD(old_da)};

		restore_rc = i3c_ccc_do_setnewda(&target_b_desc, to_old);
		if (restore_rc == 0) {
			target_b_desc.dynamic_addr = old_da;
			restore_rc = i3c_reattach_i3c_device(&target_b_desc, new_da);
		}
	}

	if (setnewda_rc != 0 || restore_rc != 0) {
		healed = (dual_board_loopback_heal_target() == 0);
	}

	/* All asserts at the end of the test to avoid a clean-up nightmare */
	zassert_ok(setnewda_rc, "SETNEWDA to 0x%02x returned %d", new_da, setnewda_rc);
	zassert_ok(reattach_rc, "reattach old=0x%02x new=0x%02x returned %d", old_da, new_da,
		   reattach_rc);
	zassert_true(new_da_taken, "new DA 0x%02x still free after reattach", new_da);
	zassert_true(old_da_freed, "old DA 0x%02x not freed by reattach", old_da);
	zassert_ok(xfer_rc, "transfer at reattached DA 0x%02x returned %d", new_da, xfer_rc);
	zassert_ok(restore_rc, "restore to DA 0x%02x returned %d (healed=%d)", old_da, restore_rc,
		   healed);
}

ZTEST(dual_board_loopback, test_attach_multiple_descriptors)
{
	void *slot[TEST_ATTACH_SCRATCH_DESCS] = {NULL};
	int detach_rc[TEST_ATTACH_SCRATCH_DESCS] = {0};
	bool all_listed = true;
	bool slots_distinct = true;
	int attached = 0;
	int rc = 0;

	for (int i = 0; i < TEST_ATTACH_SCRATCH_DESCS; i++) {
		rc = scratch_attach(i, SCRATCH_PID(SCRATCH_PART_MULTI_FIRST + i));
		if (rc != 0) {
			break;
		}
		slot[i] = scratch_desc[i].controller_priv;
		attached++;
	}

	/* Sampled while all descriptors are still attached. */
	for (int i = 0; i < attached; i++) {
		if (!scratch_on_bus_list(i)) {
			all_listed = false;
		}
		for (int j = i + 1; j < attached; j++) {
			if (slot[i] == slot[j]) {
				slots_distinct = false;
			}
		}
	}

	for (int i = 0; i < attached; i++) {
		detach_rc[i] = scratch_detach(i);
	}

	zassert_equal(attached, TEST_ATTACH_SCRATCH_DESCS, "attached %d of %d (last rc=%d)",
		      attached, TEST_ATTACH_SCRATCH_DESCS, rc);
	zassert_true(all_listed, "not every descriptor was on the attached list");
	zassert_true(slots_distinct, "two descriptors were given the same DAT slot");

	for (int i = 0; i < TEST_ATTACH_SCRATCH_DESCS; i++) {
		zassert_not_null(slot[i], "descriptor %d attached without a DAT slot", i);
		zassert_ok(detach_rc[i], "detach %d returned %d", i, detach_rc[i]);
		zassert_is_null(scratch_desc[i].controller_priv,
				"descriptor %d kept its DAT slot after detach", i);
		zassert_false(scratch_on_bus_list(i), "descriptor %d still listed after detach", i);
	}
}

ZTEST(dual_board_loopback, test_detach_not_attached)
{
	struct i3c_device_desc desc = {
		.bus = i3c_dev,
		.dev = &i3c_peer_dev,
		.pid = SCRATCH_PID(SCRATCH_PART_NOT_ATTACHED),
	};
	int rc;

	/* Detach rejects a non-member before retaining the pointer, so this
	 * descriptor may safely live on the stack.
	 */
	rc = i3c_detach_i3c_device(&desc);
	zassert_equal(rc, -EALREADY, "expected -EALREADY, got %d", rc);
}

ZTEST(dual_board_loopback, test_attach_duplicate_addr)
{
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	rc = scratch_attach_at(0, SCRATCH_PID(SCRATCH_PART_DUPLICATE_ADDR),
			       target_b_desc.dynamic_addr);
	zassert_equal(rc, -EADDRNOTAVAIL, "expected -EADDRNOTAVAIL for duplicate 0x%02x, got %d",
		      target_b_desc.dynamic_addr, rc);

	rc = scratch_detach(0);
	zassert_ok(rc, "detach returned %d", rc);
}
