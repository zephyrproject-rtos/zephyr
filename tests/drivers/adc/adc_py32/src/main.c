/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static const struct device *adc_dev(void)
{
	return DEVICE_DT_GET(DT_NODELABEL(adc1));
}

static void *adc_setup(void)
{
	struct adc_channel_cfg channel = {
		.gain = ADC_GAIN_1,
		.reference = ADC_REF_VDD_1,
		.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	};

	zassert_true(device_is_ready(adc_dev()));
	for (uint8_t id = 11U; id <= 12U; id++) {
		channel.channel_id = id;
		zassert_ok(adc_channel_setup(adc_dev(), &channel));
	}
	return NULL;
}

ZTEST(adc_py32, test_internal_channels_and_calibration)
{
	uint16_t buffer[2] = {0U};
	struct adc_sequence sequence = {
		.channels = BIT(11) | BIT(12),
		.buffer = buffer,
		.buffer_size = sizeof(buffer),
		.resolution = 12,
		.calibrate = true,
	};

	zassert_ok(adc_read(adc_dev(), &sequence));
	TC_PRINT("Temperature raw: %u; VREFINT raw: %u\n", buffer[0], buffer[1]);
	/* Both internal sources must lie strictly between ground and VDDA. */
	zassert_between_inclusive(buffer[0], 1U, 4094U);
	/* Nominal 1.2 V reference with a 3.3 V supply, allowing supply tolerance. */
	zassert_between_inclusive(buffer[1], 1000U, 2200U);
}

ZTEST(adc_py32, test_resolution)
{
	uint16_t buffer;
	struct adc_sequence sequence = {
		.channels = BIT(12),
		.buffer = &buffer,
		.buffer_size = sizeof(buffer),
		.resolution = 12,
	};
	uint16_t reference;

	zassert_ok(adc_read(adc_dev(), &sequence));
	reference = buffer;
	for (uint8_t resolution = 6U; resolution <= 12U; resolution += 2U) {
		sequence.resolution = resolution;
		zassert_ok(adc_read(adc_dev(), &sequence));
		zassert_within((uint32_t)buffer << (12U - resolution), reference, 80U);
	}
}

static enum adc_action repeat_callback(const struct device *dev,
				       const struct adc_sequence *sequence, uint16_t sampling_index)
{
	uint8_t *calls = sequence->options->user_data;

	ARG_UNUSED(dev);
	ARG_UNUSED(sampling_index);
	(*calls)++;
	return *calls == 1U ? ADC_ACTION_REPEAT : ADC_ACTION_CONTINUE;
}

ZTEST(adc_py32, test_repeated_sampling)
{
	uint8_t calls = 0U;
	uint16_t buffer[3] = {0U};
	struct adc_sequence_options options = {
		.interval_us = 1000U,
		.extra_samplings = 2U,
		.callback = repeat_callback,
		.user_data = &calls,
	};
	struct adc_sequence sequence = {
		.channels = BIT(12),
		.buffer = buffer,
		.buffer_size = sizeof(buffer),
		.resolution = 12,
		.options = &options,
	};

	zassert_ok(adc_read(adc_dev(), &sequence));
	zassert_equal(calls, 4U);
	for (size_t i = 0U; i < ARRAY_SIZE(buffer); i++) {
		zassert_between_inclusive(buffer[i], 1000U, 2200U);
	}
}

ZTEST(adc_py32, test_async)
{
	struct k_poll_signal completion;
	struct k_poll_event event =
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &completion);
	uint16_t buffer;
	struct adc_sequence sequence = {
		.channels = BIT(12),
		.buffer = &buffer,
		.buffer_size = sizeof(buffer),
		.resolution = 12,
	};

	k_poll_signal_init(&completion);
	zassert_ok(adc_read_async(adc_dev(), &sequence, &completion));
	zassert_ok(k_poll(&event, 1, K_SECONDS(1)));
	zassert_equal(completion.result, 0);
	TC_PRINT("VREFINT raw: %u\n", buffer);
	zassert_between_inclusive(buffer, 1000U, 2200U);
}

ZTEST(adc_py32, test_invalid_requests)
{
	uint16_t buffer;
	struct adc_sequence sequence = {
		.channels = BIT(12),
		.buffer = &buffer,
		.buffer_size = 1U,
		.resolution = 12,
	};
	struct adc_channel_cfg channel = {
		.channel_id = 10U,
		.gain = ADC_GAIN_1,
		.reference = ADC_REF_VDD_1,
	};

	zassert_equal(adc_channel_setup(adc_dev(), &channel), -EINVAL);
	channel.channel_id = 12U;
	channel.gain = ADC_GAIN_2;
	zassert_equal(adc_channel_setup(adc_dev(), &channel), -ENOTSUP);
	zassert_equal(adc_read(adc_dev(), &sequence), -ENOMEM);
	sequence.buffer_size = sizeof(buffer);
	sequence.resolution = 9;
	zassert_equal(adc_read(adc_dev(), &sequence), -EINVAL);
	sequence.resolution = 12;
	sequence.oversampling = 1;
	zassert_equal(adc_read(adc_dev(), &sequence), -ENOTSUP);
	sequence.oversampling = 0;
	sequence.channels = BIT(0);
	zassert_equal(adc_read(adc_dev(), &sequence), -EINVAL);
	sequence.channels = BIT(12);
	zassert_ok(adc_read(adc_dev(), &sequence));
}

ZTEST_SUITE(adc_py32, NULL, adc_setup, NULL, NULL, NULL);
