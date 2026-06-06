/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <soc.h>

#include <zephyr/console/console.h>
#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/state.h>
#include <zephyr/sys/util.h>

#define SAMPLE_LOOPS        10U
#define SAMPLE_SETTLE_MS    20U
#define SAMPLE_GAP_MS       250U
#define MARKER_TEST_LOOPS   5U
#define MARKER_TEST_PULSE_MS 100U
#define WORK_MARKER_PIN     3U

#define SMALL_WORK_ITERATIONS  20000U
#define MEDIUM_WORK_ITERATIONS 100000U
#define LARGE_WORK_ITERATIONS  300000U

#define WORK_MARKER_NODE DT_NODELABEL(rti_work_marker)

struct period_case {
	const char *name;
	uint64_t period_us;
};

struct pstate_case {
	const char *name;
	const struct pstate *state;
	uint32_t frequency_hz;
};

struct workload_case {
	const char *name;
	uint32_t iterations;
};

struct target_choice {
	const char *name;
	const struct pstate_case *source;
	const struct pstate_case *target;
};

struct scenario_result {
	uint64_t total_active_us;
	uint64_t min_active_us;
	uint64_t max_active_us;
	uint32_t deadline_misses;
	uint32_t pm_entry_counts[PM_STATE_COUNT];
};

static const struct gpio_dt_spec work_marker = GPIO_DT_SPEC_GET(WORK_MARKER_NODE, gpios);

static volatile uint32_t workload_sink;
static bool work_marker_ready;
static bool menu_idle_locked;
static uint32_t pm_entry_counts[PM_STATE_COUNT];

static const struct period_case period_cases[] = {
	{ .name = "3ms", .period_us = 3000ULL },
	{ .name = "5ms", .period_us = 5000ULL },
	{ .name = "10ms", .period_us = 10000ULL },
	{ .name = "20ms", .period_us = 20000ULL },
	{ .name = "30ms", .period_us = 30000ULL },
	{ .name = "50ms", .period_us = 50000ULL },
	{ .name = "100ms", .period_us = 100000ULL },
};

static const char period_keys[] = "ABCDEFG";

static const struct pstate_case pstate_cases[] = {
	{
		.name = "high",
		.state = PSTATE_DT_GET(DT_NODELABEL(pstate_0)),
		.frequency_hz = 150000000U,
	},
	{
		.name = "medium-3",
		.state = PSTATE_DT_GET(DT_NODELABEL(pstate_3)),
		.frequency_hz = 144000000U,
	},
	{
		.name = "medium-2",
		.state = PSTATE_DT_GET(DT_NODELABEL(pstate_4)),
		.frequency_hz = 100000000U,
	},
	{
		.name = "medium-1",
		.state = PSTATE_DT_GET(DT_NODELABEL(pstate_1)),
		.frequency_hz = 48000000U,
	},
	{
		.name = "low",
		.state = PSTATE_DT_GET(DT_NODELABEL(pstate_2)),
		.frequency_hz = 12000000U,
	},
};

static const struct workload_case workload_cases[] = {
	{ .name = "small", .iterations = SMALL_WORK_ITERATIONS },
	{ .name = "medium", .iterations = MEDIUM_WORK_ITERATIONS },
	{ .name = "large", .iterations = LARGE_WORK_ITERATIONS },
};

static const char workload_keys[] = "123";
static const char target_keys[] = "abcdefghijklmnopqrstuvwxy";

static const struct target_choice target_choices[] = {
	{ .name = "high->medium-3", .source = &pstate_cases[0], .target = &pstate_cases[1] },
	{ .name = "high->medium-2", .source = &pstate_cases[0], .target = &pstate_cases[2] },
	{ .name = "high->medium-1", .source = &pstate_cases[0], .target = &pstate_cases[3] },
	{ .name = "high->low", .source = &pstate_cases[0], .target = &pstate_cases[4] },
	{ .name = "medium-3->high", .source = &pstate_cases[1], .target = &pstate_cases[0] },
	{ .name = "medium-3->medium-2", .source = &pstate_cases[1], .target = &pstate_cases[2] },
	{ .name = "medium-3->medium-1", .source = &pstate_cases[1], .target = &pstate_cases[3] },
	{ .name = "medium-3->low", .source = &pstate_cases[1], .target = &pstate_cases[4] },
	{ .name = "medium-2->high", .source = &pstate_cases[2], .target = &pstate_cases[0] },
	{ .name = "medium-2->medium-3", .source = &pstate_cases[2], .target = &pstate_cases[1] },
	{ .name = "medium-2->medium-1", .source = &pstate_cases[2], .target = &pstate_cases[3] },
	{ .name = "medium-2->low", .source = &pstate_cases[2], .target = &pstate_cases[4] },
	{ .name = "medium-1->high", .source = &pstate_cases[3], .target = &pstate_cases[0] },
	{ .name = "medium-1->medium-3", .source = &pstate_cases[3], .target = &pstate_cases[1] },
	{ .name = "medium-1->medium-2", .source = &pstate_cases[3], .target = &pstate_cases[2] },
	{ .name = "medium-1->low", .source = &pstate_cases[3], .target = &pstate_cases[4] },
	{ .name = "low->high", .source = &pstate_cases[4], .target = &pstate_cases[0] },
	{ .name = "low->medium-3", .source = &pstate_cases[4], .target = &pstate_cases[1] },
	{ .name = "low->medium-2", .source = &pstate_cases[4], .target = &pstate_cases[2] },
	{ .name = "low->medium-1", .source = &pstate_cases[4], .target = &pstate_cases[3] },
	{ .name = "high->high", .source = &pstate_cases[0], .target = &pstate_cases[0] },
	{ .name = "medium-3->medium-3", .source = &pstate_cases[1], .target = &pstate_cases[1] },
	{ .name = "medium-2->medium-2", .source = &pstate_cases[2], .target = &pstate_cases[2] },
	{ .name = "medium-1->medium-1", .source = &pstate_cases[3], .target = &pstate_cases[3] },
	{ .name = "low->low", .source = &pstate_cases[4], .target = &pstate_cases[4] },
};

BUILD_ASSERT(ARRAY_SIZE(period_keys) == ARRAY_SIZE(period_cases) + 1U);
BUILD_ASSERT(ARRAY_SIZE(workload_keys) == ARRAY_SIZE(workload_cases) + 1U);
BUILD_ASSERT(ARRAY_SIZE(target_keys) == ARRAY_SIZE(target_choices) + 1U);

static void marker_set(const struct gpio_dt_spec *marker, int value)
{
	if (work_marker_ready) {
		(void)gpio_pin_set_dt(marker, value);
	}
}

static void configure_work_marker_signal(void)
{
	uint32_t pcr = PORT5->PCR[WORK_MARKER_PIN];

	pcr &= ~(PORT_PCR_MUX_MASK | PORT_PCR_SRE_MASK | PORT_PCR_DSE_MASK |
		 PORT_PCR_PFE_MASK | PORT_PCR_ODE_MASK | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK);
	pcr |= PORT_PCR_MUX(PORT_MUX_GPIO) | PORT_PCR_SRE(0U) | PORT_PCR_DSE(1U);
	PORT5->PCR[WORK_MARKER_PIN] = pcr;
}

static void lock_menu_idle_states(void)
{
	if (!menu_idle_locked) {
		pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
		menu_idle_locked = true;
	}
}

static void unlock_measurement_idle_states(void)
{
	if (menu_idle_locked) {
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
		menu_idle_locked = false;
	}
}

static void notify_pm_state_entry(enum pm_state state)
{
	if (state < PM_STATE_COUNT) {
		pm_entry_counts[state]++;
	}
}

static struct pm_notifier pm_tracking_notifier = {
	.state_entry = notify_pm_state_entry,
};

static void reset_pm_tracking(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(pm_entry_counts); i++) {
		pm_entry_counts[i] = 0U;
	}
}

static void copy_pm_tracking(struct scenario_result *result)
{
	for (size_t i = 0U; i < ARRAY_SIZE(pm_entry_counts); i++) {
		result->pm_entry_counts[i] = pm_entry_counts[i];
	}
}

static const char *pm_state_name(enum pm_state state)
{
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		return "runtime-idle";
	case PM_STATE_SUSPEND_TO_IDLE:
		return "suspend-to-idle";
	case PM_STATE_STANDBY:
		return "standby";
	default:
		return NULL;
	}
}

static void format_pm_states(const struct scenario_result *result, char *buf, size_t buf_size)
{
	size_t offset = 0U;

	for (enum pm_state state = 0; state < PM_STATE_COUNT; state++) {
		const char *name = pm_state_name(state);
		uint32_t count = result->pm_entry_counts[state];

		if (name == NULL || count == 0U) {
			continue;
		}

		offset += snprintk(&buf[offset], buf_size - offset, "%s%s:%u",
				    offset == 0U ? "" : ",", name, count);
		if (offset >= buf_size) {
			break;
		}
	}

	if (offset == 0U) {
		(void)snprintk(buf, buf_size, "active:0");
	}
}

static int set_pstate(const struct pstate_case *pstate_case)
{
	int ret = cpu_freq_pstate_set(pstate_case->state);

	if (ret != 0) {
		printk("failed to set %s P-state: %d\n", pstate_case->name, ret);
	}

	return ret;
}

static void run_workload(uint32_t iterations)
{
	uint32_t value = workload_sink;

	for (uint32_t i = 0U; i < iterations; i++) {
		value = (value * 1664525U) + 1013904223U + i;
		value ^= value >> 13;
	}

	workload_sink = value;
}

static bool select_period(uint8_t key, const struct period_case **period_case)
{
	for (size_t i = 0U; i < ARRAY_SIZE(period_keys) - 1U; i++) {
		if (key == period_keys[i]) {
			*period_case = &period_cases[i];
			return true;
		}
	}

	return false;
}

static bool select_workload(uint8_t key, const struct workload_case **workload_case)
{
	for (size_t i = 0U; i < ARRAY_SIZE(workload_keys) - 1U; i++) {
		if (key == workload_keys[i]) {
			*workload_case = &workload_cases[i];
			return true;
		}
	}

	return false;
}

static bool select_target(uint8_t key, const struct target_choice **target_choice)
{
	for (size_t i = 0U; i < ARRAY_SIZE(target_keys) - 1U; i++) {
		if (key == target_keys[i]) {
			*target_choice = &target_choices[i];
			return true;
		}
	}

	return false;
}

static void print_period_menu(void)
{
	printk("\nSelect period/deadline:\n");
	for (size_t period_idx = 0U; period_idx < ARRAY_SIZE(period_cases); period_idx++) {
		printk("  %c: period=%s (%llu us)\n", period_keys[period_idx],
		       period_cases[period_idx].name,
		       (unsigned long long)period_cases[period_idx].period_us);
	}
	printk("> ");
}

static void print_workload_menu(const struct period_case *period_case)
{
	printk("\nSelect workload for period=%s:\n", period_case->name);
	for (size_t workload_idx = 0U; workload_idx < ARRAY_SIZE(workload_cases); workload_idx++) {
		printk("  %c: workload=%s\n", workload_keys[workload_idx],
		       workload_cases[workload_idx].name);
	}
	printk("> ");
}

static void print_target_menu(const struct period_case *period_case,
			      const struct workload_case *workload_case)
{
	printk("\nSelect target choice for period=%s workload=%s:\n",
	       period_case->name, workload_case->name);
	for (size_t target_idx = 0U; target_idx < ARRAY_SIZE(target_choices); target_idx++) {
		printk("  %c: target-choice=%s\n", target_keys[target_idx],
		       target_choices[target_idx].name);
	}
	printk("> ");
}

static uint8_t read_menu_key(void)
{
	uint8_t key;

	do {
		key = console_getchar();
	} while (key == '\r' || key == '\n');

	console_putchar(key);
	console_putchar('\n');
	printk("selected option: %c\n", key);

	return key;
}

static void test_work_marker(void)
{
	printk("GPIO5_3 marker test start: %u pulses, %u ms high and %u ms low\n",
	       MARKER_TEST_LOOPS, MARKER_TEST_PULSE_MS, MARKER_TEST_PULSE_MS);

	for (uint32_t i = 0U; i < MARKER_TEST_LOOPS; i++) {
		marker_set(&work_marker, 1);
		k_msleep(MARKER_TEST_PULSE_MS);
		marker_set(&work_marker, 0);
		k_msleep(MARKER_TEST_PULSE_MS);
	}

	printk("GPIO5_3 marker test complete\n");
}

static void run_deadline_scenario(const struct period_case *period_case,
				  const struct workload_case *workload_case,
				  const struct target_choice *target_choice)
{
	struct scenario_result result;
	char pm_states[96];

	lock_menu_idle_states();
	marker_set(&work_marker, 0);
	printk("scenario selected: pm=deadline period=%s workload=%s source=%s target=%s\n",
	       period_case->name, workload_case->name, target_choice->source->name,
	       target_choice->target->name);
	printk("scenario start: pm=deadline period=%s period-us=%llu workload=%s "
	       "iterations=%u source=%s target=%s samples=%u\n",
	       period_case->name, (unsigned long long)period_case->period_us,
	       workload_case->name, workload_case->iterations, target_choice->source->name,
	       target_choice->target->name, SAMPLE_LOOPS);

	reset_pm_tracking();

	for (uint32_t sample = 0U; sample < SAMPLE_LOOPS; sample++) {
		k_ticks_t period_start_ticks;
		k_ticks_t period_end_ticks;
		int ret;

		marker_set(&work_marker, 0);
		ret = set_pstate(target_choice->source);
		if (ret != 0) {
			printk("deadline sample: period=%s workload=%s source=%s target=%s sample=%u "
			       "setup-ret=%d\n",
			       period_case->name, workload_case->name, target_choice->source->name,
			       target_choice->target->name, sample, ret);
			continue;
		}
		k_msleep(SAMPLE_SETTLE_MS);

		unlock_measurement_idle_states();
		period_start_ticks = k_uptime_ticks();
		period_end_ticks = period_start_ticks + k_us_to_ticks_ceil64(period_case->period_us);
		marker_set(&work_marker, 1);
		ret = cpu_freq_pstate_set(target_choice->target->state);
		run_workload(workload_case->iterations);
		marker_set(&work_marker, 0);
		k_sleep(K_TIMEOUT_ABS_TICKS(period_end_ticks));
		marker_set(&work_marker, 0);
		lock_menu_idle_states();

		printk("deadline sample: period=%s workload=%s source=%s target=%s sample=%u "
		       "ret=%d period-us=%llu\n",
		       period_case->name, workload_case->name, target_choice->source->name,
		       target_choice->target->name, sample, ret,
		       (unsigned long long)period_case->period_us);
	}

	copy_pm_tracking(&result);
	format_pm_states(&result, pm_states, sizeof(pm_states));

	printk("scenario result: pm=deadline pstate=%s workload=%s "
	       "period-us=%llu samples=%u pm-states=%s\n",
	       target_choice->name, workload_case->name,
	       (unsigned long long)period_case->period_us, SAMPLE_LOOPS, pm_states);

	k_msleep(SAMPLE_GAP_MS);
}

static int init_work_marker(void)
{
	if (!gpio_is_ready_dt(&work_marker)) {
		printk("GPIO5_3 PM marker is not ready; continuing without marker\n");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&work_marker, GPIO_OUTPUT_INACTIVE) != 0) {
		printk("Failed to configure GPIO5_3 PM marker\n");
		return -EIO;
	}
	configure_work_marker_signal();

	work_marker_ready = true;
	return 0;
}

int main(void)
{
	console_init();
	pm_notifier_register(&pm_tracking_notifier);

	printk("CPUFreq deadline measurement sample\n"
	       "Use external power measurement equipment to measure board current.\n"
	       "GPIO5_3 is high while transition plus workload is active and low during the remaining period.\n"
	       "GPIO5_3 is configured as fast slew, high drive for measurement.\n");

	(void)init_work_marker();

	while (true) {
		uint8_t key;
		const struct period_case *period_case;
		const struct workload_case *workload_case;
		const struct target_choice *target_choice;

		lock_menu_idle_states();
		print_period_menu();
		key = read_menu_key();
		if (key == 'z') {
			test_work_marker();
			continue;
		}
		if (!select_period(key, &period_case)) {
			printk("unknown period: %c\n", key);
			continue;
		}

		print_workload_menu(period_case);
		key = read_menu_key();
		if (key == 'z') {
			test_work_marker();
			continue;
		}
		if (!select_workload(key, &workload_case)) {
			printk("unknown workload: %c\n", key);
			continue;
		}

		print_target_menu(period_case, workload_case);
		key = read_menu_key();
		if (key == 'z') {
			test_work_marker();
			continue;
		}
		if (!select_target(key, &target_choice)) {
			printk("unknown target choice: %c\n", key);
			continue;
		}

		run_deadline_scenario(period_case, workload_case, target_choice);
	}

	return 0;
}
