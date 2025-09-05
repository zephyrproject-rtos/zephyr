/*
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <kernel_arch_func.h>

#include <cmsis_core.h>

#include <stm32wbaxx_ll_bus.h>
#include <stm32wbaxx_ll_cortex.h>
#include <stm32wbaxx_ll_pwr.h>
#include <stm32wbaxx_ll_rcc.h>
#include <stm32wbaxx_ll_system.h>
#include <clock_control/clock_stm32_ll_common.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);


#define HSE_ON (READ_BIT(RCC->CR, RCC_CR_HSEON) == RCC_CR_HSEON)

static uint32_t ram_waitstates_backup;
static uint32_t flash_latency_backup;

#if defined(CONFIG_PM_S2RAM)
static struct fpu_ctx_full fpu_state;
static struct scb_context scb_state;
static struct z_mpu_context_retained mpu_state;
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
		ram_waitstates_backup = READ_BIT(RAMCFG_SRAM1->CR, RAMCFG_CR_WSC);
		MODIFY_REG(RAMCFG_SRAM1->CR, RAMCFG_CR_WSC, RAMCFG_WAITSTATE_1);
		MODIFY_REG(RAMCFG_SRAM2->CR, RAMCFG_CR_WSC, RAMCFG_WAITSTATE_1);
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
			MODIFY_REG(RAMCFG_SRAM1->CR, RAMCFG_CR_WSC, ram_waitstates_backup);
			MODIFY_REG(RAMCFG_SRAM2->CR, RAMCFG_CR_WSC, ram_waitstates_backup);
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

/* Initialize STM32 Power */
void stm32_power_init(void)
{

	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_PWR);

#ifdef CONFIG_DEBUG
	LL_DBGMCU_EnableDBGStandbyMode();
	LL_DBGMCU_APB7_GRP1_FreezePeriph(LL_DBGMCU_APB7_GRP1_RTC_STOP);
	LL_DBGMCU_APB7_GRP1_FreezePeriph(LL_DBGMCU_APB7_GRP1_LPTIM1_STOP);
#else
	LL_DBGMCU_DisableDBGStandbyMode();
#endif

	/* Enable SRAM full retention */
	LL_PWR_SetSRAM1SBRetention(LL_PWR_SRAM1_SB_FULL_RETENTION);
	LL_PWR_SetSRAM2SBRetention(LL_PWR_SRAM2_SB_FULL_RETENTION);

	/* Enable Radio RAM full retention */
	LL_PWR_SetRadioSBRetention(LL_PWR_RADIO_SB_FULL_RETENTION);

	/* Enabling  Ultra Low power mode */
	LL_PWR_EnableUltraLowPowerMode();

	LL_FLASH_EnableSleepPowerDown();
}
