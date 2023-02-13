/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV32_OPENISA_RV32M1_SOC_H_
#define SOC_RISCV32_OPENISA_RV32M1_SOC_H_

#ifndef _ASMLANGUAGE

#include "fsl_device_registers.h"
#include <zephyr/types.h>

/*
 * Helpers related to interrupt handling. This SoC has two levels of
 * interrupts.
 *
 * Level 1 interrupts go straight to the SoC. Level 2 interrupts must
 * go through one of the 8 channels in the the INTMUX
 * peripheral. There are 32 level 1 interrupts, including 8 INTMUX
 * interrupts. Each INTMUX interrupt can mux at most
 * CONFIG_MAX_IRQ_PER_AGGREGATOR (which happens to be 32) interrupts
 * to its level 1 interrupt.
 *
 * See gen_isr_tables.py for details on the Zephyr multi-level IRQ
 * number encoding, which determines how these helpers work.
 */

/**
 * @brief Get an IRQ's level
 * @param irq The IRQ number in the Zephyr irq.h numbering system
 * @return IRQ level, either 1 or 2
 */
static inline unsigned int rv32m1_irq_level(unsigned int irq)
{
	return ((irq >> 8) & 0xff) == 0U ? 1 : 2;
}

/**
 * @brief Level 1 interrupt line associated with an IRQ
 *
 * Results are undefined if rv32m1_irq_level(irq) is not 1.
 *
 * @param The IRQ number in the Zephyr <irq.h> numbering system
 * @return Level 1 (i.e. event unit) IRQ number associated with irq
 */
static inline uint32_t rv32m1_level1_irq(unsigned int irq)
{
	/*
	 * There's no need to do any math; the precondition is that
	 * it's a level 1 IRQ.
	 */
	return irq;
}

/**
 * @brief INTMUX channel (i.e. level 2 aggregator number) for an IRQ
 *
 * Results are undefined if rv32m1_irq_level(irq) is not 2.
 *
 * @param irq The IRQ number whose INTMUX channel / level 2 aggregator
 *            to get, in the Zephyr <irq.h> numbering system
 * @return INTMUX channel number associated with the IRQ
 */
static inline uint32_t rv32m1_intmux_channel(unsigned int irq)
{
	/*
	 * Here we make use of these facts:
	 *
	 * - the INTMUX output IRQ numbers are arranged consecutively
	 *   by channel in the event unit IRQ numbering assignment,
	 *   starting from channel 0.
	 *
	 * - CONFIG_2ND_LVL_INTR_00_OFFSET is defined to
	 *   be the offset of the first level 2 aggregator in the parent
	 *   interrupt controller's IRQ numbers, i.e. channel 0's
	 *   IRQ number in the event unit.
	 */
	return (irq & 0xff) - CONFIG_2ND_LVL_INTR_00_OFFSET;
}

/**
 * @brief INTMUX interrupt ID number for an IRQ
 *
 * Results are undefined if rv32m1_irq_level(irq) is not 2.
 *
 * @param The IRQ number whose INTMUX interrupt ID to get, in the Zephyr
 *        <irq.h> numbering system
 * @return The INTMUX interrupt ID, in the inclusive range 0 to 31
 */
static inline uint32_t rv32m1_intmux_line(unsigned int irq)
{
	return ((irq >> 8) & 0xff) - 1;
}

void soc_interrupt_init(void);

#endif	/* !_ASMLANGUAGE */

#if defined(CONFIG_SOC_OPENISA_RV32M1_RI5CY)
#include "soc_ri5cy.h"
#elif defined(CONFIG_SOC_OPENISA_RV32M1_ZERO_RISCY)
#include "soc_zero_riscy.h"
#endif

/* helper macro to convert from a DT_INST to HAL clock_ip_name */
#define INST_DT_CLOCK_IP_NAME(n) \
	MAKE_PCC_REGADDR(DT_REG_ADDR(DT_INST_PHANDLE(n, clocks)), \
			DT_INST_CLOCKS_CELL(n, name))

#endif /* SOC_RISCV32_OPENISA_RV32M1_SOC_H_ */
