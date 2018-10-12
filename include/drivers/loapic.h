/* loapic.h - public LOAPIC APIs */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LOAPIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_LOAPIC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Local APIC Register Offset */

#define LOAPIC_ID 0x020		  /* Local APIC ID Reg */
#define LOAPIC_VER 0x030	  /* Local APIC Version Reg */
#define LOAPIC_TPR 0x080	  /* Task Priority Reg */
#define LOAPIC_APR 0x090	  /* Arbitration Priority Reg */
#define LOAPIC_PPR 0x0a0	  /* Processor Priority Reg */
#define LOAPIC_EOI 0x0b0	  /* EOI Reg */
#define LOAPIC_LDR 0x0d0	  /* Logical Destination Reg */
#define LOAPIC_DFR 0x0e0	  /* Destination Format Reg */
#define LOAPIC_SVR 0x0f0	  /* Spurious Interrupt Reg */
#define LOAPIC_ISR 0x100	  /* In-service Reg */
#define LOAPIC_TMR 0x180	  /* Trigger Mode Reg */
#define LOAPIC_IRR 0x200	  /* Interrupt Request Reg */
#define LOAPIC_ESR 0x280	  /* Error Status Reg */
#define LOAPIC_ICRLO 0x300	/* Interrupt Command Reg */
#define LOAPIC_ICRHI 0x310	/* Interrupt Command Reg */
#define LOAPIC_TIMER 0x320	/* LVT (Timer) */
#define LOAPIC_THERMAL 0x330      /* LVT (Thermal) */
#define LOAPIC_PMC 0x340	  /* LVT (PMC) */
#define LOAPIC_LINT0 0x350	/* LVT (LINT0) */
#define LOAPIC_LINT1 0x360	/* LVT (LINT1) */
#define LOAPIC_ERROR 0x370	/* LVT (ERROR) */
#define LOAPIC_TIMER 0x320	/* LVT (Timer) */
#define LOAPIC_TIMER_ICR 0x380    /* Timer Initial Count Reg */
#define LOAPIC_TIMER_CCR 0x390    /* Timer Current Count Reg */
#define LOAPIC_TIMER_CONFIG 0x3e0 /* Timer Divide Config Reg */

/* Local APIC Vector Table Bits */
#define LOAPIC_LVT_MASKED 0x00010000   /* mask */

#ifndef _ASMLANGUAGE

extern void _loapic_int_vec_set(unsigned int irq, unsigned int vector);
extern void _loapic_irq_enable(unsigned int irq);
extern void _loapic_irq_disable(unsigned int irq);

#if CONFIG_EOI_FORWARDING_BUG
extern void _lakemont_eoi(void);
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_LOAPIC_H_ */
