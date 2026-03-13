/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/nxp_pint.h>
#include <zephyr/drivers/timer/system_timer_lpm.h>
#include <fsl_power.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS)
#include "fsl_ostimer.h"
#endif

#if CONFIG_PM_POLICY_CUSTOM
#include <zephyr/pm/policy.h>
#endif /* CONFIG_PM_POLICY_CUSTOM */

#if defined(CONFIG_BT)
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <ble_controller.h>
#include <timeout_q.h>
#endif /* CONFIG_BT */

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Wakeup pin. */
#if CONFIG_GPIO && DT_NODE_EXISTS(DT_NODELABEL(btn_wk))
#define WAKEUP_PIN_ENABLE 1
#else
#define WAKEUP_PIN_ENABLE 0
#endif

#if WAKEUP_PIN_ENABLE
#define WAKEUP_BUTTON_NODE DT_NODELABEL(btn_wk)
static const struct gpio_dt_spec wakeup_pin_dt = GPIO_DT_SPEC_GET(WAKEUP_BUTTON_NODE, gpios);
#endif /* WAKEUP_PIN_ENABLE */

#if !defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS)
#error "System timer LPM hooks are needed. OS timer will replace SYSTICK during low power."
#else
/* OS Timer configuration for low power wakeup */
#define OSTIMER_NODE DT_NODELABEL(os_timer)

#if DT_NODE_HAS_STATUS(OSTIMER_NODE, okay)
#define OSTIMER_IRQ_NUM  DT_IRQN(OSTIMER_NODE)
#define OSTIMER_IRQ_PRIO DT_IRQ(OSTIMER_NODE, priority)
static OSTIMER_Type *lptim_base = (OSTIMER_Type *)DT_REG_ADDR(OSTIMER_NODE);
#else
#error "OSTIMER is required for low power operation"
#endif

/* Store OS timer count at low power entry for sleep duration calculation */
static uint64_t lptim_count;

/**
 * @brief System timer low-power companion hook called on entry
 *
 * Configures the OS timer to wake up the system after the specified time.
 * Coordinates with BLE link layer to ensure wakeup happens before next BLE event.
 */
void z_sys_clock_lpm_enter(uint64_t max_lpm_time_us)
{
	uint32_t lptim_freq;

#if defined(CONFIG_BT)
	blec_result_t ll_status;
	uint32_t ll_remaining_time;
	uint32_t next_pm_state_exit_latency_us;

	if (bt_is_ready()) {
		ll_status = BLEController_GetRemainingTimeForNextEventUnsafe(&ll_remaining_time);

		/* Is link layer busy? */
		if ((ll_status == kBLEC_Success) && (ll_remaining_time > 0U)) {
			next_pm_state_exit_latency_us =
				pm_state_next_get(_current_cpu->id)->exit_latency_us;
			/* Account for exit latency to ensure system wakes before BLE event */
			if (ll_remaining_time > next_pm_state_exit_latency_us) {
				ll_remaining_time -= next_pm_state_exit_latency_us;
			} else {
				/* This should never happen, as we already validated timing
				 * during pm_policy_next_state execution.
				 */
				assert(0);
			}

			if (ll_remaining_time < max_lpm_time_us) {
				max_lpm_time_us = ll_remaining_time;
			}
		}
	}
#endif
	/* Get OS timer frequency */
	lptim_freq = CLOCK_GetOSTimerClkFreq();
	assert(lptim_freq != 0U);

	/* Save current timer value for sleep duration calculation on exit */
	lptim_count = OSTIMER_GetCurrentTimerValue(lptim_base);
	OSTIMER_ClearStatusFlags(lptim_base, kOSTIMER_MatchInterruptFlag);

	/* Enable OS timer interrupt */
	irq_enable(OSTIMER_IRQ_NUM);

	/* Set the match value to wake up the system*/
	OSTIMER_SetMatchValue(lptim_base, lptim_count + USEC_TO_COUNT(max_lpm_time_us, lptim_freq),
			      NULL);
}

/**
 * @brief System timer low-power companion hook called on exit
 *
 * This function is called after the primary system timer resumes.
 * Calculates the sleep duration.
 */
uint64_t z_sys_clock_lpm_exit(void)
{
	uint64_t lptim_sleep_cnt;
	uint32_t lptim_freq;

	/* Check if woken up by OS timer match interrupt */
	if ((OSTIMER_GetStatusFlags(lptim_base) & (uint32_t)kOSTIMER_MatchInterruptFlag) == 0U) {
		/* Not woken up by the low power timer (woken by other interrupt source),
		 * disable further OS timer interrupts.
		 */
		irq_disable(OSTIMER_IRQ_NUM);
	}

	/* Calculate actual sleep duration */
	lptim_sleep_cnt = OSTIMER_GetCurrentTimerValue(lptim_base) - lptim_count;
	lptim_freq = CLOCK_GetOSTimerClkFreq();
	assert(lptim_freq != 0U);

	/* Return sleep duration in microseconds */
	return COUNT_TO_USEC(lptim_sleep_cnt, lptim_freq);
}
#endif /* CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS */

#if CONFIG_PM_POLICY_CUSTOM
__weak const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	uint8_t num_cpu_states;
	const struct pm_state_info *cpu_states;
	const struct pm_state_info *out_state = NULL;

#ifdef CONFIG_PM_NEED_ALL_DEVICES_IDLE
	if (pm_device_is_any_busy()) {
		return NULL;
	}
#endif

#if defined(CONFIG_BT)
	if (bt_is_ready()) {
		uint32_t ll_remaining_time;
		blec_result_t stat;

		stat = BLEController_GetRemainingTimeForNextEventUnsafe(&ll_remaining_time);
		/* Is link layer busy? */
		if ((stat != kBLEC_Success) || (ll_remaining_time == 0)) {
			return NULL;
		}
		/* Any future activity? */
		if (ll_remaining_time < 0xffffffff) {
			ticks = MIN(ticks, k_us_to_ticks_floor32(ll_remaining_time));
		}
	}
#endif /* CONFIG_BT */

	num_cpu_states = pm_state_cpu_get_all(cpu, &cpu_states);

	for (uint32_t i = 0; i < num_cpu_states; i++) {
		const struct pm_state_info *state = &cpu_states[i];
		uint32_t min_residency_ticks = 0;
		uint32_t min_residency_us = state->min_residency_us + state->exit_latency_us;

		/* If the input is zero, avoid 64-bit conversion from microseconds to ticks. */
		if (min_residency_us > 0) {
			min_residency_ticks = k_us_to_ticks_ceil32(min_residency_us);
		}

		if (ticks < min_residency_ticks) {
			/* If current state has higher residency then use the previous state; */
			break;
		}

		/* check if state is available. */
		if (!pm_policy_state_is_available(state->state, state->substate_id)) {
			continue;
		}

		out_state = state;
	}

	return out_state;
}
#endif /* CONFIG_PM_POLICY_CUSTOM */

static void pm_get_lowpower_resource_list(uint32_t *exclude_from_pd,
					  uint64_t *wakeup_sources,
					  bool is_standby)
{
	*exclude_from_pd = kLOWPOWERCFG_DCDC_BYPASS;
	*wakeup_sources  = 0;

#if defined(CONFIG_BT)
	/* BT needs 32kHz clock. */
	*exclude_from_pd |= (kLOWPOWERCFG_XTAL32K | kLOWPOWERCFG_BLE_WUP);
#endif
	/* OS_TIMER uses 32K clock as clock source, keep it running. */
	*exclude_from_pd |= kLOWPOWERCFG_XTAL32K;
	*wakeup_sources  |= kWAKEUP_OS_EVENT;

#if WAKEUP_PIN_ENABLE
	static const wakeup_irq_t pint_wakeup_sources[] = {
		kWAKEUP_PIN_INT0,
		kWAKEUP_PIN_INT1,
		kWAKEUP_PIN_INT2,
		kWAKEUP_PIN_INT3,
		kWAKEUP_PIN_INT4,
		kWAKEUP_PIN_INT5,
		kWAKEUP_PIN_INT6,
		kWAKEUP_PIN_INT7
	};

	/* PINT doesn't work in standby mode. */
	if (!is_standby) {
		int slot = nxp_pint_pin_get_slot_index(wakeup_pin_dt.pin);

		if (slot >= 0 && slot < ARRAY_SIZE(pint_wakeup_sources)) {
			*wakeup_sources |= pint_wakeup_sources[slot];
		}
	}
#endif
}

__weak void pm_state_set(enum pm_state state, uint8_t id)
{
	ARG_UNUSED(id);
	uint32_t exclude_from_pd;
	uint64_t wakeup_sources;
	status_t status;

	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		POWER_EnterSleep();
		break;

	case PM_STATE_SUSPEND_TO_IDLE:
		pm_get_lowpower_resource_list(&exclude_from_pd, &wakeup_sources, false);
		status = POWER_EnterDeepSleep(exclude_from_pd, wakeup_sources);
		if (status != kStatus_Success) {
			LOG_ERR("Failed to enter deep sleep mode: %d", status);
		}
		break;

	case PM_STATE_STANDBY:
		pm_get_lowpower_resource_list(&exclude_from_pd, &wakeup_sources, true);
		status = POWER_EnterPowerDown(exclude_from_pd, wakeup_sources, 1);
		if (status != kStatus_Success) {
			LOG_ERR("Failed to enter power down mode: %d", status);
		}
		break;

	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(id);

	/* Clear PRIMASK */
	__enable_irq();
}

#if WAKEUP_PIN_ENABLE
static void init_wakeup_gpio_pins(void)
{
	int ret;

	if (!device_is_ready(wakeup_pin_dt.port)) {
		LOG_ERR("Wake-up GPIO device not ready");
		return;
	}

	ret = gpio_pin_configure_dt(&wakeup_pin_dt, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure wakeup pin\n", ret);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&wakeup_pin_dt, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure wakeup pin interrupt\n", ret);
		return;
	}
}
#endif

#if defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS)
static void nxp_ostimer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Disable match interrupt after wakeup */
	OSTIMER_DisableMatchInterrupt(lptim_base);
}
#endif

static int nxp_mcxw2xx_power_init(void)
{
#if WAKEUP_PIN_ENABLE
	init_wakeup_gpio_pins();
#endif /* WAKEUP_PIN_ENABLE */

	return 0;
}

void nxp_mcxw2xx_power_early_init(void)
{
#if defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS)
	/* Initialize the OS timer */
	OSTIMER_Init(lptim_base);

	/* Configure OS timer interrupt for low power wakeup */
	IRQ_CONNECT(OSTIMER_IRQ_NUM, OSTIMER_IRQ_PRIO, nxp_ostimer_isr, NULL, 0);
#endif
	POWER_Init();
}

SYS_INIT(nxp_mcxw2xx_power_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
