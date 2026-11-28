/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <string.h>

#define CAPTURE_CHANNEL_COUNT 2U
#define PWM_A_CHANNEL         3U
#define PWM_B_CHANNEL         1U
#define PWM_A_PERIOD_NS       1000000U
#define PWM_B_PERIOD_NS       2500000U
#define CAPTURE_BUFFER_SIZE   256U
#define BATCH_SIZE            (CAPTURE_BUFFER_SIZE / 2U)
#define PRINT_BUFFER_COUNT    8U

struct batch_event {
	uint8_t channel;
	uint32_t *timestamps;
	size_t count;
	int status;
};

K_MSGQ_DEFINE(batch_events, sizeof(struct batch_event), 8, 4);
K_MEM_SLAB_DEFINE(print_buffers, BATCH_SIZE * sizeof(uint32_t), PRINT_BUFFER_COUNT, 4);

static uint32_t timestamp_buffers[CAPTURE_CHANNEL_COUNT]
	[CAPTURE_BUFFER_SIZE]
	__nocache __aligned(32);

static uint32_t timestamp_delta(uint32_t timestamp, uint32_t previous, uint32_t top)
{
	if (timestamp >= previous) {
		return timestamp - previous;
	}

	return (uint32_t)((uint64_t)timestamp + (uint64_t)top - previous + 1U);
}

static void capture_batch_callback(const struct device *dev, uint8_t channel,
				   const uint32_t *timestamps, size_t count, int status,
				   void *user_data)
{
	uint32_t *print_buffer;
	struct batch_event event = {
		.channel = channel,
		.count = count,
		.status = status,
	};

	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (status != 0) {
		(void)k_msgq_put(&batch_events, &event, K_NO_WAIT);
		return;
	}

	if (k_mem_slab_alloc(&print_buffers, (void **)&print_buffer, K_NO_WAIT) != 0) {
		return;
	}

	memcpy(print_buffer, timestamps, count * sizeof(*print_buffer));
	event.timestamps = print_buffer;

	if (k_msgq_put(&batch_events, &event, K_NO_WAIT) != 0) {
		k_mem_slab_free(&print_buffers, print_buffer);
	}
}

static int configure_capture(const struct device *counter)
{
	int ret;

	for (uint8_t channel = 0U; channel < CAPTURE_CHANNEL_COUNT; channel++) {
		const struct counter_capture_batch batch = {
			.chan_id = channel,
			.flags = COUNTER_CAPTURE_RISING_EDGE | COUNTER_CAPTURE_CONTINUOUS,
			.ticks = timestamp_buffers[channel],
			.count = CAPTURE_BUFFER_SIZE,
			.callback = capture_batch_callback,
		};

		ret = counter_capture_configure_batch(counter, &batch);
		if (ret != 0) {
			printk("failed to configure batch channel %u (%d)\n", channel + 1U, ret);
			return ret;
		}

		ret = counter_enable_capture(counter, channel);
		if (ret != 0) {
			printk("failed to enable capture channel %u (%d)\n", channel + 1U, ret);
			return ret;
		}
	}

	return counter_start(counter);
}

static void report_periods(const struct device *counter, uint8_t channel,
			   const uint32_t *timestamps, size_t count,
			   uint32_t counter_frequency)
{
	uint32_t top = counter_get_top_value(counter);
	uint64_t period_sum = 0U;
	uint32_t period_count = 0U;

	for (size_t index = 1U; index < count; index++) {
		uint32_t timestamp = timestamps[index];

		period_sum += timestamp_delta(timestamp, timestamps[index - 1U], top);
		period_count++;
	}

	if (period_count > 0U) {
		uint64_t average_period = period_sum / period_count;
		uint64_t frequency = (uint64_t)counter_frequency / average_period;

		printk("channel %u: average period %llu ticks, frequency %llu Hz\n",
		       channel + 1U, average_period,
		       frequency);
	}
}

int main(void)
{
	const struct device *counter = DEVICE_DT_GET(DT_ALIAS(capture_counter));
	const struct device *pwm_a = DEVICE_DT_GET(DT_ALIAS(pwm_generator_a));
	const struct device *pwm_b = DEVICE_DT_GET(DT_ALIAS(pwm_generator_b));
	uint32_t counter_frequency;
	struct batch_event event;
	int ret;

	printk("Multiple signal period capture\n");

	if (!device_is_ready(counter) || !device_is_ready(pwm_a) || !device_is_ready(pwm_b)) {
		printk("capture or generator device is not ready\n");
		return 0;
	}

	counter_frequency = counter_get_frequency(counter);
	ret = configure_capture(counter);
	if (ret != 0) {
		return 0;
	}

	ret = pwm_set(pwm_a, PWM_A_CHANNEL, PWM_A_PERIOD_NS, PWM_A_PERIOD_NS / 2U,
		      PWM_POLARITY_NORMAL);
	if (ret != 0) {
		printk("failed to start PWM generator A (%d)\n", ret);
		return 0;
	}

	ret = pwm_set(pwm_b, PWM_B_CHANNEL, PWM_B_PERIOD_NS, PWM_B_PERIOD_NS / 2U,
		      PWM_POLARITY_NORMAL);
	if (ret != 0) {
		printk("failed to start PWM generator B (%d)\n", ret);
		return 0;
	}

	printk("counter frequency: %u Hz\n", counter_frequency);
	while (true) {
		k_msgq_get(&batch_events, &event, K_FOREVER);
		if (event.status != 0) {
			printk("channel %u: DMA capture error %d\n", event.channel + 1U,
			       event.status);
			continue;
		}

		report_periods(counter, event.channel, event.timestamps, event.count,
			       counter_frequency);
		k_mem_slab_free(&print_buffers, event.timestamps);
	}

	return 0;
}
