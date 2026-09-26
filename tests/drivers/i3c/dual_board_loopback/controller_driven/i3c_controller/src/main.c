/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Controller-side fixture for the dual_board_loopback two-board test pair.
 * Owns DT handles, the Board B target descriptor (attached at suite
 * setup), and the ZTEST_SUITE registration.  Per-test ZTEST() bodies
 * live in the other src/test_*.c files.
 */

#include "test_common.h"
#include "test_identity.h"

#include <zephyr/devicetree.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>
#include <zephyr/init.h>
#include <string.h>

LOG_MODULE_REGISTER(dual_board_loopback_ctrl, LOG_LEVEL_INF);

/* Early boot marker to prove flash is working */
static int early_boot_marker(void)
{
	printk("\n\n*** CONTROLLER BUILD " __DATE__ " " __TIME__ " ***\n\n");
	return 0;
}
SYS_INIT(early_boot_marker, APPLICATION, 0);

/*
 * --------------------------------------------------------------------------
 * DT handles
 * --------------------------------------------------------------------------
 */
const struct device *const i3c_dev = DEVICE_DT_GET(DT_ALIAS(test_i3c));
const struct device *const sync_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_sync_uart));

/*
 * Stand-in driver instance for every peer descriptor this suite builds by
 * hand; see the comment on the declaration in test_common.h.
 */
const struct device i3c_peer_dev = {
	.name = "i3c-peer",
};

/*
 * Board B (I3C target) descriptor.  Both endpoints are known at build
 * time, so Board B is enumerated via SETDASA from its known static
 * address; PID is still carried so directed CCCs can verify identity.
 */
struct i3c_device_desc target_b_desc = {
	.bus = DEVICE_DT_GET(DT_ALIAS(test_i3c)),
	.dev = &i3c_peer_dev,
	.pid = TARGET_PID,
	.static_addr = TARGET_STATIC_ADDR,
	.init_dynamic_addr = TARGET_STATIC_ADDR,
	.flags = 0,
};

int dual_board_loopback_heal_target(void)
{
	struct i3c_ccc_address da = {.addr = CCC_ADDR_PAYLOAD(TARGET_STATIC_ADDR)};
	uint8_t prev_da = target_b_desc.dynamic_addr;
	int rc;

	if (prev_da == 0U) {
		prev_da = TARGET_STATIC_ADDR;
	}

	sync_send(sync_uart, "SOFT_RST");
	(void)sync_expect(sync_uart, "READY", K_MSEC(2000));
	(void)i3c_recover_bus(i3c_dev);
	k_msleep(200);
	(void)i3c_ccc_do_rstdaa_all(i3c_dev);

	/*
	 * SETDASA is only legal for a device with no dynamic address, and the
	 * driver keys the DAT entry on whatever address the descriptor holds
	 * at attach time. Detach, clear, re-attach so the CCC and the table
	 * agree the target is currently unaddressed. Detach frees the address
	 * slot named by the descriptor, so point it at the reserved address
	 * first; a stale DA would free the wrong slot and leak the real one.
	 */
	target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
	(void)i3c_detach_i3c_device(&target_b_desc);
	target_b_desc.dynamic_addr = 0U;

	rc = i3c_attach_i3c_device(&target_b_desc);
	if ((rc != 0) && (rc != -EALREADY)) {
		printk("[heal] re-attach failed (%d)\n", rc);
		target_b_desc.dynamic_addr = prev_da;
		return rc;
	}

	/* SETDASA (not ENTDAA) restores the known static address; ENTDAA
	 * could yield a different DA after arbitration against another
	 * on-bus target.
	 */
	rc = i3c_ccc_do_setdasa(&target_b_desc, da);
	if (rc != 0) {
		/* Restore prev_da so the next before_each retries instead of
		 * treating the target as permanently absent.
		 */
		target_b_desc.dynamic_addr = prev_da;
		return rc;
	}

	/*
	 * Re-attach rather than reattach: the driver only writes the DAT
	 * dynamic-address field when dynamic_addr is already set, and
	 * i3c_reattach_i3c_device() bails with -EADDRNOTAVAIL here because
	 * the attach above already claimed this address slot.
	 */
	(void)i3c_detach_i3c_device(&target_b_desc);
	target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
	rc = i3c_attach_i3c_device(&target_b_desc);
	if (rc == -EALREADY) {
		rc = 0;
	}

	return rc;
}

/* Suite setup: ready DT devices, handshake with Board B, attach and
 * enumerate it.
 */
static void *suite_setup(void)
{
	int rc;
	uint32_t t0, t1;

	printk(">>> SUITE_SETUP ENTER\n");

	t0 = (uint32_t)k_uptime_get();
	printk("[t=%u] device_is_ready checks\n", t0);
	zassert_true(device_is_ready(i3c_dev), "I3C controller %s not ready", i3c_dev->name);
	zassert_true(device_is_ready(sync_uart), "Sync UART %s not ready", sync_uart->name);

	/*
	 * Three-way HELLO with Board B over the sync UART; if it fails,
	 * skip i3c_bus_init() and let bus-traffic tests FAIL via
	 * I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA() rather than crash the suite
	 * on the DW driver's NULL-deref CCC abort path.
	 */
	t1 = (uint32_t)k_uptime_get();
	printk("[t=%u] sync_handshake start\n", t1);
	/* Drain RX junk accumulated while Board B was being flashed. */
	sync_drain(sync_uart);
	rc = sync_handshake(sync_uart, K_SECONDS(10));
	t1 = (uint32_t)k_uptime_get();
	printk("[t=%u] sync_handshake => %d\n", t1, rc);
	if (rc != 0) {
		printk("*** TARGET ABSENT (sync_handshake rc=%d) — "
		       "skipping i3c_bus_init; bus-traffic tests will FAIL "
		       "via I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA ***\n",
		       rc);
		return NULL;
	}

	k_msleep(2000);

	/*
	 * Attach target_b BEFORE bus init so the driver's PID lookup
	 * matches it during ENTDAA and fills in dynamic_addr / bcr / dcr
	 * directly on our descriptor.
	 */
	t1 = (uint32_t)k_uptime_get();
	printk("[t=%u] attach target_b (PID 0x%04x%08x)\n", t1, (uint16_t)(TARGET_PID >> 32),
	       (uint32_t)(TARGET_PID & 0xFFFFFFFFU));
	rc = i3c_attach_i3c_device(&target_b_desc);
	zassert_ok(rc, "i3c_attach_i3c_device(target_b) failed (%d)", rc);

	t1 = (uint32_t)k_uptime_get();
	printk("[t=%u] calling i3c_bus_init\n", t1);
	{
		struct i3c_dev_list dev_list = {
			.i3c = &target_b_desc,
			.i2c = NULL,
			.num_i3c = 1,
			.num_i2c = 0,
		};

		rc = i3c_bus_init(i3c_dev, &dev_list);
	}
	t1 = (uint32_t)k_uptime_get();
	printk("[t=%u] i3c_bus_init => %d\n", t1, rc);
	printk("[t=%u] target_b dynamic_addr = 0x%02x  bcr=0x%02x  dcr=0x%02x\n",
	       (uint32_t)k_uptime_get(), target_b_desc.dynamic_addr, target_b_desc.bcr,
	       target_b_desc.dcr);

	if (target_b_desc.dynamic_addr == 0) {
		int hrc = dual_board_loopback_heal_target();

		printk("[setup] bus init left no DA; heal rc=%d DA=0x%02x\n", hrc,
		       target_b_desc.dynamic_addr);
	}

	if (target_b_desc.dynamic_addr == 0) {
		printk("*** TARGET NOT FOUND ON BUS — tests will skip ***\n");
	}

	/*
	 * The bench-permanent target descriptor (if any) stays attached
	 * for the whole suite: mid-suite RSTDAA broadcasts force every
	 * on-bus target to re-arbitrate, and without a bound descriptor
	 * the driver hands the next-free DA to the unknown PID and can
	 * end up stealing target_b's slot.
	 */

	return NULL;
}

static void suite_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	sync_drain(sync_uart);
	sync_send(sync_uart, "RESET");
	{
		char ready_line[128];
		int rc = sync_expect_line(sync_uart, "READY", ready_line, sizeof(ready_line),
					  K_MSEC(100));
		if (rc == 0) {
			LOG_INF("[before_each] %s", ready_line);
		}
	}

	/* If a previous test left target_b without a DA, recover via
	 * ENTDAA.  The bench-permanent descriptor (if any) remains
	 * attached so it doesn't steal target_b's slot.
	 */
	if (target_b_desc.dynamic_addr == 0U) {
		int rc = i3c_do_daa(i3c_dev);

		if (rc != 0) {
			printk("[before_each] ENTDAA recovery failed: %d\n", rc);
		}

		/* ENTDAA cannot match a descriptor the driver has detached,
		 * so fall back to re-attach + SETDASA.
		 */
		if (target_b_desc.dynamic_addr == 0U) {
			rc = dual_board_loopback_heal_target();
			printk("[before_each] SETDASA heal rc=%d DA=0x%02x\n", rc,
			       target_b_desc.dynamic_addr);
		}
	}

	/* Recover from any HALT state left by a previous test. */
	(void)i3c_recover_bus(i3c_dev);

	/* Probe target with a 1-byte write; retry on NACK (legitimate
	 * I3C flow control while the target clears SLAVE_BUSY).  Only
	 * re-enumerate if all retries fail.
	 */
	if (target_b_desc.dynamic_addr != 0U) {
		uint8_t probe = 0x00;
		struct i3c_msg msg = {
			.buf = &probe,
			.len = 1,
			.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
		};
		int rc = -1;

		/* Probe the target: 20 × 10 ms (~200 ms total).  Memory
		 * notes: the target IP recovers from its post-large-W NACK
		 * state within ~100-200 ms.  k_msleep yields so kernel
		 * timers/IRQs can run.
		 */
		for (int attempt = 0; attempt < 20; attempt++) {
			rc = i3c_transfer(&target_b_desc, &msg, 1);
			if (rc == 0) {
				break;
			}
			k_msleep(10);
		}

		if (rc != 0) {
			printk("[before_each] probe failed (%d) after retries, "
			       "re-enumerating\n",
			       rc);

			rc = dual_board_loopback_heal_target();
			printk("[before_each] re-enum done, DA=0x%02x "
			       "rc=%d\n",
			       target_b_desc.dynamic_addr, rc);

			/* Post re-enum settle + verify, same 200 ms budget. */
			k_msleep(50);
			for (int attempt = 0; attempt < 20; attempt++) {
				rc = i3c_transfer(&target_b_desc, &msg, 1);
				if (rc == 0) {
					break;
				}
				k_msleep(10);
			}
			if (rc != 0) {
				printk("[before_each] post-renum probe still "
				       "failing (%d), keeping DA so subsequent "
				       "tests can retry recovery (this test "
				       "will fail rather than skip)\n",
				       rc);
				/*
				 * Do NOT clear target_b_desc.dynamic_addr.
				 * The target typically recovers within 100-200
				 * ms, so the next before_each's probe usually
				 * succeeds.  Clearing the DA marks the target
				 * dead and would skip the rest of the suite.
				 * Better policy: this test FAILs honestly,
				 * the next test gets a fresh recovery attempt.
				 */
			}
		}

		/* Clear sticky OVERFLOW/UNDERFLOW/PROTOCOL_ERR via directed
		 * GETSTATUS (databook §3.5.2 / §6.1.24: the controller NACKs
		 * further private xfers until GETSTATUS reads device status).
		 * If GETSTATUS itself NACKs, the IP halts — recover so the
		 * halt does not cascade.
		 */
		if (target_b_desc.dynamic_addr != 0U) {
			union i3c_ccc_getstatus gs;
			int gs_rc = i3c_ccc_do_getstatus_fmt1(&target_b_desc, &gs);

			if (gs_rc != 0) {
				LOG_WRN("[before_each] GETSTATUS failed: %d", gs_rc);
			} else if (gs.fmt1.status != 0U) {
				LOG_INF("[before_each] GETSTATUS=0x%04x", gs.fmt1.status);
			}
		}
	}
}

static void suite_after_each(void *fixture)
{
	ARG_UNUSED(fixture);

	dual_board_loopback_attach_cleanup();
}

static void suite_teardown(void *data)
{
	ARG_UNUSED(data);

	(void)i3c_detach_i3c_device(&target_b_desc);
}

ZTEST_SUITE(dual_board_loopback, NULL, suite_setup, suite_before_each, suite_after_each,
	    suite_teardown);
