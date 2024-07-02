/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmsis_core.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/sys_io.h>

/* MEC5 HAL */
#include <mec_ecs_api.h>
#include <mec_htimer_api.h>
#include <mec_pcr_api.h>

/* local */
#include "soc_power_debug.h"

#define MEC_HTMR0_NODE_ID DT_NODELABEL(hibtimer0)
#define MEC_HTMR0_REG_ADDR DT_REG_ADDR(MEC_HTMR0_NODE_ID)

/* MEC ktimer timer driver */
#ifdef CONFIG_RTOS_TIMER
/* Zephyr kernel timer using 32KHz based MEC5 RTOS timer.
 * Driver also uses one instance of a 32-bit basic timer
 * to provide 1 us accurate timing for the k_busy_wait API.
 * MEC5 basic timers have do not respect its SLP_EN request
 * signal and must be manually disabled before sleep entry
 * and re-enabled on wake. These API's are used for this
 * purpose.
 */
extern void soc_ktimer_pm_entry(bool is_deep_sleep);
extern void soc_ktimer_pm_exit(bool is_deep_sleep);
#endif

#ifdef CONFIG_SOC_MEC_PM_DEBUG_DEEP_SLEEP_CLK_REQ
#define SOC_VBAT_MEM_PM_OFS CONFIG_SOC_MEC_PM_DEBUG_DS_CLK_REQ_VBAT_MEM_OFS

void soc_debug_sleep_clk_req(void)
{
	mec_hal_pcr_save_clk_req_to_vbatm(SOC_VBAT_MEM_PM_OFS);
}
#endif

/*
 * Deep Sleep
 * Pros:
 * PLL is off resulting in lower power dissipation.
 * Cons:
 * PLL takes up to 3 ms to lock. While PLL is not locked the EC is running on the
 * ring oscillator which can vary from xx to yy MHz.
 *
 * Implementation Notes:
 * We can block entering an ISR immediately after wake by adjusting the Cortex-M's primary
 * mask and base priority registers before entering sleep.
 * NOTE 1: if a wake source(s) interrupt priorities numeric value are >= masked priority value then
 * the NVIC will not wake the Cortex-M.
 *
 * NOTE 2: Certain peripherals will prevent the MEC HW from turning off the PLL during deep sleep.
 * The processor will still halt in WFI but the PLL will remain on if these peripherals are not
 * disabled or idled.
 * JTAG/SWD debug. If an external debugger is periodically polling or has enabled breakpoints or
 * other Cortex-M debug features the Cortex-M will keep its CLK_REQ signal active preventing the
 * PLL from being turn off.
 *
 * UART, I2C, SPI, GP-SPI, eSPI: any peripheral which can transmit data off chip and has data
 * in a TX FIFO will keep CLK_REQ asserted until all data is transmitted.
 *
 * Basic Timers: if running they will assert their CLK_REQ ignoring SLP_EN signal from PCR block.
 *
 * ADC: will keep its CLK_REQ asserted until the current reading is done. If the ADC is configured
 * to convert multiple channels CLK_REQ will de-assert between channel conversions. If the chip
 * enters deep sleep upon wake the ADC will start conversion of the next channel before the PLL
 * has locked resulting in an incorrect reading.
 *
 */

#ifdef CONFIG_SOC_MEC_PM_WAKE_PLL_LOCK_WAIT
#if CONFIG_SOC_MEC_PM_WAKE_PLL_LOCK_WAIT_TIMEOUT_MS != 0
static void mec_soc_wait_pll_lock(void)
{
	struct mec_htmr_regs *htmr_regs = (struct mec_htmr_regs *)MEC_HTMR0_REG_ADDR;
	struct mec_htimer_context hctx;
	uint32_t preload = (uint32_t)CONFIG_SOC_MEC_PM_WAKE_PLL_LOCK_WAIT_TIMEOUT_MS;

	/* hibernation timer counts at 32786 Hz (30.5 us) */
	preload = preload * 30u + (preload >> 1);
	if (preload > MEC_HTIMER_COUNT_MAX) {
		preload = MEC_HTIMER_COUNT_MAX;
	}

	hctx.count = 0;
	hctx.preload = (uint16_t)preload;
	mec_hal_htimer_init(htmr_regs, &hctx, 0);

	while (!mec_hal_pcr_is_pll_locked()) {
		if (mec_hal_htimer_status(&hctx)) {
			break; /* timeout */
		}
	}

	mec_hal_htimer_stop(htmr_regs);
	mec_hal_htimer_status_clear(&hctx);
}
#else
static void mec_soc_wait_pll_lock(void)
{
	while (!mec_hal_pcr_is_pll_locked()) {
		;
	}
}
#endif
#endif

/* NOTE: Zephyr masks interrupts using BASEPRI before calling PM subsystem.
 * Cortex-Mx will not wake on pending interrupts if their priorities are masked
 * using BASEPRI. Cortex-Mx will wake if all priorities masked using PRIMASK.
 * We set PRIMASK=1 via __disable_irq and clear BASEPRI mask.
 */
static void z_power_soc_deep_sleep(void)
{
	PM_DP_ENTER();

#if !defined(CONFIG_SOC_MEC_PM_DEBUG_ON_DEEP_SLEEP)
	mec_hal_ecs_debug_ifc_save_disable();
#endif

	__disable_irq();
	__ISB();
	__DSB();
	__set_BASEPRI(0);
	__ISB();
	__DSB();

#if defined(CONFIG_RTOS_TIMER)
	/* RTOS timer + one basic timer used for kernel timing.
	 * Basic timer's do not obey the PCR SLP_EN signal.
	 * We must manually disable it.
	 */
	soc_ktimer_pm_entry(true);
#endif

	/*
	 * Enable deep sleep mode in the MEC and Cortex-M when WFI executes
	 * if no interrupt is pending. WFI becomes a NOP if any interrupt
	 * is pending.
	 */
	mec_hal_pcr_deep_sleep();
#ifdef CONFIG_SOC_MEC_PM_DEBUG_DEEP_SLEEP_CLK_REQ
	soc_debug_sleep_clk_req();
#endif
	__ISB();	/* ensure previous instructions complete */
	__DSB();	/* ensure all memory transaction are complete */
	__WFI();	/* triggers sleep hardware */
	__NOP();
	__NOP();

	/*
	 * Clear SLEEP_ALL manually since we are not vectoring to an ISR until
	 * PM post ops. This de-asserts peripheral SLP_EN signals.
	 */
	mec_hal_pcr_sleep_disable();
	__DSB();

#if !defined(CONFIG_SOC_MEC_PM_DEBUG_ON_DEEP_SLEEP)
	mec_hal_ecs_debug_ifc_restore();
#endif

#ifdef CONFIG_SOC_MEC_PM_WAKE_PLL_LOCK_WAIT
	/* Wait for PLL to lock or timeout */
	mec_soc_wait_pll_lock();
#endif

#if defined(CONFIG_RTOS_TIMER)
	/* Re-enable basic timer used in combination with RTOS timer */
	soc_ktimer_pm_exit(true);
#endif

	PM_DP_EXIT();
}

/*
 * Light Sleep
 * Pros:
 * Fast wake response:
 * Cons:
 * Higher power dissipation, 48MHz PLL remains on.
 *
 * When the kernel calls this it has masked interrupt by setting NVIC BASEPRI
 * equal to a value equal to the highest Zephyr ISR priority. Only NVIC
 * exceptions will be served.
 */
static void z_power_soc_sleep(void)
{
	__disable_irq();
	__ISB();

	mec_hal_pcr_lite_sleep();

	__set_BASEPRI(0);
	__ISB();
	__DSB();
	__WFI();	/* triggers sleep hardware */
	__NOP();
	__NOP();

	mec_hal_pcr_sleep_disable();
	__DSB();
}

/*
 * Called from pm_system_suspend(int32_t ticks) in subsys/power.c
 * For deep sleep pm_system_suspend has executed all the driver
 * power management call backs.
 */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		z_power_soc_sleep();
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		z_power_soc_deep_sleep();
		break;
	default:
		break;
	}
}

/*
 * Zephyr PM code expects us to enabled interrupt at post op exit. Zephyr used
 * arch_irq_lock() which sets BASEPRI to a non-zero value masking interrupts at
 * >= numerical priority. MCHP z_power_soc_(deep)_sleep sets PRIMASK=1 and BASEPRI=0
 * allowing wake from any enabled interrupt and prevents the CPU from entering any
 * ISR on wake except for faults. We re-enable interrupts by undoing global disable
 * and alling irq_unlock with the same value, 0 zephyr core uses.
 */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	irq_unlock(0);
	__ISB();
	__enable_irq();
	__ISB();
}
