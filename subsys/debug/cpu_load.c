/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/cpu_load.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <kthread.h>
#include <zephyr/debug/thread_analyzer.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cpu_load);

BUILD_ASSERT(!IS_ENABLED(CONFIG_CPU_LOAD_USE_COUNTER) || DT_HAS_CHOSEN(zephyr_cpu_load_counter));

#ifndef CONFIG_CPU_LOAD_LOG_PERIODICALLY
#define CONFIG_CPU_LOAD_LOG_PERIODICALLY 0
#endif

const struct device *counter = COND_CODE_1(CONFIG_CPU_LOAD_USE_COUNTER,
				(DEVICE_DT_GET(DT_CHOSEN(zephyr_cpu_load_counter))), (NULL));
static uint32_t enter_ts;
static uint32_t cyc_start;
static uint32_t ticks_idle;
static struct k_work_delayable cpu_load_log;

void cpu_load_log_control(bool enable)
{
	if (CONFIG_CPU_LOAD_LOG_PERIODICALLY == 0) {
		return;
	}

	if (enable) {
		(void)cpu_load_get(true);
		k_work_schedule(&cpu_load_log, K_MSEC(CONFIG_CPU_LOAD_LOG_PERIODICALLY));
	} else {
		k_work_cancel_delayable(&cpu_load_log);
	}
}

#if CONFIG_CPU_LOAD_DETECT_FULL_UTILIZATION
static cpu_full_load_cb_t full_load_cb = NULL;

static void thread_analyze_cb(const struct k_thread *cthread, void *user_data)
{
	struct k_thread *thread = (struct k_thread *)cthread;
	if (!z_is_idle_thread_object(thread)) {
		return;
	}

	static uint64_t last_cycles;
	struct thread_analyzer_info info;

	k_thread_runtime_stats_get(thread, &info.usage);
	if (info.usage.execution_cycles == last_cycles) {
		printk("CPU at full load during last interval \n");
		if (full_load_cb != NULL) {
			full_load_cb();
		}
	}

	last_cycles = info.usage.execution_cycles;
}

void detect_timer_handler(struct k_timer *dummy)
{
	k_thread_foreach(thread_analyze_cb, NULL);
}

K_TIMER_DEFINE(detect_timer, detect_timer_handler, NULL);

int cpu_load_full_utilization_cb_reg(cpu_full_load_cb_t cb)
{
	full_load_cb = cb;
	return 0;
}

#else
int cpu_load_full_utilization_cb_reg(cpu_full_load_cb_t cb)
{
	return -ENOTSUP;
}
#endif




#if CONFIG_CPU_LOAD_USE_COUNTER || CONFIG_CPU_LOAD_LOG_PERIODICALLY || CONFIG_CPU_LOAD_DETECT_FULL_UTILIZATION
static void cpu_load_log_fn(struct k_work *work)
{
	int load = cpu_load_get(true);
	uint32_t percent = load / 10;
	uint32_t fraction = load % 10;

	LOG_INF("Load:%d.%03d%%", percent, fraction);
	cpu_load_log_control(true);
}

static int cpu_load_init(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD_USE_COUNTER)) {
		int err = counter_start(counter);

		(void)err;
		__ASSERT_NO_MSG(err == 0);
	}

	#if CONFIG_CPU_LOAD_DETECT_FULL_UTILIZATION
		k_timer_start(&detect_timer,
			      K_MSEC(CONFIG_CPU_LOAD_DETECT_FULL_UTILIZATION_INTERVAL),
			      K_MSEC(CONFIG_CPU_LOAD_DETECT_FULL_UTILIZATION_INTERVAL));
	#endif

	if (CONFIG_CPU_LOAD_LOG_PERIODICALLY > 0) {
		k_work_init_delayable(&cpu_load_log, cpu_load_log_fn);
		return k_work_schedule(&cpu_load_log, K_MSEC(CONFIG_CPU_LOAD_LOG_PERIODICALLY));
	}

	return 0;
}

SYS_INIT(cpu_load_init, POST_KERNEL, 0);
#endif

void cpu_load_on_enter_idle(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD_USE_COUNTER)) {
		counter_get_value(counter, &enter_ts);
		return;
	}

	enter_ts = k_cycle_get_32();
}

void cpu_load_on_exit_idle(void)
{
	uint32_t now;

	if (IS_ENABLED(CONFIG_CPU_LOAD_USE_COUNTER)) {
		counter_get_value(counter, &now);
	} else {
		now = k_cycle_get_32();
	}

	ticks_idle += now - enter_ts;
}

int cpu_load_get(bool reset)
{
	uint32_t idle_us;
	uint32_t total = k_cycle_get_32() - cyc_start;
	uint32_t total_us = (uint32_t)k_cyc_to_us_floor32(total);
	uint32_t res;
	uint32_t active_us;

	if (IS_ENABLED(CONFIG_CPU_LOAD_USE_COUNTER)) {
		idle_us = counter_ticks_to_us(counter, ticks_idle);
	} else {
		idle_us = k_cyc_to_us_floor32(ticks_idle);
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
