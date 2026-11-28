/*
 * Copyright (c) 2026 Shreyas Acharya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "bmi260_emul.h"
#include "bmi260_reg_map.h"

/*
 * The same device sits on both an emulated I2C bus and an emulated SPI bus,
 * so every check runs against both and a bus-specific mistake cannot hide.
 */
struct bmi260_fixture {
	const struct device *i2c_dev;
	const struct emul *i2c_target;
	const struct device *spi_dev;
	const struct emul *spi_target;
};

static void *bmi260_setup(void)
{
	static struct bmi260_fixture fixture = {
		.i2c_dev = DEVICE_DT_GET(DT_NODELABEL(bmi260_i2c)),
		.i2c_target = EMUL_DT_GET(DT_NODELABEL(bmi260_i2c)),
		.spi_dev = DEVICE_DT_GET(DT_NODELABEL(bmi260_spi)),
		.spi_target = EMUL_DT_GET(DT_NODELABEL(bmi260_spi)),
	};

	return &fixture;
}

/*
 * The interrupt status register latches until it is read, so a test that
 * leaves a flag set would hand it to whichever test runs next. Clearing it
 * here keeps every case independent of the order they run in.
 */
static void bmi260_before(void *f)
{
	struct bmi260_fixture *fixture = f;
	static const uint8_t clear;

	bmi260_emul_set_reg(fixture->i2c_target, BMI260_REG_INT_STATUS_1, &clear, sizeof(clear));
	bmi260_emul_set_reg(fixture->spi_target, BMI260_REG_INT_STATUS_1, &clear, sizeof(clear));
}

ZTEST_SUITE(bmi260, NULL, bmi260_setup, bmi260_before, NULL, NULL);

/* Writes a signed little-endian count into one of the data registers. */
static void set_raw_sample(const struct emul *target, uint8_t reg, int16_t raw)
{
	uint8_t bytes[2] = {
		(uint8_t)((uint16_t)raw & 0xFFU),
		(uint8_t)(((uint16_t)raw >> 8) & 0xFFU),
	};

	bmi260_emul_set_reg(target, reg, bytes, sizeof(bytes));
}

/*
 * Initialization reads the chip identifier over the bus, so a ready SPI
 * instance is one whose read framing is right: the address carried the read
 * bit, and the device's padding byte was dropped rather than mistaken for
 * the register contents.
 */
ZTEST_F(bmi260, test_both_bus_instances_are_ready)
{
	zassert_true(device_is_ready(fixture->i2c_dev), "the I2C instance did not initialise");
	zassert_true(device_is_ready(fixture->spi_dev), "the SPI instance did not initialise");
}

/*
 * Two instances in one image must not share state. Both are given different
 * counts and different ranges, and each has to report its own: a cached range
 * or a sample buffer held at file scope rather than per instance would make
 * one of the two answer with the other's reading.
 */
ZTEST_F(bmi260, test_both_instances_read_independently)
{
	struct sensor_value i2c_accel;
	struct sensor_value spi_accel;
	struct sensor_value range;

	/* The SPI instance is moved to 16 g while the I2C one stays at 2 g. */
	range.val1 = 16 * SENSOR_G / 1000000;
	range.val2 = 0;
	zassert_ok(sensor_attr_set(fixture->spi_dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_FULL_SCALE, &range),
		   "setting the SPI instance's range failed");

	set_raw_sample(fixture->i2c_target, BMI260_REG_ACC_X_LSB, INT16_MAX / 2);
	set_raw_sample(fixture->spi_target, BMI260_REG_ACC_X_LSB, INT16_MAX / 2);

	zassert_ok(sensor_sample_fetch(fixture->i2c_dev), "I2C fetch failed");
	zassert_ok(sensor_sample_fetch(fixture->spi_dev), "SPI fetch failed");

	zassert_ok(sensor_channel_get(fixture->i2c_dev, SENSOR_CHAN_ACCEL_X, &i2c_accel),
		   "I2C read failed");
	zassert_ok(sensor_channel_get(fixture->spi_dev, SENSOR_CHAN_ACCEL_X, &spi_accel),
		   "SPI read failed");

	/* Same counts, eight times the range, so eight times the reading. */
	zassert_within(sensor_value_to_double(&i2c_accel), 9.80665, 0.1,
		       "the I2C instance should still be at 2 g");
	zassert_within(sensor_value_to_double(&spi_accel),
		       sensor_value_to_double(&i2c_accel) * 8.0, 0.5,
		       "the SPI instance should report its own 16 g range, not the I2C one's");

	range.val1 = 2 * SENSOR_G / 1000000;
	zassert_ok(sensor_attr_set(fixture->spi_dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_FULL_SCALE, &range),
		   "restoring the SPI instance's range failed");
}

/*
 * A device that came up at all is one whose initialization read the chip
 * identifier and accepted it, so the check is that the identifier the
 * emulator answers with is the BMI260's.
 */
ZTEST_F(bmi260, test_init_verifies_the_chip_id)
{
	uint8_t chip_id;

	bmi260_emul_get_reg(fixture->i2c_target, BMI260_REG_CHIP_ID, &chip_id, sizeof(chip_id));

	zassert_equal(chip_id, BMI260_CHIP_ID, "the emulator should report the BMI260 chip id");
	zassert_true(device_is_ready(fixture->i2c_dev), "a BMI260 should be accepted");
}

/* A part answering with the BMI270's identifier must be turned away. */
ZTEST_F(bmi260, test_a_bmi270_is_rejected)
{
	const struct device *wrong = DEVICE_DT_GET(DT_NODELABEL(bmi260_wrong));
	const struct emul *target = EMUL_DT_GET(DT_NODELABEL(bmi260_wrong));
	static const uint8_t bmi270_chip_id = 0x24;

	/* The node defers initialization, so the identifier can be set first. */
	bmi260_emul_set_reg(target, BMI260_REG_CHIP_ID, &bmi270_chip_id, sizeof(bmi270_chip_id));

	zassert_not_equal(device_init(wrong), 0, "a BMI270 should not initialise as a BMI260");
	zassert_false(device_is_ready(wrong), "a rejected device must not report ready");
}

/*
 * The device keeps its configuration across a warm restart, so initialization
 * puts it into a known state with a soft reset. The count is what proves it
 * happened; on SPI it also proves the write path works, since the command has
 * to be written to get there.
 */
ZTEST_F(bmi260, test_init_issues_a_soft_reset)
{
	zassert_equal(bmi260_emul_get_soft_reset_count(fixture->i2c_target), 1,
		      "the I2C instance should have been reset exactly once");
	zassert_equal(bmi260_emul_get_soft_reset_count(fixture->spi_target), 1,
		      "the SPI instance should have been reset exactly once");
}

/*
 * A soft reset returns the device to listening on I2C, so a SPI instance has
 * to spend a second throwaway read to move it back. Missing that leaves every
 * later SPI access talking to a device in the wrong mode.
 */
ZTEST_F(bmi260, test_init_rebinds_the_bus_after_reset)
{
	/*
	 * Counted from the reset, so the reads taken while identifying the part
	 * beforehand do not mask a missing second latch.
	 */
	uint32_t reads = bmi260_emul_get_chip_id_reads_since_reset(fixture->spi_target);

	zassert_true(reads >= 1U,
		     "the SPI instance should read the chip id again after the reset, saw %u",
		     reads);
}

/*
 * Advanced power save makes the device ignore closely spaced accesses, so it
 * is switched off for the configuration upload and switched back on after.
 * Both writes are counted, because the final value alone cannot tell a
 * deliberate setting apart from a register nobody touched.
 */
ZTEST_F(bmi260, test_init_toggles_advanced_power_save_around_the_upload)
{
	zassert_true(bmi260_emul_get_write_count(fixture->i2c_target, BMI260_REG_PWR_CONF) >= 2U,
		     "the I2C instance wrote PWR_CONF %u time(s), expected it off then on",
		     bmi260_emul_get_write_count(fixture->i2c_target, BMI260_REG_PWR_CONF));
	zassert_true(bmi260_emul_get_write_count(fixture->spi_target, BMI260_REG_PWR_CONF) >= 2U,
		     "the SPI instance wrote PWR_CONF %u time(s), expected it off then on",
		     bmi260_emul_get_write_count(fixture->spi_target, BMI260_REG_PWR_CONF));
}

/*
 * The feature engine only runs once the whole configuration image has been
 * handed to it, so a short upload is a dead sensor.
 */
ZTEST_F(bmi260, test_the_whole_config_file_is_uploaded)
{
	zassert_equal(bmi260_emul_get_init_data_byte_count(fixture->i2c_target),
		      BMI260_CONFIG_FILE_SIZE, "the I2C instance uploaded %u of %u bytes",
		      bmi260_emul_get_init_data_byte_count(fixture->i2c_target),
		      BMI260_CONFIG_FILE_SIZE);
	zassert_equal(bmi260_emul_get_init_data_byte_count(fixture->spi_target),
		      BMI260_CONFIG_FILE_SIZE, "the SPI instance uploaded %u of %u bytes",
		      bmi260_emul_get_init_data_byte_count(fixture->spi_target),
		      BMI260_CONFIG_FILE_SIZE);
}

/*
 * The address accompanying each chunk counts 16-bit words, not bytes, so the
 * last chunk of the image starts half its byte offset in. Getting this wrong
 * uploads the image to the wrong place and the engine refuses it.
 */
ZTEST_F(bmi260, test_chunks_are_word_addressed)
{
	const uint16_t last_chunk_word = (BMI260_CONFIG_FILE_SIZE - BMI260_WR_LEN) / 2U;

	zassert_equal(bmi260_emul_get_last_init_addr(fixture->i2c_target), last_chunk_word,
		      "last chunk addressed word %u, expected %u",
		      bmi260_emul_get_last_init_addr(fixture->i2c_target), last_chunk_word);
}

/* The engine reports when it has accepted the image; initialization waits. */
ZTEST_F(bmi260, test_init_reports_the_engine_started)
{
	uint8_t status;

	bmi260_emul_get_reg(fixture->i2c_target, BMI260_REG_INTERNAL_STATUS, &status,
			    sizeof(status));

	zassert_equal(status & BMI260_INST_MESSAGE_MSK, BMI260_INST_MESSAGE_INIT_OK,
		      "the feature engine should report that it started");
}

/* Power save goes back on once the device is configured. */
ZTEST_F(bmi260, test_init_restores_advanced_power_save)
{
	uint8_t pwr_conf;

	bmi260_emul_get_reg(fixture->i2c_target, BMI260_REG_PWR_CONF, &pwr_conf,
			    sizeof(pwr_conf));

	zassert_equal(pwr_conf & BMI260_PWR_CONF_ADV_PWR_SAVE_MSK,
		      BMI260_PWR_CONF_ADV_PWR_SAVE_MSK,
		      "advanced power save should be back on after initialization");
}

/*
 * The upload is 256 chunks of two writes each. A failure partway through has
 * to stop it, not grind on and report success.
 */
ZTEST_F(bmi260, test_upload_aborts_on_a_bus_error)
{
	const struct device *wrong = DEVICE_DT_GET(DT_NODELABEL(bmi260_wrong));
	const struct emul *target = EMUL_DT_GET(DT_NODELABEL(bmi260_wrong));
	static const uint8_t chip_id = BMI260_CHIP_ID;
	uint32_t uploaded;

	bmi260_emul_set_reg(target, BMI260_REG_CHIP_ID, &chip_id, sizeof(chip_id));
	/*
	 * Counted in register writes, not chunks: the setup writes plus a few
	 * chunks' worth, so the failure lands once the upload is under way.
	 */
	bmi260_emul_fail_write_after(target, 200U);

	zassert_not_equal(device_init(wrong), 0, "a failed upload should fail initialization");

	uploaded = bmi260_emul_get_init_data_byte_count(target);
	zassert_true(uploaded > 0U && uploaded < BMI260_CONFIG_FILE_SIZE,
		     "the upload should have started and then stopped early, sent %u bytes",
		     uploaded);
}

/*
 * A part whose feature engine never reports that it started must leave the
 * device not ready rather than be handed to an application that would read
 * nothing but zeroes from it.
 */
ZTEST_F(bmi260, test_init_fails_when_the_engine_never_starts)
{
	const struct device *stuck = DEVICE_DT_GET(DT_NODELABEL(bmi260_stuck));
	const struct emul *target = EMUL_DT_GET(DT_NODELABEL(bmi260_stuck));
	static const uint8_t chip_id = BMI260_CHIP_ID;
	uint32_t uploaded;

	bmi260_emul_set_reg(target, BMI260_REG_CHIP_ID, &chip_id, sizeof(chip_id));
	bmi260_emul_set_fail_config_load(target, true);

	zassert_equal(device_init(stuck), -EIO,
		      "initialization should report -EIO when INIT_OK never arrives");
	zassert_false(device_is_ready(stuck), "the device should not be ready");

	/*
	 * The whole image still went out: the driver is being failed by the
	 * handshake that follows it, not by a short upload.
	 */
	uploaded = bmi260_emul_get_init_data_byte_count(target);
	zassert_equal(uploaded, BMI260_CONFIG_FILE_SIZE,
		      "the whole image should have been sent, got %u bytes", uploaded);
}

/*
 * At the default 2 g range a full-scale count is 2 g, so half of full scale
 * read back as roughly 1 g. The check is loose because the conversion keeps
 * microsecond-level resolution, not exact fractions.
 */
ZTEST_F(bmi260, test_accel_counts_convert_to_ms2)
{
	struct sensor_value accel[3];

	set_raw_sample(fixture->i2c_target, BMI260_REG_ACC_X_LSB, INT16_MAX / 2);
	set_raw_sample(fixture->i2c_target, BMI260_REG_ACC_Y_LSB, 0);
	set_raw_sample(fixture->i2c_target, BMI260_REG_ACC_Z_LSB, -(INT16_MAX / 2));

	zassert_ok(sensor_sample_fetch(fixture->i2c_dev), "fetch failed");
	zassert_ok(sensor_channel_get(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ, accel),
		   "accel read failed");

	zassert_within(sensor_value_to_double(&accel[0]), 9.80665, 0.1,
		       "X should read about 1 g");
	zassert_within(sensor_value_to_double(&accel[1]), 0.0, 0.01, "Y should read about zero");
	zassert_within(sensor_value_to_double(&accel[2]), -9.80665, 0.1,
		       "Z should read about minus 1 g");
}

/* At the default 2000 dps range, half of full scale is about 1000 dps. */
ZTEST_F(bmi260, test_gyro_counts_convert_to_rad_per_s)
{
	struct sensor_value gyro[3];
	const double thousand_dps = 1000.0 * 3.14159265358979 / 180.0;

	set_raw_sample(fixture->i2c_target, BMI260_REG_GYR_X_LSB, INT16_MAX / 2);
	set_raw_sample(fixture->i2c_target, BMI260_REG_GYR_Y_LSB, 0);
	set_raw_sample(fixture->i2c_target, BMI260_REG_GYR_Z_LSB, -(INT16_MAX / 2));

	zassert_ok(sensor_sample_fetch(fixture->i2c_dev), "fetch failed");
	zassert_ok(sensor_channel_get(fixture->i2c_dev, SENSOR_CHAN_GYRO_XYZ, gyro),
		   "gyro read failed");

	zassert_within(sensor_value_to_double(&gyro[0]), thousand_dps, 0.5,
		       "X should read about 1000 dps");
	zassert_within(sensor_value_to_double(&gyro[1]), 0.0, 0.01, "Y should read about zero");
	zassert_within(sensor_value_to_double(&gyro[2]), -thousand_dps, 0.5,
		       "Z should read about minus 1000 dps");
}

/* The die thermometer counts 512 per degree from a zero that means 23 C. */
ZTEST_F(bmi260, test_die_temp_converts_from_512_lsb_per_degree)
{
	struct sensor_value temp;

	set_raw_sample(fixture->i2c_target, BMI260_REG_TEMPERATURE_0, 512 * 2);

	zassert_ok(sensor_sample_fetch(fixture->i2c_dev), "fetch failed");
	zassert_ok(sensor_channel_get(fixture->i2c_dev, SENSOR_CHAN_DIE_TEMP, &temp),
		   "temperature read failed");

	zassert_within(sensor_value_to_double(&temp), 25.0, 0.01, "two counts above 23 C");
}

/* The device reports a reserved value when it has no temperature to give. */
ZTEST_F(bmi260, test_an_invalid_temperature_reports_enodata)
{
	struct sensor_value temp;

	set_raw_sample(fixture->i2c_target, BMI260_REG_TEMPERATURE_0, (int16_t)BMI260_TEMP_INVALID);

	zassert_ok(sensor_sample_fetch(fixture->i2c_dev), "fetch failed");
	zassert_equal(sensor_channel_get(fixture->i2c_dev, SENSOR_CHAN_DIE_TEMP, &temp), -ENODATA,
		      "an invalid temperature should report -ENODATA");
}

/*
 * A fetch takes one snapshot; reads of it are pure. Changing the device
 * underneath without fetching again must not change what is reported.
 */
ZTEST_F(bmi260, test_channel_get_is_stable_between_fetches)
{
	struct sensor_value before[3];
	struct sensor_value after[3];

	set_raw_sample(fixture->i2c_target, BMI260_REG_ACC_X_LSB, 1000);
	zassert_ok(sensor_sample_fetch(fixture->i2c_dev), "fetch failed");
	zassert_ok(sensor_channel_get(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ, before), "read 1");

	set_raw_sample(fixture->i2c_target, BMI260_REG_ACC_X_LSB, -1000);
	zassert_ok(sensor_channel_get(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ, after), "read 2");

	zassert_equal(before[0].val1, after[0].val1, "the stored sample changed without a fetch");
	zassert_equal(before[0].val2, after[0].val2, "the stored sample changed without a fetch");
}

/*
 * A sensor_value carries its sign on both fields, so a reading between -1 and
 * 0 has val1 == 0 and a negative val2. Getting this wrong flips the sign of
 * every small negative reading.
 */
ZTEST_F(bmi260, test_negative_sub_unit_values_carry_sign_on_both_fields)
{
	struct sensor_value accel[3];

	/* About -0.5 m/s^2: a twentieth of full scale at 2 g. */
	set_raw_sample(fixture->i2c_target, BMI260_REG_ACC_X_LSB, -(INT16_MAX / 39));

	zassert_ok(sensor_sample_fetch(fixture->i2c_dev), "fetch failed");
	zassert_ok(sensor_channel_get(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ, accel), "read");

	zassert_equal(accel[0].val1, 0, "the whole part of a sub-unit value should be zero");
	zassert_true(accel[0].val2 < 0, "the fractional part should carry the sign, got %d",
		     accel[0].val2);
}

/*
 * Each single-axis channel has to report its own axis. The driver reaches the
 * cached counts by subtracting the channel of the first axis, so an off-by-one
 * or a hardcoded index would still satisfy a test that only ever asks for X.
 */
ZTEST_F(bmi260, test_each_single_axis_channel_reports_its_own_axis)
{
	static const enum sensor_channel accel_chans[] = {
		SENSOR_CHAN_ACCEL_X, SENSOR_CHAN_ACCEL_Y, SENSOR_CHAN_ACCEL_Z};
	static const enum sensor_channel gyro_chans[] = {
		SENSOR_CHAN_GYRO_X, SENSOR_CHAN_GYRO_Y, SENSOR_CHAN_GYRO_Z};
	static const uint8_t accel_regs[] = {BMI260_REG_ACC_X_LSB, BMI260_REG_ACC_Y_LSB,
					     BMI260_REG_ACC_Z_LSB};
	static const uint8_t gyro_regs[] = {BMI260_REG_GYR_X_LSB, BMI260_REG_GYR_Y_LSB,
					    BMI260_REG_GYR_Z_LSB};
	struct sensor_value triple[3];
	struct sensor_value single;

	/* Distinct counts per axis, so no two axes can be confused. */
	for (size_t axis = 0; axis < 3; axis++) {
		set_raw_sample(fixture->i2c_target, accel_regs[axis],
			       (int16_t)(1000 * (axis + 1)));
		set_raw_sample(fixture->i2c_target, gyro_regs[axis],
			       (int16_t)(-2000 * (axis + 1)));
	}

	zassert_ok(sensor_sample_fetch(fixture->i2c_dev), "fetch failed");

	zassert_ok(sensor_channel_get(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ, triple),
		   "accel read failed");
	for (size_t axis = 0; axis < ARRAY_SIZE(accel_chans); axis++) {
		zassert_ok(sensor_channel_get(fixture->i2c_dev, accel_chans[axis], &single),
			   "accel axis %zu read failed", axis);
		zassert_equal(single.val1, triple[axis].val1,
			      "accel axis %zu disagrees with the aggregate", axis);
		zassert_equal(single.val2, triple[axis].val2,
			      "accel axis %zu disagrees with the aggregate", axis);
	}

	zassert_ok(sensor_channel_get(fixture->i2c_dev, SENSOR_CHAN_GYRO_XYZ, triple),
		   "gyro read failed");
	for (size_t axis = 0; axis < ARRAY_SIZE(gyro_chans); axis++) {
		zassert_ok(sensor_channel_get(fixture->i2c_dev, gyro_chans[axis], &single),
			   "gyro axis %zu read failed", axis);
		zassert_equal(single.val1, triple[axis].val1,
			      "gyro axis %zu disagrees with the aggregate", axis);
		zassert_equal(single.val2, triple[axis].val2,
			      "gyro axis %zu disagrees with the aggregate", axis);
	}
}

/*
 * Widening the range makes each count worth more, so the same raw reading has
 * to report proportionally larger. A driver that writes the register but keeps
 * converting with the old scale passes every other test and silently reports
 * a quarter of the truth.
 */
ZTEST_F(bmi260, test_setting_the_range_rescales_subsequent_reads)
{
	struct sensor_value narrow;
	struct sensor_value wide;
	struct sensor_value attr;

	set_raw_sample(fixture->i2c_target, BMI260_REG_ACC_X_LSB, INT16_MAX / 2);

	zassert_ok(sensor_sample_fetch(fixture->i2c_dev), "fetch failed");
	zassert_ok(sensor_channel_get(fixture->i2c_dev, SENSOR_CHAN_ACCEL_X, &narrow), "read");

	/* 2 g to 16 g is three doublings. */
	attr.val1 = 16 * SENSOR_G / 1000000;
	attr.val2 = 0;
	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_FULL_SCALE, &attr),
		   "setting the range failed");

	zassert_ok(sensor_sample_fetch(fixture->i2c_dev), "fetch failed");
	zassert_ok(sensor_channel_get(fixture->i2c_dev, SENSOR_CHAN_ACCEL_X, &wide), "read");

	zassert_within(sensor_value_to_double(&wide), sensor_value_to_double(&narrow) * 8.0, 0.5,
		       "a 16 g reading should be eight times the 2 g one");

	/* Put it back so the ordering of the rest of the suite does not matter. */
	attr.val1 = 2 * SENSOR_G / 1000000;
	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_FULL_SCALE, &attr),
		   "restoring the range failed");
}

/* The output data rate lands in the low nibble of ACC_CONF. */
ZTEST_F(bmi260, test_setting_odr_writes_acc_conf)
{
	struct sensor_value attr = {.val1 = 400, .val2 = 0};
	uint8_t acc_conf;

	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &attr),
		   "setting the sampling frequency failed");

	bmi260_emul_get_reg(fixture->i2c_target, BMI260_REG_ACC_CONF, &acc_conf,
			    sizeof(acc_conf));

	zassert_equal(acc_conf & BMI260_ACC_ODR_MSK, BMI260_ACC_ODR_400_HZ,
		      "ACC_CONF holds 0x%02x, expected the 400 Hz code", acc_conf);

	/* Put it back so the rest of the suite sees the configured rate. */
	attr.val1 = 100;
	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &attr),
		   "restoring the sampling frequency failed");
}

/* The gyroscope has its own rate and range registers, reached by channel. */
ZTEST_F(bmi260, test_setting_the_gyro_range_writes_gyr_range)
{
	struct sensor_value attr;
	uint8_t gyr_range;

	/* 500 dps in rad/s, which is what the sensor API carries. */
	attr.val1 = 8;
	attr.val2 = 726646;

	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_GYRO_XYZ,
				   SENSOR_ATTR_FULL_SCALE, &attr),
		   "setting the gyroscope range failed");

	bmi260_emul_get_reg(fixture->i2c_target, BMI260_REG_GYR_RANGE, &gyr_range,
			    sizeof(gyr_range));
	zassert_equal(gyr_range & BMI260_GYR_RANGE_MSK, BMI260_GYR_RANGE_500DPS,
		      "GYR_RANGE holds 0x%02x, expected the 500 dps code", gyr_range);

	/* 2000 dps, the configured default. */
	attr.val1 = 34;
	attr.val2 = 906585;
	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_GYRO_XYZ,
				   SENSOR_ATTR_FULL_SCALE, &attr),
		   "restoring the gyroscope range failed");
}

/*
 * 3200 Hz is a gyroscope-only rate. Accepting it on the accelerometer would
 * write a code the accelerometer reads as something else entirely.
 */
ZTEST_F(bmi260, test_the_gyro_only_rate_is_refused_on_the_accelerometer)
{
	struct sensor_value attr = {.val1 = 3200, .val2 = 0};
	uint8_t gyr_conf;

	zassert_equal(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ,
				      SENSOR_ATTR_SAMPLING_FREQUENCY, &attr),
		      -EINVAL, "3200 Hz should be refused on the accelerometer");

	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_GYRO_XYZ,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &attr),
		   "3200 Hz should be accepted on the gyroscope");

	bmi260_emul_get_reg(fixture->i2c_target, BMI260_REG_GYR_CONF, &gyr_conf,
			    sizeof(gyr_conf));
	zassert_equal(gyr_conf & BMI260_GYR_ODR_MSK, BMI260_GYR_ODR_3200_HZ,
		      "GYR_CONF holds 0x%02x, expected the 3200 Hz code", gyr_conf);

	attr.val1 = 100;
	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_GYRO_XYZ,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &attr),
		   "restoring the gyroscope rate failed");
}

/* A rate the device cannot produce is refused rather than rounded silently. */
ZTEST_F(bmi260, test_an_unsupported_rate_is_rejected)
{
	struct sensor_value attr = {.val1 = 7, .val2 = 0};

	zassert_equal(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ,
				      SENSOR_ATTR_SAMPLING_FREQUENCY, &attr),
		      -EINVAL, "7 Hz is not a rate the device offers");
}

/* Attributes the driver does not implement report that, rather than lying. */
/*
 * Oversampling lands in the bandwidth field, which sits above the rate in the
 * same register, so setting one must not disturb the other.
 */
ZTEST_F(bmi260, test_setting_oversampling_writes_the_bandwidth_field)
{
	struct sensor_value attr = {.val1 = 4, .val2 = 0};
	uint8_t before;
	uint8_t after;

	bmi260_emul_get_reg(fixture->i2c_target, BMI260_REG_ACC_CONF, &before, sizeof(before));

	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_OVERSAMPLING, &attr),
		   "setting oversampling failed");

	bmi260_emul_get_reg(fixture->i2c_target, BMI260_REG_ACC_CONF, &after, sizeof(after));
	zassert_equal(FIELD_GET(BMI260_ACC_BWP_MSK, after), BMI260_ACC_BWP_OSR4_AVG1,
		      "ACC_CONF holds 0x%02x, expected the four times oversampling code", after);
	zassert_equal(after & BMI260_ACC_ODR_MSK, before & BMI260_ACC_ODR_MSK,
		      "the sampling rate should not have moved");

	/* The gyroscope has the same field in its own register. */
	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_GYRO_XYZ,
				   SENSOR_ATTR_OVERSAMPLING, &attr),
		   "setting gyroscope oversampling failed");

	bmi260_emul_get_reg(fixture->i2c_target, BMI260_REG_GYR_CONF, &after, sizeof(after));
	zassert_equal(FIELD_GET(BMI260_GYR_BWP_MSK, after), BMI260_GYR_BWP_OSR4,
		      "GYR_CONF holds 0x%02x, expected the four times oversampling code", after);

	/* Back to no oversampling, so the rest of the suite sees the default. */
	attr.val1 = 1;
	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_OVERSAMPLING, &attr),
		   "restoring oversampling failed");
	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_GYRO_XYZ,
				   SENSOR_ATTR_OVERSAMPLING, &attr),
		   "restoring gyroscope oversampling failed");
}

/* A factor the device cannot produce is refused rather than rounded. */
ZTEST_F(bmi260, test_an_unsupported_oversampling_factor_is_rejected)
{
	struct sensor_value attr = {.val1 = 3, .val2 = 0};

	zassert_equal(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ,
				      SENSOR_ATTR_OVERSAMPLING, &attr),
		      -EINVAL, "an oversampling factor of three should be refused");
}

ZTEST_F(bmi260, test_an_unsupported_attribute_is_rejected)
{
	struct sensor_value attr = {.val1 = 1, .val2 = 0};

	zassert_equal(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ACCEL_XYZ,
				      SENSOR_ATTR_SLOPE_TH, &attr),
		      -ENOTSUP, "an unimplemented attribute should report -ENOTSUP");
}

#if defined(CONFIG_BMI260_TRIGGER)

static const struct gpio_dt_spec irq_line =
	GPIO_DT_SPEC_GET(DT_NODELABEL(bmi260_i2c), irq_gpios);

static unsigned int handler_calls;

static void count_trigger(const struct device *dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);

	handler_calls++;
}

/*
 * Walks the line through the edge the driver arms itself for, having first
 * given the device a reason to raise it. The status register latches until it
 * is read, the way the real one does, so each edge needs its own cause.
 */
static void pulse_irq(const struct emul *target)
{
	static const uint8_t drdy = BMI260_INT_STATUS_1_ACC_DRDY_INT |
				    BMI260_INT_STATUS_1_GYR_DRDY_INT;

	bmi260_emul_set_reg(target, BMI260_REG_INT_STATUS_1, &drdy, sizeof(drdy));

	zassert_ok(gpio_emul_input_set(irq_line.port, irq_line.pin, 0));
	k_msleep(5);
	zassert_ok(gpio_emul_input_set(irq_line.port, irq_line.pin, 1));
	k_msleep(5);
}

/* An edge with nothing behind it must not be reported as a sample. */
static void pulse_irq_with_no_cause(void)
{
	zassert_ok(gpio_emul_input_set(irq_line.port, irq_line.pin, 0));
	k_msleep(5);
	zassert_ok(gpio_emul_input_set(irq_line.port, irq_line.pin, 1));
	k_msleep(5);
}

/* The trigger has to reach the application, and keep reaching it. */
ZTEST_F(bmi260, test_data_ready_reaches_the_handler)
{
	static const struct sensor_trigger drdy = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	handler_calls = 0;
	zassert_ok(sensor_trigger_set(fixture->i2c_dev, &drdy, count_trigger),
		   "the data ready trigger should be accepted");

	pulse_irq(fixture->i2c_target);
	zassert_equal(handler_calls, 1, "the handler should have run once");

	/* A second edge proves the interrupt was re-armed, not left masked. */
	pulse_irq(fixture->i2c_target);
	zassert_equal(handler_calls, 2, "the handler should have run again");

	zassert_ok(sensor_trigger_set(fixture->i2c_dev, &drdy, NULL), "removing the handler");
}

/*
 * The line and the cause are separate things. A driver that dispatches on the
 * edge alone would report samples the device never took.
 */
ZTEST_F(bmi260, test_an_edge_without_data_ready_is_ignored)
{
	static const struct sensor_trigger drdy = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	zassert_ok(sensor_trigger_set(fixture->i2c_dev, &drdy, count_trigger), "arming");

	handler_calls = 0;
	pulse_irq_with_no_cause();
	zassert_equal(handler_calls, 0, "an edge with no data ready flag should be ignored, "
					"handler ran %u time(s)",
		      handler_calls);

	zassert_ok(sensor_trigger_set(fixture->i2c_dev, &drdy, NULL), "disarming");
}

/*
 * Handing back a null handler has to actually detach it. Leaving a stale
 * pointer armed is a use-after-free waiting for the application to unload it.
 */
ZTEST_F(bmi260, test_a_removed_handler_is_not_called)
{
	static const struct sensor_trigger drdy = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	zassert_ok(sensor_trigger_set(fixture->i2c_dev, &drdy, count_trigger), "arming");
	zassert_ok(sensor_trigger_set(fixture->i2c_dev, &drdy, NULL), "disarming");

	handler_calls = 0;
	pulse_irq(fixture->i2c_target);

	zassert_equal(handler_calls, 0, "a removed handler should not run");
}

/* Triggers the device cannot raise are refused. */
ZTEST_F(bmi260, test_an_unsupported_trigger_is_rejected)
{
	static const struct sensor_trigger tap = {
		.type = SENSOR_TRIG_TAP,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	zassert_equal(sensor_trigger_set(fixture->i2c_dev, &tap, count_trigger), -ENOTSUP,
		      "an unsupported trigger type should report -ENOTSUP");
}

/*
 * The SPI node in these overlays describes an interrupt line, but a board that
 * leaves the pin unrouted gets a driver that still works by polling.
 */
ZTEST_F(bmi260, test_trigger_set_reports_enotsup_without_a_line)
{
	static const struct sensor_trigger drdy = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};
	const struct device *no_irq = DEVICE_DT_GET(DT_NODELABEL(bmi260_wrong));

	/*
	 * bmi260_wrong describes no irq-gpios. The refusal comes from the
	 * configuration alone, without touching the device, so it holds
	 * whether or not the node was ever initialised.
	 */
	zassert_equal(sensor_trigger_set(no_irq, &drdy, count_trigger), -ENOTSUP,
		      "a node with no interrupt line cannot offer a trigger");
}

#endif /* CONFIG_BMI260_TRIGGER */

/*
 * A value the driver writes has to arrive in the register it named, on both
 * buses: this is what proves the I2C address-then-payload framing and the SPI
 * address masking put the byte where the device expects it.
 */
ZTEST_F(bmi260, test_a_driver_write_reaches_the_device)
{
	const struct device *const devs[] = {fixture->i2c_dev, fixture->spi_dev};
	const struct emul *const targets[] = {fixture->i2c_target, fixture->spi_target};
	struct sensor_value range;
	uint8_t written;

	for (size_t i = 0; i < ARRAY_SIZE(devs); i++) {
		range.val1 = 8 * SENSOR_G / 1000000;
		range.val2 = 0;
		zassert_ok(sensor_attr_set(devs[i], SENSOR_CHAN_ACCEL_XYZ,
					   SENSOR_ATTR_FULL_SCALE, &range),
			   "setting the range failed on instance %zu", i);

		bmi260_emul_get_reg(targets[i], BMI260_REG_ACC_RANGE, &written, sizeof(written));
		zassert_equal(written, BMI260_ACC_RANGE_8G,
			      "instance %zu wrote 0x%02x to ACC_RANGE, not the 8 g field", i,
			      written);

		range.val1 = 2 * SENSOR_G / 1000000;
		zassert_ok(sensor_attr_set(devs[i], SENSOR_CHAN_ACCEL_XYZ,
					   SENSOR_ATTR_FULL_SCALE, &range),
			   "restoring the range failed on instance %zu", i);
	}
}
