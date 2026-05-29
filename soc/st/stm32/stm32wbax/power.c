/*
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include <stm32_bitops.h>
#include <zephyr/init.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <kernel_arch_func.h>

#include <cmsis_core.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_cortex.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <clock_control/clock_stm32_ll_common.h>

#include <zephyr/logging/log.h>

#if defined(CONFIG_PM) && (defined(CONFIG_BT) || defined(CONFIG_IEEE802154))
#include <zephyr/pm/policy.h>
#include "os_wrapper.h"
#include "ll_sys.h"
#endif

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#if defined(CONFIG_PM_S2RAM)
/* Retained region information (from DTS) */
#define PWRC_NODE	DT_INST(0, st_stm32wba_pwr)
#define RETAINED_SRAM12	DT_CHOSEN(zephyr_sram)
#define RETAINED_SRAM12_START	(DT_REG_ADDR(RETAINED_SRAM12))
#define RETAINED_SRAM12_END	(RETAINED_SRAM12_START + DT_REG_SIZE(RETAINED_SRAM12))

/* Hardware information */
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
#define BASE(sramn) CONCAT(sramn, _BASE_S)
#else
#define BASE(sramn) CONCAT(sramn, _BASE_NS)
#endif
#define SRAM1_PAGE_SIZE (64 * 1024)

/*
 * @param i1s Start of interval 1 (inclusive)
 * @param i1e End of interval 1 (exclusive)
 * @param i2s Start of interval 2 (inclusive)
 * @param i2e End of interval 2 (exclusive)
 *
 * Evaluates as true if there is an overlap between
 * intervals [i1s, i1e) and [i2s, i2e).
 */
#define RANGES_OVERLAP(i1s, i1e, i2s, i2e)					\
	(((i1e) > (i2s)) && ((i1s) < (i2e)))

#if !defined(PWR_STOP2_SUPPORT) || defined(PWR_STOP3_SUPPORT) /* STM32WBA5x or STM32WBA2x */
/* No page granularity on STM32WBA5x and STM32WBA2x */
#define SRAM1_RETENTION_MASK LL_PWR_SRAM1_SB_FULL_RETENTION
#else /* STM32WBA6x */
/*
 * @param pgn Page number as described in RefMan: 1~7
 *
 * Evaluates as true if page @p{pgn} is part of the retained
 * region, false otherwise.
 */
#define SRAM1_PAGE_USED(pgn)							\
	RANGES_OVERLAP(RETAINED_SRAM12_START, RETAINED_SRAM12_END,		\
		BASE(SRAM1) + (((pgn) - 1) * SRAM1_PAGE_SIZE),			\
		BASE(SRAM1) + ((pgn) * SRAM1_PAGE_SIZE))

#define SRAM1_PAGE_RETAIN_MASK(pgn, msknum)					\
	(SRAM1_PAGE_USED(pgn) ?	CONCAT(LL_PWR_SRAM1_SB_PAGE, msknum, _RETENTION) : 0)

#define SRAM1_RETENTION_MASK							\
	(SRAM1_PAGE_RETAIN_MASK(1, 1) |						\
	 SRAM1_PAGE_RETAIN_MASK(2, 2) |						\
	 SRAM1_PAGE_RETAIN_MASK(3, 3) |						\
	 SRAM1_PAGE_RETAIN_MASK(4, 4) |						\
	 SRAM1_PAGE_RETAIN_MASK(5, 567) |					\
	 SRAM1_PAGE_RETAIN_MASK(6, 567) |					\
	 SRAM1_PAGE_RETAIN_MASK(7, 567))

BUILD_ASSERT(SRAM1_RETENTION_MASK != 0U,
	"Retained SRAM1/2 region does not contain any SRAM1 page!");
#endif /* !defined(PWR_STOP2_SUPPORT) || defined(PWR_STOP3_SUPPORT) */

#define RETAINED_SRAM12_OVERLAPS_SRAM2                                 \
		(RANGES_OVERLAP(RETAINED_SRAM12_START,                 \
				RETAINED_SRAM12_END,                   \
				BASE(SRAM2),                           \
				BASE(SRAM2) + SRAM2_SIZE))
#endif /* CONFIG_PM_S2RAM */


#define HSE_ON (stm32_reg_read_bits(&RCC->CR, RCC_CR_HSEON) == RCC_CR_HSEON)

static uint32_t ram_waitstates_backup;
static uint32_t flash_latency_backup;

#if defined(CONFIG_PM_S2RAM)
static struct fpu_ctx_full fpu_state;
static struct scb_context scb_state;
#if defined(CONFIG_ARM_MPU)
static struct z_mpu_context_retained mpu_state;
#endif
#endif

static int enter_low_power_mode(void)
{
	uint32_t basepri;

	LL_LPM_EnableDeepSleep();

	while (LL_PWR_IsActiveFlag_ACTVOS() == 0) {
	}

	/*
	 * Prevent spurious entry in low-power state if any
	 * interrupt is currently pending. The WFI instruction
	 * will not enter low-power state if an interrupt with
	 * higher priority than BASEPRI is pending, regardless
	 * of the value of PRIMASK.
	 *
	 * When this function executes, Zephyr's arch_irq_lock() has
	 * configured BASEPRI to mask interrupts. Set PRIMASK to keep
	 * interrupts masked then lower BASEPRI to 0, after saving its
	 * current value, so that any pending interrupt is seen by WFI
	 * and inhibits low-power state entry.
	 */
	__disable_irq();
	basepri = __get_BASEPRI();
	__set_BASEPRI(0);
	__ISB();
	__DSB();

	/* Attempt entry in low-power state */
	__WFI();

	/*
	 * If entry in STOP mode was attempted, execution
	 * resumes here regardless of success or failure.
	 * If entry in STANDBY mode was attempted, execution
	 * will reach this point only if entry in low-power
	 * state did not occur. Restore BASEPRI and clear
	 * PRIMASK then return an error to indicate that
	 * entry in STANDBY mode failed (return value of
	 * this function is ignored for STOP mode entry).
	 */
	__set_BASEPRI(basepri);
	__enable_irq();

	/* Disable SLEEPDEEP at Cortex-M level */
	LL_LPM_EnableSleep();

	return -EBUSY;
}

#if defined(CONFIG_PM_S2RAM)
static void set_mode_suspend_to_ram_enter(void)
{
	/* Enable RTC wakeup
	 * This configures an internal pin that generates an event to wakeup the system
	 */
	LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN7);
	LL_PWR_SetWakeUpPinSignal3Selection(LL_PWR_WAKEUP_PIN7);

	/* Clear flags */
	LL_PWR_ClearFlag_SB();
	LL_PWR_ClearFlag_WU();
	LL_RCC_ClearResetFlags();

	sys_cache_instr_disable();

	/* Select standby mode */
	LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);

	/* Save FPU, SCB and MPU states */
	z_arm_save_fp_context(&fpu_state);
	z_arm_save_scb_context(&scb_state);
#if defined(CONFIG_ARM_MPU)
	z_arm_save_mpu_context(&mpu_state);
#endif /* CONFIG_ARM_MPU */
	/* Save context and enter Standby mode */
	arch_pm_s2ram_suspend(enter_low_power_mode);
}

static void set_mode_suspend_to_ram_exit(void)
{
	/* Save MPU, SCB and FPU states */
#if defined(CONFIG_ARM_MPU)
	z_arm_restore_mpu_context(&mpu_state);
#endif /* CONFIG_ARM_MPU */
	z_arm_restore_scb_context(&scb_state);
	z_arm_restore_fp_context(&fpu_state);

	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_RCC_RAMCFG_CLK_ENABLE();

	/* Restore system clock as soon as we exit standby mode */
	stm32_clock_control_standby_exit();

	if (LL_PWR_IsActiveFlag_SB() || !HSE_ON) {
		stm32_clock_control_init(NULL);
	}
	/* Resume configuration */
	stm32wba_init();
	stm32_power_init();
}
#endif

static void set_mode_stop_enter(uint8_t substate_id)
{
	LL_PWR_ClearFlag_STOP();
	LL_RCC_ClearResetFlags();

	/* Erratum 2.2.15:
	 * Disabling ICACHE is required before entering stop mode
	 */
	sys_cache_instr_disable();
	/* If stop mode is greater than 1, manage flash latency and RAMCFG */
	if (substate_id > 1) {
		flash_latency_backup = __HAL_FLASH_GET_LATENCY();
		if (flash_latency_backup < FLASH_LATENCY_1) {
			/* Set flash latency to 1, since system will restart from HSI16 */
			__HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);
			while (__HAL_FLASH_GET_LATENCY() != FLASH_LATENCY_1) {
			}
		}
		ram_waitstates_backup = stm32_reg_read_bits(&RAMCFG_SRAM1->CR, RAMCFG_CR_WSC);
		stm32_reg_modify_bits(&RAMCFG_SRAM1->CR, RAMCFG_CR_WSC, RAMCFG_WAITSTATE_1);
		stm32_reg_modify_bits(&RAMCFG_SRAM2->CR, RAMCFG_CR_WSC, RAMCFG_WAITSTATE_1);
	}
	switch (substate_id) {
	case 1:
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP0);
		break;
	case 2:
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP1);
		break;
	default:
		LOG_DBG("Unsupported power state substate-id %u", substate_id);
		return;
	}

	enter_low_power_mode();
}

static void set_mode_stop_exit(uint8_t substate_id)
{
	if (LL_PWR_IsActiveFlag_STOP() || !HSE_ON) {
		/* Reconfigure the clock (incl. RAM/FLASH latency) */
		stm32_clock_control_init(NULL);
	} else {
		/* Stop mode skipped */
		if (substate_id > 1) {
			__HAL_FLASH_SET_LATENCY(flash_latency_backup);
			while (__HAL_FLASH_GET_LATENCY() != flash_latency_backup) {
			}
			stm32_reg_modify_bits(&RAMCFG_SRAM1->CR, RAMCFG_CR_WSC,
					      ram_waitstates_backup);
			stm32_reg_modify_bits(&RAMCFG_SRAM2->CR, RAMCFG_CR_WSC,
					      ram_waitstates_backup);
		}
	}

	/* Erratum 2.2.15:
	 * Enable ICACHE when exiting stop mode
	 */
	sys_cache_instr_enable();
}

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		set_mode_stop_enter(substate_id);
		/* Resume here */
		set_mode_stop_exit(substate_id);
		break;
#if defined(CONFIG_PM_S2RAM)
	case PM_STATE_SUSPEND_TO_RAM:
		set_mode_suspend_to_ram_enter();
		/* Resume here */
		set_mode_suspend_to_ram_exit();
		break;
#endif
	default:
		LOG_DBG("Unsupported power state %u", state);
		return;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	/*
	 * System is now in active mode.
	 * Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}

#ifdef CONFIG_PM_CUSTOM_TICKS_HOOK
int64_t pm_policy_next_custom_ticks(void)
{
	int64_t ret;

	if (LL_PWR_GetRadioMode() == LL_PWR_RADIO_ACTIVE_MODE) {
		ret = 0; /* Radio is active - inhibit sleep */
	} else {
		uint64_t next_radio_evt_us = os_timer_get_earliest_time();

		if (next_radio_evt_us == LL_DP_SLP_NO_WAKEUP) {
			ret = -1LL; /* No radio event pending */
		} else {
			ret = k_us_to_ticks_floor64(next_radio_evt_us);
		}
	}
	return ret;
}
#endif /* CONFIG_PM_CUSTOM_TICKS_HOOK */

/* Initialize STM32 Power */
void stm32_power_init(void)
{

	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_PWR);

#ifdef CONFIG_DEBUG
	LL_DBGMCU_APB7_GRP1_FreezePeriph(LL_DBGMCU_APB7_GRP1_RTC_STOP);
	LL_DBGMCU_APB7_GRP1_FreezePeriph(LL_DBGMCU_APB7_GRP1_LPTIM1_STOP);
#endif

#ifdef CONFIG_PM_S2RAM
	/*
	 * According the defined RAM, only the required pages will be retained.
	 * This memory size can be modified by the overlay mechanism matching
	 * the RAM required by the application and highlighted in the building report.
	 */

	/* Retain only the required SRAM1 pages */
	LL_PWR_SetSRAM1SBRetention(SRAM1_RETENTION_MASK);

	if (DT_PROP(PWRC_NODE, retain_sram2) || RETAINED_SRAM12_OVERLAPS_SRAM2) {
		/* Retain SRAM2 if explicitly requested or used by Zephyr as system RAM */
		LL_PWR_SetSRAM2SBRetention(LL_PWR_SRAM2_SB_FULL_RETENTION);
	}

	if (IS_ENABLED(CONFIG_BT_STM32WBA) || IS_ENABLED(CONFIG_IEEE802154_STM32WBA)) {
		/* Retain radio RAM if a radio driver is enabled */
		LL_PWR_SetRadioSBRetention(LL_PWR_RADIO_SB_FULL_RETENTION);
	}
#endif

	/* Enabling  Ultra Low power mode */
	LL_PWR_EnableUltraLowPowerMode();

	LL_FLASH_EnableSleepPowerDown();
}
