/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/cpu_stats.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cpu_stats);

BUILD_ASSERT(!IS_ENABLED(CONFIG_CPU_STATS_USE_COUNTER) || DT_HAS_CHOSEN(zephyr_cpu_stats_counter));

const struct device *counter = COND_CODE_1(CONFIG_CPU_STATS_USE_COUNTER,
				(DEVICE_DT_GET(DT_CHOSEN(zephyr_cpu_stats_counter))), (NULL));
static uint32_t enter_ts;
static uint32_t cyc_start;
static uint32_t ticks_idle;

static struct k_work_delayable cpu_stats_log;

void cpu_stats_log_control(bool enable)
{
	if (CONFIG_CPU_STATS_LOG_PERIODICALLY == 0) {
		return;
	}

	if (enable) {
		(void)cpu_stats_load_get(true);
		k_work_schedule(&cpu_stats_log, K_MSEC(CONFIG_CPU_STATS_LOG_PERIODICALLY));
	} else {
		k_work_cancel_delayable(&cpu_stats_log);
	}
}

#if CONFIG_CPU_STATS_USE_COUNTER || CONFIG_CPU_STATS_LOG_PERIODICALLY
static void cpu_stats_log_fn(struct k_work *work)
{
	int load = cpu_stats_load_get(true);
	uint32_t percent = load / 10;
	uint32_t fraction = load % 10;

	LOG_INF("Load:%d,%03d%%", percent, fraction);
	cpu_stats_log_control(true);
}

static int cpu_stats_init(void)
{
	if (IS_ENABLED(CONFIG_CPU_STATS_USE_COUNTER)) {
		int err = counter_start(counter);

		(void)err;
		__ASSERT_NO_MSG(err == 0);
	}

	if (CONFIG_CPU_STATS_LOG_PERIODICALLY > 0) {
		k_work_init_delayable(&cpu_stats_log, cpu_stats_log_fn);
		return k_work_schedule(&cpu_stats_log, K_MSEC(CONFIG_CPU_STATS_LOG_PERIODICALLY));
	}

	return 0;
}

SYS_INIT(cpu_stats_init, POST_KERNEL, 0);
#endif

void cpu_stats_on_enter_cpu_idle_hook(void)
{
	if (IS_ENABLED(CONFIG_CPU_STATS_USE_COUNTER)) {
		counter_get_value(counter, &enter_ts);
		return;
	}

	enter_ts = k_cycle_get_32();
}

void cpu_stats_on_exit_cpu_idle_hook(void)
{
	uint32_t now;

	if (IS_ENABLED(CONFIG_CPU_STATS_USE_COUNTER)) {
		counter_get_value(counter, &now);
	} else {
		now = k_cycle_get_32();
	}

	ticks_idle += now - enter_ts;
}

int cpu_stats_load_get(bool reset)
{
	uint32_t idle_us;
	uint32_t total = k_cycle_get_32() - cyc_start;
	uint32_t total_us = (uint32_t)k_cyc_to_us_floor64(total);
	uint32_t res;
	uint32_t active_us;

	if (IS_ENABLED(CONFIG_CPU_STATS_USE_COUNTER)) {
		idle_us = counter_ticks_to_us(counter, ticks_idle);
	} else {
		idle_us = k_cyc_to_us_floor64(ticks_idle);
	}

	idle_us = MIN(idle_us, total_us);
	active_us = total_us - idle_us;

	res = ((1000 * active_us) / total_us);

	if (reset) {
		cyc_start = k_cycle_get_32();
		ticks_idle = 0;
	}

	return res;
}

#ifndef CONFIG_CPU_STATS_EXT_ON_ENTER_HOOK

bool z_arm_on_enter_cpu_idle(void)
{
	cpu_stats_on_enter_cpu_idle_hook();
	return true;
}

#endif

#ifndef CONFIG_CPU_STATS_EXT_ON_EXIT_HOOK

void z_arm_on_exit_cpu_idle(void)
{
	cpu_stats_on_exit_cpu_idle_hook();
}

#endif
