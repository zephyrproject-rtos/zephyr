/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Idle-hook CPU load backend. The idle time is accumulated between the
 * architecture idle enter/exit hooks and compared against the elapsed wall
 * clock time. Optionally a hardware counter can be used for higher precision
 * (single CPU only).
 *
 * The idle enter/exit hooks run on the idle CPU with interrupts locked, so
 * each CPU only ever mutates its own slot from those hooks. A per-CPU spinlock
 * serializes the idle accumulation against a reader on another CPU calling
 * cpu_load_get_cpu().
 */

#include <zephyr/sys/cpu_load.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/counter.h>

BUILD_ASSERT(!IS_ENABLED(CONFIG_CPU_LOAD_USE_COUNTER) || DT_HAS_CHOSEN(zephyr_cpu_load_counter));

static const struct device *counter = COND_CODE_1(CONFIG_CPU_LOAD_USE_COUNTER,
				(DEVICE_DT_GET(DT_CHOSEN(zephyr_cpu_load_counter))), (NULL));
static uint32_t enter_ts[CONFIG_MP_MAX_NUM_CPUS];
static uint64_t cyc_start[CONFIG_MP_MAX_NUM_CPUS];
static uint64_t ticks_idle[CONFIG_MP_MAX_NUM_CPUS];

static struct k_spinlock lock[CONFIG_MP_MAX_NUM_CPUS];

static inline unsigned int curr_cpu_id(void)
{
#ifdef CONFIG_SMP
	return arch_curr_cpu()->id;
#else
	return 0U;
#endif
}

void cpu_load_on_enter_idle(void)
{
	unsigned int cpu = curr_cpu_id();

	/* Only this CPU touches its enter_ts slot, and the hook runs with
	 * interrupts locked, so no lock is needed here.
	 */
	if (IS_ENABLED(CONFIG_CPU_LOAD_USE_COUNTER)) {
		counter_get_value(counter, &enter_ts[cpu]);
		return;
	}

	enter_ts[cpu] = k_cycle_get_32();
}

void cpu_load_on_exit_idle(void)
{
	unsigned int cpu = curr_cpu_id();
	k_spinlock_key_t key;
	uint32_t now;

	if (IS_ENABLED(CONFIG_CPU_LOAD_USE_COUNTER)) {
		counter_get_value(counter, &now);
	} else {
		now = k_cycle_get_32();
	}

	key = k_spin_lock(&lock[cpu]);
	ticks_idle[cpu] += now - enter_ts[cpu];
	k_spin_unlock(&lock[cpu], key);
}

int cpu_load_get_cpu(unsigned int cpu_id, bool reset)
{
	uint64_t idle_us;
	uint64_t now;
	uint64_t total;
	uint64_t total_us;
	uint64_t idle_ticks;
	uint32_t res;
	uint64_t active_us;
	k_spinlock_key_t key;

	if (cpu_id >= CONFIG_MP_MAX_NUM_CPUS) {
		return -EINVAL;
	}

	now = IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER) ?
		k_cycle_get_64() : k_cycle_get_32();

	key = k_spin_lock(&lock[cpu_id]);
	total = now - cyc_start[cpu_id];
	idle_ticks = ticks_idle[cpu_id];
	if (reset) {
		cyc_start[cpu_id] = now;
		ticks_idle[cpu_id] = 0;
	}
	k_spin_unlock(&lock[cpu_id], key);

	total_us = k_cyc_to_us_floor64(total);
	if (total_us == 0) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_CPU_LOAD_USE_COUNTER)) {
		if (idle_ticks > (uint64_t)UINT32_MAX) {
			return -ERANGE;
		}
		idle_us = counter_ticks_to_us(counter, (uint32_t)idle_ticks);
	} else {
		idle_us = k_cyc_to_us_floor64(idle_ticks);
	}

	idle_us = MIN(idle_us, total_us);
	active_us = total_us - idle_us;

	res = (uint32_t)((1000 * active_us) / total_us);

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
