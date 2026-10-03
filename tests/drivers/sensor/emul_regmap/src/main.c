/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_i2c_device

#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor/emul_sensor_regmap.h>
#include <zephyr/drivers/emul_stub_device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>

#define NODE DT_NODELABEL(regmap_test)
#define ADDR DT_REG_ADDR(NODE)

#define TEST_REGS(R)                                                                               \
	R(ID, 0x00, .flags = EMUL_SENSOR_REG_RO, .reset = 0xAB)                                    \
	R(CTRL, 0x01, .write_mask = GENMASK(5, 0), .self_clear = BIT(0))                           \
	R(STATUS, 0x02, .flags = EMUL_SENSOR_REG_RO, .clear_on_read = BIT(7))                      \
	R(OUT_L, 0x03, .flags = EMUL_SENSOR_REG_RO)                                                \
	R(OUT_H, 0x04, .flags = EMUL_SENSOR_REG_RO)                                                \
	R(WIDE, 0x05, .bytes = 2, .reset = 0x1234)

EMUL_SENSOR_REG_TABLE_DEFINE(test_regs, TEST_REGS);

/* clang-format off */
static const struct emul_sensor_channel test_channels[] = {
	{
		.chan = SENSOR_CHAN_AMBIENT_TEMP,
		.reg = EMUL_SENSOR_REG_ADDR(OUT_L),
		.is_signed = true,
		.bits = 12,
		.pos = 4,
		.lsb = 0.0625,
		.min = -40.0,
		.max = 125.0,
		.ready = {
			.reg = EMUL_SENSOR_REG_ADDR(STATUS),
			.mask = BIT(7),
		},
	},
	{
		.chan = SENSOR_CHAN_PROX,
		.reg = EMUL_SENSOR_REG_ADDR(WIDE),
		.bits = 16,
		.select = {
			.reg = EMUL_SENSOR_REG_ADDR(CTRL),
			.mask = GENMASK(5, 4),
		},
		.variants = {
			{ .lsb = 1.0 },
			{ .lsb = 2.0, .max = 100.0 },
		},
	},
};
/* clang-format on */

EMUL_SENSOR_REGMAP_DEFINE(test_regs, test_channels, .big_endian = TEST_BIG_ENDIAN);
DT_INST_FOREACH_STATUS_OKAY(EMUL_STUB_DEVICE)

static const struct device *bus = DEVICE_DT_GET(DT_BUS(NODE));
static const struct emul *emul = EMUL_DT_GET(NODE);

static uint16_t read_word(uint8_t addr)
{
	uint8_t buf[2];

	zassert_ok(i2c_burst_read(bus, ADDR, addr, buf, sizeof(buf)));
	return TEST_BIG_ENDIAN ? sys_get_be16(buf) : sys_get_le16(buf);
}

static q31_t q31(double value, int8_t shift)
{
	return (q31_t)(value * 2147483648.0 / (double)BIT(shift));
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(emul->init(emul, NULL));
}

ZTEST(emul_regmap, test_reset_values_and_byte_order)
{
	uint8_t buf[2];

	zassert_ok(i2c_reg_read_byte(bus, ADDR, EMUL_SENSOR_REG_ADDR(ID), &buf[0]));
	zassert_equal(buf[0], 0xAB);
	zassert_ok(i2c_burst_read(bus, ADDR, EMUL_SENSOR_REG_ADDR(WIDE), buf, sizeof(buf)));
	zassert_equal(buf[0], TEST_BIG_ENDIAN ? 0x12 : 0x34);
	zassert_equal(buf[1], TEST_BIG_ENDIAN ? 0x34 : 0x12);
	zassert_equal(read_word(EMUL_SENSOR_REG_ADDR(WIDE)), 0x1234);
}

ZTEST(emul_regmap, test_pointer_persists_and_increments)
{
	uint8_t ptr = EMUL_SENSOR_REG_ADDR(ID);
	uint8_t buf[2];

	zassert_ok(i2c_write(bus, &ptr, 1, ADDR));
	zassert_ok(i2c_read(bus, buf, 2, ADDR));
	zassert_equal(buf[0], 0xAB);
	zassert_equal(buf[1], 0x00);
	zassert_ok(i2c_read(bus, buf, 1, ADDR));
	zassert_equal(buf[0], 0x00);
}

ZTEST(emul_regmap, test_write_mask_and_self_clear)
{
	uint8_t val;

	zassert_ok(i2c_reg_write_byte(bus, ADDR, EMUL_SENSOR_REG_ADDR(CTRL), 0xFF));
	zassert_ok(i2c_reg_read_byte(bus, ADDR, EMUL_SENSOR_REG_ADDR(CTRL), &val));
	zassert_equal(val, 0x3E);
}

ZTEST(emul_regmap, test_wide_register_write)
{
	uint8_t buf[2] = {TEST_BIG_ENDIAN ? 0x56 : 0x78, TEST_BIG_ENDIAN ? 0x78 : 0x56};

	zassert_ok(i2c_burst_write(bus, ADDR, EMUL_SENSOR_REG_ADDR(WIDE), buf, sizeof(buf)));
	zassert_equal(read_word(EMUL_SENSOR_REG_ADDR(WIDE)), 0x5678);
}

ZTEST(emul_regmap, test_read_only_and_unknown_registers)
{
	uint8_t val;

	zassert_ok(i2c_reg_write_byte(bus, ADDR, EMUL_SENSOR_REG_ADDR(ID), 0x00));
	zassert_ok(i2c_reg_read_byte(bus, ADDR, EMUL_SENSOR_REG_ADDR(ID), &val));
	zassert_equal(val, 0xAB);
	zassert_equal(i2c_reg_read_byte(bus, ADDR, 0x40, &val), -EIO);
	zassert_equal(i2c_reg_write_byte(bus, ADDR, 0x40, 0x00), -EIO);
}

ZTEST(emul_regmap, test_set_channel_and_clear_on_read)
{
	struct sensor_chan_spec ch = {.chan_type = SENSOR_CHAN_AMBIENT_TEMP};
	q31_t value = q31(-40.0, 7);
	uint8_t buf[2];
	uint8_t status;

	zassert_ok(emul_sensor_backend_set_channel(emul, ch, &value, 7));
	zassert_ok(i2c_burst_read(bus, ADDR, EMUL_SENSOR_REG_ADDR(OUT_L), buf, sizeof(buf)));
	zassert_equal(buf[0], TEST_BIG_ENDIAN ? 0xD8 : 0x00);
	zassert_equal(buf[1], TEST_BIG_ENDIAN ? 0x00 : 0xD8);
	zassert_ok(i2c_reg_read_byte(bus, ADDR, EMUL_SENSOR_REG_ADDR(STATUS), &status));
	zassert_equal(status, 0x80);
	zassert_ok(i2c_reg_read_byte(bus, ADDR, EMUL_SENSOR_REG_ADDR(STATUS), &status));
	zassert_equal(status, 0x00);
}

ZTEST(emul_regmap, test_sample_range)
{
	struct sensor_chan_spec ch = {.chan_type = SENSOR_CHAN_AMBIENT_TEMP};
	q31_t lower, upper, epsilon;
	int8_t shift;

	zassert_ok(
		emul_sensor_backend_get_sample_range(emul, ch, &lower, &upper, &epsilon, &shift));
	zassert_equal(shift, 7);
	zassert_equal(lower, q31(-40.0, 7));
	zassert_equal(upper, q31(125.0, 7));
	zassert_equal(epsilon, q31(0.0625, 7));

	ch.chan_type = SENSOR_CHAN_MAGN_X;
	zassert_equal(
		emul_sensor_backend_get_sample_range(emul, ch, &lower, &upper, &epsilon, &shift),
		-ENOTSUP);
}

ZTEST(emul_regmap, test_variants)
{
	struct sensor_chan_spec ch = {.chan_type = SENSOR_CHAN_PROX};
	q31_t value = q31(50.0, 6);
	q31_t lower, upper, epsilon;
	int8_t shift;

	zassert_ok(emul_sensor_backend_set_channel(emul, ch, &value, 6));
	zassert_equal(read_word(EMUL_SENSOR_REG_ADDR(WIDE)), 50);
	zassert_ok(
		emul_sensor_backend_get_sample_range(emul, ch, &lower, &upper, &epsilon, &shift));
	zassert_equal(shift, 16);
	zassert_equal(upper, q31(65535.0, 16));

	zassert_ok(i2c_reg_write_byte(bus, ADDR, EMUL_SENSOR_REG_ADDR(CTRL), BIT(4)));
	zassert_ok(emul_sensor_backend_set_channel(emul, ch, &value, 6));
	zassert_equal(read_word(EMUL_SENSOR_REG_ADDR(WIDE)), 25);
	zassert_ok(
		emul_sensor_backend_get_sample_range(emul, ch, &lower, &upper, &epsilon, &shift));
	zassert_equal(shift, 7);
	zassert_equal(upper, q31(100.0, 7));
	zassert_equal(epsilon, q31(2.0, 7));
}

ZTEST(emul_regmap, test_split_write)
{
	uint8_t first[] = {EMUL_SENSOR_REG_ADDR(WIDE), TEST_BIG_ENDIAN ? 0x56 : 0x78};
	uint8_t last = TEST_BIG_ENDIAN ? 0x78 : 0x56;
	struct i2c_msg msgs[] = {
		{.buf = first, .len = sizeof(first), .flags = I2C_MSG_WRITE},
		{.buf = &last, .len = 1, .flags = I2C_MSG_WRITE | I2C_MSG_STOP},
	};

	zassert_ok(i2c_transfer(bus, msgs, ARRAY_SIZE(msgs), ADDR));
	zassert_equal(read_word(EMUL_SENSOR_REG_ADDR(WIDE)), 0x5678);
}

ZTEST(emul_regmap, test_invalid_input)
{
	struct sensor_chan_spec ch = {.chan_type = SENSOR_CHAN_AMBIENT_TEMP};
	q31_t value = q31(126.0, 7);

	zassert_equal(emul_sensor_backend_set_channel(emul, ch, &value, 7), -ERANGE);
	zassert_equal(emul_sensor_backend_set_channel(emul, ch, NULL, 7), -EINVAL);
	zassert_equal(emul_sensor_backend_set_channel(emul, ch, &value, 32), -EINVAL);
	ch.chan_idx = 1;
	zassert_equal(emul_sensor_backend_set_channel(emul, ch, &value, 7), -ENOTSUP);
}

ZTEST_SUITE(emul_regmap, NULL, NULL, before, NULL, NULL);
