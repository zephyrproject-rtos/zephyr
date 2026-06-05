/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_pfic

#include <hal_ch32fun.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* PFIC SCTLR bits */
#define SEVONPEND BIT(4)
#define WFITOWFE  BIT(3)

/*
 * INESTCTRL (CSR 0x804) bit definitions.
 *
 * This driver currently writes 0x0 to INESTCTRL (neither HPE nor hardware
 * nesting are enabled).  The bits are defined here for reference and for
 * future HPE support.
 *
 *   bit 0 (HWSTKEN)  - Hardware stack (HPE).  When set, the PFIC auto-saves
 *                       mepc, mstatus, ra, t0, and a0-a7 to a hardware FIFO
 *                       on ISR entry and restores them on mret.
 *   bit 1 (INESTEN)  - Hardware interrupt nesting.  When set, the PFIC can
 *                       preempt ISRs at hardware level without consulting
 *                       mstatus.MIE.  Not used; MIE-based nesting instead.
 *   bit 4 (HWSTKOV)  - Read-only hardware stack overflow flag.
 */
#define INESTCTRL_HWSTKEN  BIT(0)
#define INESTCTRL_INESTEN  BIT(1)
#define INESTCTRL_HWSTKOV  BIT(4)

#define CSR_INESTCTRL  0x804

void arch_irq_enable(unsigned int irq)
{
	PFIC->IENR[irq / 32] = BIT(irq % 32);
}

void arch_irq_disable(unsigned int irq)
{
	PFIC->IRER[irq / 32] = BIT(irq % 32);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return ((PFIC->ISR[irq >> 5] & BIT(irq & 0x1F)) != 0);
}

/**
 * @brief Write an interrupt priority to the PFIC IPRIOR register.
 *
 * Called indirectly by IRQ_CONNECT() through the generated interrupt
 * controller initialisation code.  The @p prio value is written to
 * PFIC->IPRIOR[irq] as-is; it should use the WCH_PFIC_PRIORITY()
 * macro from dt-bindings/wch-pfic.h:
 *
 *   bit[7]     = preemption group  (0 = higher priority, can preempt group 1)
 *   bits[6:5]  = sub-priority      (0 = highest, 3 = lowest within the group)
 *   bits[4:0]  = reserved
 *
 * When CONFIG_WCH_PFIC_NESTING=y, the PFIC uses these priority values
 * to arbitrate between pending interrupts when MIE is re-enabled in
 * _isr_wrapper.
 */
void z_riscv_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(flags);

	if (irq < 256) {
		PFIC->IPRIOR[irq] = (uint8_t)(prio & 0xFF);
	}
}

/**
 * @brief Initialise the PFIC at PRE_KERNEL_1 priority.
 *
 * Configures the WFI wake-up behaviour (SEVONPEND so any event including
 * interrupts wakes the core from WFI) and leaves INESTCTRL at its reset
 * value (0x0 -- HPE and hardware nesting both disabled).
 *
 * HPE and INESTEN are controlled by Kconfig options CONFIG_WCH_PFIC_HPE
 * and CONFIG_WCH_PFIC_NESTING respectively.  Currently both are effectively
 * unused: HPE is experimental and INESTEN is superseded by the MIE-based
 * nesting mechanism in _isr_wrapper.  The #ifdef blocks are kept so the
 * options can be tested without code changes.
 */
static int pfic_init(void)
{
	/* `wfi` is called with interrupts disabled. Configure the PFIC to wake up on any event,
	 * including any interrupt.
	 */
	uint32_t inest_val;

	PFIC->SCTLR = SEVONPEND | WFITOWFE;

	inest_val = 0;

#ifdef CONFIG_WCH_PFIC_HPE
	inest_val |= INESTCTRL_HWSTKEN;
#endif

#ifdef CONFIG_WCH_PFIC_NESTING
	inest_val |= INESTCTRL_INESTEN;
#endif

	if (inest_val != 0) {
		__asm__ volatile ("csrw %0, %1" :: "i"(CSR_INESTCTRL), "r"(inest_val));
	}

	return 0;
}

SYS_INIT(pfic_init, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY);
