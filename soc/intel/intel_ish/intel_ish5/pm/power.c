/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log_core.h>
#include <stdio.h>
#include <zephyr/drivers/interrupt_controller/ioapic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pm_service, CONFIG_PM_LOG_LEVEL);

/* low power modes */
#define FW_D0		0
#define FW_D0i0		1
#define FW_D0i1		2
#define FW_D0i2		3
#define	FW_D0i3		4

extern void sedi_pm_enter_power_state(int state);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	LOG_DBG(" -> SoC D0i%u, idle=%u ms", substate_id,
				k_ticks_to_ms_floor32(_kernel.idle));

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		sedi_pm_enter_power_state(FW_D0i0);
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		sedi_pm_enter_power_state(FW_D0i1);
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		sedi_pm_enter_power_state(FW_D0i2);
		break;
	case PM_STATE_SUSPEND_TO_DISK:
		sedi_pm_enter_power_state(FW_D0i3);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	LOG_DBG("SoC D0i%u ->", substate_id);
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
	__asm__ volatile ("sti" ::: "memory");
}

#if defined(CONFIG_REBOOT) && !defined(CONFIG_REBOOT_RST_CNT)
extern void sedi_pm_reset(void);

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
	sedi_pm_reset();
}
#endif
