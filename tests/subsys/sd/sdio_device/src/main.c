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

/* Function-0 identity served by the subsystem. */
#define TEST_MANF_ID    0x1234
#define TEST_MANF_CODE  0x5678
#define TEST_FUNC0_ID   0x0C
#define TEST_F0_BLKSIZE 256
#define TEST_F1_BLKSIZE 512
#define TEST_F1_RDYTO   0x00A0

static const struct sdio_device_config dev_config = {
	.cccr_revision = SDIO_CCCR_CCCR_REV_3_00,
	.sd_spec = 0x03,
	.manf_id = TEST_MANF_ID,
	.manf_code = TEST_MANF_CODE,
	.func0_id = TEST_FUNC0_ID,
	.max_blk_size = TEST_F0_BLKSIZE,
	.max_speed = 0x32,
	.caps = SDIO_CCCR_CAPS_SMB | SDIO_CCCR_CAPS_SDC,
};

static struct sdio_device endpoint;
static struct sdio_device_function f_reg = {
	.num = SDIO_FUNC_NUM_1,
	.regs = func1_regs,
	.regs_size = sizeof(func1_regs),
	.func_code = 0x07,
	.max_blk_size = TEST_F1_BLKSIZE,
	.rdy_timeout = TEST_F1_RDYTO,
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

/* Read a single function-0 register byte. */
static int host_read(uint32_t reg, uint8_t *val)
{
	return host_access(SDIO_FUNC_NUM_0, SDIO_DC_DIR_READ, reg, true, val, 1);
}

static void *setup(void)
{
	const struct device *dc = DEVICE_DT_GET(DC_NODE);

	zassert_true(device_is_ready(dc), "controller not ready");
	zassert_ok(sdio_device_init(&endpoint, dc, &dev_config));
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

/* Raising a device interrupt reaches the host-side callback and is reflected
 * in the CCCR interrupt-pending register until cleared.
 */
ZTEST(sdio_device, test_interrupt)
{
	uint32_t before = irq_count;
	uint8_t intp = 0;

	zassert_ok(sdio_device_raise_interrupt(&f_reg));
	zassert_equal(irq_count, before + 1, "interrupt not delivered");
	zassert_equal(irq_func, SDIO_FUNC_NUM_1, "wrong function");

	zassert_ok(host_read(SDIO_CCCR_INT_P, &intp));
	zassert_true(intp & BIT(SDIO_FUNC_NUM_1), "INT_P bit not set");

	zassert_ok(sdio_device_clear_interrupt(&f_reg));
	zassert_ok(host_read(SDIO_CCCR_INT_P, &intp));
	zassert_false(intp & BIT(SDIO_FUNC_NUM_1), "INT_P bit not cleared");
}

/* The subsystem serves the CCCR from the supplied configuration and tracks
 * enable/bus state written by the host.
 */
ZTEST(sdio_device, test_cccr)
{
	uint8_t v = 0;

	zassert_ok(host_read(SDIO_CCCR_CCCR, &v));
	zassert_equal(v & SDIO_CCCR_CCCR_REV_MASK, SDIO_CCCR_CCCR_REV_3_00);
	zassert_ok(host_read(SDIO_CCCR_SD, &v));
	zassert_equal(v & SDIO_CCCR_SD_SPEC_MASK, 0x03);
	zassert_ok(host_read(SDIO_CCCR_CAPS, &v));
	zassert_equal(v, SDIO_CCCR_CAPS_SMB | SDIO_CCCR_CAPS_SDC);

	/* CIS pointer of function 0. */
	uint8_t p0, p1, p2;

	zassert_ok(host_read(SDIO_CCCR_CIS, &p0));
	zassert_ok(host_read(SDIO_CCCR_CIS + 1, &p1));
	zassert_ok(host_read(SDIO_CCCR_CIS + 2, &p2));
	zassert_equal((uint32_t)p0 | (p1 << 8) | (p2 << 16), 0x1000);

	/* Enable function 1 via IO_EN; it must report ready in IO_RD. */
	v = BIT(SDIO_FUNC_NUM_1);
	zassert_ok(host_access(SDIO_FUNC_NUM_0, SDIO_DC_DIR_WRITE,
			       SDIO_CCCR_IO_EN, true, &v, 1));
	zassert_ok(host_read(SDIO_CCCR_IO_RD, &v));
	zassert_true(v & BIT(SDIO_FUNC_NUM_1), "function not ready");
}

/* FBR of a function is served from its config; block size is host-writable. */
ZTEST(sdio_device, test_fbr)
{
	uint32_t base = SDIO_FBR_BASE(SDIO_FUNC_NUM_1);
	uint8_t v, lo, hi;

	zassert_ok(host_read(base + 0x00, &v));
	zassert_equal(v & 0x0F, 0x07, "wrong function interface code");

	/* CIS pointer of function 1 = 0x1000 + 1 * 0x100. */
	zassert_ok(host_read(base + SDIO_FBR_CIS, &lo));
	zassert_ok(host_read(base + SDIO_FBR_CIS + 1, &hi));
	zassert_equal((uint32_t)lo | (hi << 8), 0x1100);

	/* Program the block size and read it back. */
	lo = 0x00;
	hi = 0x02; /* 512 */
	zassert_ok(host_access(SDIO_FUNC_NUM_0, SDIO_DC_DIR_WRITE,
			       base + SDIO_FBR_BLK_SIZE, true, &lo, 1));
	zassert_ok(host_access(SDIO_FUNC_NUM_0, SDIO_DC_DIR_WRITE,
			       base + SDIO_FBR_BLK_SIZE + 1, true, &hi, 1));
	zassert_ok(host_read(base + SDIO_FBR_BLK_SIZE, &v));
	zassert_equal(v, 0x00);
	zassert_ok(host_read(base + SDIO_FBR_BLK_SIZE + 1, &v));
	zassert_equal(v, 0x02);
}

/* The subsystem generates a spec-shaped CIS tuple chain from config. */
ZTEST(sdio_device, test_cis)
{
	uint8_t cis[24];

	/* Function-0 chain at 0x1000: MANFID, FUNCID, FUNCE, END. */
	zassert_ok(host_access(SDIO_FUNC_NUM_0, SDIO_DC_DIR_READ, 0x1000, true,
			       cis, sizeof(cis)));
	zassert_equal(cis[0], SDIO_TPL_CODE_MANIFID, "no MANFID tuple");
	zassert_equal(cis[1], 0x04, "wrong MANFID link");
	zassert_equal((uint16_t)cis[2] | (cis[3] << 8), TEST_MANF_ID);
	zassert_equal((uint16_t)cis[4] | (cis[5] << 8), TEST_MANF_CODE);
	zassert_equal(cis[6], SDIO_TPL_CODE_FUNCID, "no FUNCID tuple");
	zassert_equal(cis[8], TEST_FUNC0_ID, "wrong function id");
	zassert_equal(cis[10], SDIO_TPL_CODE_FUNCE, "no FUNCE tuple");
	zassert_equal((uint16_t)cis[13] | (cis[14] << 8), TEST_F0_BLKSIZE);

	/* Function-1 FUNCE carries the function's max block size at body+12. */
	uint8_t f1[48];

	zassert_ok(host_access(SDIO_FUNC_NUM_0, SDIO_DC_DIR_READ, 0x1100, true,
			       f1, sizeof(f1)));
	zassert_equal(f1[0], SDIO_TPL_CODE_FUNCID, "no FUNCID tuple");
	zassert_equal(f1[4], SDIO_TPL_CODE_FUNCE, "no FUNCE tuple");
	/* FUNCE body starts at f1[6]; max block size at body offset 12. */
	zassert_equal((uint16_t)f1[6 + 12] | (f1[6 + 13] << 8), TEST_F1_BLKSIZE);
}

ZTEST_SUITE(sdio_device, NULL, setup, NULL, NULL, NULL);
