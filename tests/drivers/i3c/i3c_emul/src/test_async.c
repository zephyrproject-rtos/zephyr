/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * M4 async + RTIO tests for the I3C emulator.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/drivers/i3c_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "test_target_emul.h"

#define I3C_BUS DT_NODELABEL(i3c0)
#define TARGET_A DT_NODELABEL(test_target_a)

#define TARGET_A_PID	TEST_TARGET_A_PID

static const struct device *bus = DEVICE_DT_GET(I3C_BUS);

static struct i3c_device_desc *find_desc(uint64_t pid)
{
	struct i3c_device_id id = { .pid = pid };

	return i3c_device_find(bus, &id);
}

static void *i3c_emul_async_setup(void)
{
	int rc = test_target_bus_known_state(bus, TEST_TARGET_A_PID, TEST_TARGET_A_STATIC,
					     TEST_TARGET_B_PID, TEST_TARGET_B_INIT_DA);

	zassert_ok(rc, "test_target_bus_known_state: %d", rc);
	return NULL;
}

#ifdef CONFIG_I3C_CALLBACK
static struct {
	struct k_sem sem;
	int result;
	void *userdata;
	const struct device *dev;
} g_cb;

static void async_cb(const struct device *dev, int result, void *userdata)
{
	g_cb.dev = dev;
	g_cb.result = result;
	g_cb.userdata = userdata;
	k_sem_give(&g_cb.sem);
}

ZTEST(i3c_emul_async, test_xfers_cb_fires_on_completion)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);
	uint8_t buf[3] = {0x00, 0xAB, 0xCD};
	struct i3c_msg msg = {
		.buf = buf,
		.len = sizeof(buf),
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	uint32_t marker = 0xDEADBEEFU;
	int rc;

	k_sem_init(&g_cb.sem, 0, 1);

	rc = i3c_transfer_cb(desc, &msg, 1, async_cb, &marker);
	zassert_ok(rc, "i3c_transfer_cb: %d", rc);
	zassert_ok(k_sem_take(&g_cb.sem, K_MSEC(100)), "callback semaphore timeout");
	zassert_equal(g_cb.result, 0, "callback result: %d", g_cb.result);
	zassert_equal(g_cb.userdata, &marker, "userdata propagated");
	zassert_equal(g_cb.dev, bus, "callback dev matches bus");

	zassert_equal(test_target_get_reg(EMUL_DT_GET(TARGET_A), 0), buf[1], "reg[0]");
	zassert_equal(test_target_get_reg(EMUL_DT_GET(TARGET_A), 1), buf[2], "reg[1]");
}

ZTEST(i3c_emul_async, test_do_ccc_cb_fires_on_completion)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);
	struct i3c_ccc_target_payload tp = {0};
	uint8_t pid_buf[6] = {0};
	struct i3c_ccc_payload payload = {
		.ccc = { .id = I3C_CCC_GETPID },
		.targets = { .payloads = &tp, .num_targets = 1 },
	};
	int rc;

	tp.addr = desc->dynamic_addr;
	tp.rnw = 1;
	tp.data = pid_buf;
	tp.data_len = sizeof(pid_buf);

	k_sem_init(&g_cb.sem, 0, 1);

	rc = i3c_do_ccc_cb(bus, &payload, async_cb, NULL);
	zassert_ok(rc, "i3c_do_ccc_cb: %d", rc);
	zassert_ok(k_sem_take(&g_cb.sem, K_MSEC(100)), "callback timeout");
	zassert_equal(g_cb.result, 0, "callback result: %d", g_cb.result);
	zassert_equal(sys_get_be48(pid_buf), TARGET_A_PID, "PID mismatch");
}
#endif /* CONFIG_I3C_CALLBACK */

#ifdef CONFIG_I3C_RTIO
I3C_DT_IODEV_DEFINE(rtio_iodev, DT_NODELABEL(test_target_a));
RTIO_DEFINE(rtio_ctx, 4, 4);

ZTEST(i3c_emul_async, test_rtio_iodev_submit_round_trip)
{
	uint8_t write_buf[3] = {0x00, 0x55, 0x66};
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	int rc;

	sqe = rtio_sqe_acquire(&rtio_ctx);
	zassert_not_null(sqe, "sqe alloc");
	rtio_sqe_prep_write(sqe, &rtio_iodev, RTIO_PRIO_NORM, write_buf, sizeof(write_buf), NULL);
	sqe->iodev_flags = RTIO_IODEV_I3C_STOP;

	rc = rtio_submit(&rtio_ctx, 1);
	zassert_ok(rc, "rtio_submit: %d", rc);

	cqe = rtio_cqe_consume_block(&rtio_ctx);
	zassert_not_null(cqe, "cqe");
	zassert_ok(cqe->result, "cqe result: %d", cqe->result);
	rtio_cqe_release(&rtio_ctx, cqe);

	zassert_equal(test_target_get_reg(EMUL_DT_GET(TARGET_A), 0), write_buf[1], "reg[0]");
	zassert_equal(test_target_get_reg(EMUL_DT_GET(TARGET_A), 1), write_buf[2], "reg[1]");
}
#endif /* CONFIG_I3C_RTIO */

ZTEST_SUITE(i3c_emul_async, NULL, i3c_emul_async_setup, NULL, NULL, NULL);
