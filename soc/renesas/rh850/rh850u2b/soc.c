/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <bsp_api.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

void soc_early_init_hook(void)
{
	uint32_t key = irq_lock();

	/* Configure system clocks. */
	R_BSP_InitClock();

	/* Enable Guard write access for all configured modules */
	R_BSP_GuardEnableAccessAll();

	/* Enable clock supply for all configured modules */
	R_BSP_ModuleStandbyEnableAll();

	/* Initialize interrupt controller (INTC1/INTC2/FEINT/FENMI) */
	bsp_irq_cfg();

	irq_unlock(key);
}

void z_soc_irq_enable(unsigned int irq)
{
	R_BSP_IrqEnable(irq);
}

void z_soc_irq_disable(unsigned int irq)
{
	R_BSP_IrqDisable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	uint8_t coreID;
	int     is_disabled = 0U;
	R_INTC1_PE0_Type *R_INTC1_PE;

    /* INTC1 */
	if (irq < BSP_INTC1_VECTOR_MAX_ENTRIES) {
		/* Get the current coreID */
		coreID     = R_BSP_GetCoreID();
		R_INTC1_PE = (R_INTC1_PE0_Type *)
						(R_INTC1_PE0_BASE
						+ coreID *  ((uint32_t) 0x4000U));
		/* Read bit EIMK of INTC1_PEx EIC[irq] register */
		is_disabled = R_INTC1_PE->EIC_b[irq].EIMK;
	}

    /* INTC2 */
	else if (irq < BSP_INTC_VECTOR_MAX_ENTRIES) {
		/* Read bit EIMK of INTC2 EIC[irq] register */
		is_disabled = R_INTC2->EIC_b[irq].EIMK;
	}

    /* FEINT */
	else if (irq < BSP_INT_VECTOR_MAX_ENTRIES) {
		is_disabled = ((R_FEINC->FEINTMSK) &
						(1 << (irq - BSP_INTC_VECTOR_MAX_ENTRIES)));
	} else {
		is_disabled = 1U;
	}

	return  (0U == is_disabled) ? 1 : 0;
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	R_BSP_IrqCfg(irq, prio, (void *) flags);
}

unsigned int z_soc_irq_get_active(void)
{
	return R_FSP_CurrentIrqGet();
}
