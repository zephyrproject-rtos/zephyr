/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <string.h>
#include <cy_scb_uart.h>
#include <cy_gpio.h>
#include <cy_syslib.h>
#include <cy_sysint.h>

#define _IFX_CAT1_SCB_BLOCK_ID_INVALID (0xFF)

#if (CY_CPU_CORTEX_M0P != 0)
#define IFX_CAT1_ISR_PRIORITY_DEFAULT (3)
#else
#define IFX_CAT1_ISR_PRIORITY_DEFAULT (7)
#endif /* (CY_CPU_CORTEX_M0P != 0) */

#define IFX_CAT1_IRQ_MUXING (CY_CPU_CORTEX_M0P || CPUSS_SYSTEM_IRQ_PRESENT)

#define IFX_CAT1_IRQ_LEGACY_M0 (CY_CPU_CORTEX_M0P && (1u == CY_IP_M4CPUSS_VERSION))

#if (IFX_CAT1_IRQ_MUXING)
static inline void _ifx_cat1_irq_set_priority(cy_en_intr_t system_irq, uint8_t intr_priority)
#else
static inline void _ifx_cat1_irq_set_priority(IRQn_Type system_irq, uint8_t intr_priority)
#endif
{
#if IFX_CAT1_IRQ_MUXING
#if IFX_CAT1_IRQ_LEGACY_M0
	IRQn_Type irqn = _ifx_cat1_irq_find_cm0(system_irq);
	uint8_t priority_to_set = intr_priority;
#else /* CM0+ on CPUSSv2, or CM4/CM7 on CPUSSv2 with SYSTEM_IRQ_PRESENT */
	IRQn_Type irqn = Cy_SysInt_GetNvicConnection(system_irq);

	_ifx_cat1_system_irq_store_priority(system_irq, intr_priority);
	uint8_t priority_to_set = _ifx_cat1_system_irq_lowest_priority(irqn);
#endif
#else
	IRQn_Type irqn = system_irq;
	uint8_t priority_to_set = intr_priority;
#endif
	NVIC_SetPriority(irqn, priority_to_set);
}

#if (IFX_CAT1_IRQ_MUXING)
cy_rslt_t _ifx_cat1_irq_register(cy_en_intr_t system_intr, uint8_t intr_priority,
				 cy_israddress irq_handler);
#else
cy_rslt_t _ifx_cat1_irq_register(IRQn_Type system_intr, uint8_t intr_priority,
				 cy_israddress irq_handler);
#endif

#if (IFX_CAT1_IRQ_MUXING)
void _ifx_cat1_irq_enable(cy_en_intr_t system_irq);
#else
void _ifx_cat1_irq_enable(IRQn_Type system_irq);
#endif

/* Offset for implementation-defined ISR type numbers (IRQ0 = 16) */
#define _IFX_CAT1_UTILS_IRQN_OFFSET (16U)

/* Macro to get the IRQn of the current ISR */
#define _IFX_CAT1_UTILS_GET_CURRENT_IRQN() ((IRQn_Type)(__get_IPSR() - _IFX_CAT1_UTILS_IRQN_OFFSET))

#if (IFX_CAT1_IRQ_MUXING)
static inline cy_en_intr_t _ifx_cat1_irq_get_active(void)
#else
static inline IRQn_Type _ifx_cat1_irq_get_active(void)
#endif
{
	IRQn_Type irqn = _IFX_CAT1_UTILS_GET_CURRENT_IRQN();
#if IFX_CAT1_IRQ_MUXING
#if IFX_CAT1_IRQ_LEGACY_M0
	/* No pre-built functionality for this. Need to see what CPU interrupt is active, then
	 * indirect through the NVIC mux to figure out what system IRQ it is mapped to.
	 */
	return Cy_SysInt_GetInterruptSource(irqn);
#else /* CM0+ on CPUSSv2, or CM4/CM7 on CPUSSv2 with SYSTEM_IRQ_PRESENT */
	return Cy_SysInt_GetInterruptActive(irqn);
#endif
#else
	return irqn;
#endif
}
