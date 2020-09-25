/*
 * Copyright (c) 2020 Linaro Ltd.
 * Copyright (c) 2020 ATL Electronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Cypress PSoC-6 MCU family devicetree helper macros
 */

#ifndef _CYPRESS_PSOC6_DT_H_
#define _CYPRESS_PSOC6_DT_H_

#include <devicetree.h>

/* Devicetree macros related to interrupt */
#ifdef CONFIG_CPU_CORTEX_M0PLUS
#define CY_PSOC6_DT_NVIC_MUX_INSTALL(n, isr, label)	    \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, interrupt_parent),\
		(CY_PSOC6_NVIC_MUX_INSTALL(n, isr, label)))
#define CY_PSOC6_DT_NVIC_MUX_IRQN(n) DT_IRQN(DT_INST_PHANDLE_BY_IDX(n,\
						interrupt_parent, 0))

/*
 * DT_INST_PROP_BY_IDX should be used instead DT_INST_IRQN.  DT_INST_IRQN
 * return IRQ with level translation since it uses interrupt-parent to update
 * Cortex-M0 NVIC multiplexers. See multi-level-interrupt-handling.
 */
#define CY_PSOC6_DT_NVIC_MUX_MAP(n) Cy_SysInt_SetInterruptSource( \
					DT_IRQN(DT_INST_PHANDLE_BY_IDX(n,\
						interrupt_parent, 0)), \
					DT_INST_PROP_BY_IDX(n, interrupts, 0))
#else
#define CY_PSOC6_DT_NVIC_MUX_INSTALL(n, isr, label) \
	CY_PSOC6_NVIC_MUX_INSTALL(n, isr, label)
#define CY_PSOC6_DT_NVIC_MUX_IRQN(n) DT_INST_IRQN(n)
#define CY_PSOC6_DT_NVIC_MUX_MAP(n)
#endif

#define CY_PSOC6_NVIC_MUX_INSTALL(n, isr, label)	  \
		do {					  \
		IRQ_CONNECT(CY_PSOC6_DT_NVIC_MUX_IRQN(n), \
			DT_INST_IRQ(n, priority),	  \
			isr, DEVICE_GET(label), 0);	  \
		CY_PSOC6_DT_NVIC_MUX_MAP(n);		  \
		irq_enable(CY_PSOC6_DT_NVIC_MUX_IRQN(n)); \
		} while (0)

#endif /* _CYPRESS_PSOC6_SOC_DT_H_ */
