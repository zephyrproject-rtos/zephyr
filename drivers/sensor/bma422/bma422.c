/* Copyright 2023 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "accelgyro.h"
#include "common.h"
#include "bma422_accel.h"
#include "bma422_emul.h"
#include "emul/emul_common_i2c.h"
#include "i2c.h"
#include "motion_sense.h"
#include "motion_sense_fifo.h"
#include "test/drivers/test_state.h"

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#define EMUL EMUL_DT_GET(DT_NODELABEL(bma422_emul))
#define SENSOR bma422_emul_get_sensor_data(EMUL)

LOG_MODULE_REGISTER(test_bma422, LOG_LEVEL_INF);

ZTEST_SUITE(bma422, drivers_predicate_post_main, NULL, NULL, NULL, NULL);

static int read_wrong_chip_id(const struct emul *target, int reg, uint8_t *val,
			      int bytes, void *data)
{
	if (reg == BMA422_CHIP_ID_ADDR) {
		*val = 0x13;
		return 0;
	}
	return 1;
}

ZTEST_USER(bma422, test_init)
{
	/* Basic initialization works. */
	zassert_ok(bma422_accel_drv.init(SENSOR));

	/* Sensor gets turned off if it was already on. */
	bma422_emul_set_accel_enabled(EMUL, true);
	zassert_ok(bma422_accel_drv.init(SENSOR));
	zassert_false(bma422_emul_is_accel_enabled(EMUL));

	/* Unexpected chip ID is an error. */
	i2c_common_emul_set_read_func(bma422_emul_get_i2c(EMUL),
				      read_wrong_chip_id, NULL);
	zassert_equal(EC_ERROR_HW_INTERNAL, bma422_accel_drv.init(SENSOR));
}

ZTEST_USER(bma422, test_set_range)
{
	/* Sets to ±16g successfully. */
	zassert_ok(bma422_accel_drv.set_range(SENSOR, 16, 0));
	zassert_equal(16, bma422_emul_get_accel_range(EMUL));

	/* ±3g with roundup flag is ±4. */
	zassert_ok(bma422_accel_drv.set_range(SENSOR, 3, 1));
	zassert_equal(4, bma422_emul_get_accel_range(EMUL));

	/* .. ±2g without roundup flag. */
	zassert_ok(bma422_accel_drv.set_range(SENSOR, 3, 0));
	zassert_equal(2, bma422_emul_get_accel_range(EMUL));

	/* Communication errors bubble up and don't change the range. */
	i2c_common_emul_set_write_fail_reg(bma422_emul_get_i2c(EMUL),
					   BMA422_ACCEL_RANGE_ADDR);
	zassert_not_equal(0, bma422_accel_drv.set_range(SENSOR, 8, 0));
	zassert_equal(2, bma422_emul_get_accel_range(EMUL));
}

ZTEST_USER(bma422, test_data_rate)
{
	int odr;

	/* Requesting zero ODR disables the sensor. */
	bma422_emul_set_accel_enabled(EMUL, true);
	zassert_ok(bma422_accel_drv.set_data_rate(SENSOR, 0, 0));
	zassert_false(bma422_emul_is_accel_enabled(EMUL));

	/*
	 * Minimum supported ODR is 0.78125 Hz; smaller requested values should
	 * still yield a nonzero ODR and enable a previously-disabled sensor.
	 */
	zassert_ok(bma422_accel_drv.set_data_rate(SENSOR, 200, 0));
	zassert_true(bma422_emul_is_accel_enabled(EMUL));
	odr = bma422_accel_drv.get_data_rate(SENSOR);
	zassert_equal(781, odr, "actual reported data rate was %d mHz", odr);
	zassert_equal(781, bma422_emul_get_odr(EMUL),
		      "emulator ODR did not match driver ODR");

	/*
	 * Faster than can be supported goes to the maximum possible ODR. 4 kHz
	 * rounds down to 3.2 kHz which is still too high, so we actually get
	 * 1.6 kHz.
	 */
	zassert_ok(bma422_accel_drv.set_data_rate(SENSOR, 4000 * 1000, 0));
	odr = bma422_accel_drv.get_data_rate(SENSOR);
	zassert_equal(1600 * 1000, odr, "actual reported data rate was %d mHz",
		      odr);
	zassert_equal(1600 * 1000, bma422_emul_get_odr(EMUL),
		      "emulator ODR did not match driver ODR");

	/* Rounds up only if requested, otherwise down. */
	zassert_ok(bma422_accel_drv.set_data_rate(SENSOR, 160 * 1000, 0));
	odr = bma422_emul_get_odr(EMUL);
	zassert_equal(100 * 1000, odr, "actual ODR was %d", odr);
	zassert_ok(bma422_accel_drv.set_data_rate(SENSOR, 160 * 1000, 1));
	odr = bma422_emul_get_odr(EMUL);
	zassert_equal(200 * 1000, odr, "actual ODR was %d", odr);

	/* Communication errors bubble up and reported ODR is unchanged. */
	i2c_common_emul_set_write_fail_reg(bma422_emul_get_i2c(EMUL),
					   BMA422_ACCEL_CONFIG_ADDR);
	zassert_not_equal(0,
			  bma422_accel_drv.set_data_rate(SENSOR, 100 * 1000, 0));
	zassert_equal(200 * 1000, bma422_accel_drv.get_data_rate(SENSOR));
}

ZTEST_USER(bma422, test_read_data)
{
	intv3_t acceleration;

	bma422_emul_set_accel_data(EMUL, 627, 1, -809);
	zassert_ok(bma422_accel_drv.read(SENSOR, acceleration));
	/*
	 * read() returns raw sensor data, which is shifted. The consumer of the
	 * data is expected to know this and possibly also compensate for
	 * scaling when the sensitivity changes.
	 */
	zassert_equal(acceleration[0], 627 << 4, "actual value was %d",
		      acceleration[0]);
	zassert_equal(acceleration[1], 1 << 4, "actual value was %d",
		      acceleration[1]);
	zassert_equal(acceleration[2], -(809 << 4), "actual value was %d",
		      acceleration[2]);

	/* Communication errors bubble up. */
	i2c_common_emul_set_read_fail_reg(bma422_emul_get_i2c(EMUL),
					  BMA422_DATA_8_ADDR);
	zassert_not_equal(bma422_accel_drv.read(SENSOR, acceleration), 0);
}

ZTEST_USER(bma422, test_offset)
{
	int16_t offsets[] = { 4, -40, 400 };
	int8_t offset_regs[3];
	uint16_t temperature;

	/* set_offset writes offsets successfully. */
	zassert_ok(bma422_accel_drv.set_offset(SENSOR, offsets, 0));
	bma422_emul_get_offset(EMUL, &offset_regs);
	/*
	 * Parameter to set_offset is in milli-g, but register values are 3.9mg
	 * per LSb; driver scales the inputs as needed, rounding to nearest
	 * integer.
	 */
	zassert_equal(offset_regs[0], 1, "actual value is %d", offset_regs[0]);
	zassert_equal(offset_regs[1], -10, "actual value is %d",
		      offset_regs[1]);
	zassert_equal(offset_regs[2], 103, "actual value is %d",
		      offset_regs[2]);

	/* get_offset agrees with the programmed offsets, scaled back to mg. */
	memset(offsets, 0, sizeof(offsets));
	zassert_ok(bma422_accel_drv.get_offset(SENSOR, offsets, &temperature));
	zassert_equal(offsets[0], 4, "actual value is %d", offsets[0]);
	zassert_equal(offsets[1], -39, "actual value is %d", offsets[1]);
	zassert_equal(offsets[2], 402, "actual value is %d", offsets[2]);

	/* Out of range offsets are clamped to the accepted range. */
	offsets[0] = 684;
	offsets[1] = -800;
	offsets[2] = 0;
	zassert_ok(bma422_accel_drv.set_offset(SENSOR, offsets, 0));
	bma422_emul_get_offset(EMUL, &offset_regs);
	zassert_equal(offset_regs[0], 127);
	zassert_equal(offset_regs[1], -128);
	zassert_equal(offset_regs[2], 0);

	/* Communication errors bubble up. */
	i2c_common_emul_set_write_fail_reg(bma422_emul_get_i2c(EMUL),
					   BMA422_OFFSET_1_ADDR);
	zassert_not_equal(bma422_accel_drv.set_offset(SENSOR, offsets, 0), 0);
	i2c_common_emul_set_read_fail_reg(bma422_emul_get_i2c(EMUL),
					  BMA422_OFFSET_0_ADDR);
	zassert_not_equal(
		bma422_accel_drv.get_offset(SENSOR, offsets, &temperature), 0);
}

ZTEST_USER(bma422, test_resolution)
{
	/* Resolution is always 12 bits. */
	zassert_equal(bma422_accel_drv.get_resolution(SENSOR), 12);
}

ZTEST_USER(bma422, test_interrupt)
{
	uint8_t int_map_data, int1_io_ctrl;
	bool int_latched_mode;
	const uint8_t two_frames_data[] = {
		4 << 4,
		0,
		(42 & 0xF) << 4,
		42 >> 4,
		(421 & 0xF) << 4,
		421 >> 4,
		0,
		0,
		0,
		0,
		1 << 4,
		0,
	};
	uint32_t evt = CONFIG_ACCEL_BMA422_INT_EVENT;
	struct ec_response_motion_sensor_data host_data[8];
	int host_frames_read;
	uint16_t host_data_sz;

	/* Run init to get interrupts configured. */
	zassert_ok(bma422_accel_drv.init(SENSOR));
	/* Verify interrupt configuration is as expected. */
	zassert_true(bma422_emul_is_fifo_enabled(EMUL));
	int_map_data = bma422_emul_get_interrupt_config(EMUL, &int1_io_ctrl,
							&int_latched_mode);
	zassert_equal(int_map_data, 0x07); /* all interrupts on INT1 */
	/* INT1 output enabled, active-low push-pull. */
	zassert_equal(int1_io_ctrl, 0x08);
	zassert_true(int_latched_mode);

	/* Ensure every sample gets queued to the AP. */
	SENSOR->oversampling_ratio = 1;

	/* Queue two samples of data and run the interrupt handler. */
	bma422_emul_set_fifo_data(EMUL, two_frames_data,
				  sizeof(two_frames_data));
	zassert_ok(bma422_accel_drv.irq_handler(SENSOR, &evt));

	/*
	 * Retrieve the data that should have been queued for the host by the
	 * interrupt handler.
	 */
	host_frames_read = motion_sense_fifo_read(sizeof(host_data),
						  ARRAY_SIZE(host_data),
						  &host_data, &host_data_sz);
	BUILD_ASSERT(ARRAY_SIZE(host_data) > 4,
		     "must be possible to retrieve "
		     "more than the expected number of frames");
	zassert_equal(host_frames_read, 4,
		      "actually put %d frames into the FIFO", host_frames_read);

	/* First frame is a timestamp which we don't care about. */
	zassert_equal(host_data[0].flags, MOTIONSENSE_SENSOR_FLAG_TIMESTAMP);
	/* Next is the first sample from the sensor. */
	zassert_equal(host_data[1].flags, 0);
	zassert_equal(host_data[1].sensor_num, bma422_emul_get_sensor_num(EMUL),
		      "data was generated by an unexpected sensor");
	zassert_equal(host_data[1].data[0], 0x40, "X accel was %d",
		      host_data[1].data[0]);
	zassert_equal(host_data[1].data[1], 42 << 4, "Y accel was %d",
		      host_data[1].data[1]);
	zassert_equal(host_data[1].data[2], 421 << 4, "Z accel was %d",
		      host_data[1].data[2]);
	/* Another uninteresting timestamp frame. */
	zassert_equal(host_data[2].flags, MOTIONSENSE_SENSOR_FLAG_TIMESTAMP);
	/* And the second sample from the sensor. */
	zassert_equal(host_data[3].flags, 0);
	zassert_equal(host_data[3].sensor_num, bma422_emul_get_sensor_num(EMUL),
		      "data was generated by an unexpected sensor");
	zassert_equal(host_data[3].data[0], 0, "X accel was %d",
		      host_data[3].data[0]);
	zassert_equal(host_data[3].data[1], 0, "Y accel was %d",
		      host_data[3].data[1]);
	zassert_equal(host_data[3].data[2], 1 << 4, "Z accel was %d",
		      host_data[3].data[2]);
}

/**
 * This struct captures I2C transactions during the calibration test.
 *
 * Using a bespoke log like this rather than <zephyr/fff.h> fakes is simpler
 * to understand because it records only the relevant information. The
 * calibration_read_byte and calibration_write_byte functions are responsible
 * for logging each read or write transaction into a log, for inspection later.
 */
struct calibration_log {
	struct {
		uint8_t addr;
		bool write;
		uint8_t write_val;
	} activity[256];
	size_t count;
};

static int calibration_read_byte(const struct emul *emul, int reg, uint8_t *val,
				 int bytes, void *ctx)
{
	struct calibration_log *log = ctx;
	/* Sensor is reading 0, 0, +0.992g at 4g sensitivity. */
	const uint8_t acc_data[6] = { 0, 0, 0, 0, (508 & 0xF) << 4, 508 >> 4 };

	zassert_true(log->count <= ARRAY_SIZE(log->activity),
		     "i2c log overflowed");

	reg += bytes; /* burst reads */
	log->activity[log->count].write = false;
	log->activity[log->count].addr = reg;
	log->count++;

	switch (reg) {
	case BMA422_STATUS_ADDR:
		*val = 0x90; /* drdy_acc + cmd_rdy */
		break;
	case BMA422_DATA_8_ADDR:
	case BMA422_DATA_8_ADDR + 1:
	case BMA422_DATA_8_ADDR + 2:
	case BMA422_DATA_8_ADDR + 3:
	case BMA422_DATA_8_ADDR + 4:
	case BMA422_DATA_8_ADDR + 5:
		*val = acc_data[reg - BMA422_DATA_8_ADDR];
		break;
	default:
		*val = 0;
	}
	return 0;
}

static int calibration_write_byte(const struct emul *emul, int reg, uint8_t val,
				  int bytes, void *ctx)
{
	struct calibration_log *log = ctx;

	__ASSERT_NO_MSG(bytes >= 1);
	reg += bytes - 1; /* burst writes */

	log->activity[log->count].write = true;
	log->activity[log->count].addr = reg;
	log->activity[log->count].write_val = val;
	log->count++;

	return 0;
}

ZTEST_USER(bma422, test_calibration)
{
	struct calibration_log log = {
		.count = 0,
	};
	size_t log_asserted = 0;

/**
 * Macro that walks through the calibration log, asserting on each operation.
 *
 * Each use of ASSERT_NEXT consumes the oldest entry (increments log_asserted)
 * and checks that entry for being the correct kind of transaction against the
 * expected register, and that the expected value was written if it was a write.
 */
#define ASSERT_NEXT(want_write, want_addr, want_write_val)                     \
	do {                                                                   \
		zassert_true(log_asserted < log.count,                         \
			     "no more logs to check (have %d entries)",        \
			     log.count);                                       \
                                                                               \
		bool was_write = log.activity[log_asserted].write;             \
		zassert_equal(was_write, want_write,                           \
			      "transaction at index %d was a %s",              \
			      log_asserted, was_write ? "write" : "read");     \
                                                                               \
		uint8_t addr = log.activity[log_asserted].addr;                \
		zassert_equal(addr, want_addr,                                 \
			      "transaction at index %d was with register %#x", \
			      log_asserted, addr);                             \
                                                                               \
		if (want_write) {                                              \
			uint8_t write_val =                                    \
				log.activity[log_asserted].write_val;          \
			zassert_equal(write_val, want_write_val,               \
				      "write at index %d wrote value %#x",     \
				      log_asserted, write_val);                \
		}                                                              \
		log_asserted++;                                                \
	} while (0)

#define ASSERT_R(addr) ASSERT_NEXT(false, addr, 0)
#define ASSERT_W(addr, val) ASSERT_NEXT(true, addr, val)
#define ASSERT_RW(addr, val)         \
	do {                         \
		ASSERT_R(addr);      \
		ASSERT_W(addr, val); \
	} while (0)

	/* Range must be set before calibration. */
	zassert_ok(bma422_accel_drv.set_range(SENSOR, 4, 0));

	/* Record transactions in the log. */
	i2c_common_emul_set_read_func(bma422_emul_get_i2c(EMUL),
				      calibration_read_byte, &log);
	i2c_common_emul_set_write_func(bma422_emul_get_i2c(EMUL),
				       calibration_write_byte, &log);

	zassert_ok(bma422_accel_drv.perform_calib(SENSOR, 1));

	/* Log messages help determine where failures occur when they happen. */
	LOG_INF("Verify configuration readout");
	/* Read current configuration to store back on completion. */
	ASSERT_R(BMA422_ACCEL_CONFIG_ADDR);
	ASSERT_R(BMA422_ACCEL_RANGE_ADDR);
	ASSERT_R(BMA422_FIFO_CONFIG_1_ADDR);
	ASSERT_R(BMA422_POWER_CTRL_ADDR);
	ASSERT_R(BMA422_POWER_CONF_ADDR);
	LOG_INF("Verify configuration setting for calibration");
	/* Disabled offset. */
	ASSERT_RW(BMA422_NV_CONFIG_ADDR, 0);
	/* Disabled FIFO. */
	ASSERT_W(BMA422_FIFO_CONFIG_1_ADDR, 0);
	/* Configured to 50 Hz continuous mode. */
	ASSERT_W(BMA422_ACCEL_CONFIG_ADDR, 0xb7);
	/* Enabled sensor. */
	ASSERT_RW(BMA422_POWER_CTRL_ADDR, 0x04);
	/* Power save mode disabled. */
	ASSERT_RW(BMA422_POWER_CONF_ADDR, 0);

	/* Reads 32 samples, polling for data ready before each read. */
	LOG_INF("Verify calibration data capture");
	for (int i = 0; i < 32; i++) {
		ASSERT_R(BMA422_STATUS_ADDR);
		ASSERT_R(BMA422_DATA_8_ADDR);
		ASSERT_R(BMA422_DATA_8_ADDR + 1);
		ASSERT_R(BMA422_DATA_8_ADDR + 2);
		ASSERT_R(BMA422_DATA_8_ADDR + 3);
		ASSERT_R(BMA422_DATA_8_ADDR + 4);
		ASSERT_R(BMA422_DATA_8_ADDR + 5);
	}

	/*
	 * Wrote new offset. It's a small positive Z axis offset since we're
	 * reading not quite 1g on Z.
	 */
	LOG_INF("Verify expected offset applied");
	ASSERT_W(BMA422_OFFSET_0_ADDR, 0);
	ASSERT_W(BMA422_OFFSET_1_ADDR, 0);
	ASSERT_W(BMA422_OFFSET_2_ADDR, 0x1);

	LOG_INF("Verify applied final configuration");
	/* Re-enabled offset. */
	ASSERT_RW(BMA422_NV_CONFIG_ADDR, BMA422_NV_ACCEL_OFFSET_MSK);
	/* Restored the original configuration. */
	ASSERT_W(BMA422_ACCEL_CONFIG_ADDR, 0);
	ASSERT_W(BMA422_ACCEL_RANGE_ADDR, 0);
	ASSERT_W(BMA422_FIFO_CONFIG_1_ADDR, 0);
	ASSERT_W(BMA422_POWER_CTRL_ADDR, 0);
	ASSERT_W(BMA422_POWER_CONF_ADDR, 0);

	zassert_equal(log_asserted, log.count,
		      "logged %d transactions, but only %d were checked",
		      log.count, log_asserted);
}