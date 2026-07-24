/*
 * Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/sw_isr_table.h>
#include <cy_device.h>
#include <cy_sysint.h>

#include "sw_isr_common.h"

#define DT_DRV_COMPAT infineon_cat1_intc

static void cat1_intc_mux_isr(void)
{
	cy_en_intr_t sys_int;
	unsigned int zephyr_irq;
	uint32_t table_idx;
	uint32_t active_cpu_int = __get_IPSR() - 16;

	sys_int = Cy_SysInt_GetInterruptActive((IRQn_Type)active_cpu_int);
	if (sys_int == CY_CPUSS_NOT_CONNECTED_IRQN) {
		return;
	}

	zephyr_irq = irq_to_level_2((unsigned int)sys_int) | active_cpu_int;
	table_idx = z_get_sw_isr_table_idx(zephyr_irq);

	_sw_isr_table[table_idx].isr(_sw_isr_table[table_idx].arg);
}

#define _CAT1_REGISTER_DISPATCHER(node_id)                                                         \
	IRQ_CONNECT(DT_IRQN(node_id), DT_IRQ(node_id, priority), cat1_intc_mux_isr, NULL, 0)

#define CAT1_REGISTER_DISPATCHERS(parent_id)                                                       \
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(parent_id, _CAT1_REGISTER_DISPATCHER, (;))

void z_soc_irq_enable(unsigned int irq)
{
	unsigned int parent;
	unsigned int sys_int;

	if (irq_get_level(irq) == 1) {
		arm_irq_enable(irq);
		return;
	}

	parent = irq_parent_level_2(irq);
	sys_int = irq_from_level_2(irq);

	Cy_SysInt_SetInterruptSource(parent, sys_int);
	arm_irq_enable(parent);
}

void z_soc_irq_disable(unsigned int irq)
{
	if (irq_get_level(irq) == 1) {
		arm_irq_disable(irq);
		return;
	}

	Cy_SysInt_DisableSystemInt(irq_from_level_2(irq));
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	unsigned int sys_int;

	if (irq_get_level(irq) == 1) {
		return arm_irq_is_enabled(irq);
	}

	sys_int = irq_from_level_2(irq);

#if (CY_CPU_CORTEX_M0P)
	return (CPUSS_CM0_SYSTEM_INT_CTL[sys_int] &
		CPUSS_CM0_SYSTEM_INT_CTL_CPU_INT_VALID_Msk) != 0;
#else
	if (CY_IS_CM7_CORE_0 != 0) {
		return (CPUSS_CM7_0_SYSTEM_INT_CTL[sys_int] &
			CPUSS_CM7_0_SYSTEM_INT_CTL_CPU_INT_VALID_Msk) != 0;
	} else {
		return (CPUSS_CM7_1_SYSTEM_INT_CTL[sys_int] &
			CPUSS_CM7_1_SYSTEM_INT_CTL_CPU_INT_VALID_Msk) != 0;
	}
#endif
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	/* L2 system interrupts share their parent NvicMux's priority,
	 * so only L1 priorities are programmed.
	 */
	if (irq_get_level(irq) == 1) {
		arm_irq_priority_set(irq, prio, flags);
	}
}

static int cat1_intc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	CAT1_REGISTER_DISPATCHERS(DT_NODELABEL(cpuss_intc));

	return 0;
}

DEVICE_DT_INST_DEFINE(0, cat1_intc_init, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,
		      NULL);

#define CAT1_MUX_IRQ_PARENT_ENTRY(node_id)                                                         \
	IRQ_PARENT_ENTRY_DEFINE(CONCAT(cat1_aggregator_, DT_NODE_CHILD_IDX(node_id)), NULL,        \
				DT_IRQN(node_id), INTC_CHILD_ISR_TBL_OFFSET(node_id),              \
				DT_INTC_GET_AGGREGATOR_LEVEL(node_id));

DT_INST_FOREACH_CHILD_STATUS_OKAY(0, CAT1_MUX_IRQ_PARENT_ENTRY);
