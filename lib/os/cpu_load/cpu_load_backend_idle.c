/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Idle-hook CPU load backend. The idle time is accumulated between the
 * architecture idle enter/exit hooks and compared against the elapsed wall
 * clock time. Optionally a hardware counter can be used for higher precision.
 * Limited to a single CPU.
 */

#include <zephyr/sys/cpu_load.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/counter.h>

BUILD_ASSERT(!IS_ENABLED(CONFIG_CPU_LOAD_USE_COUNTER) || DT_HAS_CHOSEN(zephyr_cpu_load_counter));

static const struct device *counter = COND_CODE_1(CONFIG_CPU_LOAD_USE_COUNTER,
				(DEVICE_DT_GET(DT_CHOSEN(zephyr_cpu_load_counter))), (NULL));
static uint32_t enter_ts;
static uint64_t cyc_start;
static uint64_t ticks_idle;

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

int cpu_load_get_cpu(unsigned int cpu_id, bool reset)
{
	uint64_t idle_us;
	uint64_t now = IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER) ?
		k_cycle_get_64() : k_cycle_get_32();
	uint64_t total = now - cyc_start;
	uint64_t total_us = k_cyc_to_us_floor64(total);
	uint32_t res;
	uint64_t active_us;

	/* This backend measures a single CPU only. */
	ARG_UNUSED(cpu_id);

	if (IS_ENABLED(CONFIG_CPU_LOAD_USE_COUNTER)) {
		if (ticks_idle > (uint64_t)UINT32_MAX) {
			return -ERANGE;
		}
		idle_us = counter_ticks_to_us(counter, (uint32_t)ticks_idle);
	} else {
		idle_us = k_cyc_to_us_floor64(ticks_idle);
	}

	idle_us = MIN(idle_us, total_us);
	active_us = total_us - idle_us;

	res = (uint32_t)((1000 * active_us) / total_us);

	if (reset) {
		cyc_start = now;
		ticks_idle = 0;
	}

	return res;
}

#ifdef CONFIG_CPU_LOAD_USE_COUNTER
static int cpu_load_counter_init(void)
{
	int err = counter_start(counter);

	(void)err;
	__ASSERT_NO_MSG(err == 0);

	return 0;
}

SYS_INIT(cpu_load_counter_init, POST_KERNEL, 0);
#endif
