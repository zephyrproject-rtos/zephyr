/*
 * Copyright (c) 2023 The ChromiumOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <zephyr/drivers/adc/voltage_divider.h>
#include <zephyr/drivers/adc/current_sense_shunt.h>
#include <zephyr/drivers/adc/current_sense_amplifier.h>
#include <zephyr/drivers/adc/temp_transducer.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* Raw to millivolt conversion doesn't handle rounding */
#define MV_OUTPUT_EPS 10

#define ADC_TEST_NODE_0 DT_NODELABEL(sensor0)
#define ADC_TEST_NODE_1 DT_NODELABEL(sensor1)
#define ADC_TEST_NODE_2 DT_NODELABEL(sensor2)
#define ADC_TEST_NODE_3 DT_NODELABEL(sensor3)
#define ADC_TEST_NODE_4 DT_NODELABEL(sensor4)
#define ADC_TEST_NODE_5 DT_NODELABEL(sensor5)
#define ADC_TEST_NODE_6 DT_NODELABEL(sensor6)

/**
 * @brief Get ADC emulated device
 *
 * @return pointer to ADC device
 */
const struct device *get_adc_device(void)
{
	const struct device *const adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));

	zassert_true(device_is_ready(adc_dev), "ADC device is not ready");

	return adc_dev;
}

static int init_adc(const struct adc_dt_spec *spec, int input_mv)
{
	int ret;

	zassert_true(adc_is_ready_dt(spec), "ADC device is not ready");

	ret = adc_channel_setup_dt(spec);
	zassert_equal(ret, 0, "Setting up of the first channel failed with code %d", ret);

	/* ADC emulator-specific setup */
	ret = adc_emul_const_value_set(spec->dev, spec->channel_id, input_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	return ret;
}

/*
 * test_adc_voltage_divider
 */
static int test_task_voltage_divider(void)
{
	int ret;
	int32_t calculated_voltage = 0;
	int32_t calculated_microvolts;
	int32_t input_mv = 1000;
	const struct voltage_divider_dt_spec adc_node_0 =
		VOLTAGE_DIVIDER_DT_SPEC_GET(ADC_TEST_NODE_0);

	ret = init_adc(&adc_node_0.port, input_mv);
	zassert_equal(ret, 0, "Setting up of the first channel failed with code %d", ret);

	struct adc_sequence sequence = {
		.buffer = &calculated_voltage,
		.buffer_size = sizeof(calculated_voltage),
	};
	adc_sequence_init_dt(&adc_node_0.port, &sequence);

	ret = adc_read_dt(&adc_node_0.port, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);
	calculated_microvolts = calculated_voltage;

	ret = adc_raw_to_millivolts_dt(&adc_node_0.port, &calculated_voltage);
	zassert_equal(ret, 0, "adc_raw_to_millivolts_dt() failed with code %d", ret);

	ret = adc_raw_to_microvolts_dt(&adc_node_0.port, &calculated_microvolts);
	zassert_equal(ret, 0, "adc_raw_to_microvolts_dt() failed with code %d", ret);
	zassert_equal(calculated_microvolts / 1000, calculated_voltage);

	ret = voltage_divider_scale_dt(&adc_node_0, &calculated_voltage);
	zassert_equal(ret, 0, "divider_scale_voltage_dt() failed with code %d", ret);

	zassert_within(calculated_voltage, input_mv * 2, MV_OUTPUT_EPS,
		       "%u != %u should have set value", calculated_voltage, input_mv * 2);

	/* test case: voltage_divider_scale64_dt */
	const int32_t divider_gain = 2;
	const int32_t expected_output_uv = input_mv * 1000 * divider_gain;
	int64_t output_uv = calculated_microvolts;

	ret = voltage_divider_scale64_dt(&adc_node_0, &output_uv);
	zassert_ok(ret, "voltage_divider_scale64_dt() failed with code %d", ret);
	zassert_within(output_uv, expected_output_uv, divider_gain * 1000,
		       "%lld != %d should have set value", output_uv, expected_output_uv);

	return TC_PASS;
}

ZTEST_USER(adc_rescale, test_adc_voltage_divider)
{
	zassert_true(test_task_voltage_divider() == TC_PASS);
}

/*
 * test_adc_current_sense_shunt
 */
static int test_task_current_sense_shunt(void)
{
	int ret;
	int32_t calculated_current = 0;
	int32_t calculated_microvolts;
	int32_t input_mv = 3000;
	const struct current_sense_shunt_dt_spec adc_node_1 =
		CURRENT_SENSE_SHUNT_DT_SPEC_GET(ADC_TEST_NODE_1);

	ret = init_adc(&adc_node_1.port, input_mv);
	zassert_equal(ret, 0, "Setting up of the second channel failed with code %d", ret);

	struct adc_sequence sequence = {
		.buffer = &calculated_current,
		.buffer_size = sizeof(calculated_current),
	};
	adc_sequence_init_dt(&adc_node_1.port, &sequence);

	ret = adc_read_dt(&adc_node_1.port, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);
	calculated_microvolts = calculated_current;

	ret = adc_raw_to_millivolts_dt(&adc_node_1.port, &calculated_current);
	zassert_equal(ret, 0, "adc_raw_to_millivolts_dt() failed with code %d", ret);

	ret = adc_raw_to_microvolts_dt(&adc_node_1.port, &calculated_microvolts);
	zassert_equal(ret, 0, "adc_raw_to_microvolts_dt() failed with code %d", ret);
	zassert_equal(calculated_microvolts / 1000, calculated_current);

	current_sense_shunt_scale_dt(&adc_node_1, &calculated_current);

	zassert_within(calculated_current, input_mv * 2, MV_OUTPUT_EPS,
		       "%u != %u should have set value", calculated_current,
		       input_mv * 2);

	return TC_PASS;
}

ZTEST_USER(adc_rescale, test_adc_current_sense_shunt)
{
	zassert_true(test_task_current_sense_shunt() == TC_PASS);
}

/*
 * test_adc_current_sense_amplifier
 */
static int test_task_current_sense_amplifier(void)
{
	int ret;
	int32_t calculated_current = 0;
	int32_t calculated_microvolts;
	int32_t input_mv = 3000;
	const struct current_sense_amplifier_dt_spec adc_node_2 =
		CURRENT_SENSE_AMPLIFIER_DT_SPEC_GET(ADC_TEST_NODE_2);

	ret = init_adc(&adc_node_2.port, input_mv);
	zassert_equal(ret, 0, "Setting up of the third channel failed with code %d", ret);

	struct adc_sequence sequence = {
		.buffer = &calculated_current,
		.buffer_size = sizeof(calculated_current),
	};
	adc_sequence_init_dt(&adc_node_2.port, &sequence);

	ret = adc_read_dt(&adc_node_2.port, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);
	calculated_microvolts = calculated_current;

	ret = adc_raw_to_millivolts_dt(&adc_node_2.port, &calculated_current);
	zassert_equal(ret, 0, "adc_raw_to_millivolts_dt() failed with code %d", ret);

	ret = adc_raw_to_microvolts_dt(&adc_node_2.port, &calculated_microvolts);
	zassert_equal(ret, 0, "adc_raw_to_microvolts_dt() failed with code %d", ret);
	zassert_equal(calculated_microvolts / 1000, calculated_current);

	current_sense_amplifier_scale_dt(&adc_node_2, &calculated_current);

	zassert_within(calculated_current, input_mv * 2, MV_OUTPUT_EPS,
		       "%u != %u should have set value", calculated_current,
		       input_mv * 2);

	return TC_PASS;
}

ZTEST_USER(adc_rescale, test_adc_current_sense_amplifier)
{
	zassert_true(test_task_current_sense_amplifier() == TC_PASS);
}

ZTEST(adc_rescale, test_adc_current_sense_amplifier_with_offset)
{
	int32_t v_to_i;
	const struct current_sense_amplifier_dt_spec amplifier_spec =
		CURRENT_SENSE_AMPLIFIER_DT_SPEC_GET(ADC_TEST_NODE_3);

	/**
	 * test a voltage that corresponds to 0 mA
	 */
	v_to_i = amplifier_spec.zero_current_voltage_mv;
	current_sense_amplifier_scale_dt(&amplifier_spec, &v_to_i);
	zassert_equal(v_to_i, 0);

	/**
	 * test a voltage that corresponds to 200 mA
	 */
	v_to_i = (200 * amplifier_spec.sense_gain_mult / amplifier_spec.sense_gain_div) / 1000;
	v_to_i = v_to_i + amplifier_spec.zero_current_voltage_mv;
	current_sense_amplifier_scale_dt(&amplifier_spec, &v_to_i);
	zassert_equal(v_to_i, 200);

	/**
	 * test a voltage that corresponds to -1100 mA
	 */
	v_to_i = (-1100 * amplifier_spec.sense_gain_mult / amplifier_spec.sense_gain_div) / 1000;
	v_to_i = v_to_i + amplifier_spec.zero_current_voltage_mv;
	current_sense_amplifier_scale_dt(&amplifier_spec, &v_to_i);
	zassert_equal(v_to_i, -1100);
}

/*
 * test_adc_temp_transducer - current type transducer (AD590)
 *
 * the AD950 is calibrate to output 298.2 μA at 298.2 K (25°C)
 * with a 8060 ohm resistor: U = R*i = 8060 * 298.2 = 2403 mV
 *
 */
ZTEST_USER(adc_rescale, test_adc_temp_transducer)
{
	int ret;
	int32_t calculated_temp = 0;
	int32_t calculated_microvolts;
	int32_t input_mv = 2403;
	const struct temp_transducer_dt_spec adc_node_4 =
		TEMP_TRANSDUCER_DT_SPEC_GET(ADC_TEST_NODE_4);

	ret = init_adc(&adc_node_4.port, input_mv);
	zassert_equal(ret, 0, "Setting up of the temp transducer channel failed with code %d", ret);

	struct adc_sequence sequence = {
		.buffer = &calculated_temp,
		.buffer_size = sizeof(calculated_temp),
	};
	adc_sequence_init_dt(&adc_node_4.port, &sequence);

	ret = adc_read_dt(&adc_node_4.port, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);
	calculated_microvolts = calculated_temp;

	ret = adc_raw_to_millivolts_dt(&adc_node_4.port, &calculated_temp);
	zassert_equal(ret, 0, "adc_raw_to_millivolts_dt() failed with code %d", ret);

	ret = adc_raw_to_microvolts_dt(&adc_node_4.port, &calculated_microvolts);
	zassert_equal(ret, 0, "adc_raw_to_microvolts_dt() failed with code %d", ret);
	zassert_equal(calculated_microvolts / 1000, calculated_temp);

	calculated_temp = temp_transducer_scale_dt(&adc_node_4, calculated_temp);

	zassert_within(calculated_temp, 25000, 1000,
		       "%d != 25000, should have calculated 25°C", calculated_temp);
}

/*
 * test_adc_temp_transducer_voltage_type - Test voltage type transducer (LTC2997)
 */
ZTEST(adc_rescale, test_adc_temp_transducer_voltage_type)
{
	int32_t millicelsius;
	const struct temp_transducer_dt_spec transducer_spec =
		TEMP_TRANSDUCER_DT_SPEC_GET(ADC_TEST_NODE_5);

	/**
	 * Test 0°C (273.15K)
	 * For LTC2997 with alpha = 4000 µV/K (4 mV/K), Rsense = 1 ohm
	 * V = 4 mV/K * 273.15K = 1092.6 mV
	 */
	millicelsius = temp_transducer_scale_dt(&transducer_spec, 1093);
	zassert_within(millicelsius, 0, 500, "Expected ~0°C, got %d mC", millicelsius);

	/**
	 * Test 25°C (298.15K)
	 * V = 4 mV/K * 298.15K = 1192.6 mV
	 */
	millicelsius = temp_transducer_scale_dt(&transducer_spec, 1193);
	zassert_within(millicelsius, 25000, 500, "Expected ~25°C, got %d mC", millicelsius);

	/**
	 * Test -40°C (233.15K)
	 * V = 4 mV/K * 233.15K = 932.6 mV
	 */
	millicelsius = temp_transducer_scale_dt(&transducer_spec, 933);
	zassert_within(millicelsius, -40000, 500, "Expected ~-40°C, got %d mC", millicelsius);

	/**
	 * Test 125°C (398.15K)
	 * V = 4 mV/K * 398.15K = 1592.6 mV
	 */
	millicelsius = temp_transducer_scale_dt(&transducer_spec, 1593);
	zassert_within(millicelsius, 125000, 500, "Expected ~125°C, got %d mC", millicelsius);
}

/*
 * test_adc_temp_transducer_tmp236 - Test TMP236 with full ADC cycle
 */
ZTEST_USER(adc_rescale, test_adc_temp_transducer_tmp236)
{
	int ret;
	int32_t calculated_temp = 0;
	int32_t calculated_microvolts;
	/* TMP236: At 25°C, VOUT = 400 mV + (19.5 mV/°C × 25) = 887.5 mV */
	int32_t input_mv = 888;
	const struct temp_transducer_dt_spec adc_node_6 =
		TEMP_TRANSDUCER_DT_SPEC_GET(ADC_TEST_NODE_6);

	ret = init_adc(&adc_node_6.port, input_mv);
	zassert_equal(ret, 0, "Setting up of the TMP236 channel failed with code %d", ret);

	struct adc_sequence sequence = {
		.buffer = &calculated_temp,
		.buffer_size = sizeof(calculated_temp),
	};
	adc_sequence_init_dt(&adc_node_6.port, &sequence);

	ret = adc_read_dt(&adc_node_6.port, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);
	calculated_microvolts = calculated_temp;

	ret = adc_raw_to_millivolts_dt(&adc_node_6.port, &calculated_temp);
	zassert_equal(ret, 0, "adc_raw_to_millivolts_dt() failed with code %d", ret);

	ret = adc_raw_to_microvolts_dt(&adc_node_6.port, &calculated_microvolts);
	zassert_equal(ret, 0, "adc_raw_to_microvolts_dt() failed with code %d", ret);
	zassert_equal(calculated_microvolts / 1000, calculated_temp);

	calculated_temp = temp_transducer_scale_dt(&adc_node_6, calculated_temp);

	/* Expected: 25°C = 25000 millicelsius */
	zassert_within(calculated_temp, 25000, 500,
		       "%d != 25000, should have calculated 25°C", calculated_temp);
}

/*
 * test_adc_temp_transducer_tmp236_datasheet - Verify TMP236 against datasheet values
 */
ZTEST(adc_rescale, test_adc_temp_transducer_tmp236_datasheet)
{
	int32_t millicelsius;
	const struct temp_transducer_dt_spec tmp236_spec =
		TEMP_TRANSDUCER_DT_SPEC_GET(ADC_TEST_NODE_6);

	/*
	 * TMP236 Transfer Function: VOUT = 400 mV + (19.5 mV/°C × T)
	 * Offset calculation: -(400 × 1,000,000) / 19500 = -20513 mC
	 */

	/**
	 * Test -40°C
	 * VOUT = 400 + (19.5 × -40) = 400 - 780 = -380 mV (below ground, not realistic)
	 * TMP236 typically doesn't go this low, but test the math
	 */
	millicelsius = temp_transducer_scale_dt(&tmp236_spec, -380);
	zassert_within(millicelsius, -40000, 100, "Expected -40°C, got %d mC", millicelsius);

	/**
	 * Test 0°C
	 * VOUT = 400 + (19.5 × 0) = 400 mV
	 */
	millicelsius = temp_transducer_scale_dt(&tmp236_spec, 400);
	zassert_within(millicelsius, 0, 100, "Expected 0°C, got %d mC", millicelsius);

	/**
	 * Test 25°C
	 * VOUT = 400 + (19.5 × 25) = 887.5 mV
	 */
	millicelsius = temp_transducer_scale_dt(&tmp236_spec, 888);
	zassert_within(millicelsius, 25000, 100, "Expected 25°C, got %d mC", millicelsius);

	/**
	 * Test 50°C
	 * VOUT = 400 + (19.5 × 50) = 1375 mV
	 */
	millicelsius = temp_transducer_scale_dt(&tmp236_spec, 1375);
	zassert_within(millicelsius, 50000, 100, "Expected 50°C, got %d mC", millicelsius);

	/**
	 * Test 100°C
	 * VOUT = 400 + (19.5 × 100) = 2350 mV
	 */
	millicelsius = temp_transducer_scale_dt(&tmp236_spec, 2350);
	zassert_within(millicelsius, 100000, 100, "Expected 100°C, got %d mC", millicelsius);

	/**
	 * Test 125°C
	 * VOUT = 400 + (19.5 × 125) = 2837.5 mV
	 */
	millicelsius = temp_transducer_scale_dt(&tmp236_spec, 2838);
	zassert_within(millicelsius, 125000, 100, "Expected 125°C, got %d mC", millicelsius);
}

void *adc_rescale_setup(void)
{
	k_object_access_grant(get_adc_device(), k_current_get());

	return NULL;
}

ZTEST_SUITE(adc_rescale, NULL, adc_rescale_setup, NULL, NULL, NULL);
