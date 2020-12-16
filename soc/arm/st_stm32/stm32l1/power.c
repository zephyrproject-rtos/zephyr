/*
 * Copyright (c) 2020 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <power/power.h>
#include <soc.h>
#include <init.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/counter.h>
#include <drivers/timer/system_timer.h>

#include <stm32l1xx_ll_bus.h>
#include <stm32l1xx_ll_cortex.h>
#include <stm32l1xx_ll_pwr.h>
#include <stm32l1xx_ll_rcc.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

const struct device *clk;

/*
 * For STM32L1x SoCs currently only 'stop mode' support implemented,
 * see POWER_STATE_SLEEP_1
 *
 * Currently only should be used when combined with wake-up source,
 * and continuous (doesn't shut off in stop mode) kernel system clock !!
 */

/**
 * @brief Put processor into a power state.
 *
 * This function implements the SoC specific details necessary
 * to put the processor into available power states.
 */
void pm_power_state_set(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_PM_SLEEP_STATES
#ifdef CONFIG_HAS_POWER_STATE_SLEEP_1
	case POWER_STATE_SLEEP_1:

#ifdef CONFIG_DEBUG
		/* Enable the Debug Module during STOP mode */
		LL_DBGMCU_EnableDBGStopMode();
#endif /* CONFIG_DEBUG */

		/** Request to enter STOP mode
		* Following procedure described in STM32L1xx Reference Manual
		* See PWR part, section Low-power modes, STOP mode
		*/

		/* Enable ultra low power mode */
		LL_PWR_EnableUltraLowPower();

		/** Set the regulator to low power before setting MODE_STOP.
		* If the regulator remains in "main mode",
		* it consumes more power without providing any additional feature. */
		LL_PWR_SetRegulModeLP(LL_PWR_REGU_LPMODES_LOW_POWER);

		/* Set STOP mode when CPU enters deepsleep */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP);

		/* Set SLEEPDEEP bit of Cortex System Control Register */
		LL_LPM_EnableDeepSleep();

		/* enter SLEEP mode : WFE or WFI */
		k_cpu_idle();

		break;

#endif /* CONFIG_HAS_POWER_STATE_SLEEP_1 */
#ifdef CONFIG_HAS_POWER_STATE_SLEEP_2
	case POWER_STATE_SLEEP_2:
#endif /* CONFIG_HAS_POWER_STATE_SLEEP_2 */
#ifdef CONFIG_SYS_POWER_STATE_SLEEP_3
	case POWER_STATE_SLEEP_3:
#endif /* CONFIG_HAS_POWER_STATE_SLEEP_3 */
#endif /* CONFIG_PM_SLEEP_STATES */
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void _pm_power_state_exit_post_ops(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_PM_SLEEP_STATES
#ifdef CONFIG_HAS_POWER_STATE_SLEEP_1
	case POWER_STATE_SLEEP_1:

		/* reconfigure clock settings after waking up from
		* stop mode, see issue #28572
		*/
		clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

		if (stm32_clock_control_real_init(clk) == 0){
			/* clock reconfiguration successful */
		}
		else{
			/*error*/
		}


#endif /* CONFIG_HAS_POWER_STATE_SLEEP_1 */
#ifdef CONFIG_HAS_POWER_STATE_SLEEP_2
	case POWER_STATE_SLEEP_2:
#endif /* CONFIG_HAS_POWER_STATE_SLEEP_2 */
#ifdef CONFIG_HAS_POWER_STATE_SLEEP_3
	case POWER_STATE_SLEEP_3:
#endif /* CONFIG_HAS_POWER_STATE_SLEEP_3 */
		LL_LPM_DisableSleepOnExit();
		break;
#endif /* CONFIG_PM_SLEEP_STATES */
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	/*
	 * System is now in active mode.
	 * Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}


/* Initialize STM32 Power */
static int stm32_power_init(const struct device *dev)
{
	unsigned int ret;

	ARG_UNUSED(dev);

	ret = irq_lock();

	/* enable Power clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

	irq_unlock(ret);

	return 0;
}

SYS_INIT(stm32_power_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
