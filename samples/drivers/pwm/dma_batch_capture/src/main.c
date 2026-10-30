/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

#define PWM_CAPTURE_CHANNEL   1
#define PWM_GENERATOR_CHANNEL 1
#define BATCH_SIZE            512

/* Generator: timer triangular frequency wave. */
#define GEN_PERIOD_STEP_NS    2000U
#define GEN_PERIOD_MIN_NS     20000U
#define GEN_PERIOD_MAX_NS     200000U

/* Application-owned capture buffers */
static uint32_t period_buffer[BATCH_SIZE] __aligned(32);
static uint32_t pulse_buffer[BATCH_SIZE] __aligned(32);

struct batch_summary {
	uint32_t count;
	uint64_t sum_period;
	uint64_t sum_pulse;
	int status;
};

K_MSGQ_DEFINE(batch_msgq, sizeof(struct batch_summary), 8, 4);

static void capture_batch_handler(const struct device *dev, uint32_t channel,
				  const uint32_t *period_cycles, const uint32_t *pulse_cycles,
				  uint32_t count, int status, void *user_data)
{
	struct batch_summary summary = {
		.count = count,
		.sum_period = 0,
		.sum_pulse = 0,
		.status = status,
	};

	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(user_data);

	if (status == 0 && (period_cycles != NULL) && (pulse_cycles != NULL)) {
		for (uint32_t i = 0; i < count; i++) {
			summary.sum_period += period_cycles[i];
			summary.sum_pulse += pulse_cycles[i];
		}
	}

	(void)k_msgq_put(&batch_msgq, &summary, K_NO_WAIT);
}

int main(void)
{
	int ret;
	uint64_t cyc_per_sec;
	uint32_t gen_period_ns = GEN_PERIOD_MIN_NS;
	double duty_cycle = 0.5;
	bool gen_period_rising = true;
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(capture));
	const struct device *const gen = DEVICE_DT_GET(DT_ALIAS(pwm_gen));

	printk("PWM DMA batch capture %s\n", CONFIG_BOARD_TARGET);
	if (!device_is_ready(dev) || !device_is_ready(gen)) {
		printk("device is not ready\n");
		return 0;
	}

	ret = pwm_get_cycles_per_sec(dev, PWM_CAPTURE_CHANNEL, &cyc_per_sec);
	if (ret != 0) {
		printk("failed to get cycles per second %d\n", ret);
		return 0;
	}

	ret = pwm_set(gen, PWM_GENERATOR_CHANNEL, gen_period_ns, gen_period_ns * duty_cycle, 0);
	if (ret != 0) {
		printk("generator setup failed %d\n", ret);
		return 0;
	}

	/* Configure batch capture with application-owned buffers */
	const struct pwm_capture_batch batch_cfg = {
		.channel = PWM_CAPTURE_CHANNEL,
		.flags = PWM_CAPTURE_TYPE_BOTH | PWM_CAPTURE_MODE_CONTINUOUS,
		.period_buf = period_buffer,
		.pulse_buf = pulse_buffer,
		.buf_len_samples = BATCH_SIZE,
		.callback = capture_batch_handler,
		.user_data = NULL,
	};

	ret = pwm_configure_capture_batch(dev, &batch_cfg);
	if (ret != 0) {
		printk("configure batch capture failed %d\n", ret);
		return 0;
	}

	ret = pwm_enable_capture(dev, PWM_CAPTURE_CHANNEL);
	if (ret != 0) {
		printk("enable capture failed %d\n", ret);
		return 0;
	}

	while (1) {
		struct batch_summary summary;

		k_msgq_get(&batch_msgq, &summary, K_FOREVER);

		if (summary.status != 0) {
			printk("capture error %d\n", summary.status);
			continue;
		}

		if (summary.count == 0 || summary.sum_period == 0) {
			continue;
		}

		uint64_t avg_period = summary.sum_period / summary.count;
		uint64_t avg_pulse = summary.sum_pulse / summary.count;
		uint64_t avg_duty_cycle = (avg_pulse * 100) / avg_period;

		printk("batch of %u samples: avg period: %llu cycles, avg frequency: %llu Hz, avg "
		       "duty cycle: %llu %%\n",
		       summary.count, avg_period, cyc_per_sec / avg_period, avg_duty_cycle);

		if (gen_period_rising) {
			if (gen_period_ns >= (GEN_PERIOD_MAX_NS - GEN_PERIOD_STEP_NS)) {
				gen_period_ns = GEN_PERIOD_MAX_NS;
				gen_period_rising = false;
			} else {
				gen_period_ns += GEN_PERIOD_STEP_NS;
			}
		} else {
			if (gen_period_ns <= (GEN_PERIOD_MIN_NS + GEN_PERIOD_STEP_NS)) {
				gen_period_ns = GEN_PERIOD_MIN_NS;
				gen_period_rising = true;
				/* increase the duty cycle every triangular wave */
				duty_cycle = (duty_cycle <= 0.8) ? (duty_cycle + 0.1) : 0.1;
			} else {
				gen_period_ns -= GEN_PERIOD_STEP_NS;
			}
		}

		ret = pwm_set(gen, PWM_GENERATOR_CHANNEL, gen_period_ns,
			      gen_period_ns * duty_cycle, 0);
		if (ret != 0) {
			printk("generator update failed %d\n", ret);
			return 0;
		}
	}

	return 0;
}
