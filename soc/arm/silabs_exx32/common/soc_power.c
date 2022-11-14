/*
 * Copyright (c) 2018, Piotr Mienkowski
 * Copyright (c) 2023, Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <em_emu.h>

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#ifdef CONFIG_SOC_GECKO_DEV_INIT
#include <sl_power_manager.h>

static const struct device *const counter_dev = DEVICE_DT_GET(DT_NODELABEL(stimer0));
static struct counter_alarm_cfg wakeup_cfg = {
	.callback = NULL
};
#endif

/*
 * Power state map:
 * PM_STATE_RUNTIME_IDLE: EM1 Sleep
 * PM_STATE_SUSPEND_TO_IDLE: EM2 Deep Sleep
 * PM_STATE_STANDBY: EM3 Stop
 */

/* Invoke Low Power/System Off specific Tasks */
__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	LOG_DBG("SoC entering power state %d", state);

#ifdef CONFIG_SOC_GECKO_DEV_INIT
	sl_power_manager_em_t energy_mode;

	/* Save RTCC IRQ priority */
	uint32_t rtcc_prio = NVIC_GetPriority(RTCC_IRQn);
	/*
	 * When this function is entered the Kernel has disabled handling interrupts
	 * with priority other than 0. The Interrupt for the timer used to wake up
	 * the cpu has priority equal to 1. Manually set this priority to 0 so that
	 * cpu could exit sleep state.
	 *
	 * Note on priority value: set priority to -1 because z_arm_irq_priority_set
	 * offsets the priorities by 1.
	 * This function call will effectively set the priority to 0.
	 */
	z_arm_irq_priority_set(RTCC_IRQn, -1, 0);

	switch (state) {

	case PM_STATE_STANDBY:
		energy_mode = SL_POWER_MANAGER_EM3;

		/* Limit sleep level to given state */
		sl_power_manager_add_em_requirement(energy_mode);

		counter_start(counter_dev);
		counter_set_channel_alarm(counter_dev, 0, &wakeup_cfg);
		sl_power_manager_sleep();
		k_cpu_idle();
		counter_stop(counter_dev);

		/* Remove sleep level limit */
		sl_power_manager_remove_em_requirement(energy_mode);
		break;

	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	LOG_DBG("SoC leaving power state %d", state);

	/* Restore RTCC IRQ priority */
	z_arm_irq_priority_set(RTCC_IRQn, (rtcc_prio - 1), 0);
#else

	/* FIXME: When this function is entered the Kernel has disabled
	 * interrupts using BASEPRI register. This is incorrect as it prevents
	 * waking up from any interrupt which priority is not 0. Work around the
	 * issue and disable interrupts using PRIMASK register as recommended
	 * by ARM.
	 */

	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		EMU_EnterEM1();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		EMU_EnterEM2(true);
		break;
	case PM_STATE_STANDBY:
		EMU_EnterEM3(true);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	LOG_DBG("SoC leaving power state %d", state);

	/* Clear PRIMASK */
	__enable_irq();

#endif
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}

#if defined(CONFIG_SOC_GECKO_DEV_INIT) && defined(CONFIG_PM_POLICY_CUSTOM)
/* CONFIG_PM_POLICY_CUSTOM allows us to set the next alarm to a specific number
 * of ticks in the future. This is needed for the Gecko SleepTimer to wake up
 * the device properly.
 */
static const struct pm_state_info pm_min_residency[] =
	PM_STATE_INFO_LIST_FROM_DT_CPU(DT_NODELABEL(cpu0));
struct pm_state_info pm_state_active = {PM_STATE_ACTIVE, 0, 0, 0};

struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	int i;

	for (i = ARRAY_SIZE(pm_min_residency) - 1; i >= 0; i--) {

		if ((ticks == K_TICKS_FOREVER) ||
		    (ticks >= k_us_to_ticks_ceil32(
			     pm_min_residency[i].min_residency_us))) {
			LOG_DBG("Selected power state %d "
				"(ticks: %d, min_residency: %u)",
				pm_min_residency[i].state, ticks,
				pm_min_residency[i].min_residency_us);

			/* Busy waiting for 1 tick to flush UART buffer */
			k_busy_wait(k_ticks_to_us_floor32(1));
			wakeup_cfg.ticks = ticks;

			return ((struct pm_state_info *) &pm_min_residency[i]);
		}
	}
	return (struct pm_state_info *)(&pm_state_active);
}
#endif
