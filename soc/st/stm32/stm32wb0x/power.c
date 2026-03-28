/*
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 * STM32WB0 Deepstop implementation for Power Management framework
 *
 * TODO:
 *	- document the control flow on PM transitions
 *	- assertions around system configuration
 *	  (e.g., valid slow clock selected, RTC enabled, ...)
 *	- ...
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys_clock.h>
#include <zephyr/init.h>
#include <zephyr/arch/common/pm_s2ram.h>

/* Private headers in zephyr/drivers/... */
#include <clock_control/clock_stm32_ll_common.h>

#include <soc.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_cortex.h>
#include <stm32_ll_system.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#if defined(CONFIG_SOC_STM32WB05XX) || defined(CONFIG_SOC_STM32WB09XX)
#define HAS_GPIO_RETENTION 1
#else
#define HAS_GPIO_RETENTION 0
#endif /* CONFIG_SOC_STM32WB05XX || CONFIG_SOC_STM32WB09XX */

/* System-level state managed by PM callbacks
 * Things that need to be preserved across Deepstop, but
 * have no associated driver to backup and restore them.
 */
#define STM32WB0_SRAM		DT_CHOSEN(zephyr_sram)
#define STM32WB0_BL_STACK_SIZE	(20 * 4)	/* in bytes */
#define STM32WB0_BL_STACK_TOP	((void *)(DT_REG_ADDR(STM32WB0_SRAM) + DT_REG_SIZE(STM32WB0_SRAM) \
					  - STM32WB0_BL_STACK_SIZE))

static uint8_t bl_stk_area_backup[STM32WB0_BL_STACK_SIZE];
static uint32_t rcc_apb1enr_vr, rcc_ahbenr_vr;

/* Callback for arch_pm_s2ram_suspend */
static int suspend_system_to_deepstop(void)
{
	/* Enable SLEEPDEEP to allow entry in Deepstop */
	LL_LPM_EnableDeepSleep();

	/* Complete all memory transactions */
	__DSB();


	/* Attempt entry in Deepstop */
	__WFI();

	/* Make sure no meaningful instruction is
	 * executed during the two cycles latency
	 * it takes to power-gate the CPU.
	 */
	__NOP();
	__NOP();

	/* This code is reached only if the device did not
	 * enter Deepstop mode (e.g., because an interrupt
	 * became pending during preparatory work).
	 * Disable SLEEPDEEP and return the appropriate error.
	 */
	LL_LPM_EnableSleep();

	return -EBUSY;
}

/* Backup system state to save and configure power
 * controller before entry in Deepstop mode
 */
static void prepare_for_deepstop_entry(void)
{
	/* DEEPSTOP2 configuration is performed in familiy-wide code
	 * instead of here (see `soc/st/stm32/common/soc_config.c`).
	 * RAMRET configuration is performed once during SoC init,
	 * since it is retained across Deepstop (see `soc.c`).
	 */

	/* Save the clock configuration. */
	rcc_apb1enr_vr = RCC->APB1ENR;
	rcc_ahbenr_vr = RCC->AHBENR;

	/* Clear wakeup reason flags (which inhibit Deepstop) */
	LL_PWR_ClearWakeupSource(LL_PWR_WAKEUP_ALL);
	LL_SYSCFG_PWRC_ClearIT(LL_SYSCFG_PWRC_WKUP);
	LL_PWR_ClearDeepstopSeqFlag();
	LL_PWR_EnableWU_EWBLEHCPU();

#if HAS_GPIO_RETENTION
	/* Enable GPIO state retention in Deepstop if available.
	 * Do not enable this if low-power mode debugging has been
	 * enabled via Kconfig, because it prevents the debugger
	 * from staying connected to the SoC.
	 */
	if (!IS_ENABLED(CONFIG_STM32_ENABLE_DEBUG_SLEEP_STOP)) {
		LL_PWR_EnableGPIORET();
		LL_PWR_EnableDBGRET();
	}
#endif /* HAS_GPIO_RETENTION */

	/* The STM32WB0 bootloader uses the end of SRAM as stack.
	 * Since it is executed on every reset, including wakeup
	 * from Deepstop, any data placed at the end of SRAM would
	 * be corrupted.
	 * Backup these words for later restoration to avoid data
	 * corruption. A much better solution would be to mark this part
	 * of SRAM as unusable, but no easy solution was found to
	 * achieve this.
	 */
	memcpy(bl_stk_area_backup, STM32WB0_BL_STACK_TOP, STM32WB0_BL_STACK_SIZE);
}

/* Restore SoC-level configuration lost in Deepstop
 * This function must be called right after wakeup.
 */
static void post_resume_configuration(void)
{
	__ASSERT_NO_MSG(LL_PWR_GetDeepstopSeqFlag() == 1);

	/* VTOR has been reset to its default value: restore it.
	 * (Note that RAM_VR.AppBase was filled during SoC init)
	 */
	SCB->VTOR = RAM_VR.AppBase;

	/* Restore the clock configuration. */
	RCC->AHBENR = rcc_ahbenr_vr;
	RCC->APB1ENR = rcc_apb1enr_vr;

	/* Restore bootloader stack area */
	memcpy(STM32WB0_BL_STACK_TOP, bl_stk_area_backup, STM32WB0_BL_STACK_SIZE);
}

/* Power Management subsystem callbacks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	/* Ignore substate: STM32WB0 has only one low-power mode */
	ARG_UNUSED(substate_id);

	if (state != PM_STATE_SUSPEND_TO_RAM) {

		/* Deepstop is a suspend-to-RAM state.
		 * Something is wrong if a different
		 * power state has been requested.
		 */
		LOG_ERR("Unsupported power state %u", state);
	}

	prepare_for_deepstop_entry();

	/* Select Deepstop low-power mode and suspend system */
	LL_PWR_SetPowerMode(LL_PWR_MODE_DEEPSTOP);

	if (arch_pm_s2ram_suspend(suspend_system_to_deepstop) >= 0) {

		/* Restore system configuration only if the SoC actually
		 * entered Deepstop - otherwise, no state has been lost
		 * and it would be a waste of time to do so.
		 */
		post_resume_configuration();
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	/* We restore system state in @ref{post_resume_configuration}.
	 * The only thing we may have to do is release GPIO retention,
	 * which we have not done yet because we wanted the driver to
	 * restore all configuration first.
	 * We also need to enable IRQs to fullfill the API contract.
	 */
#if HAS_GPIO_RETENTION
	LL_PWR_DisableGPIORET();
	LL_PWR_DisableDBGRET();
#endif /* HAS_GPIO_RETENTION */

	__enable_irq();
}
