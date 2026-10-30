/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Tests of the ADS1x1x driver with a simulated chip: reading several channels in one sequence,
 * and that a driver with one channel still behaves as before.
 */

#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/ztest.h>

#include "ads1x1x_emul.h"

#define ADS1115_NODE       DT_NODELABEL(ads1115)
#define ADS1115_FRESH_NODE DT_NODELABEL(ads1115_fresh)
#define ADS1115_ALERT_NODE DT_NODELABEL(ads1115_alert)
#define ADS1114_NODE       DT_NODELABEL(ads1114)

/* The data rate setting 7 (860 readings per second) keeps the tests short. */
#define FAST ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 7)
/* The setting 4 (128 readings per second) is the default. */
#define SLOW ADC_ACQ_TIME_DEFAULT

/* Bits of the configuration register, to check what the driver wrote. */
#define CFG_OS           BIT(15)
#define CFG_MUX(c)       (((c) >> 12) & 0x7)
#define CFG_PGA(c)       (((c) >> 9) & 0x7)
#define CFG_DR(c)        (((c) >> 5) & 0x7)
#define MUX_DIFF_0_3     1
#define MUX_SINGLE(n)    (4 + (n))
#define PGA_2048         2
#define PGA_1024         3
#define SINGLE_ENDED_RES 15
#define DIFFERENTIAL_RES 16

static const struct device *const adc = DEVICE_DT_GET(ADS1115_NODE);
static const struct device *const adc_fresh = DEVICE_DT_GET(ADS1115_FRESH_NODE);
static const struct device *const adc_alert = DEVICE_DT_GET(ADS1115_ALERT_NODE);
static const struct device *const adc_1114 = DEVICE_DT_GET(ADS1114_NODE);
static const struct emul *const emul = EMUL_DT_GET(ADS1115_NODE);
static struct ads1x1x_emul_data *state;

static int16_t samples[16];

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	state = emul->data;
	ads1x1x_emul_reset(emul);
	memset(samples, 0, sizeof(samples));
}

ZTEST_SUITE(ads1x1x, NULL, NULL, before, NULL, NULL);

/* Make a channel read one input against GND. */
static int setup_single(const struct device *dev, uint8_t channel, uint8_t input,
			enum adc_gain gain, uint16_t acquisition_time)
{
	const struct adc_channel_cfg cfg = {
		.gain = gain,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = acquisition_time,
		.channel_id = channel,
		.differential = false,
		.input_positive = input,
	};

	return adc_channel_setup(dev, &cfg);
}

/* Make a channel read input_positive minus input_negative. */
static int setup_differential(const struct device *dev, uint8_t channel, uint8_t positive,
			      uint8_t negative, enum adc_gain gain)
{
	const struct adc_channel_cfg cfg = {
		.gain = gain,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = FAST,
		.channel_id = channel,
		.differential = true,
		.input_positive = positive,
		.input_negative = negative,
	};

	return adc_channel_setup(dev, &cfg);
}

/* Read the channels of a mask in one go. */
static int read_channels(const struct device *dev, uint32_t mask, uint8_t resolution, void *buffer,
			 size_t buffer_size, uint8_t extra_samplings)
{
	const struct adc_sequence_options options = {
		.extra_samplings = extra_samplings,
	};
	const struct adc_sequence sequence = {
		.options = extra_samplings ? &options : NULL,
		.channels = mask,
		.buffer = buffer,
		.buffer_size = buffer_size,
		.resolution = resolution,
	};

	return adc_read(dev, &sequence);
}

static void set_input(uint8_t input, int32_t microvolts)
{
	state->input_uv[input] = microvolts;
}

ZTEST(ads1x1x, test_one_channel_reads_its_input)
{
	/* 1.65 V at +-2.048 V full scale: one count is 62.5 uV, so 26400 counts. */
	set_input(0, 1650000);

	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, FAST));
	zassert_ok(read_channels(adc, BIT(0), SINGLE_ENDED_RES, samples, sizeof(samples[0]), 0));

	zassert_equal(samples[0], 26400);
	zassert_equal(state->conversions, 1);
}

ZTEST(ads1x1x, test_channel_zero_can_read_any_input)
{
	set_input(2, 1000000);

	zassert_ok(setup_single(adc, 0, 2, ADC_GAIN_1, FAST));
	zassert_ok(read_channels(adc, BIT(0), SINGLE_ENDED_RES, samples, sizeof(samples[0]), 0));

	zassert_equal(samples[0], 16000);
}

ZTEST(ads1x1x, test_channel_zero_can_be_pointed_at_another_input_before_every_read)
{
	/*
	 * This is how the pointer stick was read before the driver had several channels: set up
	 * channel 0 for one input, read it, set it up for the other input, read it.
	 */
	set_input(0, 1650000);
	set_input(1, 1600000);

	for (int i = 0; i < 3; i++) {
		zassert_ok(setup_single(adc, 0, 1, ADC_GAIN_1, SLOW));
		zassert_ok(read_channels(adc, BIT(0), SINGLE_ENDED_RES, samples, sizeof(samples[0]),
					 0));
		zassert_equal(samples[0], 25600);

		zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, SLOW));
		zassert_ok(read_channels(adc, BIT(0), SINGLE_ENDED_RES, samples, sizeof(samples[0]),
					 0));
		zassert_equal(samples[0], 26400);
	}
}

ZTEST(ads1x1x, test_setting_up_a_channel_does_not_touch_the_chip)
{
	size_t transfers = state->transfers;

	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, FAST));
	zassert_ok(setup_single(adc, 1, 1, ADC_GAIN_2, SLOW));
	zassert_ok(setup_differential(adc, 2, 0, 3, ADC_GAIN_8));

	/* The settings are kept, and written when a conversion starts. */
	zassert_equal(state->transfers, transfers);
	zassert_equal(state->config_writes, 0);
}

static K_THREAD_STACK_DEFINE(reader_stack, 1024);
static struct k_thread reader_thread;
static volatile int reader_result;

static void reader_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	reader_result =
		read_channels(adc, BIT(0), SINGLE_ENDED_RES, samples, sizeof(samples[0]), 0);
}

ZTEST(ads1x1x, test_setting_up_a_channel_waits_for_a_conversion_in_progress)
{
	int64_t waited;

	set_input(0, 1650000);

	/* At 128 readings per second a conversion takes about 8 ms. */
	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, SLOW));
	state->busy_polls = 1;
	reader_result = -1;

	k_thread_create(&reader_thread, reader_stack, K_THREAD_STACK_SIZEOF(reader_stack),
			reader_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

	/* The reader has started its conversion, and the chip is still busy with it. */
	k_msleep(2);
	zassert_true(state->converting, "the conversion should still be running");

	waited = k_uptime_get();
	zassert_ok(setup_single(adc, 1, 1, ADC_GAIN_1, FAST));
	waited = k_uptime_get() - waited;

	/* Setting up another channel has to wait until the conversion is over. */
	zassert_false(state->converting, "the setup returned in the middle of the conversion");
	zassert_true(waited >= 3, "the setup did not wait (%lld ms)", waited);
	zassert_equal(state->writes_while_converting, 0);

	k_thread_join(&reader_thread, K_FOREVER);
	zassert_ok(reader_result);
	zassert_equal(samples[0], 26400);
}

ZTEST(ads1x1x, test_two_channels_are_read_in_one_sequence)
{
	set_input(0, 1650000);
	set_input(1, 1600000);

	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, FAST));
	zassert_ok(setup_single(adc, 1, 1, ADC_GAIN_1, FAST));
	state->config_writes = 0;

	zassert_ok(read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
				 2 * sizeof(samples[0]), 0));

	zassert_equal(samples[0], 26400);
	zassert_equal(samples[1], 25600);
	zassert_equal(state->conversions, 2);

	/* One conversion for each channel, each with its own input selected. */
	zassert_equal(state->config_writes, 2);
	zassert_true(state->config_log[0] & CFG_OS);
	zassert_equal(CFG_MUX(state->config_log[0]), MUX_SINGLE(0));
	zassert_true(state->config_log[1] & CFG_OS);
	zassert_equal(CFG_MUX(state->config_log[1]), MUX_SINGLE(1));
}

ZTEST(ads1x1x, test_samples_are_ordered_by_channel_number_not_by_input)
{
	set_input(0, 1000000);
	set_input(1, 2000000);

	/* Channel 0 reads input 1, and channel 1 reads input 0. */
	zassert_ok(setup_single(adc, 0, 1, ADC_GAIN_1, FAST));
	zassert_ok(setup_single(adc, 1, 0, ADC_GAIN_1, FAST));

	zassert_ok(read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
				 2 * sizeof(samples[0]), 0));

	zassert_equal(samples[0], 32000, "channel 0 comes first");
	zassert_equal(samples[1], 16000);
}

ZTEST(ads1x1x, test_a_sparse_mask_reads_only_the_selected_channels)
{
	for (uint8_t i = 0; i < 4; i++) {
		set_input(i, 500000 * (i + 1));
		zassert_ok(setup_single(adc, i, i, ADC_GAIN_1, FAST));
	}

	zassert_ok(read_channels(adc, BIT(1) | BIT(3), SINGLE_ENDED_RES, samples,
				 2 * sizeof(samples[0]), 0));

	zassert_equal(samples[0], 16000);
	zassert_equal(samples[1], 32000);
	zassert_equal(state->conversions, 2);
}

ZTEST(ads1x1x, test_every_channel_keeps_its_own_gain_and_speed)
{
	set_input(0, 500000);
	set_input(1, 500000);

	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, FAST));
	zassert_ok(setup_single(adc, 1, 1, ADC_GAIN_2, SLOW));
	state->config_writes = 0;

	zassert_ok(read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
				 2 * sizeof(samples[0]), 0));

	/* The same 0.5 V is twice as many counts at half the range. */
	zassert_equal(samples[0], 8000);
	zassert_equal(samples[1], 16000);

	zassert_equal(CFG_PGA(state->config_log[0]), PGA_2048);
	zassert_equal(CFG_DR(state->config_log[0]), 7);
	zassert_equal(CFG_PGA(state->config_log[1]), PGA_1024);
	zassert_equal(CFG_DR(state->config_log[1]), 4);
}

ZTEST(ads1x1x, test_extra_samplings_repeat_all_the_channels)
{
	set_input(0, 1000000);
	set_input(1, 2000000);

	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, FAST));
	zassert_ok(setup_single(adc, 1, 1, ADC_GAIN_1, FAST));

	zassert_ok(read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
				 4 * sizeof(samples[0]), 1));

	zassert_equal(samples[0], 16000);
	zassert_equal(samples[1], 32000);
	zassert_equal(samples[2], 16000);
	zassert_equal(samples[3], 32000);
	zassert_equal(state->conversions, 4);
}

ZTEST(ads1x1x, test_waits_while_the_chip_is_converting)
{
	set_input(0, 1650000);
	set_input(1, 1600000);
	state->busy_polls = 3;

	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, FAST));
	zassert_ok(setup_single(adc, 1, 1, ADC_GAIN_1, FAST));
	zassert_ok(read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
				 2 * sizeof(samples[0]), 0));

	zassert_equal(samples[0], 26400);
	zassert_equal(samples[1], 25600);
}

ZTEST(ads1x1x, test_two_reads_in_a_row_both_work)
{
	set_input(0, 1650000);
	set_input(1, 1600000);

	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, FAST));
	zassert_ok(setup_single(adc, 1, 1, ADC_GAIN_1, FAST));

	for (int i = 0; i < 3; i++) {
		memset(samples, 0, sizeof(samples));
		zassert_ok(read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
					 2 * sizeof(samples[0]), 0));
		zassert_equal(samples[0], 26400);
		zassert_equal(samples[1], 25600);
	}
}

ZTEST(ads1x1x, test_a_differential_channel_can_be_read_alone)
{
	set_input(0, 1650000);
	set_input(3, 1640000);

	zassert_ok(setup_differential(adc, 0, 0, 3, ADC_GAIN_8));
	zassert_ok(read_channels(adc, BIT(0), DIFFERENTIAL_RES, samples, sizeof(samples[0]), 0));

	/* 10 mV at +-0.256 V full scale: 7.8125 uV per count. */
	zassert_equal(samples[0], 1280);
	zassert_equal(CFG_MUX(state->config_log[state->config_writes - 1]), MUX_DIFF_0_3);
}

ZTEST(ads1x1x, test_single_ended_and_differential_channels_cannot_share_a_sequence)
{
	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, FAST));
	zassert_ok(setup_differential(adc, 1, 0, 3, ADC_GAIN_8));

	/* They need a different resolution, so one sequence cannot have both. */
	zassert_not_ok(read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
				     2 * sizeof(samples[0]), 0));
	zassert_not_ok(read_channels(adc, BIT(0) | BIT(1), DIFFERENTIAL_RES, samples,
				     2 * sizeof(samples[0]), 0));

	/* Each one still works alone. */
	zassert_ok(read_channels(adc, BIT(0), SINGLE_ENDED_RES, samples, sizeof(samples[0]), 0));
	zassert_ok(read_channels(adc, BIT(1), DIFFERENTIAL_RES, samples, sizeof(samples[0]), 0));
}

ZTEST(ads1x1x, test_a_channel_that_was_not_set_up_is_refused)
{
	/* This chip is not used by any other test, so no channel is set up. */
	zassert_ok(setup_single(adc_fresh, 0, 0, ADC_GAIN_1, FAST));

	zassert_equal(read_channels(adc_fresh, BIT(0) | BIT(2), SINGLE_ENDED_RES, samples,
				    2 * sizeof(samples[0]), 0),
		      -EINVAL);
	zassert_equal(
		read_channels(adc_fresh, BIT(1), SINGLE_ENDED_RES, samples, sizeof(samples[0]), 0),
		-EINVAL);
}

ZTEST(ads1x1x, test_an_empty_or_out_of_range_mask_is_refused)
{
	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, FAST));

	zassert_not_ok(read_channels(adc, 0, SINGLE_ENDED_RES, samples, sizeof(samples[0]), 0));
	zassert_not_ok(
		read_channels(adc, BIT(4), SINGLE_ENDED_RES, samples, sizeof(samples[0]), 0));
	zassert_not_ok(read_channels(adc, BIT(0) | BIT(7), SINGLE_ENDED_RES, samples,
				     2 * sizeof(samples[0]), 0));
}

ZTEST(ads1x1x, test_the_buffer_must_hold_every_sample)
{
	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, FAST));
	zassert_ok(setup_single(adc, 1, 1, ADC_GAIN_1, FAST));

	zassert_not_ok(read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
				     sizeof(samples[0]), 0));
	/* Extra samplings multiply the need. */
	zassert_not_ok(read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
				     2 * sizeof(samples[0]), 1));
	zassert_ok(read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
				 4 * sizeof(samples[0]), 1));
}

ZTEST(ads1x1x, test_channel_numbers_beyond_the_chip_are_refused)
{
	zassert_not_ok(setup_single(adc, 4, 0, ADC_GAIN_1, FAST));
	zassert_not_ok(setup_single(adc, 7, 0, ADC_GAIN_1, FAST));
}

ZTEST(ads1x1x, test_a_chip_without_a_multiplexer_has_only_channel_zero)
{
	zassert_not_ok(setup_single(adc_1114, 1, 0, ADC_GAIN_1, FAST));
	zassert_ok(setup_single(adc_1114, 0, 0, ADC_GAIN_1, FAST));
}

ZTEST(ads1x1x, test_several_channels_are_refused_when_the_alert_pin_is_used)
{
	/* The pin reports one conversion at a time, so a sequence can only have one channel. */
	zassert_ok(setup_single(adc_alert, 0, 0, ADC_GAIN_1, FAST));
	zassert_ok(setup_single(adc_alert, 1, 1, ADC_GAIN_1, FAST));

	zassert_equal(read_channels(adc_alert, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
				    2 * sizeof(samples[0]), 0),
		      -ENOTSUP);
}

ZTEST(ads1x1x, test_oversampling_is_still_refused)
{
	const struct adc_sequence sequence = {
		.channels = BIT(0),
		.buffer = samples,
		.buffer_size = sizeof(samples[0]),
		.resolution = SINGLE_ENDED_RES,
		.oversampling = 2,
	};

	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, FAST));
	zassert_not_ok(adc_read(adc, &sequence));
}

ZTEST(ads1x1x, test_an_i2c_failure_is_reported_and_the_driver_recovers)
{
	size_t transfers;

	set_input(0, 1650000);
	set_input(1, 1600000);

	zassert_ok(setup_single(adc, 0, 0, ADC_GAIN_1, FAST));
	zassert_ok(setup_single(adc, 1, 1, ADC_GAIN_1, FAST));

	/* How many I2C transfers does a clean two channel read need? */
	state->transfers = 0;
	zassert_ok(read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
				 2 * sizeof(samples[0]), 0));
	transfers = state->transfers;
	zassert_true(transfers >= 2, "a read needs at least one transfer per channel");

	/* Fail at every one of them, one at a time. */
	for (size_t step = 1; step <= transfers; step++) {
		int ret;

		state->fail_in = step;
		state->fail_error = -EIO;
		ret = read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
				    2 * sizeof(samples[0]), 0);

		zassert_equal(ret, -EIO, "step %zu returned %d", step, ret);

		/* The next read works, with nothing left over from the failure. */
		state->fail_in = 0;
		memset(samples, 0, sizeof(samples));
		zassert_ok(read_channels(adc, BIT(0) | BIT(1), SINGLE_ENDED_RES, samples,
					 2 * sizeof(samples[0]), 0),
			   "no recovery after step %zu", step);
		zassert_equal(samples[0], 26400, "step %zu", step);
		zassert_equal(samples[1], 25600, "step %zu", step);
	}
}
