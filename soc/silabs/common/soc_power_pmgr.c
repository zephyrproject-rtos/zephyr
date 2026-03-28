/*
 * Copyright (c) 2018, Piotr Mienkowski
 * Copyright (c) 2023, Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <sl_power_manager.h>
#include <sl_hfxo_manager.h>
#include <sli_hfxo_manager.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/*
 * Power state map:
 * PM_STATE_RUNTIME_IDLE: EM1 Sleep
 * PM_STATE_SUSPEND_TO_IDLE: EM2 Deep Sleep
 * PM_STATE_SOFT_OFF: EM4
 */


/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	sl_power_manager_em_t energy_mode = SL_POWER_MANAGER_EM0;

	LOG_DBG("SoC entering power state %d", state);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		energy_mode = SL_POWER_MANAGER_EM1;
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
	case PM_STATE_STANDBY:
		energy_mode = SL_POWER_MANAGER_EM2;
		break;
	case PM_STATE_SOFT_OFF:
		energy_mode = SL_POWER_MANAGER_EM4;
		break;
	default:
		LOG_DBG("Unsupported power state %d", state);
		break;
	}

	LOG_DBG("Entry to energy mode %d", energy_mode);

	if (energy_mode == SL_POWER_MANAGER_EM4) {
		sl_power_manager_enter_em4();
	} else if (energy_mode != SL_POWER_MANAGER_EM0) {
		/* Calling the tracing and hook functions provided in arch_cpu_idle(). */
#if defined(CONFIG_TRACING)
		sys_trace_idle();
#endif
#if CONFIG_ARM_ON_ENTER_CPU_IDLE_PREPARE_HOOK
		z_arm_on_enter_cpu_idle_prepare();
#endif

		sl_power_manager_add_em_requirement(energy_mode);
		sl_power_manager_sleep();
		sl_power_manager_remove_em_requirement(energy_mode);
	}

	LOG_DBG("Exit from energy mode %d", energy_mode);
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}

/* This function is called by sl_power_manager_sleep() after it has set the PRIMASK. */
bool sl_power_manager_is_ok_to_sleep(void)
{
	/* FIXME: When this function is entered the Kernel has disabled
	 * interrupts using BASEPRI register. This is incorrect as it prevents
	 * waking up from any interrupt which priority is not 0. Work around the
	 * issue and disable interrupts using PRIMASK register as recommended
	 * by ARM.
	 */
	/* Set BASEPRI to 0. */
	irq_unlock(0);

	return true;
}

#if !defined(CONFIG_SOC_SILABS_PM_LOW_INTERRUPT_LATENCY)
/* This function is called by sl_power_manager_sleep() right after it was woken up from WFI. */
void sli_power_manager_on_wakeup(void)
{
#if defined(HFXO_MANAGER_SLEEPTIMER_SYSRTC_INTEGRATION_ON)
	/* Handle the HFXO IRQ as soon as possible to retrieve the startup time. */
	sl_hfxo_manager_irq_handler();
#endif
	/* Forces a clock restore before handling interrupts. */
	sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
	sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
}
#endif

/**
 * Some SiLabs blobs, such as RAIL, call directly into sl_power_manager, and
 * for that they had to include sl_power_manager.h during build. Some of those
 * blobs have been compiled with -DSL_POWER_MANAGER_DEBUG=1, making inlined
 * functions from that header to rely on
 * sli_power_manager_debug_log_em_requirement() callback.
 *
 * This is irrespective of whether *we* enable SL_POWER_MANAGER_DEBUG when
 * compiling sl_power_manager code as part of Zephyr build.
 *
 * Therefore, we provide sli_power_manager_debug_log_em_requirement()
 * definition here just to satisfy those blobs. It will also be used if we
 * attempt to build sl_power_manager with SL_POWER_MANAGER_DEBUG enabled.
 *
 * @note Please keep this at the end of the file.
 */

#ifdef sli_power_manager_debug_log_em_requirement
#undef sli_power_manager_debug_log_em_requirement
#endif

void sli_power_manager_debug_init(void)
{
}

void sli_power_manager_debug_log_em_requirement(
	sl_power_manager_em_t em, bool add, const char *name)
{
	LOG_DBG("Set PM requirement em=%d add=%s name=%s",
			(int)em, add ? "true" : "false", name);
}
