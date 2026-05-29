/*
 * Copyright (c) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/init.h>

#include "sedi_driver_pm.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ish_pm, CONFIG_PM_LOG_LEVEL);

/* low power modes */
#define FW_D0		0
#define FW_D0i0		1
#define FW_D0i1		2
#define FW_D0i2		3
#define	FW_D0i3		4

extern void sedi_pm_enter_power_state(int state);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	if (state >= PM_STATE_SUSPEND_TO_RAM) {
		/* Avoid tedious logs for power states lower than SUSPEND_TO_RAM */
		LOG_DBG(" -> SoC D0i%u, idle=%u ms", substate_id,
				k_ticks_to_ms_floor32(_kernel.idle));
	}

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
		LOG_ERR("Unsupported power state %u", state);
		break;
	}

	if (state >= PM_STATE_SUSPEND_TO_RAM) {
		LOG_DBG("SoC D0i%u ->", substate_id);
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
	__asm__ volatile ("sti" ::: "memory");
}

#if defined(CONFIG_REBOOT) && !defined(CONFIG_REBOOT_RST_CNT)
void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
	sedi_pm_reset();
}
#endif

#define ISH_SOC_IRQ(_node_name, _cell) DT_IRQ_BY_NAME(DT_NODELABEL(soc), _node_name, _cell)

#define DO_PM_IRQ_CONNECT(node) \
	IRQ_CONNECT(ISH_SOC_IRQ(node, irq), ISH_SOC_IRQ(node, priority), \
			sedi_pm_isr, ISH_SOC_IRQ(node, irq), \
			ISH_SOC_IRQ(node, sense))

static int ish_soc_pm_init(void)
{
	sedi_pm_init();

	DO_PM_IRQ_CONNECT(reset_prep);
	DO_PM_IRQ_CONNECT(pcidev);
	DO_PM_IRQ_CONNECT(pmu2ioapic);

	irq_enable(ISH_SOC_IRQ(reset_prep, irq));
	irq_enable(ISH_SOC_IRQ(pcidev, irq));

	return 0;
}

SYS_INIT(ish_soc_pm_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
