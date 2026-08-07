/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/logging/log.h>

#include <cy_syspm.h>
#include <cy_sysclk.h>
#include <cy_syslib.h>
#include <cy_wdt.h>

#include <soc.h>

LOG_MODULE_REGISTER(soc_power, CONFIG_SOC_LOG_LEVEL);

#if defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS)

#define WDT_IRQ_NUM  DT_IRQN(DT_NODELABEL(watchdog0))
#define WDT_IRQ_PRIO DT_IRQ(DT_NODELABEL(watchdog0), priority)
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_ilo))
#define ILO_SRC_FREQ DT_PROP(DT_NODELABEL(clk_ilo), clock_frequency)
#else
#define ILO_SRC_FREQ 40000U
#endif

#define DS_EXIT_LATENCY_US    DT_PROP_OR(DT_NODELABEL(deepsleep), exit_latency_us, 0)
#define DEEPSLEEP_LATENCY_NUM ((uint64_t)DS_EXIT_LATENCY_US * CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define DEEPSLEEP_EXIT_LATENCY_TICKS                                                               \
	(IS_ENABLED(CONFIG_PM_PREWAKEUP_CONV_MODE_NEAR)                                            \
		 ? (DEEPSLEEP_LATENCY_NUM + USEC_PER_SEC / 2U) / USEC_PER_SEC                      \
	 : IS_ENABLED(CONFIG_PM_PREWAKEUP_CONV_MODE_CEIL)                                          \
		 ? (DEEPSLEEP_LATENCY_NUM + USEC_PER_SEC - 1U) / USEC_PER_SEC                      \
		 : DEEPSLEEP_LATENCY_NUM / USEC_PER_SEC)

BUILD_ASSERT(DEEPSLEEP_EXIT_LATENCY_TICKS >= 1,
	     "deepsleep exit-latency-us converts to 0 system ticks under the selected "
	     "CONFIG_PM_PREWAKEUP_CONV_MODE; the LPM companion WDT would never be armed "
	     "and Deep Sleep would have no wakeup source. Increase exit-latency-us.");

/*
 * The PDL's own safe minimum is "3 + 1 for sure" (4 cycles). We keep a little
 * extra headroom and use 8.
 */
#define MIN_WDT_CYCLES 8U

#define MAX_WDT_WINDOW_CYCLES (UINT16_MAX - 256U)

#define ILO_MEASURE_POLL_US 100U
#define ILO_MEASURE_RETRIES 50U

static uint32_t ilo_freq_hz = ILO_SRC_FREQ;
static uint32_t wdt_start_count;
static uint64_t wdt_target_cycles;
static uint64_t wdt_elapsed_cycles;

static void wdt_lpm_calibrate_ilo(void)
{
	cy_en_sysclk_status_t status;
	uint32_t cycles_per_second = 0U;
	uint32_t retries = ILO_MEASURE_RETRIES;

	Cy_SysClk_IloStartMeasurement();

	do {
		status = Cy_SysClk_IloCompensate(USEC_PER_SEC, &cycles_per_second);
		if (status != CY_SYSCLK_STARTED) {
			break;
		}
		Cy_SysLib_DelayUs(ILO_MEASURE_POLL_US);
	} while (--retries > 0U);

	Cy_SysClk_IloStopMeasurement();

	if (status == CY_SYSCLK_SUCCESS && cycles_per_second != 0U) {
		ilo_freq_hz = cycles_per_second;
	} else {
		LOG_WRN("ILO measurement failed (%d), using nominal %u Hz", status, ilo_freq_hz);
	}
}

static void wdt_lpm_isr(const void *arg)
{
	ARG_UNUSED(arg);
	Cy_WDT_ClearInterrupt();
}

static void wdt_lpm_arm_window(uint32_t from_count, uint64_t remaining_cycles)
{
	uint32_t chunk = (uint32_t)CLAMP(remaining_cycles, (uint64_t)MIN_WDT_CYCLES,
					 (uint64_t)MAX_WDT_WINDOW_CYCLES);

	Cy_WDT_SetMatch((from_count + chunk) & UINT16_MAX);
}

/*
 * When SysTick is the primary system timer, it stops during Deep Sleep
 * because High-Frequency (HF) clocks are gated.
 * Watchdog Timer (WDT) on the always-on Internal Low-Speed Oscillator (ILO, ~40 kHz)
 * is used as the wakeup source and implements the z_sys_clock_lpm_enter
 * and z_sys_clock_lpm_exit hooks so the Zephyr system timer can reconcile
 * elapsed time duration after wakeup.
 */
void z_sys_clock_lpm_enter(uint64_t max_lpm_time_us)
{
	wdt_target_cycles = (max_lpm_time_us * (uint64_t)ilo_freq_hz) / USEC_PER_SEC;
	if (wdt_target_cycles < MIN_WDT_CYCLES) {
		wdt_target_cycles = MIN_WDT_CYCLES;
	}
	wdt_elapsed_cycles = 0U;

	Cy_WDT_ClearInterrupt();
	Cy_WDT_MaskInterrupt();

	wdt_start_count = Cy_WDT_GetCount();
	wdt_lpm_arm_window(wdt_start_count, wdt_target_cycles);
	Cy_WDT_SetIgnoreBits(0U);
	Cy_WDT_UnmaskInterrupt();

	/*
	 * WDT counter is free-running and cannot be disabled (TRM 13.3.1), so it
	 * does not need Cy_WDT_Enable() to start.
	 */

	NVIC_ClearPendingIRQ(WDT_IRQ_NUM);
	irq_enable(WDT_IRQ_NUM);
}

static bool wdt_lpm_continue(void)
{
	uint32_t pending = NVIC->ISPR[0U] & NVIC->ISER[0U];
	uint32_t now_value;

	if (pending != BIT(WDT_IRQ_NUM)) {
		return false;
	}

	now_value = Cy_WDT_GetCount();
	wdt_elapsed_cycles += (now_value - wdt_start_count) & UINT16_MAX;
	wdt_start_count = now_value;

	if (wdt_elapsed_cycles >= wdt_target_cycles) {
		return false;
	}

	Cy_WDT_ClearInterrupt();
	NVIC_ClearPendingIRQ(WDT_IRQ_NUM);
	wdt_lpm_arm_window(now_value, wdt_target_cycles - wdt_elapsed_cycles);

	return true;
}

uint64_t z_sys_clock_lpm_exit(void)
{
	uint32_t end_count;
	uint64_t total_cycles;

	end_count = Cy_WDT_GetCount();

	Cy_WDT_MaskInterrupt();
	Cy_WDT_ClearInterrupt();

	irq_disable(WDT_IRQ_NUM);
	NVIC_ClearPendingIRQ(WDT_IRQ_NUM);

	total_cycles = wdt_elapsed_cycles + ((end_count - wdt_start_count) & UINT16_MAX);

	return (total_cycles * USEC_PER_SEC) / ilo_freq_hz;
}

#else /* CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS */

static bool wdt_lpm_continue(void)
{
	return false;
}
#endif

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	__disable_irq();

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		Cy_SysPm_CpuEnterSleep();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		do {
			if (Cy_SysPm_CpuEnterDeepSleep() != CY_SYSPM_SUCCESS) {
				Cy_SysPm_CpuEnterSleepNoCallbacks();
				break;
			}
		} while (wdt_lpm_continue());
		SCB_SCR &= (uint32_t)~SCB_SCR_SLEEPDEEP_Msk;
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	__enable_irq();
}

int pm_init(void)
{
#if defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS)
	wdt_lpm_calibrate_ilo();

	IRQ_CONNECT(WDT_IRQ_NUM, WDT_IRQ_PRIO, wdt_lpm_isr, NULL, 0);
#else
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
#endif

	return 0;
}
