/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>

#include <zephyr.h>
#include <soc_power.h>
#include <nrf_power.h>

extern void nrf_gpiote_clear_port_event(void);
#if defined(CONFIG_SYS_POWER_DEEP_SLEEP)
/* System_OFF is deepest Power state available, On exiting from this
 * state CPU including all peripherals reset
 */
static void _system_off(void)
{
	nrf_power_system_off();
}
#endif

static void _issue_low_power_command(void)
{
	__WFE();
	__SEV();
	__WFE();
}

/* Name: _low_power_mode
 * Parameter : Low Power Task ID
 *
 * Set Notdic SOC specific Low Power Task and invoke
 * _WFE event to put the nordic SOC into Low Power State.
 */
static void _low_power_mode(enum power_states state)
{
	switch (state) {
		/* CONSTANT LATENCY TASK */
	case SYS_POWER_STATE_CPU_LPS:
		nrf_power_task_trigger(NRF_POWER_TASK_CONSTLAT);
		break;
		/* LOW POWER TASK */
	case SYS_POWER_STATE_CPU_LPS_1:
		nrf_power_task_trigger(NRF_POWER_TASK_LOWPWR);
		break;

	default:
		/* Unsupported State */
		SYS_LOG_ERR("Unsupported State\n");
		break;
	}

	/* Issue __WFE*/
	_issue_low_power_command();
	/* Clear the Port Event */
	nrf_gpiote_clear_port_event();
}

/* Invoke Low Power/System Off specific Tasks */
void _sys_soc_set_power_state(enum power_states state)
{
	switch (state) {
	case SYS_POWER_STATE_CPU_LPS:
		_low_power_mode(SYS_POWER_STATE_CPU_LPS);
		break;
	case SYS_POWER_STATE_CPU_LPS_1:
		_low_power_mode(SYS_POWER_STATE_CPU_LPS_1);
		break;
#if defined(CONFIG_SYS_POWER_DEEP_SLEEP)
	case SYS_POWER_STATE_DEEP_SLEEP:
		_system_off();
		break;
#endif
	default:
		/* Unsupported State */
		SYS_LOG_ERR("Unsupported State\n");
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void _sys_soc_power_state_post_ops(enum power_states state)
{
	/* This is placeholder for nrf52 SOC specific task.
	 * Currently there is no nrf52 SOC specific activity to perform.
	 */
	switch (state) {
	case SYS_POWER_STATE_CPU_LPS:
		break;
	case SYS_POWER_STATE_CPU_LPS_1:
		break;
#if defined(CONFIG_SYS_POWER_DEEP_SLEEP)
	case SYS_POWER_STATE_DEEP_SLEEP:
		break;
#endif
	default:
		/* Unsupported State */
		SYS_LOG_ERR("Unsupported State\n");
		break;
	}
}
