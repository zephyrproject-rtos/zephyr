/*
 * Copyright (c) 2026 Analog Devices Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Ztest suite for the MAXM86161 optical biosensor driver.
 * Supports both real hardware and emulated (native_sim) targets.
 *
 * Two twister scenarios build this file (see tests.yaml):
 *   - drivers.sensor.maxm86161.emul   : core config/attr paths (no trigger)
 *   - drivers.sensor.maxm86161.stream : trigger + RTIO streaming + decoder
 * Tests that need the trigger/stream stack are guarded with
 * CONFIG_MAXM86161_TRIGGER / CONFIG_MAXM86161_STREAM so they compile only in
 * the stream scenario.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/maxm86161.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "maxm86161.h"

#ifdef CONFIG_EMUL
#include <zephyr/drivers/emul.h>
#include "maxm86161_emul.h"
#endif

#ifdef CONFIG_MAXM86161_TRIGGER
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#endif

#ifdef CONFIG_MAXM86161_STREAM
#include <zephyr/rtio/rtio.h>
#endif

#define MAXM86161_NODE DT_NODELABEL(maxm86161_test)

/*
 * Every MAXM86161 attribute is accessed through the single PPG configuration
 * channel; the driver rejects any other channel for attr_get/attr_set.
 */
#define MAXM86161_ATTR_CHAN ((enum sensor_channel)SENSOR_CHAN_MAXM86161_PPG)

struct maxm86161_fixture {
	const struct device *dev;
#ifdef CONFIG_EMUL
	const struct emul *mock;
#endif
};

static struct maxm86161_fixture test_fixture;

/*
 * Set an attribute then read it back and assert the value round-trips. Use only
 * for readable, non-self-clearing attributes; self-clearing / write-only bits
 * (reset, FIFO flush, DAC calibration, SW force sync) will not read back the
 * written value and must be exercised with attr_set_only().
 */
static void attr_set_validate(const struct device *dev, int32_t attr, int32_t value,
			      const char *label)
{
	struct sensor_value set_val = {.val1 = value, .val2 = 0};
	struct sensor_value get_val = {0};

	zassert_ok(sensor_attr_set(dev, MAXM86161_ATTR_CHAN, (enum sensor_attribute)attr, &set_val),
		   "Failed to set %s", label);
	zassert_ok(sensor_attr_get(dev, MAXM86161_ATTR_CHAN, (enum sensor_attribute)attr, &get_val),
		   "Failed to get %s", label);
	zassert_equal(get_val.val1, value,
		      "Readback mismatch for %s: set %d, got %d", label, value,
		      get_val.val1);
}

/* Write a self-clearing / write-only attribute without read-back validation. */
static void attr_set_only(const struct device *dev, int32_t attr, int32_t value, const char *label)
{
	struct sensor_value set_val = {.val1 = value, .val2 = 0};

	zassert_ok(sensor_attr_set(dev, MAXM86161_ATTR_CHAN, (enum sensor_attribute)attr, &set_val),
		   "Failed to set %s", label);
}

static void *maxm86161_setup(void)
{
	test_fixture.dev = DEVICE_DT_GET(MAXM86161_NODE);
#ifdef CONFIG_EMUL
	test_fixture.mock = EMUL_DT_GET(MAXM86161_NODE);
#endif

	zassert_not_null(test_fixture.dev, "Device not found");
#ifdef CONFIG_EMUL
	zassert_not_null(test_fixture.mock, "Emulator not found");
#endif
	return &test_fixture;
}

/*
 * Reset emulator-controlled test state before each test so error-injection and
 * held self-clearing bits from one test never leak into the next.
 */
static void maxm86161_before(void *f)
{
	ARG_UNUSED(f);
#ifdef CONFIG_EMUL
	void *data = test_fixture.mock->data;

	maxm86161_mock_clear_fault(data);
	maxm86161_mock_set_selfclear(data, true);
	/* Clear volatile status / FIFO-count registers between tests. */
	maxm86161_mock_set_register(data, MAXM86161_REG_INT_STATUS1, 0);
	maxm86161_mock_set_register(data, MAXM86161_REG_INT_STATUS2, 0);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_OVF_COUNTER, 0);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_COUNTER, 0);
#endif
}

ZTEST_F(maxm86161, test_device_ready)
{
	zassert_true(device_is_ready(fixture->dev),
		     "MAXM86161 device not ready");
}

ZTEST_F(maxm86161, test_part_id)
{
	struct sensor_value val;

	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_CHIP_ID,
				   &val),
		   "Failed to get part ID");
	zassert_equal(val.val1, MAXM86161_PART_ID_VAL,
		      "Expected part ID 0x%02x, got 0x%02x",
		      MAXM86161_PART_ID_VAL, val.val1);
}

#ifdef CONFIG_EMUL
/* SINGLE_PPG must be set and the device must be left enabled (SHDN cleared) after probe. */
ZTEST_F(maxm86161, test_init_system_control_state)
{
	uint8_t reg;

	zassert_ok(maxm86161_mock_get_register(fixture->mock->data,
					       MAXM86161_REG_SYSTEM_CONTROL, &reg),
		   "Failed to read SYSTEM_CONTROL");
	zassert_true(reg & MAXM86161_MSK_SYSTEM_CONTROL_SINGLE_PPG,
		     "SINGLE_PPG not set after init");
	zassert_false(reg & MAXM86161_MSK_SYSTEM_CONTROL_SHDN,
		      "Device still shut down after init");
}

/*
 * the software-reset attribute re-runs the probe/config sequence.
 * After a reset the device must again be configured and enabled.
 *
 * Restricted to the no-trigger (emul) scenario: re-running probe at runtime
 * re-initialises the interrupt thread, so it must not be exercised while the
 * own-thread trigger stack is active.
 */
#ifndef CONFIG_MAXM86161_TRIGGER
ZTEST_F(maxm86161, test_reset_reconfigures)
{
	struct sensor_value val = {.val1 = 1};
	uint8_t reg;

	/* Corrupt SYSTEM_CONTROL so we can prove reset rewrote it. */
	zassert_ok(maxm86161_mock_set_register(fixture->mock->data,
					       MAXM86161_REG_SYSTEM_CONTROL, 0xFF),
		   "Failed to seed SYSTEM_CONTROL");

	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_RESET, &val),
		   "Reset attribute failed");

	zassert_ok(maxm86161_mock_get_register(fixture->mock->data,
					       MAXM86161_REG_SYSTEM_CONTROL, &reg),
		   "Failed to read SYSTEM_CONTROL after reset");
	zassert_true(reg & MAXM86161_MSK_SYSTEM_CONTROL_SINGLE_PPG,
		     "SINGLE_PPG not re-set after reset");
	zassert_false(reg & MAXM86161_MSK_SYSTEM_CONTROL_SHDN,
		      "Device left shut down after reset");
}
#endif /* !CONFIG_MAXM86161_TRIGGER */
#endif /* CONFIG_EMUL */

ZTEST_F(maxm86161, test_attr_set_sampling_rate)
{
	struct sensor_value val;

	val.val1 = PPG_SR_50p027SPS_1PPS;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_SAMPLING_FREQUENCY,
				   &val),
		   "Failed to set valid sampling rate");

	val.val1 = PPG_SR_4096SPS_1PPS;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_SAMPLING_FREQUENCY,
				   &val),
		   "Failed to set max sampling rate");

	/* Invalid: beyond the enum range */
	val.val1 = 0xFF;
	zassert_equal(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				      (enum sensor_attribute)SENSOR_ATTR_SAMPLING_FREQUENCY,
				      &val),
		      -EINVAL, "Expected -EINVAL for invalid sampling rate");
}

ZTEST_F(maxm86161, test_attr_set_adc_range)
{
	struct sensor_value val;

	val.val1 = ADC_RANGE_4096NA;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_ADC_RANGE,
				   &val),
		   "Failed to set ADC range 4096nA");

	val.val1 = ADC_RANGE_32768NA;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_ADC_RANGE,
				   &val),
		   "Failed to set ADC range 32768nA");

	/* Invalid */
	val.val1 = 0x10;
	zassert_equal(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				      (enum sensor_attribute)SENSOR_ATTR_MAXM86161_ADC_RANGE,
				      &val),
		      -EINVAL, "Expected -EINVAL for invalid ADC range");
}

ZTEST_F(maxm86161, test_attr_set_led_current)
{
	struct sensor_value val;

	val.val1 = 128;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_LED1_GREEN_CURRENT,
				   &val),
		   "Failed to set LED1 green current");

	val.val1 = 64;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_LED2_IR_CURRENT,
				   &val),
		   "Failed to set LED2 IR current");

	val.val1 = 200;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_LED3_RED_CURRENT,
				   &val),
		   "Failed to set LED3 red current");
}

ZTEST_F(maxm86161, test_attr_set_led_sequence)
{
	struct sensor_value val;

	val.val1 = LED_SEQ_GREEN_LED;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_LED_SEQUENCE_1,
				   &val),
		   "Failed to set LED seq 1 to green");

	val.val1 = LED_SEQ_IR_LED;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_LED_SEQUENCE_2,
				   &val),
		   "Failed to set LED seq 2 to IR");

	val.val1 = LED_SEQ_RED_LED;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_LED_SEQUENCE_3,
				   &val),
		   "Failed to set LED seq 3 to red");

	val.val1 = LED_SEQ_NONE;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_LED_SEQUENCE_4,
				   &val),
		   "Failed to set LED seq 4 to none");
}

/*
 * setting the pilot-on-green exposure exercises the proximity mapping
 * branch of the LED-state map.
 */
ZTEST_F(maxm86161, test_attr_set_led_sequence_pilot)
{
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED_SEQUENCE_1,
			  LED_SEQ_PILOT_ON_GREEN_LED, "led_sequence_1 pilot-on-green");
	/* restore a normal green exposure for later fetch tests */
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED_SEQUENCE_1,
			  LED_SEQ_GREEN_LED, "led_sequence_1 restore green");
}

/*
 * The watermark attribute is expressed in FIFO samples remaining. The driver
 * converts it into the device's A_FULL field, which counts empty slots, as
 * CLAMP(FIFO_DEPTH - watermark, 1, FIFO_WMARK_MAX). Out-of-range requests are
 * clamped rather than rejected, so every non-negative input succeeds.
 */
ZTEST_F(maxm86161, test_attr_set_fifo_threshold)
{
	struct sensor_value val;

	/* Typical watermark: A_FULL = 128 - 15 = 113 */
	val.val1 = 15;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_WATERMARK,
				   &val),
		   "Failed to set FIFO watermark 15");

	/* Smallest watermark maps to the largest A_FULL: 128 - 1 = 127. */
	val.val1 = 1;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_WATERMARK,
				   &val),
		   "Failed to set FIFO watermark 1");

	/* Full-depth watermark maps to the smallest A_FULL: 128 - 127 = 1. */
	val.val1 = MAXM86161_FIFO_WMARK_MAX;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_WATERMARK,
				   &val),
		   "Failed to set FIFO watermark 127");

	/* A watermark of 0 disables the almost-full threshold (A_FULL = 0). */
	val.val1 = 0;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_WATERMARK,
				   &val),
		   "Failed to set FIFO watermark 0");

	/* Out-of-range watermark is clamped to A_FULL = 1, not rejected. */
	val.val1 = MAXM86161_FIFO_DEPTH + 1;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_WATERMARK,
				   &val),
		   "Expected out-of-range watermark to be clamped, not rejected");

#ifdef CONFIG_EMUL
	uint8_t reg_value;

	/* The last set (clamped) should have written A_FULL = 1. */
	zassert_ok(maxm86161_mock_get_register(fixture->mock->data,
					       MAXM86161_REG_FIFO_CONFIG1, &reg_value),
		   "Failed to read mock FIFO_CONFIG1 register");
	zassert_equal(FIELD_GET(MAXM86161_MSK_FIFO_A_FULL, reg_value), 1,
		      "Expected clamped A_FULL = 1, got %d",
		      FIELD_GET(MAXM86161_MSK_FIFO_A_FULL, reg_value));

	/* A mid-range watermark round-trips through the FIFO_DEPTH - N mapping. */
	val.val1 = 15;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_WATERMARK,
				   &val),
		   "Failed to set FIFO watermark 15");
	zassert_ok(maxm86161_mock_get_register(fixture->mock->data,
					       MAXM86161_REG_FIFO_CONFIG1, &reg_value),
		   "Failed to read mock FIFO_CONFIG1 register");
	zassert_equal(FIELD_GET(MAXM86161_MSK_FIFO_A_FULL, reg_value),
		      MAXM86161_FIFO_DEPTH - 15,
		      "Expected A_FULL = %d, got %d", MAXM86161_FIFO_DEPTH - 15,
		      FIELD_GET(MAXM86161_MSK_FIFO_A_FULL, reg_value));
#endif
}

/* ========================================================================== */
/* Attribute retrieval and round-trip                                         */
/* ========================================================================== */

ZTEST_F(maxm86161, test_attr_get_config)
{
	struct sensor_value val;
	enum sensor_attribute attr;

	/* PPG configuration */
	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &val),
		   "Failed to get sampling_frequency");
	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_SAMPLE_AVERAGING,
				   &val),
		   "Failed to get sample_averaging");
	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_ADC_RANGE, &val),
		   "Failed to get adc_range");
	attr = (enum sensor_attribute)SENSOR_ATTR_MAXM86161_LED_INTEGRATION_TIME;
	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN, attr, &val),
		   "Failed to get led_integration_time");
	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_ALC_DISABLE, &val),
		   "Failed to get alc_disable");

	/* LED configuration */
	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_LED1_GREEN_CURRENT,
				   &val),
		   "Failed to get led1_green_current");
	attr = (enum sensor_attribute)SENSOR_ATTR_MAXM86161_LED1_GREEN_CURRENT_RANGE;
	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN, attr, &val),
		   "Failed to get led1_green_current_range");

	/* FIFO configuration */
	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_WATERMARK,
				   &val),
		   "Failed to get fifo_watermark");
	attr = (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_A_FULL_TYPE;
	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN, attr, &val),
		   "Failed to get fifo_a_full_type");
	attr = (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_ROLLOVER;
	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN, attr, &val),
		   "Failed to get fifo_rollover");
	attr = (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_DATA_COUNT;
	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN, attr, &val),
		   "Failed to get fifo_data_count");
	zassert_ok(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_OVERFLOW_COUNT,
				   &val),
		   "Failed to get fifo_overflow_count");
}

ZTEST_F(maxm86161, test_attr_fifo_config)
{
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_FIFO_WATERMARK, 64,
			  "fifo_watermark");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_FIFO_A_FULL_TYPE, 1,
			  "fifo_a_full_type");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_FIFO_ROLLOVER, 1,
			  "fifo_rollover");
}

ZTEST_F(maxm86161, test_attr_system_control)
{
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LOW_POWER_MODE, 1,
			  "low_power_mode");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_TIME_STAMP_ENABLE, 1,
			  "time_stamp_enable");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_DAC_CODE_CHANGE_TAG_ENABLE, 1,
			  "dac_code_change_tag_enable");
}

ZTEST_F(maxm86161, test_attr_ppg_config)
{
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_ALC_DISABLE, 1, "alc_disable");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_ADD_OFFSET_ENABLE, 1,
			  "add_offset_enable");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_ADC_RANGE, ADC_RANGE_16384NA,
			  "adc_range");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED_INTEGRATION_TIME,
			  LED_INTEGRATION_TIME_117p3US, "led_integration_time");
	attr_set_validate(fixture->dev, SENSOR_ATTR_SAMPLING_FREQUENCY, PPG_SR_99p902SPS_1PPS,
			  "sampling_frequency");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_SAMPLE_AVERAGING, SAMPLE_AVERAGING_4,
			  "sample_averaging");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED_SETTLING_TIME,
			  LED_SETTLING_TIME_8US, "led_settling_time");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_DIGITAL_FILTER_SELECT,
			  DIG_FILT_USE_FDM, "digital_filter_select");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_BURST_RATE, BURST_RATE_84HZ,
			  "burst_rate");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_PROX_INT_THRESHOLD, 100,
			  "prox_int_threshold");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_PHOTODIODE_BIAS,
			  PD_CAP_BIAS_0PF_65PF, "photodiode_bias");
}

/* burst mode is accepted or rejected based on the (sample-rate, burst-rate) compatibility table. */
ZTEST_F(maxm86161, test_attr_burst_compatibility)
{
	struct sensor_value val;

	/*
	 * 99.902 SPS + 84 Hz is marked invalid by the burst-count table, so the
	 * driver must reject enabling burst mode with this combination.
	 */
	attr_set_validate(fixture->dev, SENSOR_ATTR_SAMPLING_FREQUENCY, PPG_SR_99p902SPS_1PPS,
			  "sampling_frequency");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_BURST_RATE, BURST_RATE_84HZ,
			  "burst_rate");

	val.val1 = 1;
	zassert_equal(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				      (enum sensor_attribute)SENSOR_ATTR_MAXM86161_BURST_ENABLE,
				      &val),
		      -EINVAL,
		      "Expected -EINVAL enabling burst with incompatible SR/burst rate");

	/*
	 * 399.610 SPS + 84 Hz is a valid combination, so burst mode must now be
	 * accepted.
	 */
	attr_set_validate(fixture->dev, SENSOR_ATTR_SAMPLING_FREQUENCY, PPG_SR_399p610SPS_1PPS,
			  "sampling_frequency (burst-compatible)");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_BURST_RATE, BURST_RATE_84HZ,
			  "burst_rate (burst-compatible)");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_BURST_ENABLE, 1,
			  "burst_enable (compatible)");
}

ZTEST_F(maxm86161, test_attr_led_sequence_full)
{
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED_SEQUENCE_1, LED_SEQ_GREEN_LED,
			  "led_sequence_1");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED_SEQUENCE_2, LED_SEQ_IR_LED,
			  "led_sequence_2");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED_SEQUENCE_3, LED_SEQ_RED_LED,
			  "led_sequence_3");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED_SEQUENCE_4, LED_SEQ_NONE,
			  "led_sequence_4");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED_SEQUENCE_5, LED_SEQ_NONE,
			  "led_sequence_5");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED_SEQUENCE_6, LED_SEQ_NONE,
			  "led_sequence_6");
}

ZTEST_F(maxm86161, test_attr_led_currents_full)
{
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED1_GREEN_CURRENT, 128,
			  "led1_green_current");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED2_IR_CURRENT, 64,
			  "led2_ir_current");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED3_RED_CURRENT, 32,
			  "led3_red_current");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_PILOT_ON_GREEN_LED_CURRENT, 16,
			  "pilot_on_green_led_current");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED1_GREEN_CURRENT_RANGE,
			  LED_CURRENT_RANGE_62MA, "led1_green_current_range");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED2_IR_CURRENT_RANGE,
			  LED_CURRENT_RANGE_62MA, "led2_ir_current_range");
	attr_set_validate(fixture->dev, SENSOR_ATTR_MAXM86161_LED3_RED_CURRENT_RANGE,
			  LED_CURRENT_RANGE_62MA, "led3_red_current_range");
}

ZTEST_F(maxm86161, test_attr_write_only)
{
	attr_set_only(fixture->dev, SENSOR_ATTR_MAXM86161_SW_FORCE_SYNC_ENABLE, 1,
		      "sw_force_sync_enable");
	attr_set_only(fixture->dev, SENSOR_ATTR_MAXM86161_DAC_CALIBRATION, 1, "dac_calibration");
	attr_set_only(fixture->dev, SENSOR_ATTR_MAXM86161_FIFO_FLUSH, 1, "fifo_flush");

	/* Reading a write-only attribute must be rejected. */
	struct sensor_value val;

	zassert_equal(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN,
				      (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_FLUSH,
				      &val),
		      -ENOTSUP, "Expected -ENOTSUP reading write-only FIFO_FLUSH");
}

/* ========================================================================== */
/* error paths                                                                */
/* ========================================================================== */

ZTEST_F(maxm86161, test_attr_read_only_rejects_set)
{
	struct sensor_value val = {.val1 = 1};

	zassert_equal(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				      (enum sensor_attribute)SENSOR_ATTR_MAXM86161_STATUS1, &val),
		      -ENOTSUP, "Expected -ENOTSUP writing read-only STATUS1");

	zassert_equal(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				      (enum sensor_attribute)SENSOR_ATTR_MAXM86161_FIFO_DATA_COUNT,
				      &val),
		      -ENOTSUP, "Expected -ENOTSUP writing read-only FIFO_DATA_COUNT");
}

ZTEST_F(maxm86161, test_attr_wrong_channel)
{
	struct sensor_value val = {.val1 = PPG_SR_50p027SPS_1PPS};

	zassert_equal(sensor_attr_set(fixture->dev, SENSOR_CHAN_GREEN,
				      SENSOR_ATTR_SAMPLING_FREQUENCY, &val),
		      -ENOTSUP, "Expected -ENOTSUP for attr_set on non-PPG channel");

	zassert_equal(sensor_attr_get(fixture->dev, SENSOR_CHAN_GREEN,
				      SENSOR_ATTR_SAMPLING_FREQUENCY, &val),
		      -ENOTSUP, "Expected -ENOTSUP for attr_get on non-PPG channel");
}

ZTEST_F(maxm86161, test_attr_unsupported)
{
	struct sensor_value val = {0};

	/* SENSOR_ATTR_OFFSET is a standard attribute the driver does not implement. */
	zassert_equal(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				      SENSOR_ATTR_OFFSET, &val),
		      -ENOTSUP,
		      "Expected -ENOTSUP for unsupported attribute");

	zassert_equal(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN,
				      SENSOR_ATTR_OFFSET, &val),
		      -ENOTSUP,
		      "Expected -ENOTSUP for unsupported get attribute");
}

#ifdef CONFIG_EMUL
/*
 * a bus fault during attr_set/attr_get must propagate as the injected errno.
 * Covers both the whole-register write path (LED PA)
 * and the read-modify-write path (ADC range), plus the attr_get read path.
 */
ZTEST_F(maxm86161, test_attr_set_bus_error)
{
	struct sensor_value val = {.val1 = 100};
	enum sensor_attribute attr;

	/* Whole-register write path: LED1 PA lives at its own 8-bit register. */
	maxm86161_mock_set_fault(fixture->mock->data, MAXM86161_REG_LED1_PA, -EIO);
	attr = (enum sensor_attribute)SENSOR_ATTR_MAXM86161_LED1_GREEN_CURRENT;
	zassert_equal(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN, attr, &val), -EIO,
		      "Expected -EIO on faulted LED1 PA write");
	maxm86161_mock_clear_fault(fixture->mock->data);

	/* Read-modify-write path: ADC range is a bitfield in PPG_CONFIG1. */
	val.val1 = ADC_RANGE_8192NA;
	maxm86161_mock_set_fault(fixture->mock->data, MAXM86161_REG_PPG_CONFIG1, -EIO);
	zassert_equal(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				      (enum sensor_attribute)SENSOR_ATTR_MAXM86161_ADC_RANGE, &val),
		      -EIO, "Expected -EIO on faulted PPG_CONFIG1 update");
	maxm86161_mock_clear_fault(fixture->mock->data);
}

ZTEST_F(maxm86161, test_attr_get_bus_error)
{
	struct sensor_value val;

	maxm86161_mock_set_fault(fixture->mock->data, MAXM86161_REG_PPG_CONFIG1, -EIO);
	zassert_equal(sensor_attr_get(fixture->dev, MAXM86161_ATTR_CHAN,
				      (enum sensor_attribute)SENSOR_ATTR_MAXM86161_ADC_RANGE, &val),
		      -EIO, "Expected -EIO on faulted attr_get read");
	maxm86161_mock_clear_fault(fixture->mock->data);
}

/*
 * a bus fault on any register touched during the probe/config
 * sequence must abort re-configuration and surface the error through the RESET
 * attribute (which re-runs probe). Iterating over the config registers walks the
 * error-return branch of every maxm86161_config_* helper.
 *
 * Restricted to the no-trigger (emul) scenario for the same reason as
 * test_reset_reconfigures: the RESET attribute re-runs probe, which would
 * re-initialise the already-running interrupt thread under the trigger stack.
 */
#ifndef CONFIG_MAXM86161_TRIGGER
ZTEST_F(maxm86161, test_probe_config_bus_errors)
{
	static const uint8_t cfg_regs[] = {
		MAXM86161_REG_SYSTEM_CONTROL,   /* shutdown + config_sys */
		MAXM86161_REG_FIFO_CONFIG1,     /* config_fifo */
		MAXM86161_REG_FIFO_CONFIG2,     /* config_fifo */
		MAXM86161_REG_PPG_SYNC_CONTROL, /* config_ppg_sync */
		MAXM86161_REG_PPG_CONFIG1,      /* config_ppg */
		MAXM86161_REG_PPG_CONFIG2,      /* config_ppg */
		MAXM86161_REG_PPG_CONFIG3,      /* config_ppg */
		MAXM86161_REG_PPG_PROX_INT_THRESH,
		MAXM86161_REG_PPG_PD_BIAS,
		MAXM86161_REG_PPG_PICKET_FENCE, /* config_picket_fence */
		MAXM86161_REG_LED_SEQ_REG1,     /* config_led */
		MAXM86161_REG_LED1_PA,
		MAXM86161_REG_LED_RANGE_1,
	};
	struct sensor_value val = {.val1 = 1};

	for (size_t i = 0; i < ARRAY_SIZE(cfg_regs); i++) {
		maxm86161_mock_set_fault(fixture->mock->data, cfg_regs[i], -EIO);
		zassert_true(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
					     (enum sensor_attribute)SENSOR_ATTR_MAXM86161_RESET,
					     &val) < 0,
			     "Expected reset/probe failure with fault on reg 0x%02x",
			     cfg_regs[i]);
		maxm86161_mock_clear_fault(fixture->mock->data);
	}

	/* A clean reset must succeed and restore a working configuration. */
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_MAXM86161_RESET, &val),
		   "Clean reset after fault sweep failed");
}
#endif /* !CONFIG_MAXM86161_TRIGGER */
#endif /* CONFIG_EMUL */

/* ========================================================================== */
/* synchronous fetch / channel_get                                            */
/* ========================================================================== */

ZTEST_F(maxm86161, test_sample_fetch)
{
	/*
	 * The die-temperature channel is fetchable through the synchronous fetch
	 * API, and an all-channel fetch drains die-temperature and the configured
	 * PPG channels.
	 */
	zassert_ok(sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_DIE_TEMP),
		   "sensor_sample_fetch_chan(DIE_TEMP) failed");

	zassert_ok(sensor_sample_fetch(fixture->dev),
		   "Expected all-channel fetch to succeed");
}

ZTEST_F(maxm86161, test_channel_get)
{
	struct sensor_value val;

	/*
	 * PPG channels are populated from the FIFO; the synchronous channel_get
	 * returns the last cached value for any channel present in the configured
	 * LED sequence. Trigger a die-temperature measurement first so the fetch
	 * path is exercised.
	 */
	zassert_ok(sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_DIE_TEMP),
		   "sensor_sample_fetch_chan(DIE_TEMP) failed");

	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_GREEN, &val),
		   "Failed to get green channel");

	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_IR, &val),
		   "Failed to get IR channel");

	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_RED, &val),
		   "Failed to get red channel");

	/* Unsupported channel */
	zassert_equal(sensor_channel_get(fixture->dev, SENSOR_CHAN_ACCEL_X, &val),
		      -ENOTSUP,
		      "Expected -ENOTSUP for unsupported channel");
}

#ifdef CONFIG_EMUL
/*
 * fetching a PPG channel drains one frame from the FIFO and demuxes it
 * by tag into the per-channel cache. The overlay configures the sequence Green, IR, Red,
 * so those three tags are read.
 */
ZTEST_F(maxm86161, test_fetch_ppg_channels)
{
	struct sensor_value val;
	void *data = fixture->mock->data;

	/*
	 * The emulator returns the three bytes at FIFO_DATA (0x08..0x0A) for
	 * every 3-byte FIFO read, so seed a Green (LEDC1) sample there. The
	 * fetch reads num_active_channels samples; all decode to the same value.
	 */
	uint32_t word = ((uint32_t)MAXM86161_FIFO_TAG_LEDC1 << 19) | (0x12345 & 0x7FFFF);

	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_REGISTER,
				    (word >> 16) & 0xFF);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_REGISTER + 1,
				    (word >> 8) & 0xFF);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_REGISTER + 2,
				    word & 0xFF);

	zassert_ok(sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_GREEN),
		   "Failed to fetch green channel from FIFO");
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_GREEN, &val),
		   "Failed to get green channel value");
	zassert_equal(val.val1, 0x12345,
		      "Green FIFO value mismatch: got 0x%x", val.val1);
}

/*
 * light/proximity channels are only fetchable when their exposure is in
 * the configured LED sequence. With the default overlay sequence they are not,
 * so fetch must return -ENODATA.
 */
ZTEST_F(maxm86161, test_fetch_absent_channel)
{
	zassert_equal(sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_AMBIENT_LIGHT),
		      -ENODATA, "Expected -ENODATA fetching unconfigured LIGHT channel");
	zassert_equal(sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_PROX),
		      -ENODATA, "Expected -ENODATA fetching unconfigured PROX channel");
}

/*
 * die-temperature timeout path. With self-clear disabled the TEMP_EN bit is held set,
 * so the driver polls the full wait window; the seeded integer/fraction registers
 * are then read back.
 */
ZTEST_F(maxm86161, test_die_temp_hold)
{
	struct sensor_value val;
	void *data = fixture->mock->data;

	maxm86161_mock_set_selfclear(data, false);
	maxm86161_mock_set_register(data, MAXM86161_REG_DIE_TEMP_INTEGER, 25);
	maxm86161_mock_set_register(data, MAXM86161_REG_DIE_TEMP_FRACTION, 8);

	/* The poll loop runs to its limit, then the pair is read as a burst. */
	(void)sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_DIE_TEMP);
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_DIE_TEMP, &val),
		   "Failed to get die temperature");
	zassert_equal(val.val1, 25, "Die temp integer mismatch: got %d", val.val1);

	maxm86161_mock_set_selfclear(data, true);
}
#endif /* CONFIG_EMUL */

/* ========================================================================== */
/* register readback after attr_set (emulator only)                           */
/* ========================================================================== */

#ifdef CONFIG_EMUL
ZTEST_F(maxm86161, test_register_readback)
{
	struct sensor_value val;
	uint8_t reg_value;

	/* Set a known sampling rate */
	val.val1 = PPG_SR_84p021SPS_1PPS;
	zassert_ok(sensor_attr_set(fixture->dev, MAXM86161_ATTR_CHAN,
				   (enum sensor_attribute)SENSOR_ATTR_SAMPLING_FREQUENCY,
				   &val),
		   "Failed to set sampling rate for readback test");

	/* Verify the register was written correctly via the emulator */
	zassert_ok(maxm86161_mock_get_register(fixture->mock->data,
					       MAXM86161_REG_PPG_CONFIG2, &reg_value),
		   "Failed to read mock register");

	uint8_t sr_field = FIELD_GET(MAXM86161_MSK_PPG_CONFIG2_PPG_SR, reg_value);

	zassert_equal(sr_field, PPG_SR_84p021SPS_1PPS,
		      "Expected SR %d in register, got %d",
		      PPG_SR_84p021SPS_1PPS, sr_field);
}
#endif

ZTEST_SUITE(maxm86161, NULL, maxm86161_setup, maxm86161_before, NULL, NULL);

/* ========================================================================== */
/* FIFO tag decoder                                                           */
/* ========================================================================== */

#ifdef CONFIG_MAXM86161_STREAM

/* Encode a 19-bit sample under a 5-bit tag into a big-endian 3-byte FIFO word. */
static void put_fifo_sample(uint8_t *p, uint8_t tag, uint32_t data)
{
	uint32_t word = ((uint32_t)tag << 19) | (data & 0x7FFFF);

	sys_put_be24(word, p);
}

/* Populate a FIFO header + sample buffer for the decoder under test. */
static uint16_t build_fifo_buffer(uint8_t *buf, const uint8_t *tags,
				  const uint32_t *vals, uint16_t n, uint8_t int_status)
{
	struct maxm86161_fifo_hdr *hdr = (struct maxm86161_fifo_hdr *)buf;
	uint8_t *s = buf + sizeof(*hdr);

	memset(buf, 0, sizeof(*hdr));
	hdr->is_fifo = 1;
	hdr->odr = 100;
	hdr->timestamp = 1000000000ULL;
	hdr->int_status = int_status;
	hdr->fifo_ovf_count = 0;

	for (uint16_t i = 0; i < n; i++) {
		put_fifo_sample(s + i * MAXM86161_FIFO_SAMPLE_SIZE, tags[i], vals[i]);
	}

	hdr->fifo_byte_count = n * MAXM86161_FIFO_SAMPLE_SIZE;
	hdr->fifo_samples = n;
	return sizeof(*hdr) + n * MAXM86161_FIFO_SAMPLE_SIZE;
}

static const struct sensor_decoder_api *get_decoder(void)
{
	const struct sensor_decoder_api *dec = NULL;

	zassert_ok(maxm86161_get_decoder(DEVICE_DT_GET(MAXM86161_NODE), &dec),
		   "Failed to get decoder");
	zassert_not_null(dec, "Decoder is NULL");
	return dec;
}

ZTEST(maxm86161_decoder, test_decoder_frame_count)
{
	const struct sensor_decoder_api *dec = get_decoder();
	uint8_t buf[256];
	const uint8_t tags[] = {
		MAXM86161_FIFO_TAG_LEDC1, MAXM86161_FIFO_TAG_LEDC2,
		MAXM86161_FIFO_TAG_LEDC1, MAXM86161_FIFO_TAG_LEDC3,
		MAXM86161_FIFO_TAG_LEDC1,
	};
	const uint32_t vals[] = {1, 2, 3, 4, 5};

	build_fifo_buffer(buf, tags, vals, ARRAY_SIZE(tags),
			  MAXM86161_MSK_INT_STATUS1_A_FULL);

	struct sensor_chan_spec c1 = {
		.chan_type = SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1, .chan_idx = 0};
	uint16_t count = 0;

	zassert_ok(dec->get_frame_count(buf, c1, &count), "get_frame_count failed");
	zassert_equal(count, 3, "Expected 3 LEDC1 frames, got %d", count);

	struct sensor_chan_spec c2 = {
		.chan_type = SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC2, .chan_idx = 0};

	zassert_ok(dec->get_frame_count(buf, c2, &count), "get_frame_count failed");
	zassert_equal(count, 1, "Expected 1 LEDC2 frame, got %d", count);
}

ZTEST(maxm86161_decoder, test_decoder_size_info)
{
	const struct sensor_decoder_api *dec = get_decoder();
	size_t base = 0, frame = 0;

	zassert_ok(dec->get_size_info(
			   (struct sensor_chan_spec){
				   .chan_type = SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1},
			   &base, &frame),
		   "get_size_info failed for LEDC1");
	zassert_true(base > 0 && frame > 0, "Zero size info");

	/* A non-FIFO channel is not decodable. */
	zassert_equal(dec->get_size_info(
			      (struct sensor_chan_spec){.chan_type = SENSOR_CHAN_ACCEL_X},
			      &base, &frame),
		      -ENOTSUP, "Expected -ENOTSUP size info for non-FIFO channel");
}

ZTEST(maxm86161_decoder, test_decoder_decode_values)
{
	const struct sensor_decoder_api *dec = get_decoder();
	uint8_t buf[256];
	const uint8_t tags[] = {
		MAXM86161_FIFO_TAG_LEDC1, MAXM86161_FIFO_TAG_LEDC2,
		MAXM86161_FIFO_TAG_LEDC1,
	};
	/* Include a value with the top 19th bit set to prove masking. */
	const uint32_t vals[] = {0x7FFFF, 0x100, 0x2ABCD};

	build_fifo_buffer(buf, tags, vals, ARRAY_SIZE(tags),
			  MAXM86161_MSK_INT_STATUS1_A_FULL);

	struct sensor_chan_spec c1 = {
		.chan_type = SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1, .chan_idx = 0};
	uint8_t out[256] = {0};
	uint32_t fit = 0;

	int n = dec->decode(buf, c1, &fit, 8, out);

	zassert_equal(n, 2, "Expected 2 decoded LEDC1 samples, got %d", n);

	struct sensor_q31_data *d = (struct sensor_q31_data *)out;

	zassert_equal(d->header.reading_count, 2, "reading_count mismatch");
	zassert_equal((uint32_t)d->readings[0].value, 0x7FFFFU,
		      "First LEDC1 value mismatch: 0x%x", (uint32_t)d->readings[0].value);
	zassert_equal((uint32_t)d->readings[1].value, 0x2ABCDU,
		      "Second LEDC1 value mismatch: 0x%x", (uint32_t)d->readings[1].value);
}

ZTEST(maxm86161_decoder, test_decoder_decode_partial)
{
	const struct sensor_decoder_api *dec = get_decoder();
	uint8_t buf[256];
	const uint8_t tags[] = {
		MAXM86161_FIFO_TAG_LEDC1, MAXM86161_FIFO_TAG_LEDC1,
		MAXM86161_FIFO_TAG_LEDC1, MAXM86161_FIFO_TAG_LEDC1,
	};
	const uint32_t vals[] = {10, 20, 30, 40};

	build_fifo_buffer(buf, tags, vals, ARRAY_SIZE(tags), 0);

	struct sensor_chan_spec c1 = {
		.chan_type = SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1, .chan_idx = 0};
	uint8_t out[256] = {0};
	uint32_t fit = 0;

	/* First pass: cap at 2 of the 4 available samples. */
	int n = dec->decode(buf, c1, &fit, 2, out);

	zassert_equal(n, 2, "First partial decode expected 2, got %d", n);
	zassert_equal(fit, 2, "fit not advanced to 2, got %u", fit);

	/* Second pass: resumes from fit and returns the remaining 2. */
	n = dec->decode(buf, c1, &fit, 8, out);
	zassert_equal(n, 2, "Second partial decode expected 2, got %d", n);
	zassert_equal(fit, 4, "fit not advanced to 4, got %u", fit);
}

ZTEST(maxm86161_decoder, test_decoder_special_tags)
{
	const struct sensor_decoder_api *dec = get_decoder();
	uint8_t buf[256];
	const uint8_t tags[] = {
		MAXM86161_FIFO_TAG_PROX, MAXM86161_FIFO_TAG_SUB_DAC,
		MAXM86161_FIFO_TAG_TIMESTAMP,
	};
	const uint32_t vals[] = {0x111, 0x222, 0x333};

	build_fifo_buffer(buf, tags, vals, ARRAY_SIZE(tags), 0);

	uint16_t count = 0;

	zassert_ok(dec->get_frame_count(
			   buf,
			   (struct sensor_chan_spec){
				   .chan_type = SENSOR_CHAN_MAXM86161_FIFO_PROX},
			   &count),
		   "get_frame_count PROX failed");
	zassert_equal(count, 1, "Expected 1 PROX frame");

	zassert_ok(dec->get_frame_count(
			   buf,
			   (struct sensor_chan_spec){
				   .chan_type = SENSOR_CHAN_MAXM86161_FIFO_TIMESTAMP},
			   &count),
		   "get_frame_count TIMESTAMP failed");
	zassert_equal(count, 1, "Expected 1 TIMESTAMP frame");
}

ZTEST(maxm86161_decoder, test_decoder_invalid_tag)
{
	const struct sensor_decoder_api *dec = get_decoder();
	uint8_t buf[256];
	const uint8_t tags[] = {MAXM86161_FIFO_TAG_INVALID};
	const uint32_t vals[] = {0};

	build_fifo_buffer(buf, tags, vals, ARRAY_SIZE(tags), 0);

	/* An empty/invalid tag is not counted for any channel. */
	uint16_t count = 1;

	zassert_ok(dec->get_frame_count(
			   buf,
			   (struct sensor_chan_spec){
				   .chan_type = SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1},
			   &count),
		   "get_frame_count invalid failed");
	zassert_equal(count, 0, "Invalid tag must not be counted");
}

ZTEST(maxm86161_decoder, test_decoder_has_trigger)
{
	const struct sensor_decoder_api *dec = get_decoder();
	uint8_t buf[64];
	const uint8_t tags[] = {MAXM86161_FIFO_TAG_LEDC1};
	const uint32_t vals[] = {1};

	build_fifo_buffer(buf, tags, vals, ARRAY_SIZE(tags),
			  MAXM86161_MSK_INT_STATUS1_A_FULL);

	zassert_true(dec->has_trigger(buf, SENSOR_TRIG_FIFO_WATERMARK),
		     "Expected FIFO_WATERMARK trigger present");
	zassert_true(dec->has_trigger(buf, SENSOR_TRIG_FIFO_FULL),
		     "Expected FIFO_FULL trigger present");
	zassert_false(dec->has_trigger(buf, SENSOR_TRIG_DATA_READY),
		      "DATA_READY must not be reported for an A_FULL buffer");

	/* Without A_FULL set, no FIFO trigger is present. */
	build_fifo_buffer(buf, tags, vals, ARRAY_SIZE(tags), 0);
	zassert_false(dec->has_trigger(buf, SENSOR_TRIG_FIFO_WATERMARK),
		      "FIFO_WATERMARK must be absent without A_FULL");
}

ZTEST_SUITE(maxm86161_decoder, NULL, NULL, NULL, NULL, NULL);

#endif /* CONFIG_MAXM86161_STREAM */

/* ========================================================================== */
/* interrupt/trigger management + dispatch                                    */
/* ========================================================================== */

/*
 * The trigger suite runs in the dedicated own-thread trigger scenario (no
 * streaming). Excluding it from the stream build keeps INTB dispatch from
 * racing/sharing driver runtime state (stream_mode, status caches) with the
 * RTIO pipeline, which owns the interrupt edge while a stream is active.
 */
#if defined(CONFIG_MAXM86161_TRIGGER) && !defined(CONFIG_MAXM86161_STREAM)

static volatile int g_trig_count;
static enum sensor_trigger_type g_last_trig_type;

static void test_trig_handler(const struct device *dev,
			      const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	g_trig_count++;
	g_last_trig_type = trig->type;
}

struct maxm86161_trigger_fixture {
	const struct device *dev;
	const struct emul *mock;
	const struct device *intb_port;
	gpio_pin_t intb_pin;
};

static struct maxm86161_trigger_fixture trig_fix;

static void *trig_setup(void)
{
	trig_fix.dev = DEVICE_DT_GET(MAXM86161_NODE);
	trig_fix.mock = EMUL_DT_GET(MAXM86161_NODE);
	trig_fix.intb_port =
		DEVICE_DT_GET(DT_GPIO_CTLR(MAXM86161_NODE, interrupt_gpios));
	trig_fix.intb_pin = DT_GPIO_PIN(MAXM86161_NODE, interrupt_gpios);
	zassert_true(device_is_ready(trig_fix.dev), "device not ready");
	zassert_true(device_is_ready(trig_fix.intb_port), "INTB gpio not ready");
	return &trig_fix;
}

static void trig_before(void *f)
{
	ARG_UNUSED(f);
	g_trig_count = 0;
	g_last_trig_type = 0;
	maxm86161_mock_clear_fault(trig_fix.mock->data);
	maxm86161_mock_set_register(trig_fix.mock->data, MAXM86161_REG_INT_STATUS1, 0);
	maxm86161_mock_set_register(trig_fix.mock->data, MAXM86161_REG_INT_STATUS2, 0);
	maxm86161_mock_set_register(trig_fix.mock->data, MAXM86161_REG_INT_ENABLE1, 0);
	maxm86161_mock_set_register(trig_fix.mock->data, MAXM86161_REG_INT_ENABLE2, 0);
}

/* Generate a falling edge on INTB and let the trigger thread run. */
static void fire_intb(struct maxm86161_trigger_fixture *f)
{
	gpio_emul_input_set(f->intb_port, f->intb_pin, 1);
	gpio_emul_input_set(f->intb_port, f->intb_pin, 0);
	k_sleep(K_MSEC(50));
}

ZTEST_F(maxm86161_trigger, test_trigger_set_enable_bits)
{
	struct sensor_trigger drdy = {.type = SENSOR_TRIG_DATA_READY,
				      .chan = (enum sensor_channel)SENSOR_CHAN_MAXM86161_PPG};
	struct sensor_trigger afull = {.type = SENSOR_TRIG_FIFO_WATERMARK,
				       .chan = (enum sensor_channel)SENSOR_CHAN_MAXM86161_PPG};
	uint8_t reg;

	/* Enabling a trigger sets its INT_ENABLE1 bit. */
	zassert_ok(sensor_trigger_set(fixture->dev, &drdy, test_trig_handler),
		   "Failed to set DATA_READY trigger");
	maxm86161_mock_get_register(fixture->mock->data, MAXM86161_REG_INT_ENABLE1, &reg);
	zassert_true(reg & MAXM86161_MSK_INT_ENABLE1_DATA_RDY_EN,
		     "DATA_RDY enable bit not set");

	zassert_ok(sensor_trigger_set(fixture->dev, &afull, test_trig_handler),
		   "Failed to set FIFO_WATERMARK trigger");
	maxm86161_mock_get_register(fixture->mock->data, MAXM86161_REG_INT_ENABLE1, &reg);
	zassert_true(reg & MAXM86161_MSK_INT_ENABLE1_A_FULL_EN,
		     "A_FULL enable bit not set");

	/* Disabling (NULL handler) clears the bit. */
	zassert_ok(sensor_trigger_set(fixture->dev, &drdy, NULL),
		   "Failed to clear DATA_READY trigger");
	maxm86161_mock_get_register(fixture->mock->data, MAXM86161_REG_INT_ENABLE1, &reg);
	zassert_false(reg & MAXM86161_MSK_INT_ENABLE1_DATA_RDY_EN,
		      "DATA_RDY enable bit not cleared");
}

ZTEST_F(maxm86161_trigger, test_trigger_set_die_temp)
{
	struct sensor_trigger t = {
		.type = SENSOR_TRIG_DATA_READY, .chan = SENSOR_CHAN_DIE_TEMP};
	uint8_t reg;

	zassert_ok(sensor_trigger_set(fixture->dev, &t, test_trig_handler),
		   "Failed to set die-temp DRDY trigger");
	maxm86161_mock_get_register(fixture->mock->data, MAXM86161_REG_INT_ENABLE1, &reg);
	zassert_true(reg & MAXM86161_MSK_INT_ENABLE1_DIE_TEMP_RDY_EN,
		     "DIE_TEMP_RDY enable bit not set");
}

ZTEST_F(maxm86161_trigger, test_trigger_set_custom)
{
	const struct {
		enum sensor_trigger_type type;
		uint8_t reg;
		uint8_t mask;
	} cases[] = {
		{(enum sensor_trigger_type)SENSOR_TRIG_MAXM86161_ALC_OVERFLOW,
		 MAXM86161_REG_INT_ENABLE1, MAXM86161_MSK_INT_ENABLE1_ALC_OVF_EN},
		{(enum sensor_trigger_type)SENSOR_TRIG_MAXM86161_PROXIMITY,
		 MAXM86161_REG_INT_ENABLE1, MAXM86161_MSK_INT_ENABLE1_PROX_INT_EN},
		{(enum sensor_trigger_type)SENSOR_TRIG_MAXM86161_LED_COMPB,
		 MAXM86161_REG_INT_ENABLE1, MAXM86161_MSK_INT_ENABLE1_LED_COMPB_EN},
		{(enum sensor_trigger_type)SENSOR_TRIG_MAXM86161_SHA_DONE,
		 MAXM86161_REG_INT_ENABLE2, MAXM86161_MSK_INT_ENABLE2_SHA_DONE_EN},
	};
	uint8_t reg;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		struct sensor_trigger t = {.type = cases[i].type,
					   .chan = (enum sensor_channel)SENSOR_CHAN_MAXM86161_PPG};

		zassert_ok(sensor_trigger_set(fixture->dev, &t, test_trig_handler),
			   "Failed to set custom trigger %d", cases[i].type);
		maxm86161_mock_get_register(fixture->mock->data, cases[i].reg, &reg);
		zassert_true(reg & cases[i].mask,
			     "Enable bit not set for custom trigger %d", cases[i].type);
	}
}

ZTEST_F(maxm86161_trigger, test_trigger_unsupported)
{
	struct sensor_trigger t = {.type = SENSOR_TRIG_TAP,
				   .chan = (enum sensor_channel)SENSOR_CHAN_MAXM86161_PPG};

	zassert_equal(sensor_trigger_set(fixture->dev, &t, test_trig_handler),
		      -ENOTSUP, "Expected -ENOTSUP for unsupported trigger");
}

/* an INTB assertion with DATA_RDY set in STATUS1 must invoke the registered data-ready handler. */
ZTEST_F(maxm86161_trigger, test_trigger_dispatch_data_ready)
{
	struct sensor_trigger drdy = {.type = SENSOR_TRIG_DATA_READY,
				      .chan = (enum sensor_channel)SENSOR_CHAN_MAXM86161_PPG};

	zassert_ok(sensor_trigger_set(fixture->dev, &drdy, test_trig_handler),
		   "Failed to register DATA_READY handler");

	maxm86161_mock_set_register(fixture->mock->data, MAXM86161_REG_INT_STATUS1,
				    MAXM86161_MSK_INT_STATUS1_DATA_RDY);
	fire_intb(fixture);

	zassert_true(g_trig_count > 0, "DATA_READY handler was not invoked");
	zassert_equal(g_last_trig_type, SENSOR_TRIG_DATA_READY,
		      "Wrong trigger type dispatched");
}

/*
 * proximity trigger enable toggles the driver's runtime proximity
 * state and its INTB dispatch invokes the proximity handler.
 */
ZTEST_F(maxm86161_trigger, test_trigger_dispatch_proximity)
{
	struct sensor_trigger prox = {
		.type = (enum sensor_trigger_type)SENSOR_TRIG_MAXM86161_PROXIMITY,
		.chan = (enum sensor_channel)SENSOR_CHAN_MAXM86161_PPG};

	zassert_ok(sensor_trigger_set(fixture->dev, &prox, test_trig_handler),
		   "Failed to register PROXIMITY handler");

	maxm86161_mock_set_register(fixture->mock->data, MAXM86161_REG_INT_STATUS1,
				    MAXM86161_MSK_INT_STATUS1_PROX_INT);
	fire_intb(fixture);

	zassert_true(g_trig_count > 0, "PROXIMITY handler was not invoked");

	/* Disable proximity again to restore default runtime state. */
	zassert_ok(sensor_trigger_set(fixture->dev, &prox, NULL),
		   "Failed to clear PROXIMITY handler");
}

/* die-temperature data-ready dispatches through its dedicated slot. */
ZTEST_F(maxm86161_trigger, test_trigger_dispatch_die_temp)
{
	struct sensor_trigger t = {
		.type = SENSOR_TRIG_DATA_READY, .chan = SENSOR_CHAN_DIE_TEMP};

	zassert_ok(sensor_trigger_set(fixture->dev, &t, test_trig_handler),
		   "Failed to register die-temp DRDY handler");

	maxm86161_mock_set_register(fixture->mock->data, MAXM86161_REG_INT_STATUS1,
				    MAXM86161_MSK_INT_STATUS1_DIE_TEMP_RDY);
	fire_intb(fixture);

	zassert_true(g_trig_count > 0, "Die-temp DRDY handler was not invoked");
}

/* FIFO watermark dispatches the registered handler when no proximity settling is in effect. */
ZTEST_F(maxm86161_trigger, test_trigger_dispatch_watermark)
{
	struct sensor_trigger afull = {.type = SENSOR_TRIG_FIFO_WATERMARK,
				       .chan = (enum sensor_channel)SENSOR_CHAN_MAXM86161_PPG};

	zassert_ok(sensor_trigger_set(fixture->dev, &afull, test_trig_handler),
		   "Failed to register FIFO_WATERMARK handler");

	maxm86161_mock_set_register(fixture->mock->data, MAXM86161_REG_INT_STATUS1,
				    MAXM86161_MSK_INT_STATUS1_A_FULL);
	fire_intb(fixture);

	zassert_true(g_trig_count > 0, "FIFO_WATERMARK handler was not invoked");
}

/*
 * while proximity mode is active, a watermark interrupt that arrives
 * within the picket-fence settling window is suppressed (the FIFO is flushed
 * instead of dispatched) so unstable samples are dropped.
 * The A_FULL handler must NOT run for the settling burst.
 */
ZTEST_F(maxm86161_trigger, test_trigger_watermark_prox_settle)
{
	struct sensor_trigger prox = {
		.type = (enum sensor_trigger_type)SENSOR_TRIG_MAXM86161_PROXIMITY,
		.chan = (enum sensor_channel)SENSOR_CHAN_MAXM86161_PPG};
	struct sensor_trigger afull = {.type = SENSOR_TRIG_FIFO_WATERMARK,
				       .chan = (enum sensor_channel)SENSOR_CHAN_MAXM86161_PPG};

	zassert_ok(sensor_trigger_set(fixture->dev, &prox, test_trig_handler),
		   "Failed to enable proximity");
	zassert_ok(sensor_trigger_set(fixture->dev, &afull, test_trig_handler),
		   "Failed to register FIFO_WATERMARK handler");

	/* Trigger a proximity transition to start the settling window. */
	maxm86161_mock_set_register(fixture->mock->data, MAXM86161_REG_INT_STATUS1,
				    MAXM86161_MSK_INT_STATUS1_PROX_INT);
	fire_intb(fixture);
	g_trig_count = 0;

	/* An A_FULL arriving immediately after must be suppressed (flushed). */
	maxm86161_mock_set_register(fixture->mock->data, MAXM86161_REG_INT_STATUS1,
				    MAXM86161_MSK_INT_STATUS1_A_FULL);
	fire_intb(fixture);

	zassert_equal(g_trig_count, 0,
		      "Watermark handler must be suppressed during prox settling");

	zassert_ok(sensor_trigger_set(fixture->dev, &prox, NULL),
		   "Failed to disable proximity");
}

ZTEST_SUITE(maxm86161_trigger, NULL, trig_setup, trig_before, NULL, NULL);

#endif /* CONFIG_MAXM86161_TRIGGER */

/* ========================================================================== */
/* RTIO submit / streaming                                                    */
/* ========================================================================== */

#ifdef CONFIG_MAXM86161_STREAM

RTIO_DEFINE_WITH_MEMPOOL(maxm_oneshot_rtio, 4, 4, 4, 128, sizeof(void *));
SENSOR_DT_READ_IODEV(maxm_oneshot_iodev, MAXM86161_NODE,
		     {SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1, 0});

RTIO_DEFINE_WITH_MEMPOOL(maxm_stream_rtio, 8, 8, 8, 128, sizeof(void *));
SENSOR_DT_STREAM_IODEV(maxm_stream_iodev, MAXM86161_NODE,
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_INCLUDE});

RTIO_DEFINE_WITH_MEMPOOL(maxm_drop_rtio, 8, 8, 8, 128, sizeof(void *));
SENSOR_DT_STREAM_IODEV(maxm_drop_iodev, MAXM86161_NODE,
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_DROP});

RTIO_DEFINE_WITH_MEMPOOL(maxm_nop_rtio, 8, 8, 8, 128, sizeof(void *));
SENSOR_DT_STREAM_IODEV(maxm_nop_iodev, MAXM86161_NODE,
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_NOP});

struct maxm86161_stream_fixture {
	const struct device *dev;
	const struct emul *mock;
	const struct device *intb_port;
	gpio_pin_t intb_pin;
};

static struct maxm86161_stream_fixture stream_fix;

/* Minimal handler used only to enable proximity mode from the stream suite. */
static void test_prox_handler_stub(const struct device *dev,
				   const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);
}

static void *stream_setup(void)
{
	stream_fix.dev = DEVICE_DT_GET(MAXM86161_NODE);
	stream_fix.mock = EMUL_DT_GET(MAXM86161_NODE);
	stream_fix.intb_port =
		DEVICE_DT_GET(DT_GPIO_CTLR(MAXM86161_NODE, interrupt_gpios));
	stream_fix.intb_pin = DT_GPIO_PIN(MAXM86161_NODE, interrupt_gpios);
	zassert_true(device_is_ready(stream_fix.dev), "device not ready");
	return &stream_fix;
}

static void stream_before(void *f)
{
	ARG_UNUSED(f);
	void *data = stream_fix.mock->data;

	maxm86161_mock_clear_fault(data);
	maxm86161_mock_set_register(data, MAXM86161_REG_INT_STATUS1, 0);
	maxm86161_mock_set_register(data, MAXM86161_REG_INT_STATUS2, 0);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_OVF_COUNTER, 0);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_COUNTER, 0);
}

/*
 * a non-streaming (one-shot) read is not supported and must fail with
 * -ENOTSUP from the submit path (maxm86161_submit / maxm86161_rtio.c).
 */
ZTEST_F(maxm86161_stream, test_submit_oneshot_unsupported)
{
	int rc = sensor_read_async_mempool(&maxm_oneshot_iodev, &maxm_oneshot_rtio, NULL);

	zassert_ok(rc, "Failed to submit one-shot read");

	struct rtio_cqe *cqe = NULL;

	for (int i = 0; i < 100 && cqe == NULL; i++) {
		cqe = rtio_cqe_consume(&maxm_oneshot_rtio);
		if (cqe == NULL) {
			k_sleep(K_MSEC(2));
		}
	}
	zassert_not_null(cqe, "No completion for one-shot read");

	int res = cqe->result;

	rtio_cqe_release(&maxm_oneshot_rtio, cqe);
	zassert_equal(res, -ENOTSUP, "Expected -ENOTSUP for one-shot read, got %d", res);
}

/*
 * starting a stream arms the A_FULL watermark interrupt, flushes the
 * FIFO, and clears status (maxm86161_submit_stream). Firing INTB with FIFO data
 * pre-loaded then drives the RTIO status->count->drain->complete chain and
 * delivers a decodable buffer.
 */
ZTEST_F(maxm86161_stream, test_stream_watermark_drain)
{
	void *data = fixture->mock->data;
	uint8_t reg;

	/* Pre-load a FIFO with 2 samples available and no overflow. */
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_OVF_COUNTER, 0);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_COUNTER, 2);
	uint32_t word = ((uint32_t)MAXM86161_FIFO_TAG_LEDC1 << 19) | (0x5AA55 & 0x7FFFF);

	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_REGISTER,
				    (word >> 16) & 0xFF);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_REGISTER + 1,
				    (word >> 8) & 0xFF);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_REGISTER + 2,
				    word & 0xFF);

	struct rtio_sqe *handle = NULL;
	int rc = sensor_stream(&maxm_stream_iodev, &maxm_stream_rtio, NULL, &handle);

	zassert_ok(rc, "sensor_stream failed");
	k_sleep(K_MSEC(20));

	/* submit_stream must have enabled the A_FULL watermark interrupt. */
	maxm86161_mock_get_register(data, MAXM86161_REG_INT_ENABLE1, &reg);
	zassert_true(reg & MAXM86161_MSK_INT_ENABLE1_A_FULL_EN,
		     "A_FULL watermark interrupt not enabled by stream submit");

	/* Signal a watermark interrupt and drive the drain chain. */
	maxm86161_mock_set_register(data, MAXM86161_REG_INT_STATUS1,
				    MAXM86161_MSK_INT_STATUS1_A_FULL);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 1);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 0);

	struct rtio_cqe *cqe = NULL;

	for (int i = 0; i < 200 && cqe == NULL; i++) {
		cqe = rtio_cqe_consume(&maxm_stream_rtio);
		if (cqe == NULL) {
			k_sleep(K_MSEC(5));
		}
	}
	zassert_not_null(cqe, "No stream completion delivered");

	int res = cqe->result;
	uint8_t *buf = NULL;
	uint32_t buf_len = 0;

	if (res == 0) {
		rtio_cqe_get_mempool_buffer(&maxm_stream_rtio, cqe, &buf, &buf_len);
	}
	rtio_cqe_release(&maxm_stream_rtio, cqe);
	zassert_true(res >= 0, "Stream completion returned error %d", res);

	if (buf != NULL) {
		/* The delivered buffer decodes as a FIFO frame. */
		const struct sensor_decoder_api *dec = NULL;

		zassert_ok(maxm86161_get_decoder(fixture->dev, &dec), "no decoder");
		uint16_t count = 0;

		zassert_ok(dec->get_frame_count(
				   buf,
				   (struct sensor_chan_spec){
					   .chan_type =
						   SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1},
				   &count),
			   "decode of streamed buffer failed");
		rtio_release_buffer(&maxm_stream_rtio, buf, buf_len);
	}

	if (handle != NULL) {
		rtio_sqe_cancel(handle);
	}
	k_sleep(K_MSEC(20));
}

/*
 * a non-zero OVF_COUNTER on drain is detected and reported as lost
 * data while the available samples are still delivered.
 */
ZTEST_F(maxm86161_stream, test_stream_overflow_report)
{
	void *data = fixture->mock->data;

	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_OVF_COUNTER, 5);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_COUNTER, 3);
	uint32_t word = ((uint32_t)MAXM86161_FIFO_TAG_LEDC1 << 19) | 0x33333;

	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_REGISTER,
				    (word >> 16) & 0xFF);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_REGISTER + 1,
				    (word >> 8) & 0xFF);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_REGISTER + 2,
				    word & 0xFF);

	struct rtio_sqe *handle = NULL;

	zassert_ok(sensor_stream(&maxm_stream_iodev, &maxm_stream_rtio, NULL, &handle),
		   "sensor_stream (overflow) failed");
	k_sleep(K_MSEC(20));

	maxm86161_mock_set_register(data, MAXM86161_REG_INT_STATUS1,
				    MAXM86161_MSK_INT_STATUS1_A_FULL);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 1);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 0);

	struct rtio_cqe *cqe = NULL;

	for (int i = 0; i < 200 && cqe == NULL; i++) {
		cqe = rtio_cqe_consume(&maxm_stream_rtio);
		if (cqe == NULL) {
			k_sleep(K_MSEC(5));
		}
	}
	zassert_not_null(cqe, "No overflow-stream completion delivered");

	int res = cqe->result;
	uint8_t *buf = NULL;
	uint32_t buf_len = 0;

	if (res == 0) {
		rtio_cqe_get_mempool_buffer(&maxm_stream_rtio, cqe, &buf, &buf_len);
	}
	rtio_cqe_release(&maxm_stream_rtio, cqe);
	zassert_true(res >= 0, "Overflow-stream completion returned error %d", res);

	if (buf != NULL) {
		const struct maxm86161_fifo_hdr *hdr =
			(const struct maxm86161_fifo_hdr *)buf;

		zassert_equal(hdr->fifo_ovf_count, 5,
			      "Overflow count not reported in FIFO header: %u",
			      hdr->fifo_ovf_count);
		rtio_release_buffer(&maxm_stream_rtio, buf, buf_len);
	}

	if (handle != NULL) {
		rtio_sqe_cancel(handle);
	}
	k_sleep(K_MSEC(20));
}

/* Consume one completion from @p r, releasing any mempool buffer. Returns the
 * completion result, or -EAGAIN if none arrived within the poll window.
 */
static int stream_wait_result(struct rtio *r)
{
	struct rtio_cqe *cqe = NULL;

	for (int i = 0; i < 200 && cqe == NULL; i++) {
		cqe = rtio_cqe_consume(r);
		if (cqe == NULL) {
			k_sleep(K_MSEC(5));
		}
	}
	if (cqe == NULL) {
		return -EAGAIN;
	}

	int res = cqe->result;
	uint8_t *buf = NULL;
	uint32_t buf_len = 0;

	if (res == 0) {
		rtio_cqe_get_mempool_buffer(r, cqe, &buf, &buf_len);
	}
	rtio_cqe_release(r, cqe);
	if (buf != NULL) {
		rtio_release_buffer(r, buf, buf_len);
	}
	return res;
}

/*
 * with SENSOR_STREAM_DATA_DROP, a watermark interrupt delivers an
 * empty (header-only) buffer to the caller and flushes the FIFO on the device
 * instead of draining it (exercises maxm86161_flush_fifo and its RTIO callback
 * chain in maxm86161_stream.c).
 */
ZTEST_F(maxm86161_stream, test_stream_drop_flushes)
{
	void *data = fixture->mock->data;
	uint8_t reg;

	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_OVF_COUNTER, 0);
	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_COUNTER, 4);

	struct rtio_sqe *handle = NULL;

	zassert_ok(sensor_stream(&maxm_drop_iodev, &maxm_drop_rtio, NULL, &handle),
		   "sensor_stream (drop) failed");
	k_sleep(K_MSEC(20));

	maxm86161_mock_get_register(data, MAXM86161_REG_INT_ENABLE1, &reg);
	zassert_true(reg & MAXM86161_MSK_INT_ENABLE1_A_FULL_EN,
		     "A_FULL not enabled for drop stream");

	maxm86161_mock_set_register(data, MAXM86161_REG_INT_STATUS1,
				    MAXM86161_MSK_INT_STATUS1_A_FULL);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 1);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 0);

	int res = stream_wait_result(&maxm_drop_rtio);

	zassert_true(res >= 0, "Drop-stream completion returned %d", res);

	if (handle != NULL) {
		rtio_sqe_cancel(handle);
	}
	k_sleep(K_MSEC(20));
}

/*
 * SENSOR_STREAM_DATA_NOP delivers a header-only buffer without
 * draining or flushing the FIFO (the "keep data" branch of
 * maxm86161_process_status_cb).
 */
ZTEST_F(maxm86161_stream, test_stream_nop_delivers_header)
{
	void *data = fixture->mock->data;

	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_COUNTER, 2);

	struct rtio_sqe *handle = NULL;

	zassert_ok(sensor_stream(&maxm_nop_iodev, &maxm_nop_rtio, NULL, &handle),
		   "sensor_stream (nop) failed");
	k_sleep(K_MSEC(20));

	maxm86161_mock_set_register(data, MAXM86161_REG_INT_STATUS1,
				    MAXM86161_MSK_INT_STATUS1_A_FULL);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 1);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 0);

	int res = stream_wait_result(&maxm_nop_rtio);

	zassert_true(res >= 0, "NOP-stream completion returned %d", res);

	if (handle != NULL) {
		rtio_sqe_cancel(handle);
	}
	k_sleep(K_MSEC(20));
}

/*
 * an INTB assertion whose STATUS1 does not have A_FULL set is not a
 * watermark event; the pipeline simply re-arms the interrupt and no completion
 * is produced (spurious-interrupt branch of maxm86161_process_status_cb).
 */
ZTEST_F(maxm86161_stream, test_stream_spurious_interrupt)
{
	void *data = fixture->mock->data;

	struct rtio_sqe *handle = NULL;

	zassert_ok(sensor_stream(&maxm_stream_iodev, &maxm_stream_rtio, NULL, &handle),
		   "sensor_stream (spurious) failed");
	k_sleep(K_MSEC(20));

	/* Assert INTB with no A_FULL flag: not a watermark interrupt. */
	maxm86161_mock_set_register(data, MAXM86161_REG_INT_STATUS1, 0);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 1);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 0);
	k_sleep(K_MSEC(20));

	/* No completion should have been generated for a non-watermark IRQ. */
	struct rtio_cqe *cqe = rtio_cqe_consume(&maxm_stream_rtio);

	zassert_is_null(cqe, "Unexpected completion for spurious interrupt");

	if (handle != NULL) {
		rtio_sqe_cancel(handle);
	}
	k_sleep(K_MSEC(20));
}

/*
 * while proximity mode is active, a watermark burst arriving within
 * the settling window after a proximity transition is dropped and the FIFO
 * flushed (settle-suppression branch of maxm86161_process_status_cb.
 * Proximity is enabled through the trigger API, which is compiled into the stream build.
 */
ZTEST_F(maxm86161_stream, test_stream_prox_settle_drop)
{
	void *data = fixture->mock->data;
	struct sensor_trigger prox = {
		.type = (enum sensor_trigger_type)SENSOR_TRIG_MAXM86161_PROXIMITY,
		.chan = (enum sensor_channel)SENSOR_CHAN_MAXM86161_PPG};

	zassert_ok(sensor_trigger_set(fixture->dev, &prox, test_prox_handler_stub),
		   "Failed to enable proximity for settle test");

	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_COUNTER, 4);

	struct rtio_sqe *handle = NULL;

	zassert_ok(sensor_stream(&maxm_stream_iodev, &maxm_stream_rtio, NULL, &handle),
		   "sensor_stream (prox settle) failed");
	k_sleep(K_MSEC(20));

	/*
	 * A watermark interrupt that also carries a proximity transition begins
	 * the settling window; the accompanying FIFO burst is suppressed.
	 */
	maxm86161_mock_set_register(data, MAXM86161_REG_INT_STATUS1,
				    MAXM86161_MSK_INT_STATUS1_A_FULL |
					    MAXM86161_MSK_INT_STATUS1_PROX_INT);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 1);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 0);

	int res = stream_wait_result(&maxm_stream_rtio);

	zassert_true(res >= 0, "Prox-settle stream completion returned %d", res);

	zassert_ok(sensor_trigger_set(fixture->dev, &prox, NULL),
		   "Failed to disable proximity");
	if (handle != NULL) {
		rtio_sqe_cancel(handle);
	}
	k_sleep(K_MSEC(20));
}

/*
 * a bus fault on the FIFO count read during drain must
 * abort the streaming SQE with the error rather than delivering data.
 *
 * Named with a 'zz' prefix so Ztest (which runs tests alphabetically) executes
 * it last in this suite: a mid-chain abort tears down the shared driver RTIO
 * context, and running it before the clean pipeline tests would contaminate
 * them.
 */
ZTEST_F(maxm86161_stream, test_zz_stream_bus_error_aborts)
{
	void *data = fixture->mock->data;

	maxm86161_mock_set_register(data, MAXM86161_REG_FIFO_DATA_COUNTER, 3);

	struct rtio_sqe *handle = NULL;

	zassert_ok(sensor_stream(&maxm_stream_iodev, &maxm_stream_rtio, NULL, &handle),
		   "sensor_stream (bus error) failed");
	k_sleep(K_MSEC(20));

	/* Fault the OVF/COUNT burst read that begins the drain. */
	maxm86161_mock_set_fault(data, MAXM86161_REG_FIFO_OVF_COUNTER, -EIO);
	maxm86161_mock_set_register(data, MAXM86161_REG_INT_STATUS1,
				    MAXM86161_MSK_INT_STATUS1_A_FULL);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 1);
	gpio_emul_input_set(fixture->intb_port, fixture->intb_pin, 0);

	int res = stream_wait_result(&maxm_stream_rtio);

	zassert_equal(res, -EIO, "Expected -EIO abort on faulted drain, got %d", res);

	maxm86161_mock_clear_fault(data);
	if (handle != NULL) {
		rtio_sqe_cancel(handle);
	}
	k_sleep(K_MSEC(20));
}

ZTEST_SUITE(maxm86161_stream, NULL, stream_setup, stream_before, NULL, NULL);

#endif /* CONFIG_MAXM86161_STREAM */
