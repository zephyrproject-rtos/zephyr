/*
 * Copyright (c) 2022 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ext_driver/ext_pm.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <stimer.h>
#include <zephyr/zephyr.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#define DT_DRV_COMPAT telink_machine_timer

#define MTIME_REG	DT_INST_REG_ADDR(0)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 8)

#define mticks_to_systicks(mticks)                                                                 \
	(((uint64_t)(mticks)*SYSTEM_TIMER_TICK_1S) / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
#define systicks_to_mticks(sticks)                                                                 \
	(((uint64_t)(sticks)*CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) / SYSTEM_TIMER_TICK_1S)

#define SYSTICKS_MAX_SLEEP 0xe0000000
#define SYSTICKS_MIN_SLEEP 18352

/**
 * @brief This define converts Machine Timer ticks to B91 System Timer ticks.
 */
#define MTIME_TO_STIME_SCALE (SYSTEM_TIMER_TICK_1S / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)

/**
 * @brief Get Machine Timer Compare value.
 */
static uint64_t get_mtime_compare(void)
{
	return *(const volatile uint64_t *const)((uint32_t)(MTIMECMP_REG +
						 (_current_cpu->id * sizeof(uint64_t))));
}

/**
 * @brief Get Machine Timer value.
 */
static uint64_t get_mtime(void)
{
	const volatile uint32_t *const rl = (const volatile uint32_t *const)MTIME_REG;
	const volatile uint32_t *const rh =
		(const volatile uint32_t *const)(MTIME_REG + sizeof(uint32_t));
	uint32_t mtime_l, mtime_h;

	do {
		mtime_h = *rh;
		mtime_l = *rl;
	} while (mtime_h != *rh);
	return (((uint64_t)mtime_h) << 32) | mtime_l;
}

/**
 * @brief Set Machine Timer value.
 */
static void set_mtime(uint64_t time)
{
	volatile uint32_t *const rl = (volatile uint32_t *const)MTIME_REG;
	volatile uint32_t *const rh =
		(volatile uint32_t *const)(MTIME_REG + sizeof(uint32_t));

	*rl = 0;
	*rh = (uint32_t)(time >> 32);
	*rl = (uint32_t)time;
}

/**
 * @brief PM state set API implementation.
 */
__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	uint32_t tl_sleep_tick = stimer_get_tick();
	uint64_t current_time = get_mtime();
	uint64_t wakeup_time = get_mtime_compare();

	if (wakeup_time <= current_time) {
		LOG_DBG("Sleep Time = 0 or less\n");
		return;
	}

	uint64_t stimer_sleep_ticks = mticks_to_systicks(wakeup_time - current_time);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		if (stimer_sleep_ticks < SYSTICKS_MIN_SLEEP) {
			k_cpu_idle();
		} else {
			if (stimer_sleep_ticks > SYSTICKS_MAX_SLEEP) {
				stimer_sleep_ticks = SYSTICKS_MAX_SLEEP;
			}
			cpu_sleep_wakeup_32k_rc(SUSPEND_MODE, PM_WAKEUP_TIMER,
						tl_sleep_tick + stimer_sleep_ticks);
			current_time += systicks_to_mticks(stimer_get_tick() - tl_sleep_tick);
			set_mtime(current_time);
		}
		break;

	default:
		LOG_DBG("Unsupported power state %u", state);
		k_cpu_idle();
		break;
	}
}

/**
 * @brief PM state exit post operations API implementation.
 */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	/*
	 * System is now in active mode. Enabling interrupts which were
	 * disabled when OS started idle code.
	 */
	arch_irq_unlock(MSTATUS_IEN);
}
