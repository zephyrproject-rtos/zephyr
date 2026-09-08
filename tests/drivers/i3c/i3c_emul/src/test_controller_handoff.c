/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Controller-handoff tests for the I3C emulator. Split from
 * test_target.c so the more elaborate (and more likely to flake)
 * handoff scenarios stay isolated from the simpler write/read/FIFO
 * coverage in i3c_emul_target.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/target_device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include <string.h>

#include "test_target_emul.h"

#define I3C_BUS DT_NODELABEL(i3c0)

#define TARGET_A_PID		TEST_TARGET_A_PID

static const struct device *bus = DEVICE_DT_GET(I3C_BUS);

static struct i3c_device_desc *find_desc(uint64_t pid)
{
	struct i3c_device_id id = { .pid = pid };

	return i3c_device_find(bus, &id);
}

static struct {
	atomic_t handoff;
} g;

static int cb_handoff(struct i3c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	atomic_inc(&g.handoff);
	return 0;
}

static const struct i3c_target_callbacks tcb = {
	.controller_handoff_cb = cb_handoff,
};

static struct i3c_target_config tcfg = {
	.callbacks = &tcb,
};

static void *handoff_setup(void)
{
	int rc = test_target_bus_known_state(bus, TEST_TARGET_A_PID, TEST_TARGET_A_STATIC,
					     TEST_TARGET_B_PID, TEST_TARGET_B_INIT_DA);

	zassert_ok(rc, "test_target_bus_known_state: %d", rc);
	return NULL;
}

static void handoff_before(void *fixture)
{
	ARG_UNUSED(fixture);
	atomic_set(&g.handoff, 0);

	/*
	 * Re-establish the canonical address state — earlier handoff
	 * tests (RSTDAA, sec_handoffed re-attach) can leave the bus in
	 * a different shape than they found it.
	 */
	(void)test_target_bus_known_state(bus, TEST_TARGET_A_PID, TEST_TARGET_A_STATIC,
					  TEST_TARGET_B_PID, TEST_TARGET_B_INIT_DA);
}

ZTEST(i3c_emul_controller_handoff, test_controller_handoff_accept_records)
{
	int rc;

	/*
	 * i3c_target_controller_handoff(true) only stores the application's
	 * willingness to accept a future handoff. It does NOT itself fire
	 * controller_handoff_cb (real drivers fire that callback only when
	 * the wire-level handoff actually completes — see i3c_dw.c and
	 * i3c_cdns.c). Without a controller-side GETACCCR, the cb stays
	 * silent.
	 */
	rc = i3c_target_register(bus, &tcfg);
	zassert_ok(rc, "target_register: %d", rc);

	rc = i3c_target_controller_handoff(bus, true);
	zassert_ok(rc, "handoff(true): %d", rc);
	zassert_equal(atomic_get(&g.handoff), 0,
		      "handoff cb must not fire from the API call alone");

	rc = i3c_target_controller_handoff(bus, false);
	zassert_ok(rc, "handoff(false): %d", rc);
	zassert_equal(atomic_get(&g.handoff), 0, "handoff cb still silent on nack");

	(void)i3c_target_unregister(bus, &tcfg);
}

ZTEST(i3c_emul_controller_handoff, test_handoff_completes_on_getacccr_to_registered_target)
{
	struct i3c_config_target out_cfg = { 0 };
	struct i3c_device_desc *desc_a;
	int rc;

	/*
	 * Spec model:
	 * - Application brings the device's dynamic address up the proper
	 *   way (SETDASA / DAA), then registers as a target at that same
	 *   address. The bus emulator now routes CCCs at that address to
	 *   the application's target_cfg instead of the peripheral emul.
	 * - Application opts in via i3c_target_controller_handoff(true).
	 * - Active controller initiates handoff via
	 *   i3c_device_controller_handoff(desc, true), which issues
	 *   GETACCCR at the address.
	 * - The bus emulator detects "GETACCCR addressed to a registered
	 *   target_cfg with handoff_accept set", replies parity-correctly,
	 *   fires controller_handoff_cb, and flips
	 *   target_config.enabled = false (we are the active controller
	 *   now, not a target).
	 *
	 * The bus controller's self-DA is set from DT (primary-controller-da)
	 * to the same address that target A's DAA-assigned DA lands at, so
	 * i3c_target_register() intercepts CCCs targeted at that address.
	 */
	desc_a = find_desc(TARGET_A_PID);
	zassert_not_null(desc_a, "target A desc");
	zassert_not_equal(desc_a->dynamic_addr, 0U,
			  "before-hook should have established target A's DA");

	rc = i3c_target_register(bus, &tcfg);
	zassert_ok(rc, "target_register at 0x%02x: %d", desc_a->dynamic_addr, rc);

	rc = i3c_config_get(bus, I3C_CONFIG_TARGET, &out_cfg);
	zassert_ok(rc, "config_get: %d", rc);
	zassert_true(out_cfg.enabled, "registering as target should set enabled");

	rc = i3c_target_controller_handoff(bus, true);
	zassert_ok(rc, "handoff(true): %d", rc);

	rc = i3c_device_controller_handoff(desc_a, true);
	zassert_ok(rc, "i3c_device_controller_handoff: %d", rc);

	zassert_equal(atomic_get(&g.handoff), 1,
		      "controller_handoff_cb fires on completed handoff");

	rc = i3c_config_get(bus, I3C_CONFIG_TARGET, &out_cfg);
	zassert_ok(rc, "config_get post-handoff: %d", rc);
	zassert_false(out_cfg.enabled,
		      "after handoff completes, the new active controller is no "
		      "longer behaving as a target");

	(void)i3c_target_unregister(bus, &tcfg);
}

ZTEST(i3c_emul_controller_handoff, test_handoff_nacked_by_application_fails)
{
	struct i3c_device_desc *desc_a;
	int rc;

	/* Same setup as the accept test, but the application explicitly
	 * NACKs the handoff offer. The controller-side initiation must
	 * fail and the callback must stay silent.
	 */
	desc_a = find_desc(TARGET_A_PID);
	zassert_not_null(desc_a, "target A desc");
	zassert_not_equal(desc_a->dynamic_addr, 0U,
			  "before-hook should have established target A's DA");

	rc = i3c_target_register(bus, &tcfg);
	zassert_ok(rc, "target_register: %d", rc);

	rc = i3c_target_controller_handoff(bus, false);
	zassert_ok(rc, "handoff(false): %d", rc);

	rc = i3c_device_controller_handoff(desc_a, true);
	zassert_not_equal(rc, 0,
			  "controller-side handoff must fail when the target NACKs");
	zassert_equal(atomic_get(&g.handoff), 0, "callback must stay silent on NACK");

	(void)i3c_target_unregister(bus, &tcfg);
}

ZTEST(i3c_emul_controller_handoff, test_controller_handoff_to_peripheral_via_getacccr)
{
	struct i3c_device_desc *desc;
	int rc;

	/*
	 * No target_cfg registered at desc->dynamic_addr — the GETACCCR
	 * routes to the registered peripheral emul, which replies
	 * parity-correctly. This is the "controller initiates handoff
	 * to a secondary controller it knows about, but no application
	 * on this device is acting as that target" scenario; the bus
	 * emulator's CCC dispatch + the peripheral's GETACCCR responder
	 * are what get exercised.
	 */
	desc = find_desc(TARGET_A_PID);
	zassert_not_null(desc, "target A desc");
	zassert_not_equal(desc->dynamic_addr, 0U,
			  "before-hook should have established target A's DA");

	rc = i3c_device_controller_handoff(desc, true);
	zassert_ok(rc, "i3c_device_controller_handoff: %d", rc);
}

ZTEST(i3c_emul_controller_handoff, test_handoff_completes_and_sec_handoffed_repopulates_bus)
{
#if !defined(CONFIG_I3C_USE_IBI) || !defined(CONFIG_I3C_IBI_WORKQUEUE)
	ztest_test_skip();
#else
	struct i3c_device_desc *desc_a;
	struct i3c_device_desc *desc;
	int n_attached;
	int rc;

	/*
	 * End-to-end secondary-controller handoff:
	 *  1. As the active controller, broadcast DEFTGTS — peripherals
	 *     receive it AND the bus emulator stores a (de-shifted) deep
	 *     copy in data->common.deftgts because a target_cfg is
	 *     registered.
	 *  2. Active controller initiates handoff via GETACCCR direct CCC
	 *     at the registered target_cfg's address.
	 *  3. Bus emulator's GETACCCR-to-target_cfg interception replies
	 *     parity-correctly, fires controller_handoff_cb, flips
	 *     target_config.enabled = false, and enqueues
	 *     i3c_sec_handoffed on the IBI workqueue.
	 *  4. i3c_sec_handoffed runs: i3c_sec_bus_reset detaches all descs,
	 *     then for each entry in data->common.deftgts it calls
	 *     i3c_sec_get_basic_info, which attaches a temp_desc and
	 *     issues GETPID. The bus emulator's wire-level dynamic-address
	 *     fallback finds the peripheral, GETPID returns the right PID,
	 *     the framework looks up the static desc by PID and re-attaches
	 *     it, and this driver's PID-keyed attach hook re-links
	 *     desc->controller_priv.
	 *
	 * After the workqueue drains, every desc in DEFTGTS should be
	 * re-attached and have controller_priv set, proving the device-now-
	 * acting-as-active-controller can immediately drive the bus.
	 */
	desc_a = find_desc(TARGET_A_PID);
	zassert_not_null(desc_a, "target A desc");
	zassert_not_equal(desc_a->dynamic_addr, 0U,
			  "before-hook should have established target A's DA");

	rc = i3c_target_register(bus, &tcfg);
	zassert_ok(rc, "target_register: %d", rc);

	rc = i3c_target_controller_handoff(bus, true);
	zassert_ok(rc, "handoff(true): %d", rc);

	rc = i3c_bus_deftgts(bus);
	zassert_ok(rc, "i3c_bus_deftgts: %d", rc);

	rc = i3c_device_controller_handoff(desc_a, true);
	zassert_ok(rc, "i3c_device_controller_handoff: %d", rc);

	zassert_equal(atomic_get(&g.handoff), 1, "controller_handoff_cb fired");

	/*
	 * Wait for the IBI workqueue to run i3c_sec_handoffed and re-attach
	 * descs from DEFTGTS. Poll the bus list for the first desc whose
	 * controller_priv is non-NULL — that's the observable condition,
	 * no arbitrary sleep needed.
	 */
	bool any_relinked = false;

	zassert_true(WAIT_FOR(({
		any_relinked = false;
		I3C_BUS_FOR_EACH_I3CDEV(bus, desc) {
			if (desc->controller_priv != NULL) {
				any_relinked = true;
				break;
			}
		}
		any_relinked;
	}), USEC_PER_MSEC * 100, k_msleep(1)),
		     "i3c_sec_handoffed did not re-attach within 100ms");

	n_attached = 0;
	I3C_BUS_FOR_EACH_I3CDEV(bus, desc) {
		n_attached++;
		zassert_not_null((void *)desc->controller_priv,
				 "desc 0x%02x relinked after handoff", desc->dynamic_addr);
	}
	zassert_true(n_attached >= 1,
		     "expected at least one desc re-attached from DEFTGTS, got %d", n_attached);

	(void)i3c_target_unregister(bus, &tcfg);
#endif
}

ZTEST_SUITE(i3c_emul_controller_handoff, NULL, handoff_setup, handoff_before, NULL, NULL);
