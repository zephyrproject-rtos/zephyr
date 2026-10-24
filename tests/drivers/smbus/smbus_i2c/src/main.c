/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/smbus.h>
#include <zephyr/sys/crc.h>
#include <zephyr/ztest.h>

#define TARGET_ADDR DT_PHA_BY_IDX(DT_NODELABEL(i2c0), forwards, 0, addr)
#define ADDR_W      (TARGET_ADDR << 1)
#define ADDR_R      ((TARGET_ADDR << 1) | 1)
#define BUF_SIZE    40

static const struct device *const smbus = DEVICE_DT_GET(DT_NODELABEL(smbus0));
static const struct device *const target_bus = DEVICE_DT_GET(DT_NODELABEL(i2c1));

static struct {
	uint8_t rx[BUF_SIZE];
	size_t rx_len;
	uint8_t tx[BUF_SIZE];
	size_t tx_pos;
	int writes;
	int reads;
	int stops;
	bool nack;
} target;

static int target_write_requested(struct i2c_target_config *cfg)
{
	ARG_UNUSED(cfg);

	if (target.nack) {
		return -EIO;
	}

	target.writes++;

	return 0;
}

static int target_write_received(struct i2c_target_config *cfg, uint8_t val)
{
	ARG_UNUSED(cfg);

	zassert_true(target.rx_len < sizeof(target.rx));
	target.rx[target.rx_len++] = val;

	return 0;
}

static int target_read_processed(struct i2c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);

	zassert_true(target.tx_pos < sizeof(target.tx));
	*val = target.tx[target.tx_pos++];

	return 0;
}

static int target_read_requested(struct i2c_target_config *cfg, uint8_t *val)
{
	target.reads++;

	return target_read_processed(cfg, val);
}

static int target_stop(struct i2c_target_config *cfg)
{
	ARG_UNUSED(cfg);

	target.stops++;

	return 0;
}

static const struct i2c_target_callbacks target_callbacks = {
	.write_requested = target_write_requested,
	.write_received = target_write_received,
	.read_requested = target_read_requested,
	.read_processed = target_read_processed,
	.stop = target_stop,
};

static struct i2c_target_config target_cfg = {
	.address = TARGET_ADDR,
	.callbacks = &target_callbacks,
};

static void reset_target(void)
{
	memset(&target, 0, sizeof(target));
}

static void set_pec(bool enable)
{
	uint32_t cfg = SMBUS_MODE_CONTROLLER | (enable ? SMBUS_MODE_PEC : 0U);

	zassert_ok(smbus_configure(smbus, cfg));
}

/* The wire bytes start with the address byte, which the target does not receive */
static void check_write(const uint8_t *wire, size_t len, bool pec)
{
	zassert_equal(target.writes, 1);
	zassert_equal(target.reads, 0);
	zassert_equal(target.stops, 1);
	zassert_equal(target.rx_len, len - 1U + (pec ? 1U : 0U));
	zassert_mem_equal(target.rx, &wire[1], len - 1U);
	if (pec) {
		zassert_equal(target.rx[len - 1U], crc8_ccitt(0, wire, len));
	}
}

/* The last data_len wire bytes are the response, followed by its PEC */
static void respond(const uint8_t *wire, size_t len, size_t data_len)
{
	memcpy(target.tx, &wire[len - data_len], data_len);
	target.tx[data_len] = crc8_ccitt(0, wire, len);
}

static void check_read(const uint8_t *wire, size_t wr_len)
{
	zassert_equal(target.writes, (wr_len == 0U) ? 0 : 1);
	zassert_equal(target.reads, 1);
	zassert_equal(target.stops, 1);
	zassert_equal(target.rx_len, wr_len);
	zassert_mem_equal(target.rx, &wire[1], wr_len);
}

ZTEST(smbus_i2c, test_configure)
{
	uint32_t cfg;

	zassert_ok(smbus_get_config(smbus, &cfg));
	zassert_equal(cfg, SMBUS_MODE_CONTROLLER);

	zassert_equal(smbus_configure(smbus, 0U), -ENOTSUP);
	zassert_equal(smbus_configure(smbus, SMBUS_MODE_CONTROLLER | SMBUS_MODE_SMBALERT),
		      -ENOTSUP);
	zassert_equal(smbus_configure(smbus, SMBUS_MODE_CONTROLLER | SMBUS_MODE_HOST_NOTIFY),
		      -ENOTSUP);

	set_pec(true);
	zassert_ok(smbus_get_config(smbus, &cfg));
	zassert_equal(cfg, SMBUS_MODE_CONTROLLER | SMBUS_MODE_PEC);
}

ZTEST(smbus_i2c, test_quick)
{
	zassert_ok(smbus_quick(smbus, TARGET_ADDR, SMBUS_MSG_WRITE));
	zassert_ok(smbus_quick(smbus, TARGET_ADDR, SMBUS_MSG_READ));

	zassert_equal(target.rx_len, 0);
	zassert_equal(target.tx_pos, 0);
	zassert_equal(target.stops, 2);
}

static void check_byte(bool pec)
{
	const uint8_t wire_wr[] = {ADDR_W, 0x03};
	const uint8_t wire_rd[] = {ADDR_R, 0xA5};
	uint8_t byte;

	set_pec(pec);

	zassert_ok(smbus_byte_write(smbus, TARGET_ADDR, 0x03));
	check_write(wire_wr, sizeof(wire_wr), pec);

	reset_target();
	respond(wire_rd, sizeof(wire_rd), 1);
	zassert_ok(smbus_byte_read(smbus, TARGET_ADDR, &byte));
	zassert_equal(byte, 0xA5);
	check_read(wire_rd, 0);
}

ZTEST(smbus_i2c, test_byte)
{
	check_byte(false);
}

ZTEST(smbus_i2c, test_byte_pec)
{
	check_byte(true);
}

static void check_byte_data(bool pec)
{
	const uint8_t wire_wr[] = {ADDR_W, 0x10, 0x5A};
	const uint8_t wire_rd[] = {ADDR_W, 0x11, ADDR_R, 0xC3};
	uint8_t byte;

	set_pec(pec);

	zassert_ok(smbus_byte_data_write(smbus, TARGET_ADDR, 0x10, 0x5A));
	check_write(wire_wr, sizeof(wire_wr), pec);

	reset_target();
	respond(wire_rd, sizeof(wire_rd), 1);
	zassert_ok(smbus_byte_data_read(smbus, TARGET_ADDR, 0x11, &byte));
	zassert_equal(byte, 0xC3);
	check_read(wire_rd, 1);
}

ZTEST(smbus_i2c, test_byte_data)
{
	check_byte_data(false);
}

ZTEST(smbus_i2c, test_byte_data_pec)
{
	check_byte_data(true);
}

static void check_word_data(bool pec)
{
	const uint8_t wire_wr[] = {ADDR_W, 0x21, 0x34, 0x12};
	const uint8_t wire_rd[] = {ADDR_W, 0x22, ADDR_R, 0x55, 0xAA};
	uint16_t word;

	set_pec(pec);

	zassert_ok(smbus_word_data_write(smbus, TARGET_ADDR, 0x21, 0x1234));
	check_write(wire_wr, sizeof(wire_wr), pec);

	reset_target();
	respond(wire_rd, sizeof(wire_rd), 2);
	zassert_ok(smbus_word_data_read(smbus, TARGET_ADDR, 0x22, &word));
	zassert_equal(word, 0xAA55);
	check_read(wire_rd, 1);
}

ZTEST(smbus_i2c, test_word_data)
{
	check_word_data(false);
}

ZTEST(smbus_i2c, test_word_data_pec)
{
	check_word_data(true);
}

static void check_pcall(bool pec)
{
	const uint8_t wire[] = {ADDR_W, 0x30, 0x34, 0x12, ADDR_R, 0xCB, 0xED};
	uint16_t word;

	set_pec(pec);

	respond(wire, sizeof(wire), 2);
	zassert_ok(smbus_pcall(smbus, TARGET_ADDR, 0x30, 0x1234, &word));
	zassert_equal(word, 0xEDCB);
	check_read(wire, 3);
}

ZTEST(smbus_i2c, test_pcall)
{
	check_pcall(false);
}

ZTEST(smbus_i2c, test_pcall_pec)
{
	check_pcall(true);
}

ZTEST(smbus_i2c, test_block_write)
{
	const uint8_t wire[] = {ADDR_W, 0x40, 5, 1, 2, 3, 4, 5};
	uint8_t buf[] = {1, 2, 3, 4, 5};

	set_pec(true);

	zassert_ok(smbus_block_write(smbus, TARGET_ADDR, 0x40, sizeof(buf), buf));
	check_write(wire, sizeof(wire), true);
}

ZTEST(smbus_i2c, test_block_read_not_supported)
{
	uint8_t buf[SMBUS_BLOCK_BYTES_MAX];
	uint8_t count;

	zassert_equal(smbus_block_read(smbus, TARGET_ADDR, 0x40, &count, buf), -ENOSYS);
	zassert_equal(smbus_block_pcall(smbus, TARGET_ADDR, 0x40, 1, buf, &count, buf), -ENOSYS);
}

/* Reference PEC values computed with CRC-8 poly 0x07 over the bytes on the wire */
ZTEST(smbus_i2c, test_pec_vectors)
{
	uint16_t word;

	set_pec(true);

	zassert_ok(smbus_word_data_write(smbus, TARGET_ADDR, 0x21, 0x1234));
	zassert_equal(target.rx[3], 0x60);

	target.tx[0] = 0x55;
	target.tx[1] = 0xAA;
	target.tx[2] = 0xD5;
	zassert_ok(smbus_word_data_read(smbus, TARGET_ADDR, 0x22, &word));
	zassert_equal(word, 0xAA55);
}

ZTEST(smbus_i2c, test_pec_error)
{
	const uint8_t wire[] = {ADDR_W, 0x22, ADDR_R, 0x55, 0xAA};
	uint8_t byte;
	uint16_t word;

	set_pec(true);

	respond(wire, sizeof(wire), 2);
	target.tx[2] ^= 0xFFU;
	zassert_equal(smbus_word_data_read(smbus, TARGET_ADDR, 0x22, &word), -EIO);

	target.tx_pos = 0;
	zassert_equal(smbus_byte_data_read(smbus, TARGET_ADDR, 0x22, &byte), -EIO);

	target.tx_pos = 0;
	zassert_equal(smbus_byte_read(smbus, TARGET_ADDR, &byte), -EIO);

	target.tx_pos = 0;
	zassert_equal(smbus_pcall(smbus, TARGET_ADDR, 0x22, 0x1234, &word), -EIO);
}

ZTEST(smbus_i2c, test_nack)
{
	uint8_t byte;

	target.nack = true;

	zassert_equal(smbus_byte_data_write(smbus, TARGET_ADDR, 0x10, 0x5A), -EIO);
	zassert_equal(smbus_byte_data_read(smbus, TARGET_ADDR, 0x11, &byte), -EIO);
	zassert_equal(target.rx_len, 0);
}

ZTEST(smbus_i2c, test_no_target)
{
	uint8_t byte;

	zassert_equal(smbus_byte_data_read(smbus, TARGET_ADDR + 1, 0x00, &byte), -EIO);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	reset_target();
	zassert_ok(smbus_configure(smbus, SMBUS_MODE_CONTROLLER));
}

static void *setup(void)
{
	zassert_true(device_is_ready(smbus));
	zassert_ok(i2c_target_register(target_bus, &target_cfg));

	return NULL;
}

ZTEST_SUITE(smbus_i2c, NULL, setup, before, NULL, NULL);
