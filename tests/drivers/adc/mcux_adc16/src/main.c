/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/fff.h>
#include <zephyr/irq_offload.h>
#include <zephyr/ztest.h>

#include <fsl_adc16.h>

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(ADC16_Init, ADC_Type *, const adc16_config_t *);
FAKE_VOID_FUNC(ADC16_GetDefaultConfig, adc16_config_t *);
FAKE_VOID_FUNC(ADC16_SetHardwareAverage, ADC_Type *, adc16_hardware_average_mode_t);
FAKE_VOID_FUNC(ADC16_SetChannelMuxMode, ADC_Type *, adc16_channel_mux_mode_t);
FAKE_VOID_FUNC(ADC16_EnableHardwareTrigger, ADC_Type *, bool);
FAKE_VOID_FUNC(ADC16_SetChannelConfig, ADC_Type *, uint32_t, const adc16_channel_config_t *);
FAKE_VALUE_FUNC(uint32_t, ADC16_GetChannelConversionValue, ADC_Type *, uint32_t);

/* Exercise the real static ISR and ADC context with only the peripheral mocked. */
#include "adc_mcux_adc16.c"

FAKE_VALUE_FUNC(int, pinctrl_configure_pins, const pinctrl_soc_pin_t *, uint8_t, uintptr_t);

BUILD_ASSERT(!IS_ENABLED(CONFIG_ADC_MCUX_ADC16_ENABLE_EDMA));
BUILD_ASSERT(!IS_ENABLED(CONFIG_ADC_MCUX_ADC16_HW_TRIGGER));

static ADC_Type registers;
static struct mcux_adc16_data data;
static size_t callback_count;
static uint16_t callback_indices[3];
static bool repeat_first_sampling;

static void irq_config(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static const struct pinctrl_state pin_state = {
	.id = PINCTRL_STATE_DEFAULT,
};
static const struct pinctrl_dev_config pin_config = {
	.states = &pin_state,
	.state_cnt = 1U,
};
static const struct mcux_adc16_config config = {
	.base = &registers,
	.irq_config_func = irq_config,
	.pincfg = &pin_config,
};
static const struct device adc = {
	.name = "adc16_test",
	.config = &config,
	.data = &data,
	.api = &mcux_adc16_driver_api,
};

static void set_channel(ADC_Type *base, uint32_t group, const adc16_channel_config_t *channel)
{
	zassert_equal(group, 0U);
	base->SC1[group] =
		channel->channelNumber |
		(channel->enableInterruptOnConversionCompleted ? ADC_SC1_AIEN_MASK : 0U) |
		(channel->enableDifferentialConversion ? ADC_SC1_DIFF_MASK : 0U);
}

static uint32_t read_result(ADC_Type *base, uint32_t group)
{
	zassert_equal(group, 0U);
	base->SC1[group] &= ~ADC_SC1_COCO_MASK;
	return base->R[group];
}

static void invoke_isr(const void *arg)
{
	mcux_adc16_isr(arg);
}

static void inject_completion(uint16_t result)
{
	registers.R[0] = result;
	registers.SC1[0] |= ADC_SC1_COCO_MASK;
	irq_offload(invoke_isr, &adc);
}

static enum adc_action on_sampling_done(const struct device *dev,
					const struct adc_sequence *sequence,
					uint16_t sampling_index)
{
	ARG_UNUSED(sequence);
	zassert_equal_ptr(dev, &adc);
	zassert_equal(registers.SC1[0], ADC_SC1_ADCH_MASK,
		      "Channel must be disabled before the context callback");
	zassert_true(callback_count < ARRAY_SIZE(callback_indices));
	callback_indices[callback_count++] = sampling_index;

	return (repeat_first_sampling && callback_count == 1U) ? ADC_ACTION_REPEAT
							       : ADC_ACTION_CONTINUE;
}

static void start_sampling(uint16_t *buffer, size_t count, uint32_t channels,
			   uint16_t extra_samplings)
{
	const struct adc_sequence_options options = {
		.callback = on_sampling_done,
		.extra_samplings = extra_samplings,
	};
	const struct adc_sequence sequence = {
		.options = &options,
		.channels = channels,
		.buffer = buffer,
		.buffer_size = count * sizeof(*buffer),
		.resolution = 12U,
	};

	data.buffer = buffer;
	adc_context_start_read(&data.ctx, &sequence);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	memset(&registers, 0, sizeof(registers));
	memset(&data, 0, sizeof(data));
	memset(callback_indices, 0, sizeof(callback_indices));
	callback_count = 0U;
	repeat_first_sampling = false;
	RESET_FAKE(ADC16_Init);
	RESET_FAKE(ADC16_GetDefaultConfig);
	RESET_FAKE(ADC16_SetHardwareAverage);
	RESET_FAKE(ADC16_SetChannelMuxMode);
	RESET_FAKE(ADC16_EnableHardwareTrigger);
	RESET_FAKE(ADC16_SetChannelConfig);
	RESET_FAKE(ADC16_GetChannelConversionValue);
	RESET_FAKE(pinctrl_configure_pins);
	FFF_RESET_HISTORY();
	ADC16_SetChannelConfig_fake.custom_fake = set_channel;
	ADC16_GetChannelConversionValue_fake.custom_fake = read_result;
	adc_context_init(&data.ctx);
	zassert_ok(mcux_adc16_init(&adc));
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	k_timer_stop(&data.ctx.timer);
}

/** @brief An injected idle interrupt must not consume a result or complete a read. */
ZTEST(mcux_adc16, test_idle_interrupt)
{
	uint16_t canary = 0xA55AU;

	data.buffer = &canary;
	data.channel_id = 3U;
	registers.SC1[0] = 3U | ADC_SC1_AIEN_MASK;
	inject_completion(0x1234U);

	zassert_equal(canary, 0xA55AU);
	zassert_equal_ptr(data.buffer, &canary);
	zassert_equal(data.channels, 0U);
	zassert_equal(ADC16_GetChannelConversionValue_fake.call_count, 0U);
	zassert_equal(k_sem_count_get(&data.ctx.sync), 0U);
	zassert_equal(registers.SC1[0], ADC_SC1_ADCH_MASK);
}

/** @brief Complete one sampling with the channel disabled before the callback. */
ZTEST(mcux_adc16, test_single_channel)
{
	uint16_t buffer[1];

	start_sampling(buffer, ARRAY_SIZE(buffer), BIT(3), 0U);
	inject_completion(0x1234U);

	zassert_equal(buffer[0], 0x1234U);
	zassert_equal_ptr(data.buffer, buffer + ARRAY_SIZE(buffer));
	zassert_equal(data.channels, 0U);
	zassert_equal(callback_count, 1U);
	zassert_equal(ADC16_GetChannelConversionValue_fake.call_count, 1U);
	zassert_equal(k_sem_count_get(&data.ctx.sync), 1U);
}

/** @brief An idle interrupt after a completed read must not leave a stale sync token. */
ZTEST(mcux_adc16, test_sync_completion_once)
{
	uint16_t buffer[2] = {0U, 0xA55AU};

	start_sampling(buffer, 1U, BIT(3), 0U);
	inject_completion(0x1234U);
	zassert_ok(k_sem_take(&data.ctx.sync, K_NO_WAIT));
	inject_completion(0x5678U);

	zassert_equal(buffer[0], 0x1234U);
	zassert_equal(buffer[1], 0xA55AU);
	zassert_equal_ptr(data.buffer, buffer + 1);
	zassert_equal(callback_count, 1U);
	zassert_equal(ADC16_GetChannelConversionValue_fake.call_count, 1U);
	zassert_equal(k_sem_count_get(&data.ctx.sync), 0U);
}

/** @brief Completing a nonfinal channel must arm the next channel without completing. */
ZTEST(mcux_adc16, test_multiple_channels)
{
	uint16_t buffer[2];

	start_sampling(buffer, ARRAY_SIZE(buffer), BIT(2) | BIT(5), 0U);
	inject_completion(0x1111U);
	zassert_equal(data.channels, BIT(5));
	zassert_equal(registers.SC1[0], 5U | ADC_SC1_AIEN_MASK);
	zassert_equal(callback_count, 0U);
	zassert_equal(k_sem_count_get(&data.ctx.sync), 0U);

	inject_completion(0x2222U);
	zassert_equal(buffer[0], 0x1111U);
	zassert_equal(buffer[1], 0x2222U);
	zassert_equal(data.channels, 0U);
	zassert_equal(callback_count, 1U);
	zassert_equal(k_sem_count_get(&data.ctx.sync), 1U);
}

/** @brief Synchronous repeats and extra samplings remain armed after the ISR returns. */
ZTEST(mcux_adc16, test_repeat_and_extra_sampling)
{
	uint16_t buffer[2];

	repeat_first_sampling = true;
	start_sampling(buffer, ARRAY_SIZE(buffer), BIT(4), 1U);
	inject_completion(0x1111U);
	zassert_equal_ptr(data.buffer, buffer);
	zassert_equal(data.channels, BIT(4));
	zassert_equal(registers.SC1[0], 4U | ADC_SC1_AIEN_MASK);
	zassert_equal(k_sem_count_get(&data.ctx.sync), 0U);

	inject_completion(0x2222U);
	zassert_equal_ptr(data.buffer, buffer + 1);
	zassert_equal(data.channels, BIT(4));
	zassert_equal(registers.SC1[0], 4U | ADC_SC1_AIEN_MASK);
	zassert_equal(k_sem_count_get(&data.ctx.sync), 0U);

	inject_completion(0x3333U);
	zassert_equal(buffer[0], 0x2222U);
	zassert_equal(buffer[1], 0x3333U);
	zassert_equal(data.channels, 0U);
	zassert_equal(registers.SC1[0], ADC_SC1_ADCH_MASK);
	zassert_equal(callback_count, 3U);
	zassert_equal(callback_indices[0], 0U);
	zassert_equal(callback_indices[1], 0U);
	zassert_equal(callback_indices[2], 1U);
	zassert_equal(k_sem_count_get(&data.ctx.sync), 1U);
}

/** @brief A late injected interrupt cannot signal or release an asynchronous read twice. */
ZTEST(mcux_adc16, test_async_completion_once)
{
	uint16_t buffer[2] = {0U, 0xA55AU};
	struct k_poll_signal signal;
	unsigned int signaled;
	int result;
	const struct adc_sequence sequence = {
		.channels = BIT(3),
		.buffer = buffer,
		.buffer_size = sizeof(buffer[0]),
		.resolution = 12U,
	};

	k_poll_signal_init(&signal);
	zassert_ok(mcux_adc16_read_async(&adc, &sequence, &signal));
	inject_completion(0x1234U);
	k_poll_signal_check(&signal, &signaled, &result);
	zassert_equal(signaled, 1U);
	zassert_equal(result, 0);
	zassert_equal(buffer[0], 0x1234U);
	zassert_equal(k_sem_count_get(&data.ctx.sync), 0U);

	k_poll_signal_reset(&signal);
	zassert_ok(k_sem_take(&data.ctx.lock, K_NO_WAIT));
	inject_completion(0x5678U);
	k_poll_signal_check(&signal, &signaled, &result);
	zassert_equal(signaled, 0U);
	zassert_equal(k_sem_count_get(&data.ctx.lock), 0U);
	zassert_equal(ADC16_GetChannelConversionValue_fake.call_count, 1U);
	zassert_equal_ptr(data.buffer, buffer + 1);
	zassert_equal(buffer[1], 0xA55AU);
}

ZTEST_SUITE(mcux_adc16, NULL, NULL, before, after, NULL);
