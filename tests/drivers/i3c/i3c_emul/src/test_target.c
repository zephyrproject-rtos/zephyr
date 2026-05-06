/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Target-mode tests for the I3C emulator: register, basic write/read
 * callback invocation, target TX FIFO. Controller-handoff scenarios
 * live in test_controller_handoff.c.
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
/*
 * Must match primary-controller-da in boards/native_sim.overlay — that's
 * the address the bus controller answers at in target mode, so xfers
 * from a shadow desc need to go there to land on our registered tcfg.
 */
#define TARGET_MODE_ADDR 0x55

static const struct device *bus = DEVICE_DT_GET(I3C_BUS);

static struct {
	atomic_t write_requested;
	atomic_t write_received_total;
	atomic_t read_requested;
	atomic_t read_processed;
	atomic_t stop;
	uint8_t last_write[16];
	uint8_t last_write_idx;
	uint8_t read_seq;
} g;

static int cb_write_requested(struct i3c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	atomic_inc(&g.write_requested);
	g.last_write_idx = 0;
	return 0;
}

static int cb_write_received(struct i3c_target_config *cfg, uint8_t val)
{
	ARG_UNUSED(cfg);
	atomic_inc(&g.write_received_total);
	if (g.last_write_idx < sizeof(g.last_write)) {
		g.last_write[g.last_write_idx++] = val;
	}
	return 0;
}

static int cb_read_requested(struct i3c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);
	atomic_inc(&g.read_requested);
	*val = g.read_seq++;
	return 0;
}

static int cb_read_processed(struct i3c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);
	atomic_inc(&g.read_processed);
	*val = g.read_seq++;
	return 0;
}

static int cb_stop(struct i3c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	atomic_inc(&g.stop);
	return 0;
}

static const struct i3c_target_callbacks tcb = {
	.write_requested_cb = cb_write_requested,
	.write_received_cb = cb_write_received,
	.read_requested_cb = cb_read_requested,
	.read_processed_cb = cb_read_processed,
	.stop_cb = cb_stop,
};

static struct i3c_target_config tcfg = {
	.callbacks = &tcb,
};

static struct i3c_device_desc shadow_desc;

static void *target_setup(void)
{
	int rc = test_target_bus_known_state(bus, TEST_TARGET_A_PID, TEST_TARGET_A_STATIC,
					     TEST_TARGET_B_PID, TEST_TARGET_B_INIT_DA);

	zassert_ok(rc, "test_target_bus_known_state: %d", rc);

	shadow_desc.bus = bus;
	shadow_desc.dynamic_addr = TARGET_MODE_ADDR;
	return NULL;
}

static void target_before(void *fixture)
{
	ARG_UNUSED(fixture);
	atomic_set(&g.write_requested, 0);
	atomic_set(&g.write_received_total, 0);
	atomic_set(&g.read_requested, 0);
	atomic_set(&g.read_processed, 0);
	atomic_set(&g.stop, 0);
	g.last_write_idx = 0;
	g.read_seq = 0xA0;
	memset(g.last_write, 0, sizeof(g.last_write));
}

ZTEST(i3c_emul_target, test_register_then_xfer_invokes_callbacks)
{
	uint8_t out[3] = {0xAA, 0xBB, 0xCC};
	uint8_t in[2] = {0};
	struct i3c_msg msgs[2] = {
		{ .buf = out, .len = sizeof(out), .flags = I3C_MSG_WRITE | I3C_MSG_STOP },
		{ .buf = in, .len = sizeof(in), .flags = I3C_MSG_READ | I3C_MSG_STOP },
	};
	int rc;

	rc = i3c_target_register(bus, &tcfg);
	zassert_ok(rc, "target_register: %d", rc);

	rc = i3c_transfer(&shadow_desc, &msgs[0], 1);
	zassert_ok(rc, "write transfer: %d", rc);
	zassert_equal(atomic_get(&g.write_requested), 1, "write_requested fired");
	zassert_equal(atomic_get(&g.write_received_total), 3, "write_received per byte");
	zassert_mem_equal(g.last_write, out, sizeof(out), "bytes match");
	zassert_equal(atomic_get(&g.stop), 1, "stop fired");

	rc = i3c_transfer(&shadow_desc, &msgs[1], 1);
	zassert_ok(rc, "read transfer: %d", rc);
	zassert_equal(atomic_get(&g.read_requested), 1, "read_requested fired");
	zassert_equal(atomic_get(&g.read_processed), 1, "read_processed fired");
	zassert_equal(in[0], 0xA0, "first byte from cb");
	zassert_equal(in[1], 0xA1, "second byte from cb");

	rc = i3c_target_unregister(bus, &tcfg);
	zassert_ok(rc, "target_unregister: %d", rc);
}

ZTEST(i3c_emul_target, test_target_tx_write_fifo)
{
	uint8_t pushed[4] = {0x10, 0x20, 0x30, 0x40};
	uint8_t in[4] = {0};
	struct i3c_msg msg = {
		.buf = in, .len = sizeof(in), .flags = I3C_MSG_READ | I3C_MSG_STOP,
	};
	int rc;

	rc = i3c_target_register(bus, &tcfg);
	zassert_ok(rc, "target_register: %d", rc);

	rc = i3c_target_tx_write(bus, pushed, sizeof(pushed), 0);
	zassert_equal(rc, sizeof(pushed), "tx_write returned %d", rc);

	rc = i3c_transfer(&shadow_desc, &msg, 1);
	zassert_ok(rc, "read transfer: %d", rc);
	zassert_mem_equal(in, pushed, sizeof(pushed), "FIFO bytes consumed");
	zassert_equal(atomic_get(&g.read_requested), 0,
		      "read_requested should not fire when FIFO satisfies the whole msg");

	(void)i3c_target_unregister(bus, &tcfg);
}

ZTEST_SUITE(i3c_emul_target, NULL, target_setup, target_before, NULL, NULL);
