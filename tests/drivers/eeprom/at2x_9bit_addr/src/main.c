/*
 * Copyright (c) 2026 Axon Enterprise, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/ztest.h>

/*
 * The AT25 driver's 9-bit addressing mode addresses a 512 byte EEPROM using
 * only 8 address bits on the wire: the 9th (most significant) bit is instead
 * OR'd into the opcode byte. Offsets below 256 ("low bank") and offsets at or
 * above 256 ("high bank") are therefore only distinguished by that opcode
 * bit, not by the address bytes themselves.
 */
#define EEPROM_NODE       DT_NODELABEL(eeprom9)
#define LOW_BANK_SIZE     256
#define HIGH_BANK_OFFSET  LOW_BANK_SIZE

static const struct device *const eeprom = DEVICE_DT_GET(EEPROM_NODE);

static void *eeprom_9bit_addr_setup(void)
{
	zassert_true(device_is_ready(eeprom), "EEPROM device not ready");

	return NULL;
}

ZTEST_SUITE(eeprom_at25_9bit_addr, NULL, eeprom_9bit_addr_setup, NULL, NULL, NULL);

ZTEST(eeprom_at25_9bit_addr, test_size)
{
	zassert_equal(eeprom_get_size(eeprom), 512, "Unexpected EEPROM size");
}

ZTEST(eeprom_at25_9bit_addr, test_low_bank_write_read)
{
	const uint8_t wr_buf[4] = { 0x11, 0x22, 0x33, 0x44 };
	uint8_t rd_buf[sizeof(wr_buf)];

	zassert_ok(eeprom_write(eeprom, 0x10, wr_buf, sizeof(wr_buf)));
	zassert_ok(eeprom_read(eeprom, 0x10, rd_buf, sizeof(rd_buf)));
	zassert_mem_equal(wr_buf, rd_buf, sizeof(wr_buf));
}

ZTEST(eeprom_at25_9bit_addr, test_high_bank_write_read)
{
	const uint8_t wr_buf[4] = { 0x55, 0x66, 0x77, 0x88 };
	uint8_t rd_buf[sizeof(wr_buf)];
	off_t offset = HIGH_BANK_OFFSET + 0x10;

	zassert_ok(eeprom_write(eeprom, offset, wr_buf, sizeof(wr_buf)));
	zassert_ok(eeprom_read(eeprom, offset, rd_buf, sizeof(rd_buf)));
	zassert_mem_equal(wr_buf, rd_buf, sizeof(wr_buf));
}

/*
 * Regression test: if the driver failed to OR the 9th address bit into the
 * opcode (e.g. it only sent offset & 0xFF), a write to the high bank would
 * silently alias onto the same address in the low bank, corrupting it.
 */
ZTEST(eeprom_at25_9bit_addr, test_banks_do_not_alias)
{
	const uint8_t low_pattern[4] = { 0xAA, 0xAA, 0xAA, 0xAA };
	const uint8_t high_pattern[4] = { 0x55, 0x55, 0x55, 0x55 };
	uint8_t rd_buf[4];
	off_t low_offset = 0x20;
	off_t high_offset = HIGH_BANK_OFFSET + 0x20;

	zassert_ok(eeprom_write(eeprom, low_offset, low_pattern, sizeof(low_pattern)));
	zassert_ok(eeprom_write(eeprom, high_offset, high_pattern, sizeof(high_pattern)));

	zassert_ok(eeprom_read(eeprom, low_offset, rd_buf, sizeof(rd_buf)));
	zassert_mem_equal(low_pattern, rd_buf, sizeof(rd_buf),
			  "Write to high bank corrupted low bank data");

	zassert_ok(eeprom_read(eeprom, high_offset, rd_buf, sizeof(rd_buf)));
	zassert_mem_equal(high_pattern, rd_buf, sizeof(rd_buf),
			  "High bank did not retain its own data");
}

ZTEST(eeprom_at25_9bit_addr, test_bank_boundary)
{
	const uint8_t last_low_byte = 0x5A;
	const uint8_t first_high_byte = 0xA5;
	uint8_t rd_value;

	zassert_ok(eeprom_write(eeprom, LOW_BANK_SIZE - 1, &last_low_byte, 1));
	zassert_ok(eeprom_write(eeprom, HIGH_BANK_OFFSET, &first_high_byte, 1));

	zassert_ok(eeprom_read(eeprom, LOW_BANK_SIZE - 1, &rd_value, 1));
	zassert_equal(last_low_byte, rd_value);

	zassert_ok(eeprom_read(eeprom, HIGH_BANK_OFFSET, &rd_value, 1));
	zassert_equal(first_high_byte, rd_value);
}

ZTEST(eeprom_at25_9bit_addr, test_last_byte)
{
	const uint8_t value = 0x5A;
	uint8_t rd_value;
	off_t offset = 511;

	zassert_ok(eeprom_write(eeprom, offset, &value, 1));
	zassert_ok(eeprom_read(eeprom, offset, &rd_value, 1));
	zassert_equal(value, rd_value);
}

ZTEST(eeprom_at25_9bit_addr, test_out_of_bounds)
{
	const uint8_t data = 0x00;
	uint8_t rd_data;
	size_t size = eeprom_get_size(eeprom);

	zassert_equal(-EINVAL, eeprom_write(eeprom, size - 1, &data, 2));
	zassert_equal(-EINVAL, eeprom_read(eeprom, size - 1, &rd_data, 2));
}
