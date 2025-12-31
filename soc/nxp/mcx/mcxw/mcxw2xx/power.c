/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/nxp_pint.h>
#include <fsl_power.h>
#include <zephyr/logging/log.h>

#if CONFIG_PM_POLICY_CUSTOM
#include <zephyr/pm/policy.h>
#endif /* CONFIG_PM_POLICY_CUSTOM */

#ifdef CONFIG_BT
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

#if defined(CONFIG_BT) && !defined(CONFIG_PM_POLICY_CUSTOM)
#error Select CONFIG_PM_POLICY_CUSTOM when CONFIG_BT is selected
#endif

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

#ifdef CONFIG_BT
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

#ifdef CONFIG_BT
	/* BT needs 32kHz clock. */
	*exclude_from_pd |= (kLOWPOWERCFG_XTAL32K | kLOWPOWERCFG_BLE_WUP);
#endif

#ifdef CONFIG_MCUX_OS_TIMER
	/* OS_TIMER uses 32K clock as clock source, keep it running. */
	*exclude_from_pd |= kLOWPOWERCFG_XTAL32K;
	*wakeup_sources  |= kWAKEUP_OS_EVENT;
#endif

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

static bool pm_connectivity_lp_prepare(enum pm_state state)
{
	bool ret = true;
#ifdef CONFIG_BT
	uint32_t ll_remaining_time;
	blec_result_t ll_status;
	int32_t timeout_expiry;
	uint32_t exit_latency_ticks;
	const struct pm_state_info *state_info;

	do {
		if (!bt_is_ready()) {
			break;
		}

		state_info = pm_state_get(0, state, 0);
		ll_status = BLEController_GetRemainingTimeForNextEventUnsafe(&ll_remaining_time);
		timeout_expiry = z_get_next_timeout_expiry();
		exit_latency_ticks = k_us_to_ticks_ceil32(state_info->exit_latency_us);

		/* Is link layer busy? */
		if ((ll_status != kBLEC_Success) || (ll_remaining_time == 0)) {
			ret = false;
			break;
		}
		/* Enough time to go to this state? */
		if (ll_remaining_time <
		    state_info->min_residency_us + state_info->exit_latency_us) {
			ret = false;
			break;
		}

		/* Convert to ticks */
		ll_remaining_time = k_us_to_ticks_floor32(ll_remaining_time) - exit_latency_ticks;

		/* Any close activity? */
		if (ll_remaining_time < (uint32_t)timeout_expiry - exit_latency_ticks) {
			if ((ll_remaining_time <= 1)) {
				ret = false;
				break;
			}
			sys_clock_set_timeout(ll_remaining_time, true);
		}
	} while (false);
#endif
	return ret;
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
		if (pm_connectivity_lp_prepare(PM_STATE_SUSPEND_TO_IDLE)) {
			status = POWER_EnterDeepSleep(exclude_from_pd, wakeup_sources);
			if (status != kStatus_Success) {
				LOG_ERR("Failed to enter deep sleep mode: %d", status);
			}
		}
		break;

	case PM_STATE_STANDBY:
		pm_get_lowpower_resource_list(&exclude_from_pd, &wakeup_sources, true);
		if (pm_connectivity_lp_prepare(PM_STATE_STANDBY)) {
			status = POWER_EnterPowerDown(exclude_from_pd, wakeup_sources, 1);
			if (status != kStatus_Success) {
				LOG_ERR("Failed to enter power down mode: %d", status);
			}
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

static int nxp_mcxw2xx_power_init(void)
{
#if WAKEUP_PIN_ENABLE
	init_wakeup_gpio_pins();
#endif /* WAKEUP_PIN_ENABLE */

	return 0;
}

void nxp_mcxw2xx_power_early_init(void)
{
	POWER_Init();
}

SYS_INIT(nxp_mcxw2xx_power_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
