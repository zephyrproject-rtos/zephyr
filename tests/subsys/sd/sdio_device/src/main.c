/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sd/sdio_device.h>
#include <zephyr/drivers/sdio_dc.h>
#include <zephyr/drivers/sdio_dc_virtual.h>

#define DC_NODE DT_NODELABEL(sdio_dc0)

/* Function 1: register window. */
static uint8_t func1_regs[256];

/* Function 2: FIFO/data port backed by a software buffer. */
static uint8_t fifo_store[512];
static uint32_t fifo_last_len;
static enum sdio_io_dir fifo_last_dir;

static int fifo_cb(struct sdio_device_function *func, enum sdio_io_dir dir,
		   uint8_t *data, uint32_t len, void *user)
{
	ARG_UNUSED(func);
	ARG_UNUSED(user);

	if (len > sizeof(fifo_store)) {
		return -EIO;
	}

	fifo_last_dir = dir;
	fifo_last_len = len;

	if (dir == SDIO_IO_WRITE) {
		memcpy(fifo_store, data, len);
	} else {
		memcpy(data, fifo_store, len);
	}

	return 0;
}

static struct sdio_device endpoint;
static struct sdio_device_function f_reg = {
	.num = SDIO_FUNC_NUM_1,
	.regs = func1_regs,
	.regs_size = sizeof(func1_regs),
};
static struct sdio_device_function f_fifo = {
	.num = SDIO_FUNC_NUM_2,
	.fifo_reg = 0,
	.fifo_cb = fifo_cb,
};

/* Host-side interrupt bookkeeping. */
static volatile uint32_t irq_count;
static volatile enum sdio_func_num irq_func;

static void irq_cb(const struct device *dc, enum sdio_func_num func, void *user)
{
	ARG_UNUSED(dc);
	ARG_UNUSED(user);

	irq_func = func;
	irq_count++;
}

/* Simulate a host CMD52/CMD53 hitting the controller. */
static int host_access(enum sdio_func_num func, enum sdio_dc_dir dir,
		       uint32_t reg, bool increment, uint8_t *data, uint32_t len)
{
	const struct device *dc = DEVICE_DT_GET(DC_NODE);
	struct sdio_dc_xfer xfer = {
		.func = func,
		.dir = dir,
		.reg = reg,
		.increment = increment,
		.data = data,
		.len = len,
	};

	return sdio_dc_virtual_access(dc, &xfer);
}

static void *setup(void)
{
	const struct device *dc = DEVICE_DT_GET(DC_NODE);

	zassert_true(device_is_ready(dc), "controller not ready");
	zassert_ok(sdio_device_init(&endpoint, dc));
	zassert_ok(sdio_device_register_function(&endpoint, &f_reg));
	zassert_ok(sdio_device_register_function(&endpoint, &f_fifo));
	zassert_ok(sdio_device_enable(&endpoint));
	sdio_dc_virtual_set_irq_cb(dc, irq_cb, NULL);

	return NULL;
}

/* A single-byte direct (CMD52) access lands in the register window. */
ZTEST(sdio_device, test_register_byte)
{
	uint8_t val = 0xA5;

	zassert_ok(host_access(SDIO_FUNC_NUM_1, SDIO_DC_DIR_WRITE, 0x10, true,
			       &val, 1));
	zassert_equal(func1_regs[0x10], 0xA5, "write not stored");

	val = 0;
	zassert_ok(host_access(SDIO_FUNC_NUM_1, SDIO_DC_DIR_READ, 0x10, true,
			       &val, 1));
	zassert_equal(val, 0xA5, "read-back mismatch");
}

/* An incrementing-address (CMD53) access walks the register window. */
ZTEST(sdio_device, test_register_window)
{
	uint8_t tx[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	uint8_t rx[8] = {0};

	zassert_ok(host_access(SDIO_FUNC_NUM_1, SDIO_DC_DIR_WRITE, 0x20, true,
			       tx, sizeof(tx)));
	zassert_mem_equal(&func1_regs[0x20], tx, sizeof(tx), "window write");

	zassert_ok(host_access(SDIO_FUNC_NUM_1, SDIO_DC_DIR_READ, 0x20, true,
			       rx, sizeof(rx)));
	zassert_mem_equal(rx, tx, sizeof(tx), "window read-back");
}

/* A fixed-address (FIFO) access is routed to the function handler. */
ZTEST(sdio_device, test_fifo)
{
	uint8_t tx[16];
	uint8_t rx[16];

	for (int i = 0; i < (int)sizeof(tx); i++) {
		tx[i] = (uint8_t)(i * 2 + 1);
	}

	zassert_ok(host_access(SDIO_FUNC_NUM_2, SDIO_DC_DIR_WRITE, 0, false,
			       tx, sizeof(tx)));
	zassert_equal(fifo_last_dir, SDIO_IO_WRITE, "wrong direction");
	zassert_equal(fifo_last_len, sizeof(tx), "wrong length");
	zassert_mem_equal(fifo_store, tx, sizeof(tx), "fifo write");

	memset(rx, 0, sizeof(rx));
	zassert_ok(host_access(SDIO_FUNC_NUM_2, SDIO_DC_DIR_READ, 0, false,
			       rx, sizeof(rx)));
	zassert_equal(fifo_last_dir, SDIO_IO_READ, "wrong direction");
	zassert_mem_equal(rx, tx, sizeof(tx), "fifo read-back");
}

/* Accessing an unregistered function is reported as an error. */
ZTEST(sdio_device, test_unknown_function)
{
	uint8_t val = 0;

	zassert_not_equal(host_access(SDIO_FUNC_NUM_5, SDIO_DC_DIR_READ, 0,
				      true, &val, 1),
			  0, "unknown function should fail");
}

/* Raising a device interrupt reaches the host-side callback. */
ZTEST(sdio_device, test_interrupt)
{
	uint32_t before = irq_count;

	zassert_ok(sdio_device_raise_interrupt(&f_reg));
	zassert_equal(irq_count, before + 1, "interrupt not delivered");
	zassert_equal(irq_func, SDIO_FUNC_NUM_1, "wrong function");
}

ZTEST_SUITE(sdio_device, NULL, setup, NULL, NULL, NULL);
