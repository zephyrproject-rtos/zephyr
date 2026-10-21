/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include <zephyr/init.h>

#include <stm32_ll_utils.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_cortex.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <clock_control/clock_stm32_ll_common.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* select MSIS as wake-up system clock if configured, HSI otherwise */
#if STM32_SYSCLK_SRC_MSIS
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUPCLOCK_MSIS
#else
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUPCLOCK_HSI
#endif

#if defined(CONFIG_SOC_STM32U575XX) || defined(CONFIG_SOC_STM32U585XX)
/* STM32U575/585 silicon revisions, see errata sheet ES0499 */
#define STM32U575_585_REV_X 0x2001U
#define STM32U575_585_REV_W 0x3001U

/* Max time for the oscillators and PLLs to stop, in microseconds */
#define STOP_ENTRY_CLK_OFF_TIMEOUT_US 100U

/*
 * ES0499 2.2.5 Rev 12 (device revision X and W): the device can hang if a
 * wakeup request arrives a few clock cycles before Stop 2 or Stop 3 entry
 * while PLL2, PLL3, HSI48 or SHSI is running.
 */
static bool errata_stop_entry_hang;

/*
 * ES0499 2.2.11 Rev 12 (device revision X): after Stop 2 or Stop 3, the first
 * fetch from the ICACHE line that was accessed last before entry may be
 * corrupted.
 */
static bool errata_stop2_icache;

static bool stop_entry_clocks_stopped(void)
{
	bool running = LL_RCC_PLL2_IsReady() || LL_RCC_PLL3_IsReady() || LL_RCC_HSI48_IsReady();

#if defined(RCC_CR_SHSION)
	running = running || LL_RCC_SHSI_IsReady();
#endif

	return !running;
}

static void stop_entry_clocks_off(void)
{
	/*
	 * Stop entry switches all of these off anyway: switch them off just
	 * before, and wait for them to stop, to close the window. After
	 * wakeup, stm32_clock_control_init() enables the ones in use again.
	 * The SHSI bits are read-as-zero/write-ignored from the non-secure
	 * side when SAES is secure: then only the secure side can do this.
	 */
	LL_RCC_PLL2_Disable();
	LL_RCC_PLL3_Disable();
	LL_RCC_HSI48_Disable();
#if defined(RCC_CR_SHSION)
	LL_RCC_SHSI_Disable();
#endif

	if (!WAIT_FOR(stop_entry_clocks_stopped(), STOP_ENTRY_CLK_OFF_TIMEOUT_US, NULL)) {
		LOG_DBG("Clocks still running at Stop entry");
	}
}

static void stop_errata_init(void)
{
	uint32_t rev_id = LL_DBGMCU_GetRevisionID();

	errata_stop_entry_hang = (rev_id == STM32U575_585_REV_X) || (rev_id == STM32U575_585_REV_W);
	errata_stop2_icache = (rev_id == STM32U575_585_REV_X);
}
#else
/* Other STM32U5 devices are not affected (statics are zero-initialized: false) */
static const bool errata_stop_entry_hang;
static const bool errata_stop2_icache;

static inline void stop_entry_clocks_off(void)
{
}

static inline void stop_errata_init(void)
{
}
#endif /* CONFIG_SOC_STM32U575XX || CONFIG_SOC_STM32U585XX */

#ifdef CONFIG_STM32_STOP3_LP_MODE
static void pwr_stop3_isr(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Clear all wake-up flags */
	LL_PWR_ClearFlag_WU();
}
#endif

void set_mode_stop(uint8_t substate_id)
{
	/* ensure the proper wake-up system clock */
	LL_RCC_SetClkAfterWakeFromStop(RCC_STOP_WAKEUPCLOCK_SELECTED);

	switch (substate_id) {
	case 1: /* enter STOP0 mode */
		LL_PWR_SetPowerMode(LL_PWR_STOP0_MODE);
		break;
	case 2: /* enter STOP1 mode */
		LL_PWR_SetPowerMode(LL_PWR_STOP1_MODE);
		break;
	case 3: /* enter STOP2 mode */
		if (errata_stop2_icache) {
			sys_cache_instr_disable();
		}
		if (errata_stop_entry_hang) {
			stop_entry_clocks_off();
		}
		LL_PWR_SetPowerMode(LL_PWR_STOP2_MODE);
		break;
#ifdef CONFIG_STM32_STOP3_LP_MODE
	case 4: /* enter STOP3 mode */

		LL_PWR_SetSRAM2SBRetention(LL_PWR_SRAM2_SB_FULL_RETENTION);
		/* Enable RTC wakeup
		 * This configures an internal pin that generates an event to wakeup the system
		 */
		LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN7);
		LL_PWR_SetWakeUpPinSignal3Selection(LL_PWR_WAKEUP_PIN7);

		/* Clear flags */
		LL_PWR_ClearFlag_SB();
		LL_PWR_ClearFlag_WU();

		sys_cache_instr_disable();

		if (errata_stop_entry_hang) {
			stop_entry_clocks_off();
		}
		LL_PWR_SetPowerMode(LL_PWR_STOP3_MODE);
		break;
#endif
	default:
		LOG_DBG("Unsupported power state substate-id %u", substate_id);
		break;
	}
}

void set_mode_standby(uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	/* Select standby mode */
	LL_PWR_SetPowerMode(LL_PWR_STANDBY_MODE);
}

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		set_mode_stop(substate_id);
		break;
	case PM_STATE_STANDBY:
		/* To be tested */
		set_mode_standby(substate_id);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		return;
	}

	/* Set SLEEPDEEP bit of Cortex System Control Register */
	LL_LPM_EnableDeepSleep();

	/* Select mode entry : WFE or WFI and enter the CPU selected mode */
	k_cpu_idle();
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		if (substate_id <= 3) {
			if ((substate_id == 3) && errata_stop2_icache) {
				sys_cache_instr_enable();
			}
			LL_LPM_DisableSleepOnExit();
			LL_LPM_EnableSleep();
#ifdef CONFIG_STM32_STOP3_LP_MODE
		} else if (substate_id == 4) {
			stm32_clock_control_standby_exit();

			sys_cache_instr_enable();

			LL_LPM_DisableSleepOnExit();
			LL_LPM_EnableSleep();
#endif
		} else {
			LOG_DBG("Unsupported power substate-id %u",
							substate_id);
		}
		break;
	case PM_STATE_STANDBY:
		/* To be tested */
		LL_LPM_EnableSleep();
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		__fallthrough;
	case PM_STATE_SUSPEND_TO_DISK:
		__fallthrough;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
	/* need to restore the clock */
	stm32_clock_control_init(NULL);

	/*
	 * System is now in active mode.
	 * Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}

/* Initialize STM32 Power */
void stm32_power_init(void)
{

	/* enable Power clock */
	LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_PWR);

	stop_errata_init();

#ifdef CONFIG_STM32_STOP3_LP_MODE
	IRQ_CONNECT(PWR_S3WU_IRQn, 0,
		    pwr_stop3_isr, 0, 0);
	irq_enable(PWR_S3WU_IRQn);
#endif
}
