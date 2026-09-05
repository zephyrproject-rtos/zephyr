/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/safety_test/safety_test.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "sample_common.h"

/* Every category, so the sweep visits every registered test. */
#define ALL_CATEGORIES (BIT(SAFETY_TEST_CAT_COUNT) - 1U)

static const struct gpio_dt_spec ok_led = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec beat_led = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

static const char *result_str(enum safety_test_result result)
{
	switch (result) {
	case SAFETY_TEST_RESULT_PASS:
		return "PASS";
	case SAFETY_TEST_RESULT_FAIL:
		return "FAIL";
	case SAFETY_TEST_RESULT_SKIP:
		return "SKIP";
	default:
		return "NOT-RUN";
	}
}

static const char *level_str(enum safety_test_init_level level)
{
	switch (level) {
	case SAFETY_TEST_LEVEL_EARLY:
		return "EARLY";
	case SAFETY_TEST_LEVEL_PRE_KERNEL_1:
		return "PRE_KERNEL_1";
	case SAFETY_TEST_LEVEL_PRE_KERNEL_2:
		return "PRE_KERNEL_2";
	case SAFETY_TEST_LEVEL_POST_KERNEL:
		return "POST_KERNEL";
	default:
		return "APPLICATION";
	}
}

static bool print_one(const struct safety_test *test, void *user_data)
{
	ARG_UNUSED(user_data);

	printk("  %-18s %-13s %-8s %7u us%s\n", test->name, level_str(test->init_level),
	       result_str(test->result->result), test->result->duration_us,
	       test->result->over_budget ? "  OVER-BUDGET" : "");

	return true;
}

static void report_boot(void)
{
	struct safety_test_summary summary;
	bool passed = false;
	int ret;

	/*
	 * ram_march leaves the block zeroed, so the count is already 0 after a
	 * normal boot. Reset anyway: the block is __noinit and holds garbage if
	 * the march was configured out.
	 */
	sample_history_reset();

	printk("\nsafety_test sensor chain sample\n");
	printk("boot results:\n");
	safety_test_foreach(print_one, NULL);

	ret = safety_test_get_summary(&summary);
	if (ret == 0) {
		printk("boot summary: %u total, %u passed, %u failed, %u skipped, %u not run\n",
		       summary.global.total, summary.global.passed, summary.global.failed,
		       summary.global.skipped, summary.global.not_run);
	}

	ret = safety_test_boot_passed(&passed);
	if (ret == -ENOENT) {
		printk("boot chain: NONE (no critical boot test registered)\n");
	} else if (ret == 0) {
		printk("boot chain: %s\n", passed ? "PASS" : "FAIL");
	} else {
		printk("boot chain: error %d\n", ret);
	}

	if (gpio_is_ready_dt(&ok_led)) {
		(void)gpio_pin_configure_dt(&ok_led,
					    passed ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE);
	}

	if (gpio_is_ready_dt(&beat_led)) {
		(void)gpio_pin_configure_dt(&beat_led, GPIO_OUTPUT_INACTIVE);
	}
}

int main(void)
{
	uint32_t cycle = 0U;

	report_boot();

	while (1) {
		struct safety_test_stats stats;

		cycle++;

		if (gpio_is_ready_dt(&beat_led)) {
			(void)gpio_pin_toggle_dt(&beat_led);
		}

		(void)safety_test_run_category(ALL_CATEGORIES, &stats);

		sample_history_append(sample_sensor_last_mc());

		printk("cycle %u: %u visited, %u passed, %u failed, %u skipped, "
		       "%u over budget, temp %d mC, history %u%s\n",
		       cycle, stats.total, stats.passed, stats.failed, stats.skipped,
		       stats.over_budget, sample_sensor_last_mc(), sample_history_count(),
		       sample_fault_latched() ? ", FAULT LATCHED" : "");

		k_msleep(CONFIG_SAMPLE_RUNTIME_PERIOD_MS);
	}

	return 0;
}
