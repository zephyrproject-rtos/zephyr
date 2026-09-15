/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/interrupt_controller/intc_root.h>
#include <zephyr/drivers/interrupt_controller/riscv_clic.h>
#include <hal/nrf_vpr_clic.h>

void riscv_clic_irq_enable(uint32_t irq)
{
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, irq, true);
}

void riscv_clic_irq_disable(uint32_t irq)
{
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, irq, false);
}

int riscv_clic_irq_is_enabled(uint32_t irq)
{
	return nrf_vpr_clic_int_enable_check(NRF_VPRCLIC, irq);
}

void riscv_clic_irq_priority_set(uint32_t irq, uint32_t pri, uint32_t flags)
{
	nrf_vpr_clic_int_priority_set(NRF_VPRCLIC, irq, NRF_VPR_CLIC_INT_TO_PRIO(pri));
}

void riscv_clic_irq_set_pending(uint32_t irq)
{
	nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, irq);
}

#if defined(CONFIG_RISCV_HAS_CLIC)
/* The CLIC is the root interrupt controller: alias its functions to the API names */
FUNC_ALIAS(riscv_clic_irq_enable, intc_root_enable, void);
FUNC_ALIAS(riscv_clic_irq_disable, intc_root_disable, void);
FUNC_ALIAS(riscv_clic_irq_is_enabled, intc_root_is_enabled, int);
FUNC_ALIAS(riscv_clic_irq_priority_set, intc_root_priority_set, void);
#endif /* CONFIG_RISCV_HAS_CLIC */
