/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * I3C controller — legacy I2C device test suite.
 *
 * Tests an I3C controller communicating with legacy I2C devices on the
 * I3C bus.  The board overlay provides:
 *
 *   test-i3c        : the I3C controller under test
 *   test-i2c-target : an I2C controller wired in target role on the I3C
 *                     bus, used as the loopback target
 *   test-i2c-target2: (optional) a second I2C target on the same bus,
 *                     used by the "multiple targets" test
 *
 * Static I2C device addresses are taken from the I3C controller's child
 * `test_i2c_dev*` nodes (reg property index 0).
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i2c.h>
#include <string.h>

/* I3C controller under test */
#define I3C_NODE DT_ALIAS(test_i3c)

/* Primary I2C target (loopback device on the I3C bus) */
#define I2C_TARGET_NODE DT_ALIAS(test_i2c_target)

/* Optional second I2C target — only instantiated when the alias resolves
 * to an enabled device tree node.
 */
#define I2C_TARGET2_NODE DT_ALIAS(test_i2c_target2)
#define HAS_I2C_TARGET2  DT_NODE_HAS_STATUS(I2C_TARGET2_NODE, okay)

/* Static I2C addresses, taken from the I3C child nodes in the overlay. */
#define TEST_I2C_ADDR DT_PROP_BY_IDX(DT_NODELABEL(test_i2c_dev), reg, 0)
#if HAS_I2C_TARGET2
#define TEST_I2C_ADDR2 DT_PROP_BY_IDX(DT_NODELABEL(test_i2c_dev2), reg, 0)
#endif

static const struct device *i3c_dev = DEVICE_DT_GET(I3C_NODE);
static const struct device *i2c_target_dev = DEVICE_DT_GET(I2C_TARGET_NODE);
#if HAS_I2C_TARGET2
static const struct device *i2c_target2_dev = DEVICE_DT_GET(I2C_TARGET2_NODE);
#endif

/* ---------------------------------------------------------------------------
 * I2C target state used by the loopback target.
 *
 * Tracks received bytes and stop callbacks for write data-integrity tests.
 * Callbacks may execute in interrupt context — keep them short, no printk,
 * no blocking ops.
 * ---------------------------------------------------------------------------
 */
#define TGT_RX_BUF_MAX 16

static struct {
	/* Write-side state (populated by write_received) */
	uint8_t rx_buf[TGT_RX_BUF_MAX]; /* bytes received, in order           */
	int rx_count;                   /* total bytes received               */
	/* Read-side state (consumed by read_requested / read_processed) */
	uint8_t read_buf[TGT_RX_BUF_MAX]; /* bytes to serve on reads            */
	int read_idx;                     /* next byte index to serve           */
	/* Event counters */
	int stop_count; /* stop callbacks since last reset    */
} tgt_state;

#if HAS_I2C_TARGET2
/* Separate state for the second target. */
static struct {
	uint8_t rx_buf[TGT_RX_BUF_MAX];
	int rx_count;
	uint8_t read_buf[TGT_RX_BUF_MAX];
	int read_idx;
	int stop_count;
} tgt2_state;
#endif

static int tgt_write_requested(struct i2c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	return 0;
}

static int tgt_write_received(struct i2c_target_config *cfg, uint8_t val)
{
	ARG_UNUSED(cfg);
	if (tgt_state.rx_count < TGT_RX_BUF_MAX) {
		tgt_state.rx_buf[tgt_state.rx_count] = val;
	}
	tgt_state.rx_count++;
	return 0;
}

static int tgt_read_requested(struct i2c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);
	/* Register-addressing model for write-then-read:
	 * if a write phase preceded the read in the same transfer, rx_buf[0]
	 * holds the register address.  Use it to seek read_buf to the right
	 * offset.  For pure reads (rx_count == 0) read_idx stays at 0.
	 */
	if (tgt_state.rx_count > 0 && tgt_state.read_idx == 0) {
		uint8_t reg = tgt_state.rx_buf[0];

		tgt_state.read_idx = (reg < TGT_RX_BUF_MAX) ? reg : 0;
	}
	*val = (tgt_state.read_idx < TGT_RX_BUF_MAX) ? tgt_state.read_buf[tgt_state.read_idx++] : 0;
	return 0;
}

static int tgt_read_processed(struct i2c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);
	*val = (tgt_state.read_idx < TGT_RX_BUF_MAX) ? tgt_state.read_buf[tgt_state.read_idx++] : 0;
	return 0;
}

static int tgt_stop(struct i2c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	tgt_state.stop_count++;
	return 0;
}

static const struct i2c_target_callbacks tgt_callbacks = {
	.write_requested = tgt_write_requested,
	.write_received = tgt_write_received,
	.read_requested = tgt_read_requested,
	.read_processed = tgt_read_processed,
	.stop = tgt_stop,
};

static struct i2c_target_config tgt_config = {
	.address = TEST_I2C_ADDR,
	.callbacks = &tgt_callbacks,
};

#if HAS_I2C_TARGET2
/* ---------------------------------------------------------------------------
 * Callbacks and config for the second target.  Identical logic to tgt_*,
 * but operates on tgt2_state.
 * ---------------------------------------------------------------------------
 */
static int tgt2_write_requested(struct i2c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	return 0;
}

static int tgt2_write_received(struct i2c_target_config *cfg, uint8_t val)
{
	ARG_UNUSED(cfg);
	if (tgt2_state.rx_count < TGT_RX_BUF_MAX) {
		tgt2_state.rx_buf[tgt2_state.rx_count] = val;
	}
	tgt2_state.rx_count++;
	return 0;
}

static int tgt2_read_requested(struct i2c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);
	if (tgt2_state.rx_count > 0 && tgt2_state.read_idx == 0) {
		uint8_t reg = tgt2_state.rx_buf[0];

		tgt2_state.read_idx = (reg < TGT_RX_BUF_MAX) ? reg : 0;
	}
	*val = (tgt2_state.read_idx < TGT_RX_BUF_MAX) ? tgt2_state.read_buf[tgt2_state.read_idx++]
						      : 0;
	return 0;
}

static int tgt2_read_processed(struct i2c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);
	*val = (tgt2_state.read_idx < TGT_RX_BUF_MAX) ? tgt2_state.read_buf[tgt2_state.read_idx++]
						      : 0;
	return 0;
}

static int tgt2_stop(struct i2c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	tgt2_state.stop_count++;
	return 0;
}

static const struct i2c_target_callbacks tgt2_callbacks = {
	.write_requested = tgt2_write_requested,
	.write_received = tgt2_write_received,
	.read_requested = tgt2_read_requested,
	.read_processed = tgt2_read_processed,
	.stop = tgt2_stop,
};

static struct i2c_target_config tgt2_config = {
	.address = TEST_I2C_ADDR2,
	.callbacks = &tgt2_callbacks,
};
#endif /* HAS_I2C_TARGET2 */

/*
 * Bus initialization
 *
 * Verifies that:
 *  1. The I3C controller initialises successfully (device_is_ready).
 *  2. The primary test I2C device declared in the device tree appears in
 *     the controller's attached-device list after init, confirming the
 *     subsystem called i3c_attach_i2c_device() for it and marked the
 *     address slot as occupied.
 */
ZTEST(i3c_i2c, test_bus_init)
{
	struct i3c_i2c_device_desc *desc;

	/* 1. Controller must be ready */
	zassert_true(device_is_ready(i3c_dev), "I3C controller %s not ready after init",
		     i3c_dev->name);

	/*
	 * 2. The test I2C device must appear in the attached-device list.
	 *    i3c_dev_list_i2c_addr_find() searches the live slist of attached
	 *    I2C devices, so a non-NULL return confirms the subsystem processed
	 *    the device's DTS entry and registered its address.
	 */
	desc = i3c_dev_list_i2c_addr_find(i3c_dev, TEST_I2C_ADDR);
	zassert_not_null(desc,
			 "I2C device at 0x%02x not found in attached device list "
			 "after bus init",
			 TEST_I2C_ADDR);
}

/*
 * Device attach / detach
 *
 * Verifies that:
 *  1. i3c_detach_i2c_device() removes the device from the attached-device
 *     list without error.
 *  2. A transfer attempt after detach fails with -ENODEV (the I3C subsystem
 *     rejects it before any bus access, because the device is no longer in
 *     the address-slot slist).
 *  3. i3c_attach_i2c_device() re-adds the device without error.
 *  4. A transfer attempt after re-attach succeeds end-to-end (the I2C
 *     target ACKs the write transaction on the physical bus).
 *  5. Attempting to attach a second descriptor with the same static address
 *     is rejected with an error (the address slot is already occupied).
 */
ZTEST(i3c_i2c, test_attach_detach)
{
	struct i3c_i2c_device_desc *desc;
	struct i3c_i2c_device_desc dup;
	uint8_t tx_byte = 0xA5;
	struct i2c_msg msg = {
		.buf = &tx_byte,
		.len = 1,
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
	};
	int ret;

	/* ---- 1. Detach ---- */
	desc = i3c_dev_list_i2c_addr_find(i3c_dev, TEST_I2C_ADDR);
	zassert_not_null(desc, "Device not in list before detach");

	ret = i3c_detach_i2c_device(desc);
	zassert_ok(ret, "i3c_detach_i2c_device returned %d", ret);

	zassert_is_null(i3c_dev_list_i2c_addr_find(i3c_dev, TEST_I2C_ADDR),
			"Device still in list after detach");

	/* ---- 2. Transfer after detach must fail with -ENODEV ---- */
	ret = i2c_transfer(i3c_dev, &msg, 1, TEST_I2C_ADDR);
	zassert_equal(ret, -ENODEV, "Expected -ENODEV after detach, got %d", ret);

	/* ---- 3. Re-attach ---- */
	ret = i3c_attach_i2c_device(desc);
	zassert_ok(ret, "i3c_attach_i2c_device returned %d", ret);

	zassert_not_null(i3c_dev_list_i2c_addr_find(i3c_dev, TEST_I2C_ADDR),
			 "Device not in list after re-attach");

	/* ---- 4. Transfer after re-attach must succeed ---- */
	k_msleep(5); /* allow hardware to settle */

	ret = i2c_transfer(i3c_dev, &msg, 1, TEST_I2C_ADDR);
	zassert_ok(ret, "Transfer after re-attach failed (%d)", ret);

	/* ---- 5. Duplicate address must be rejected ---- */
	dup = (struct i3c_i2c_device_desc){
		.bus = i3c_dev,
		.addr = TEST_I2C_ADDR,
		.lvr = desc->lvr,
	};
	ret = i3c_attach_i2c_device(&dup);
	zassert_equal(ret, -EADDRNOTAVAIL,
		      "Expected -EADDRNOTAVAIL attaching duplicate addr 0x%02x, got %d",
		      TEST_I2C_ADDR, ret);
}

/*
 * Write transfer — data integrity
 *
 * Verifies that:
 *  1. A single-byte write is captured by the target's write_received
 *     callback with the exact byte sent.
 *  2. A multi-byte write delivers all bytes in order.
 *  3. The stop callback fires exactly once after each transfer completes.
 *
 * The write_received and stop callbacks may fire from interrupt context
 * (driver-dependent).  A short sleep after i2c_transfer() ensures the
 * callbacks have run before the assertions.
 */
ZTEST(i3c_i2c, test_write_data_integrity)
{
	int ret;

	/* ---- 1. Single-byte write ---- */
	uint8_t single_byte = 0x5A;
	struct i2c_msg single_msg = {
		.buf = &single_byte,
		.len = 1,
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
	};

	memset(&tgt_state, 0, sizeof(tgt_state));

	ret = i2c_transfer(i3c_dev, &single_msg, 1, TEST_I2C_ADDR);
	zassert_ok(ret, "Single-byte write failed (%d)", ret);

	/* Allow target write-complete callbacks to run */
	k_msleep(5);

	zassert_equal(tgt_state.rx_count, 1, "Single-byte: expected 1 byte received, got %d",
		      tgt_state.rx_count);
	zassert_equal(tgt_state.rx_buf[0], single_byte, "Single-byte: expected 0x%02x, got 0x%02x",
		      single_byte, tgt_state.rx_buf[0]);
	zassert_equal(tgt_state.stop_count, 1, "Single-byte: expected 1 stop callback, got %d",
		      tgt_state.stop_count);

	/* ---- 2. Multi-byte write ---- */
	uint8_t multi_buf[] = {0x11, 0x22, 0x33, 0x44};
	struct i2c_msg multi_msg = {
		.buf = multi_buf,
		.len = ARRAY_SIZE(multi_buf),
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
	};

	memset(&tgt_state, 0, sizeof(tgt_state));

	ret = i2c_transfer(i3c_dev, &multi_msg, 1, TEST_I2C_ADDR);
	zassert_ok(ret, "Multi-byte write failed (%d)", ret);

	k_msleep(5);

	zassert_equal(tgt_state.rx_count, (int)ARRAY_SIZE(multi_buf),
		      "Multi-byte: expected %d bytes, got %d", (int)ARRAY_SIZE(multi_buf),
		      tgt_state.rx_count);
	for (int i = 0; i < (int)ARRAY_SIZE(multi_buf); i++) {
		zassert_equal(tgt_state.rx_buf[i], multi_buf[i],
			      "Multi-byte[%d]: expected 0x%02x, got 0x%02x", i, multi_buf[i],
			      tgt_state.rx_buf[i]);
	}
	zassert_equal(tgt_state.stop_count, 1, "Multi-byte: expected 1 stop callback, got %d",
		      tgt_state.stop_count);
}

/*
 * Read transfer — data integrity
 *
 * Verifies that:
 *  1. A single-byte read returns the exact byte preloaded in read_buf[0].
 *  2. A multi-byte read returns all bytes in the correct order.
 *  3. The stop callback fires exactly once after each read completes.
 *
 * The target serves reads byte-by-byte: read_requested supplies the first
 * byte, then read_processed is called for each subsequent byte.  read_idx
 * tracks the position in read_buf.
 */
ZTEST(i3c_i2c, test_read_data_integrity)
{
	int ret;

	/* ---- 1. Single-byte read ---- */
	uint8_t rx1;

	memset(&tgt_state, 0, sizeof(tgt_state));
	tgt_state.read_buf[0] = 0x7E;

	ret = i2c_read(i3c_dev, &rx1, 1, TEST_I2C_ADDR);
	zassert_ok(ret, "Single-byte read failed (%d)", ret);

	k_msleep(5);

	zassert_equal(rx1, 0x7E, "Single-byte: expected 0x7E, got 0x%02x", rx1);
	zassert_equal(tgt_state.stop_count, 1, "Single-byte: expected 1 stop, got %d",
		      tgt_state.stop_count);

	/* ---- 2. Multi-byte read ---- */
	uint8_t rx4[4];
	static const uint8_t expected[4] = {0xDE, 0xAD, 0xBE, 0xEF};

	memset(&tgt_state, 0, sizeof(tgt_state));
	memcpy(tgt_state.read_buf, expected, sizeof(expected));

	ret = i2c_read(i3c_dev, rx4, 4, TEST_I2C_ADDR);
	zassert_ok(ret, "Multi-byte read failed (%d)", ret);

	k_msleep(5);

	for (int i = 0; i < 4; i++) {
		zassert_equal(rx4[i], expected[i], "Multi-byte[%d]: expected 0x%02x, got 0x%02x", i,
			      expected[i], rx4[i]);
	}
	zassert_equal(tgt_state.stop_count, 1, "Multi-byte: expected 1 stop, got %d",
		      tgt_state.stop_count);
}

/*
 * Write-then-read (repeated start / register read pattern)
 *
 * Verifies that:
 *  1. A single i2c_write_read call (write + read, no intervening STOP) works
 *     end-to-end.
 *  2. The byte(s) written in the write phase are captured by the target
 *     (rx_count and rx_buf[0] match the register address sent).
 *  3. The byte(s) written are used as a register address to select the
 *     data returned in the read phase.
 *  4. Data returned matches the register model for the written address.
 *
 * Note: stop_count semantics are implementation-defined for i2c_write_read
 * (some I2C target drivers fire the stop callback on the repeated START
 * boundary as well as on the final STOP).  This test does not assert a
 * specific stop_count to remain portable.
 */
ZTEST(i3c_i2c, test_write_then_read)
{
	int ret;

	static const uint8_t reg_values[] = {0xAA, 0xBB, 0xCC, 0xDD};

	/* ---- 1. Single register read (reg addr = 2) ---- */
	uint8_t reg_addr = 2;
	uint8_t rx1;

	memset(&tgt_state, 0, sizeof(tgt_state));
	memcpy(tgt_state.read_buf, reg_values, sizeof(reg_values));

	ret = i2c_write_read(i3c_dev, TEST_I2C_ADDR, &reg_addr, 1, &rx1, 1);
	zassert_ok(ret, "Write-then-read (single) failed (%d)", ret);

	k_msleep(5);

	zassert_equal(tgt_state.rx_count, 1, "Reg[2]: expected 1 byte written, got %d",
		      tgt_state.rx_count);
	zassert_equal(tgt_state.rx_buf[0], reg_addr,
		      "Reg[2]: expected reg_addr 0x%02x written, got 0x%02x", reg_addr,
		      tgt_state.rx_buf[0]);
	zassert_equal(rx1, reg_values[2], "Reg[2]: expected 0x%02x, got 0x%02x", reg_values[2],
		      rx1);

	/* ---- 2. Multi-byte register read (reg addr = 1, 2 bytes) ---- */
	reg_addr = 1;
	uint8_t rx2[2];

	memset(&tgt_state, 0, sizeof(tgt_state));
	memcpy(tgt_state.read_buf, reg_values, sizeof(reg_values));

	ret = i2c_write_read(i3c_dev, TEST_I2C_ADDR, &reg_addr, 1, rx2, 2);
	zassert_ok(ret, "Write-then-read (multi) failed (%d)", ret);

	k_msleep(5);

	zassert_equal(tgt_state.rx_count, 1, "Reg[1..2]: expected 1 byte written, got %d",
		      tgt_state.rx_count);
	zassert_equal(tgt_state.rx_buf[0], reg_addr,
		      "Reg[1..2]: expected reg_addr 0x%02x written, got 0x%02x", reg_addr,
		      tgt_state.rx_buf[0]);
	zassert_equal(rx2[0], reg_values[1], "Reg[1]: expected 0x%02x, got 0x%02x", reg_values[1],
		      rx2[0]);
	zassert_equal(rx2[1], reg_values[2], "Reg[2]: expected 0x%02x, got 0x%02x", reg_values[2],
		      rx2[1]);
}

/*
 * Burst / sequential register read
 *
 * Verifies that reading N consecutive bytes in a single i2c_read transfer
 * returns data matching the target's sequentially provided bytes.  The
 * first byte is supplied by the read_requested callback and each subsequent
 * byte by a separate read_processed invocation.  No bytes shall be dropped,
 * duplicated, or re-ordered across the full burst.
 *
 * Two burst sizes are exercised:
 *   - 8 bytes:  exercises a moderate-length burst.
 *   - 16 bytes: fills the maximum TGT_RX_BUF_MAX, exercising the full depth
 *               of the read_processed chain.
 */
ZTEST(i3c_i2c, test_burst_sequential_read)
{
	int ret;

	/* ---- 1. 8-byte burst ---- */
	static const uint8_t pattern8[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	uint8_t rx8[8];

	memset(&tgt_state, 0, sizeof(tgt_state));
	memcpy(tgt_state.read_buf, pattern8, sizeof(pattern8));

	ret = i2c_read(i3c_dev, rx8, sizeof(rx8), TEST_I2C_ADDR);
	zassert_ok(ret, "8-byte burst read failed (%d)", ret);

	k_msleep(5);

	for (int i = 0; i < (int)ARRAY_SIZE(pattern8); i++) {
		zassert_equal(rx8[i], pattern8[i], "burst8[%d]: expected 0x%02x, got 0x%02x", i,
			      pattern8[i], rx8[i]);
	}
	zassert_equal(tgt_state.stop_count, 1, "8-byte burst: expected 1 stop, got %d",
		      tgt_state.stop_count);

	/* ---- 2. 16-byte burst (full read_buf depth) ---- */
	static const uint8_t pattern16[16] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
					      0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0xff};
	uint8_t rx16[16];

	memset(&tgt_state, 0, sizeof(tgt_state));
	memcpy(tgt_state.read_buf, pattern16, sizeof(pattern16));

	ret = i2c_read(i3c_dev, rx16, sizeof(rx16), TEST_I2C_ADDR);
	zassert_ok(ret, "16-byte burst read failed (%d)", ret);

	k_msleep(5);

	for (int i = 0; i < (int)ARRAY_SIZE(pattern16); i++) {
		zassert_equal(rx16[i], pattern16[i], "burst16[%d]: expected 0x%02x, got 0x%02x", i,
			      pattern16[i], rx16[i]);
	}
	zassert_equal(tgt_state.stop_count, 1, "16-byte burst: expected 1 stop, got %d",
		      tgt_state.stop_count);
}

#if HAS_I2C_TARGET2
/*
 * Multiple I2C targets on the bus
 *
 * Two I2C targets are registered on the same I3C bus.  A write to one
 * address must be received only by the intended target; the other target
 * must remain untouched.
 */
ZTEST(i3c_i2c, test_multiple_targets)
{
	const uint8_t data1 = 0xAA;
	const uint8_t data2 = 0x55;
	int ret;

	/* ---- 1. Write to first target; second target must see nothing ---- */
	memset(&tgt_state, 0, sizeof(tgt_state));
	memset(&tgt2_state, 0, sizeof(tgt2_state));

	ret = i2c_write(i3c_dev, &data1, 1, TEST_I2C_ADDR);
	zassert_ok(ret, "Write to addr 0x%02x failed (%d)", TEST_I2C_ADDR, ret);

	k_msleep(5);

	zassert_equal(tgt_state.rx_count, 1, "Target1: expected 1 byte, got %d",
		      tgt_state.rx_count);
	zassert_equal(tgt_state.rx_buf[0], data1, "Target1: expected 0x%02x, got 0x%02x", data1,
		      tgt_state.rx_buf[0]);
	zassert_equal(tgt2_state.rx_count, 0,
		      "Target2 should not receive write to Target1, got %d bytes",
		      tgt2_state.rx_count);

	/* ---- 2. Write to second target; first target must stay untouched ---- */
	memset(&tgt_state, 0, sizeof(tgt_state));
	memset(&tgt2_state, 0, sizeof(tgt2_state));

	ret = i2c_write(i3c_dev, &data2, 1, TEST_I2C_ADDR2);
	zassert_ok(ret, "Write to addr 0x%02x failed (%d)", TEST_I2C_ADDR2, ret);

	k_msleep(5);

	zassert_equal(tgt2_state.rx_count, 1, "Target2: expected 1 byte, got %d",
		      tgt2_state.rx_count);
	zassert_equal(tgt2_state.rx_buf[0], data2, "Target2: expected 0x%02x, got 0x%02x", data2,
		      tgt2_state.rx_buf[0]);
	zassert_equal(tgt_state.rx_count, 0,
		      "Target1 should not receive write to Target2, got %d bytes",
		      tgt_state.rx_count);
}
#endif /* HAS_I2C_TARGET2 */

/*
 * Address probe (1-byte read)
 *
 * Note on probe technique:
 *   The Zephyr I2C API does not contractually guarantee that zero-length
 *   transfers are supported on every controller.  This test uses a 1-byte
 *   read instead, which is a non-destructive bus probe supported on every
 *   controller and well-defined under the I2C specification.
 *
 * Verifies that:
 *  1. A 1-byte read from a registered, present I2C address succeeds: the
 *     target ACKs the address phase and supplies one byte.
 *  2. A 1-byte read from a completely unregistered address fails with
 *     -ENODEV: the I3C subsystem rejects the transfer before bus access
 *     because the address is not in the attached-device list.
 *  3. A 1-byte read from a registered-but-absent address fails with
 *     -ENXIO: the controller drives the address on the bus, receives a
 *     NACK, and returns the error.  The temporary descriptor is detached
 *     afterwards to restore clean bus state.
 */
ZTEST(i3c_i2c, test_address_probe)
{
	uint8_t byte;
	int ret;

	/* ---- 1. Probe registered, present device ---- */
	memset(&tgt_state, 0, sizeof(tgt_state));

	ret = i2c_read(i3c_dev, &byte, 1, TEST_I2C_ADDR);
	zassert_ok(ret, "Probe of present addr 0x%02x failed (%d)", TEST_I2C_ADDR, ret);
	zassert_equal(byte, 0x00, "Probe: expected 0x00 (read_buf[0] preset), got 0x%02x", byte);

	/* ---- 2. Probe completely unregistered address (subsystem gate) ---- */
	/* Address 0x40 is not in the DT device list at all.  The I3C subsystem
	 * returns -ENODEV before any bus access, confirming no spurious traffic.
	 */
	ret = i2c_read(i3c_dev, &byte, 1, 0x40);
	zassert_equal(ret, -ENODEV, "Expected -ENODEV for unregistered addr 0x40, got %d", ret);

	/* ---- 3. Probe registered-but-absent address (bus NACK) ---- */
	struct i3c_i2c_device_desc ghost = {
		.bus = i3c_dev,
		.addr = 0x5A, /* arbitrary unused address — no physical device responds */
		/* Same LVR as the main test device (FM mode, index 0). */
		.lvr = 0x10,
	};

	ret = i3c_attach_i2c_device(&ghost);
	zassert_ok(ret, "Failed to attach ghost descriptor (%d)", ret);

	ret = i2c_read(i3c_dev, &byte, 1, 0x5A);
	zassert_equal(ret, -ENXIO, "Expected -ENXIO (NACK) for absent addr 0x5A, got %d", ret);

	(void)i3c_detach_i2c_device(&ghost);
}

/*
 * Transfer timeout / bus recovery
 *
 * Simulates a target that disappears from the bus by calling
 * i2c_target_unregister(), which disables the target hardware so it NACKs
 * any address cycle (equivalent to a hung or powered-off target).
 *
 * Note: in-flight unregistration (calling unregister while a transfer is
 * blocked in k_sem_take) is not testable from a single Ztest thread without
 * spawning a helper thread.  Unregistering before the transfer produces the
 * same bus condition (target absent → NACK) and exercises the controller
 * driver's NACK recovery path.
 *
 * Verifies that:
 *  1. A transfer to an unregistered (target-disabled) address returns
 *     -ENXIO (address NACK).
 *  2. After the error, i2c_target_register() re-enables the target.
 *  3. A subsequent valid write transfer succeeds end-to-end.
 */
ZTEST(i3c_i2c, test_bus_recovery)
{
	int ret;

	/* ---- 1. Disable the target (simulates hung / powered-off device) ---- */
	ret = i2c_target_unregister(i2c_target_dev, &tgt_config);
	zassert_ok(ret, "i2c_target_unregister failed (%d)", ret);

	/* Transfer while target is disabled — expect address NACK (-ENXIO) */
	uint8_t dummy = 0xAB;

	ret = i2c_write(i3c_dev, &dummy, 1, TEST_I2C_ADDR);
	zassert_equal(ret, -ENXIO, "Expected -ENXIO with target disabled, got %d", ret);

	/* ---- 2. Re-enable the target ---- */
	ret = i2c_target_register(i2c_target_dev, &tgt_config);
	zassert_ok(ret, "i2c_target_register failed after recovery (%d)", ret);

	/* Allow target hardware to fully initialise after re-enable */
	k_msleep(1);

	/* ---- 3. Verify bus is healthy: a normal write must succeed ---- */
	uint8_t recovery_byte = 0xC3;
	struct i2c_msg recovery_msg = {
		.buf = &recovery_byte,
		.len = 1,
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
	};

	memset(&tgt_state, 0, sizeof(tgt_state));

	ret = i2c_transfer(i3c_dev, &recovery_msg, 1, TEST_I2C_ADDR);
	zassert_ok(ret, "Post-recovery write failed (%d)", ret);

	k_msleep(5);

	zassert_equal(tgt_state.rx_count, 1, "Post-recovery: expected 1 byte, got %d",
		      tgt_state.rx_count);
	zassert_equal(tgt_state.rx_buf[0], recovery_byte,
		      "Post-recovery: expected 0x%02x, got 0x%02x", recovery_byte,
		      tgt_state.rx_buf[0]);
}

/*
 * Stress / repeated transfers
 *
 * Runs N consecutive write-then-read cycles against the target.  Each cycle
 * uses i2c_write_read (repeated START) to write a 1-byte register address
 * then read back the value at that register.
 *
 * Data pattern: read_buf[i] = i + 1 for i in [0, TGT_RX_BUF_MAX).
 * The register address cycles through [0, TGT_RX_BUF_MAX) so every register
 * is exercised repeatedly across the full run.
 */
#define STRESS_NCYCLES 500
ZTEST(i3c_i2c, test_stress_repeated_transfers)
{
	int ret;

	/* Preload the read register model (persists across cycles) */
	memset(&tgt_state, 0, sizeof(tgt_state));
	for (int i = 0; i < TGT_RX_BUF_MAX; i++) {
		tgt_state.read_buf[i] = (uint8_t)(i + 1);
	}

	for (int cycle = 0; cycle < STRESS_NCYCLES; cycle++) {
		uint8_t reg_addr = (uint8_t)(cycle % TGT_RX_BUF_MAX);
		uint8_t rx;

		/* Reset per-cycle state; keep read_buf intact */
		tgt_state.rx_count = 0;
		tgt_state.read_idx = 0;
		tgt_state.stop_count = 0;

		ret = i2c_write_read(i3c_dev, TEST_I2C_ADDR, &reg_addr, 1, &rx, 1);
		zassert_ok(ret, "Cycle %d: i2c_write_read failed (%d)", cycle, ret);

		zassert_equal(rx, tgt_state.read_buf[reg_addr],
			      "Cycle %d: reg[%d] expected 0x%02x, got 0x%02x", cycle, reg_addr,
			      tgt_state.read_buf[reg_addr], rx);

		/* At least one stop callback must have fired (the final STOP).
		 * Some I2C target drivers also fire it on the repeated START
		 * boundary; do not over-constrain.
		 */
		zassert_true(tgt_state.stop_count >= 1,
			     "Cycle %d: expected >= 1 stop callback, got %d", cycle,
			     tgt_state.stop_count);
	}
}

static void *i3c_i2c_suite_setup(void)
{
	zassert_true(device_is_ready(i2c_target_dev), "I2C target device %s not ready",
		     i2c_target_dev->name);
	zassert_ok(i2c_target_register(i2c_target_dev, &tgt_config),
		   "Failed to register I2C target at 0x%02x", TEST_I2C_ADDR);

#if HAS_I2C_TARGET2
	zassert_true(device_is_ready(i2c_target2_dev), "I2C target device %s not ready",
		     i2c_target2_dev->name);
	zassert_ok(i2c_target_register(i2c_target2_dev, &tgt2_config),
		   "Failed to register I2C target at 0x%02x", TEST_I2C_ADDR2);
#endif
	return NULL;
}

static void i3c_i2c_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Reset all shared target state so no test inherits stale data */
	memset(&tgt_state, 0, sizeof(tgt_state));
#if HAS_I2C_TARGET2
	memset(&tgt2_state, 0, sizeof(tgt2_state));
#endif

	/* Ensure I2C target(s) are registered before every test.
	 * The bus-recovery test intentionally unregisters the target; if it
	 * fails mid-way the target stays disabled and poisons subsequent
	 * tests.  Calling i2c_target_register() on an already-registered
	 * target is harmless.
	 */
	(void)i2c_target_register(i2c_target_dev, &tgt_config);
#if HAS_I2C_TARGET2
	(void)i2c_target_register(i2c_target2_dev, &tgt2_config);
#endif

	/* Brief settle — lets any in-flight bus activity from a previous test
	 * (or error-recovery) complete before the next test touches the bus.
	 */
	k_msleep(5);
}

static void i3c_i2c_suite_teardown(void *data)
{
	ARG_UNUSED(data);
	i2c_target_unregister(i2c_target_dev, &tgt_config);
#if HAS_I2C_TARGET2
	i2c_target_unregister(i2c_target2_dev, &tgt2_config);
#endif
}

ZTEST_SUITE(i3c_i2c, NULL, i3c_i2c_suite_setup, i3c_i2c_before_each, NULL, i3c_i2c_suite_teardown);
