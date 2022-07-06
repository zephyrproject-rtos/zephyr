/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <zephyr/zephyr.h>
#include <ztest.h>

#define ADC_DEVICE_NODE		DT_INST(0, zephyr_adc_emul)
#define ADC_REF_INTERNAL_MV	DT_PROP(DT_INST(0, zephyr_adc_emul), ref_internal_mv)
#define ADC_REF_EXTERNAL1_MV	DT_PROP(DT_INST(0, zephyr_adc_emul), ref_external1_mv)
#define ADC_RESOLUTION		14
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	0
#define ADC_2ND_CHANNEL_ID	1

#define INVALID_ADC_VALUE	SHRT_MIN
/* Raw to millivolt conversion doesn't handle rounding */
#define MV_OUTPUT_EPS		2
#define SEQUENCE_STEP		100

#define BUFFER_SIZE  6
static ZTEST_BMEM int16_t m_sample_buffer[BUFFER_SIZE];

/**
 * @brief Get ADC emulated device
 *
 * @return pointer to ADC device
 */
const struct device *get_adc_device(void)
{
	const struct device *adc_dev = DEVICE_DT_GET(ADC_DEVICE_NODE);

	zassert_true(device_is_ready(adc_dev), "ADC device is not ready");

	return adc_dev;
}

/**
 * @brief Setup channel with specific reference and gain
 *
 * @param adc_dev Pointer to ADC device
 * @param ref ADC reference voltage source
 * @param gain Gain applied to ADC @p channel
 * @param channel ADC channel which is being setup
 *
 * @return none
 */
static void channel_setup(const struct device *adc_dev, enum adc_reference ref,
			  enum adc_gain gain, int channel)
{
	int ret;
	struct adc_channel_cfg channel_cfg = {
		.gain             = gain,
		.reference        = ref,
		.acquisition_time = ADC_ACQUISITION_TIME,
		.channel_id       = channel,
	};

	ret = adc_channel_setup(adc_dev, &channel_cfg);
	zassert_ok(ret, "Setting up of the %d channel failed with code %d",
		   channel, ret);
}

/**
 * @brief Check if samples for specific channel are correct. It can check
 *        samples with arithmetic sequence with common difference.
 *
 * @param expected_count Number of samples that are expected to be set
 * @param start_mv_value Voltage in mV that was set on input at the beginning
 *                       of sampling
 * @param step Common difference in arithmetic sequence which describes how
 *             samples were generated
 * @param num_channels Number of channels that were sampled
 * @param channel_id ADC channel from which samples are checked
 * @param ref_mv Reference voltage in mV
 * @param gain Gain applied to ADC @p channel_id
 *
 * @return none
 */
static void check_samples(int expected_count, int32_t start_mv_value, int step,
			  int num_channels, int channel_id, int32_t ref_mv,
			  enum adc_gain gain)
{
	int32_t output, expected;
	int i, ret;

	for (i = channel_id; i < expected_count; i += num_channels) {
		expected = start_mv_value + i / num_channels * step;
		output = m_sample_buffer[i];
		ret = adc_raw_to_millivolts(ref_mv, gain, ADC_RESOLUTION,
					    &output);
		zassert_ok(ret, "adc_raw_to_millivolts() failed with code %d",
			   ret);
		zassert_within(expected, output, MV_OUTPUT_EPS,
			       "%u != %u [%u] should has set value",
			       expected, output, i);
	}

}

/**
 * @brief Check if any values in buffer were set after expected samples.
 *
 * @param expected_count Number of samples that are expected to be set
 *
 * @return none
 */
static void check_empty_samples(int expected_count)
{
	int i;

	for (i = expected_count; i < BUFFER_SIZE; i++) {
		zassert_equal(INVALID_ADC_VALUE, m_sample_buffer[i],
			      "[%u] should be empty", i);
	}
}

/**
 * @brief Run adc_read for given channels and collect specified number of
 *        samples.
 *
 * @param adc_dev Pointer to ADC device
 * @param channel_mask Mask of channels that will be sampled
 * @param samples Number of requested samples for each channel
 *
 * @return none
 */
static void start_adc_read(const struct device *adc_dev, uint32_t channel_mask,
			   int samples)
{
	int ret;
	const struct adc_sequence_options *options_ptr;

	const struct adc_sequence_options options = {
		.extra_samplings = samples - 1,
	};

	if (samples > 1) {
		options_ptr = &options;
	} else {
		options_ptr = NULL;
	}

	const struct adc_sequence sequence = {
		.options     = options_ptr,
		.channels    = channel_mask,
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = ADC_RESOLUTION,
	};

	ret = adc_read(adc_dev, &sequence);
	zassert_ok(ret, "adc_read() failed with code %d", ret);
}

/** @brief Data for handle_seq function */
struct handle_seq_params {
	/** Current input value in mV */
	unsigned int value;
};

/**
 * @brief Simple custom function to set as value input function for emulated
 *        ADC channel. It returns arithmetic sequence with SEQUENCE_STEP
 *        as common difference, starting from param value.
 *
 * @param dev Pointer to ADC device
 * @param channel ADC channel for which input value is requested
 * @param data Pointer to function parameters. It has to be
 *             struct handle_seq_params type
 * @param result Pointer where input value should be stored
 *
 * @return 0 on success
 * @return -EINVAL when current input value equals 0
 */
static int handle_seq(const struct device *dev, unsigned int channel,
		      void *data, uint32_t *result)
{
	struct handle_seq_params *param = data;

	if (param->value == 0) {
		return -EINVAL;
	}

	*result = param->value;
	param->value += SEQUENCE_STEP;

	return 0;
}

/** @brief Test setting one channel with constant output. */
static void test_adc_emul_single_value(void)
{
	const uint16_t input_mv = 1500;
	const int samples = 4;
	int ret, i;

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	}

	/* Generic ADC setup */
	const struct device *adc_dev = get_adc_device();

	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1,
		      ADC_1ST_CHANNEL_ID);

	/* ADC emulator-specific setup */
	ret = adc_emul_const_value_set(adc_dev, ADC_1ST_CHANNEL_ID, input_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	/* Test sampling */
	start_adc_read(adc_dev, BIT(ADC_1ST_CHANNEL_ID), samples);

	/* Check samples */
	check_samples(samples, input_mv, 0 /* step */, 1 /* channels */,
		      0 /* first channel data */, ADC_REF_INTERNAL_MV,
		      ADC_GAIN_1);

	check_empty_samples(samples);
}

/** @brief Test setting two channels with different constant output */
static void test_adc_emul_single_value_2ch(void)
{
	const uint16_t input1_mv = 3000;
	const uint16_t input2_mv = 2000;
	const int samples = 3;
	int ret, i;

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	}

	/* Generic ADC setup */
	const struct device *adc_dev = get_adc_device();

	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1,
		      ADC_1ST_CHANNEL_ID);
	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1,
		      ADC_2ND_CHANNEL_ID);

	/* ADC emulator-specific setup */
	ret = adc_emul_const_value_set(adc_dev, ADC_1ST_CHANNEL_ID, input1_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	ret = adc_emul_const_value_set(adc_dev, ADC_2ND_CHANNEL_ID, input2_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	/* Test sampling */
	start_adc_read(adc_dev, BIT(ADC_1ST_CHANNEL_ID) |
			BIT(ADC_2ND_CHANNEL_ID), samples);

	/* Check samples */
	check_samples(samples * 2, input1_mv, 0 /* step */, 2 /* channels */,
		      0 /* first channel data */, ADC_REF_INTERNAL_MV,
		      ADC_GAIN_1);
	check_samples(samples * 2, input2_mv, 0 /* step */, 2 /* channels */,
		      1 /* first channel data */, ADC_REF_INTERNAL_MV,
		      ADC_GAIN_1);

	check_empty_samples(samples * 2);
}

/** @brief Test setting one channel with custom function. */
static void test_adc_emul_custom_function(void)
{
	struct handle_seq_params channel1_param;
	const uint16_t input_mv = 1500;
	const int samples = 4;
	int ret, i;

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	}

	/* Generic ADC setup */
	const struct device *adc_dev = get_adc_device();

	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1,
		      ADC_1ST_CHANNEL_ID);

	/* ADC emulator-specific setup */
	channel1_param.value = input_mv;

	ret = adc_emul_value_func_set(adc_dev, ADC_1ST_CHANNEL_ID,
				      handle_seq, &channel1_param);
	zassert_ok(ret, "adc_emul_value_func_set() failed with code %d", ret);

	/* Test sampling */
	start_adc_read(adc_dev, BIT(ADC_1ST_CHANNEL_ID), samples);

	/* Check samples */
	check_samples(samples, input_mv, SEQUENCE_STEP, 1 /* channels */,
		      0 /* first channel data */, ADC_REF_INTERNAL_MV,
		      ADC_GAIN_1);

	check_empty_samples(samples);
}

/**
 * @brief Test setting two channels with custom function and different
 *        params.
 */
static void test_adc_emul_custom_function_2ch(void)
{
	struct handle_seq_params channel1_param;
	struct handle_seq_params channel2_param;
	const uint16_t input1_mv = 1500;
	const uint16_t input2_mv = 1000;
	const int samples = 3;
	int ret, i;

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	}

	/* Generic ADC setup */
	const struct device *adc_dev = get_adc_device();

	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1,
		      ADC_1ST_CHANNEL_ID);
	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1,
		      ADC_2ND_CHANNEL_ID);

	/* ADC emulator-specific setup */
	channel1_param.value = input1_mv;
	channel2_param.value = input2_mv;

	ret = adc_emul_value_func_set(adc_dev, ADC_1ST_CHANNEL_ID,
				      handle_seq, &channel1_param);
	zassert_ok(ret, "adc_emul_value_func_set() failed with code %d", ret);

	ret = adc_emul_value_func_set(adc_dev, ADC_2ND_CHANNEL_ID,
				      handle_seq, &channel2_param);
	zassert_ok(ret, "adc_emul_value_func_set() failed with code %d", ret);

	/* Test sampling */
	start_adc_read(adc_dev, BIT(ADC_1ST_CHANNEL_ID) |
			BIT(ADC_2ND_CHANNEL_ID), samples);

	/* Check samples */
	check_samples(samples * 2, input1_mv, SEQUENCE_STEP,
		      2 /* channels */, 0 /* first channel data */,
		      ADC_REF_INTERNAL_MV, ADC_GAIN_1);
	check_samples(samples * 2, input2_mv, SEQUENCE_STEP,
		      2 /* channels */, 1 /* first channel data */,
		      ADC_REF_INTERNAL_MV, ADC_GAIN_1);

	check_empty_samples(samples * 2);
}

/**
 * @brief Test setting two channels, one with custom function and
 *        one with constant value.
 */
static void test_adc_emul_custom_function_and_value(void)
{
	struct handle_seq_params channel1_param;
	const uint16_t input1_mv = 1500;
	const uint16_t input2_mv = 1000;
	const int samples = 3;
	int ret, i;

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	}

	/* Generic ADC setup */
	const struct device *adc_dev = get_adc_device();

	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1,
		      ADC_1ST_CHANNEL_ID);
	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1,
		      ADC_2ND_CHANNEL_ID);

	/* ADC emulator-specific setup */
	channel1_param.value = input1_mv;

	ret = adc_emul_value_func_set(adc_dev, ADC_1ST_CHANNEL_ID,
				      handle_seq, &channel1_param);
	zassert_ok(ret, "adc_emul_value_func_set() failed with code %d", ret);

	ret = adc_emul_const_value_set(adc_dev, ADC_2ND_CHANNEL_ID, input2_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	/* Test sampling */
	start_adc_read(adc_dev, BIT(ADC_1ST_CHANNEL_ID) |
			BIT(ADC_2ND_CHANNEL_ID), samples);

	/* Check samples */
	check_samples(samples * 2, input1_mv, SEQUENCE_STEP,
		      2 /* channels */, 0 /* first channel data */,
		      ADC_REF_INTERNAL_MV, ADC_GAIN_1);
	check_samples(samples * 2, input2_mv, 0 /* step */, 2 /* channels */,
		      1 /* first channel data */, ADC_REF_INTERNAL_MV,
		      ADC_GAIN_1);

	check_empty_samples(samples * 2);
}

/** @brief Test few different settings of gain argument. */
static void test_adc_emul_gain(void)
{
	const uint16_t input_mv = 1000;
	uint32_t channel_mask;
	const int samples = 3;
	int ret, i;

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	}

	/* Generic ADC setup */
	const struct device *adc_dev = get_adc_device();

	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1_6,
		      ADC_1ST_CHANNEL_ID);
	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_3,
		      ADC_2ND_CHANNEL_ID);

	/* ADC emulator-specific setup */
	channel_mask = BIT(ADC_1ST_CHANNEL_ID) | BIT(ADC_2ND_CHANNEL_ID);

	ret = adc_emul_const_value_set(adc_dev, ADC_1ST_CHANNEL_ID, input_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	ret = adc_emul_const_value_set(adc_dev, ADC_2ND_CHANNEL_ID, input_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	/* Test sampling */
	start_adc_read(adc_dev, channel_mask, samples);

	/* Check samples */
	check_samples(samples * 2, input_mv, 0 /* step */, 2 /* channels */,
		      0 /* first channel data */, ADC_REF_INTERNAL_MV,
		      ADC_GAIN_1_6);
	check_samples(samples * 2, input_mv, 0 /* step */, 2 /* channels */,
		      1 /* first channel data */, ADC_REF_INTERNAL_MV,
		      ADC_GAIN_3);

	check_empty_samples(samples * 2);

	/* Change gain and re-run test */
	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1_4,
		      ADC_1ST_CHANNEL_ID);
	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_2_3,
		      ADC_2ND_CHANNEL_ID);

	/* Test sampling */
	start_adc_read(adc_dev, channel_mask, samples);

	/* Check samples */
	check_samples(samples * 2, input_mv, 0 /* step */, 2 /* channels */,
		      0 /* first channel data */, ADC_REF_INTERNAL_MV,
		      ADC_GAIN_1_4);
	check_samples(samples * 2, input_mv, 0 /* step */, 2 /* channels */,
		      1 /* first channel data */, ADC_REF_INTERNAL_MV,
		      ADC_GAIN_2_3);

	check_empty_samples(samples * 2);
}

/**
 * @brief Test behaviour on input higher than reference. Return value should be
 *        cropped to reference value and cannot exceed resolution requested in
 *        adc_read().
 */
static void test_adc_emul_input_higher_than_ref(void)
{
	const uint16_t input_mv = ADC_REF_INTERNAL_MV + 100;
	const int samples = 4;
	int ret, i;

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	}

	/* Generic ADC setup */
	const struct device *adc_dev = get_adc_device();

	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1,
		      ADC_1ST_CHANNEL_ID);

	/* ADC emulator-specific setup */
	ret = adc_emul_const_value_set(adc_dev, ADC_1ST_CHANNEL_ID, input_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	/* Test sampling */
	start_adc_read(adc_dev, BIT(ADC_1ST_CHANNEL_ID), samples);

	/*
	 * Check samples - returned value should max out on reference value.
	 * Raw value shouldn't exceed resolution.
	 */
	check_samples(samples, ADC_REF_INTERNAL_MV, 0 /* step */,
		      1 /* channels */, 0 /* first channel data */,
		      ADC_REF_INTERNAL_MV, ADC_GAIN_1);

	check_empty_samples(samples);

	for (i = 0; i < samples; i++) {
		zassert_equal(BIT_MASK(ADC_RESOLUTION), m_sample_buffer[i],
			      "[%u] raw value isn't max value", i);
	}
}

/**
 * @brief Test different reference sources and if error is reported when
 *        unconfigured reference source is requested.
 */
static void test_adc_emul_reference(void)
{
	const uint16_t input1_mv = 4000;
	const uint16_t input2_mv = 2000;
	const int samples = 3;
	int ret, i;

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	}

	/* Generic ADC setup */
	const struct device *adc_dev = get_adc_device();

	channel_setup(adc_dev, ADC_REF_EXTERNAL1, ADC_GAIN_1,
		      ADC_1ST_CHANNEL_ID);

	struct adc_channel_cfg channel_cfg = {
		.gain             = ADC_GAIN_1,
		/* Reference value not setup in DTS */
		.reference        = ADC_REF_EXTERNAL0,
		.acquisition_time = ADC_ACQUISITION_TIME,
		.channel_id       = ADC_2ND_CHANNEL_ID,
	};

	ret = adc_channel_setup(adc_dev, &channel_cfg);
	zassert_not_equal(ret, 0,
			  "Setting up of the %d channel shouldn't succeeded",
			  ADC_2ND_CHANNEL_ID);

	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1,
		      ADC_2ND_CHANNEL_ID);

	/* ADC emulator-specific setup */
	ret = adc_emul_const_value_set(adc_dev, ADC_1ST_CHANNEL_ID, input1_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	ret = adc_emul_const_value_set(adc_dev, ADC_2ND_CHANNEL_ID, input2_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	/* Test sampling */
	start_adc_read(adc_dev, BIT(ADC_1ST_CHANNEL_ID) |
			BIT(ADC_2ND_CHANNEL_ID), samples);

	/* Check samples */
	check_samples(samples * 2, input1_mv, 0 /* step */, 2 /* channels */,
		      0 /* first channel data */, ADC_REF_EXTERNAL1_MV,
		      ADC_GAIN_1);
	check_samples(samples * 2, input2_mv, 0 /* step */, 2 /* channels */,
		      1 /* first channel data */, ADC_REF_INTERNAL_MV,
		      ADC_GAIN_1);

	check_empty_samples(samples * 2);
}

/** @brief Test setting reference value. */
static void test_adc_emul_ref_voltage_set(void)
{
	const uint16_t input1_mv = 4000;
	const uint16_t input2_mv = 2000;
	const uint16_t ref1_mv = 6000;
	const uint16_t ref2_mv = 9000;
	const int samples = 3;
	int ret, i;

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	}

	/* Generic ADC setup */
	const struct device *adc_dev = get_adc_device();

	channel_setup(adc_dev, ADC_REF_EXTERNAL1, ADC_GAIN_1,
		      ADC_1ST_CHANNEL_ID);
	channel_setup(adc_dev, ADC_REF_INTERNAL, ADC_GAIN_1,
		      ADC_2ND_CHANNEL_ID);

	/* ADC emulator-specific setup */
	ret = adc_emul_const_value_set(adc_dev, ADC_1ST_CHANNEL_ID, input1_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	ret = adc_emul_const_value_set(adc_dev, ADC_2ND_CHANNEL_ID, input2_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	/* Change reference voltage */
	ret = adc_emul_ref_voltage_set(adc_dev, ADC_REF_EXTERNAL1, ref1_mv);
	zassert_ok(ret, "adc_emul_ref_voltage_set() failed with code %d", ret);

	ret = adc_emul_ref_voltage_set(adc_dev, ADC_REF_INTERNAL, ref2_mv);
	zassert_ok(ret, "adc_emul_ref_voltage_set() failed with code %d", ret);

	/* Test sampling */
	start_adc_read(adc_dev, BIT(ADC_1ST_CHANNEL_ID) |
			BIT(ADC_2ND_CHANNEL_ID), samples);

	/* Check samples */
	check_samples(samples * 2, input1_mv, 0 /* step */, 2 /* channels */,
		      0 /* first channel data */, ref1_mv, ADC_GAIN_1);
	check_samples(samples * 2, input2_mv, 0 /* step */, 2 /* channels */,
		      1 /* first channel data */, ref2_mv, ADC_GAIN_1);

	check_empty_samples(samples * 2);

	/* Set previous reference voltage value */
	ret = adc_emul_ref_voltage_set(adc_dev, ADC_REF_EXTERNAL1,
				       ADC_REF_EXTERNAL1_MV);
	zassert_ok(ret, "adc_emul_ref_voltage_set() failed with code %d", ret);

	ret = adc_emul_ref_voltage_set(adc_dev, ADC_REF_INTERNAL,
				       ADC_REF_INTERNAL_MV);
	zassert_ok(ret, "adc_emul_ref_voltage_set() failed with code %d", ret);

	/* Test sampling */
	start_adc_read(adc_dev, BIT(ADC_1ST_CHANNEL_ID) |
			BIT(ADC_2ND_CHANNEL_ID), samples);

	/* Check samples */
	check_samples(samples * 2, input1_mv, 0 /* step */, 2 /* channels */,
		      0 /* first channel data */, ADC_REF_EXTERNAL1_MV,
		      ADC_GAIN_1);
	check_samples(samples * 2, input2_mv, 0 /* step */, 2 /* channels */,
		      1 /* first channel data */, ADC_REF_INTERNAL_MV,
		      ADC_GAIN_1);

	check_empty_samples(samples * 2);
}

void test_main(void)
{
	k_object_access_grant(get_adc_device(), k_current_get());

	ztest_test_suite(adc_basic_test,
			 ztest_user_unit_test(test_adc_emul_single_value),
			 ztest_user_unit_test(test_adc_emul_single_value_2ch),
			 ztest_user_unit_test(test_adc_emul_custom_function),
			 ztest_user_unit_test(test_adc_emul_custom_function_2ch),
			 ztest_user_unit_test(test_adc_emul_custom_function_and_value),
			 ztest_user_unit_test(test_adc_emul_gain),
			 ztest_user_unit_test(test_adc_emul_input_higher_than_ref),
			 ztest_user_unit_test(test_adc_emul_reference),
			 ztest_user_unit_test(test_adc_emul_ref_voltage_set));
	ztest_run_test_suite(adc_basic_test);
}
