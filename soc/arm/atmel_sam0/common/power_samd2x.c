/*
 * Copyright (c) 2019, Steven Slupsky
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power/power.h>
#include <soc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#define TICK_THRESHOLD 7

static volatile u32_t rtc_sleep_count;

/*
 * Power state map:
 * SYS_POWER_STATE_SLEEP_1: IDLE
 *	Idle Mode 1:  The CPU and AHB clock domains are stopped
 * SYS_POWER_STATE_SLEEP_2: STANDBY
 */

/* Invoke low power specific tasks */
void sys_set_power_state(enum power_states state)
{
	/*
	 * Save the current value of the COUNT register
	 * so we can verify the RTC is ticking when we wake
	 * up
	 */
	rtc_sleep_count = z_timer_cycle_get_32();
	LOG_DBG("SoC entering power state %d", state);

	switch (state) {
#ifdef CONFIG_SYS_POWER_SLEEP_STATES
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_1
	case SYS_POWER_STATE_SLEEP_1:
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
		PM->SLEEP.reg = 1;
		__DSB();
		__WFI();
		__NOP();
		__NOP();
		break;
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_1 */
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_2
	case SYS_POWER_STATE_SLEEP_2:
#ifdef CONFIG_CORTEX_M_SYSTICK
/*
 * Disable SysTick interrupt before entering sleep
 * See link below for additional information about this problem
 * https://www.avrfreaks.net/forum/samd21-samd21e16b-sporadically-locks-and-
 * does-not-wake-standby-sleep-mode
 */
		SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
#endif
		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
		__DSB();
		__WFI();
		__NOP();
		__NOP();
#ifdef CONFIG_CORTEX_M_SYSTICK
/* Re-enable SysTick interrupt after sleep */
		SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
#endif
		break;
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_2 */
#endif /* CONFIG_SYS_POWER_SLEEP_STATES */
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	LOG_DBG("SoC leaving power state %d", state);

}

/* Handle SOC specific activity after Low Power Mode Exit */
void _sys_pm_power_state_exit_post_ops(enum power_states state)
{
	ARG_UNUSED(state);

	/*
	 * The kernel does not like it when the RTC does not tick
	 * after waking from sleep.  RCONT enables continuously syncing the
	 * COUNT register but after sleep, we need to wait for a new read
	 * request to complete.  However, the SYNCBUSY flag is not set
	 * when RCONT is enabled so we cannot use that to do the sync.
	 * So, we wait for the RTC COUNT register to begin ticking again by
	 * reading the COUNT register.
	 *
	 * We should wait a minimum of the TICK_THRESHOLD which is the amount
	 * of time it takes to sync the register.
	 */
	while (z_timer_cycle_get_32() - rtc_sleep_count < TICK_THRESHOLD) {
		/* wait */
	}
	irq_unlock(0);
}
