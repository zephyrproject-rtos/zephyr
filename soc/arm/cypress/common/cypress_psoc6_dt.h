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

/*
 * Devicetree macros related to interrupt
 *
 * The main "API" macro is CY_PSOC6_IRQ_CONFIG. It is an internal definition
 * used to configure the PSoC-6 interrupts in a generic way. This is necessary
 * because Cortex-M0+ can handle a limited number of interrupts and have
 * multiplexers in front of any NVIC interrupt line.
 *
 * The CY_PSOC6_IRQ_CONFIG expands from CY_PSOC6_DT_INST_NVIC_INSTALL, the
 * public API used by drivers. See below code fragment:
 *
 * static void <driver>_psoc6_isr(const struct device *dev)
 * {
 *     ...
 * }
 *
 * #define <DRIVER>_PSOC6_INIT(n)					\
 *    ...								\
 * static void <driver>_psoc6_irq_config(const struct device *port)	\
 * {									\
 *      CY_PSOC6_DT_INST_NVIC_INSTALL(n,				\
 *                                    <driver>_psoc6_isr);		\
 *      };								\
 * };
 *
 * where:
 *   n   - driver instance number
 *   isr - isr function to be called
 *
 * Cortex-M4 simple pass the parameter and constructs an usual NVIC
 * configuration code.
 *
 * The Cortex-M0+ must get from interrupt parent the interrupt line and
 * configure the interrupt channel to connect PSoC-6 peripheral interrupt to
 * Cortex-M0+ NVIC. The multiplexer is configured by CY_PSOC6_DT_NVIC_MUX_MAP
 * using the interrupt value from the interrupt parent.
 *
 * see cypress,psoc6-int-mux.yaml for devicetree documentation.
 */
#ifdef CONFIG_CPU_CORTEX_M0PLUS
/* Cortex-M0+
 * - install config only when exists an interrupt_parent property
 * - get peripheral irq using PROP_BY_INDEX, to avoid translation from
 *   interrupt-parent node property.
 * - configure interrupt channel using the channel number register value from
 *   interrupt-parent node.
 */
#define CY_PSOC6_DT_INST_NVIC_INSTALL(n, isr)	              \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, interrupt_parent),\
		(CY_PSOC6_IRQ_CONFIG(n, isr)))
#define CY_PSOC6_NVIC_MUX_IRQN(n) DT_IRQN(DT_INST_PHANDLE_BY_IDX(n,\
						interrupt_parent, 0))
/*
 * DT_INST_PROP_BY_IDX should be used get interrupt and configure, instead
 * DT_INST_IRQN. The DT_INST_IRQN return IRQ number with level translation,
 * since it uses interrupt-parent, and the value at Cortex-M0 NVIC multiplexers
 * will be wrong.
 *
 * See multi-level-interrupt-handling.
 */
#define CY_PSOC6_NVIC_MUX_MAP(n) Cy_SysInt_SetInterruptSource( \
					DT_IRQN(DT_INST_PHANDLE_BY_IDX(n,\
						interrupt_parent, 0)), \
					DT_INST_PROP_BY_IDX(n, interrupts, 0))
#else
/* Cortex-M4
 * - bypass config
 * - uses irq directly from peripheral devicetree definition
 * - no map/translations
 */
#define CY_PSOC6_DT_INST_NVIC_INSTALL(n, isr) CY_PSOC6_IRQ_CONFIG(n, isr)
#define CY_PSOC6_NVIC_MUX_IRQN(n) DT_INST_IRQN(n)
#define CY_PSOC6_NVIC_MUX_MAP(n)
#endif

#define CY_PSOC6_IRQ_CONFIG(n, isr)			\
	do {						\
		IRQ_CONNECT(CY_PSOC6_NVIC_MUX_IRQN(n),	\
			    DT_INST_IRQ(n, priority),	\
			    isr, DEVICE_DT_INST_GET(n), 0);\
		CY_PSOC6_NVIC_MUX_MAP(n);		\
		irq_enable(CY_PSOC6_NVIC_MUX_IRQN(n));	\
	} while (0)

#endif /* _CYPRESS_PSOC6_SOC_DT_H_ */
