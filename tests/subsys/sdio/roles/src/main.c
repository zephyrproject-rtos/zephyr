/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Dual-role SDIO subsystem test. A device/slave endpoint exposes three SDIO
 * functions (a register window, a second register window and a FIFO/data
 * port) on the virtual SDIO device controller. A host/master endpoint binds
 * to the same controller through its in-process loopback transport and drives
 * the full host -> bus -> device path: register access, fixed-address FIFO
 * access, incrementing-address windows, block-mode transfers and interrupts.
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sdio/sdio_core.h>
#include <zephyr/sdio/sdio_device.h>
#include <zephyr/drivers/sdio_dc.h>
#include <zephyr/drivers/sdio_dc_virtual.h>

#define DC_NODE DT_NODELABEL(sdio_dc0)

/* --- Device/slave side ---------------------------------------------------- */

static uint8_t func0_regs[0x400];
static uint8_t func1_regs[256];
static uint8_t func2_fifo_store[512];
static uint32_t func2_fifo_last_len;

static int func2_fifo_handler(struct sdio_device_function *func,
			      enum sdio_io_dir dir, uint8_t *data,
			      uint32_t len, void *user)
{
	ARG_UNUSED(func);
	ARG_UNUSED(user);

	if (len > sizeof(func2_fifo_store)) {
		return -EIO;
	}
	func2_fifo_last_len = len;
	if (dir == SDIO_IO_WRITE) {
		memcpy(func2_fifo_store, data, len);
	} else {
		memcpy(data, func2_fifo_store, len);
	}
	return 0;
}

static struct sdio_device dev_endpoint;
static struct sdio_device_function dev_func0 = {
	.num = SDIO_FUNC_NUM_0,
	.regs = func0_regs,
	.regs_size = sizeof(func0_regs),
};
static struct sdio_device_function dev_func1 = {
	.num = SDIO_FUNC_NUM_1,
	.regs = func1_regs,
	.regs_size = sizeof(func1_regs),
};
static struct sdio_device_function dev_func2 = {
	.num = SDIO_FUNC_NUM_2,
	.fifo_reg = 0x00,
	.fifo_cb = func2_fifo_handler,
};

/* --- Host/master side ----------------------------------------------------- */

static struct sdio_dev host;
static struct sdio_function host_func1;
static struct sdio_function host_func2;

/* --- Interrupt plumbing --------------------------------------------------- */

static volatile enum sdio_func_num irq_func;
static volatile uint32_t irq_count;

static void host_irq_cb(const struct device *dc, enum sdio_func_num func,
			void *user)
{
	ARG_UNUSED(dc);
	ARG_UNUSED(user);

	irq_func = func;
	irq_count++;
}

static void *sdio_roles_setup(void)
{
	const struct device *dc = DEVICE_DT_GET(DC_NODE);
	int ret;

	zassert_true(device_is_ready(dc), "virtual SDIO controller not ready");

	/* Device/slave side: register functions and start answering */
	ret = sdio_device_init(&dev_endpoint, dc);
	zassert_ok(ret, "sdio_device_init failed: %d", ret);
	zassert_ok(sdio_device_register_function(&dev_endpoint, &dev_func0));
	zassert_ok(sdio_device_register_function(&dev_endpoint, &dev_func1));
	zassert_ok(sdio_device_register_function(&dev_endpoint, &dev_func2));
	zassert_ok(sdio_device_enable(&dev_endpoint));

	/* Host/master side: bind to the controller's loopback transport */
	ret = sdio_dev_init(&host, SDIO_ROLE_HOST,
			    sdio_dc_virtual_loopback_api(),
			    sdio_dc_virtual_loopback_ctx(dc),
			    SDIO_CAP_MULTIBLOCK);
	zassert_ok(ret, "sdio_dev_init (host) failed: %d", ret);
	host.max_blk_size = 512;

	zassert_ok(sdio_func_bind(&host, &host_func1, SDIO_FUNC_NUM_1));
	zassert_ok(sdio_func_bind(&host, &host_func2, SDIO_FUNC_NUM_2));

	sdio_dc_virtual_set_irq_cb(dc, host_irq_cb, NULL);
	return NULL;
}

ZTEST(sdio_roles, test_caps)
{
	const struct device *dc = DEVICE_DT_GET(DC_NODE);
	struct sdio_dc_caps caps = {0};

	zassert_ok(sdio_dc_get_caps(dc, &caps));
	zassert_equal(caps.num_funcs, 3, "unexpected num_funcs %d",
		      caps.num_funcs);
	zassert_equal(caps.max_blk_size, 512);
	zassert_true(caps.interrupt_supported);
}

ZTEST(sdio_roles, test_register_byte_access)
{
	uint8_t val = 0;

	/* Host writes a register, device backing store must observe it */
	zassert_ok(sdio_func_write_reg(&host_func1, 0x10, 0xA5));
	zassert_equal(func1_regs[0x10], 0xA5, "device did not see write");

	/* Host reads it back through the bus */
	zassert_ok(sdio_func_read_reg(&host_func1, 0x10, &val));
	zassert_equal(val, 0xA5, "read-back mismatch: 0x%02x", val);

	/* Write-then-read (CMD52 RAW) */
	val = 0;
	zassert_ok(sdio_func_rw_reg(&host_func1, 0x10, 0x5A, &val));
	zassert_equal(val, 0x5A, "RAW read-back mismatch: 0x%02x", val);
	zassert_equal(func1_regs[0x10], 0x5A);
}

ZTEST(sdio_roles, test_incrementing_window)
{
	uint8_t tx[16];
	uint8_t rx[16];

	for (int i = 0; i < (int)sizeof(tx); i++) {
		tx[i] = (uint8_t)(0xC0 + i);
	}

	zassert_ok(sdio_func_write_addr(&host_func1, 0x20, tx, sizeof(tx)));
	zassert_mem_equal(&func1_regs[0x20], tx, sizeof(tx),
			  "incrementing write landed wrong");

	memset(rx, 0, sizeof(rx));
	zassert_ok(sdio_func_read_addr(&host_func1, 0x20, rx, sizeof(rx)));
	zassert_mem_equal(rx, tx, sizeof(tx), "incrementing read-back mismatch");
}

ZTEST(sdio_roles, test_set_block_size)
{
	/* Setting the block size issues CMD52 writes to func0 FBR registers */
	zassert_ok(sdio_func_set_block_size(&host_func2, 64));
	zassert_equal(host_func2.block_size, 64);

	/* func2 FBR block-size registers live at SDIO_FBR_BASE(2)+0x10 */
	uint32_t base = SDIO_FBR_BASE(SDIO_FUNC_NUM_2) + SDIO_FBR_BLK_SIZE;

	zassert_equal(func0_regs[base], 64, "FBR low byte wrong");
	zassert_equal(func0_regs[base + 1], 0, "FBR high byte wrong");
}

ZTEST(sdio_roles, test_fixed_address_fifo)
{
	uint8_t tx[32];
	uint8_t rx[32];

	for (int i = 0; i < (int)sizeof(tx); i++) {
		tx[i] = (uint8_t)(i * 3 + 1);
	}

	/* Byte-mode fixed-address (FIFO) write/read */
	zassert_ok(sdio_func_write_fifo(&host_func2, 0x00, tx, sizeof(tx)));
	zassert_equal(func2_fifo_last_len, sizeof(tx), "fifo write len wrong");
	zassert_mem_equal(func2_fifo_store, tx, sizeof(tx), "fifo data wrong");

	memset(rx, 0, sizeof(rx));
	zassert_ok(sdio_func_read_fifo(&host_func2, 0x00, rx, sizeof(rx)));
	zassert_mem_equal(rx, tx, sizeof(tx), "fifo read-back mismatch");
}

ZTEST(sdio_roles, test_block_mode_fifo)
{
	uint8_t tx[128];
	uint8_t rx[128];

	host_func2.block_size = 64;
	for (int i = 0; i < (int)sizeof(tx); i++) {
		tx[i] = (uint8_t)(0xFF - i);
	}

	/* 2 blocks of 64 bytes -> a single 128-byte fixed-address transfer */
	zassert_ok(sdio_func_write_blocks_fifo(&host_func2, 0x00, tx, 2));
	zassert_equal(func2_fifo_last_len, 128, "block write len wrong");
	zassert_mem_equal(func2_fifo_store, tx, sizeof(tx), "block data wrong");

	memset(rx, 0, sizeof(rx));
	zassert_ok(sdio_func_read_blocks_fifo(&host_func2, 0x00, rx, 2));
	zassert_mem_equal(rx, tx, sizeof(tx), "block read-back mismatch");
}

ZTEST(sdio_roles, test_device_interrupt)
{
	uint32_t before = irq_count;

	zassert_ok(sdio_device_raise_interrupt(&dev_func1));
	zassert_equal(irq_count, before + 1, "host did not receive interrupt");
	zassert_equal(irq_func, SDIO_FUNC_NUM_1, "wrong interrupt source");
}

ZTEST_SUITE(sdio_roles, NULL, sdio_roles_setup, NULL, NULL, NULL);
