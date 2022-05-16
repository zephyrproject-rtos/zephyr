/* ioapic_priv.h - private IOAPIC APIs */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_IOAPIC_PRIV_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_IOAPIC_PRIV_H_

/* IO APIC direct register offsets */

#define IOAPIC_IND 0x00   /* Index Register */
#define IOAPIC_DATA 0x10  /* IO window (data) - pc.h */
#define IOAPIC_IRQPA 0x20 /* IRQ Pin Assertion Register */
#define IOAPIC_EOI 0x40   /* EOI Register */

/* IO APIC indirect register offset */

#define IOAPIC_ID 0x00     /* IOAPIC ID */
#define IOAPIC_VERS 0x01   /* IOAPIC Version */
#define IOAPIC_ARB 0x02    /* IOAPIC Arbitration ID */
#define IOAPIC_BOOT 0x03   /* IOAPIC Boot Configuration */
#define IOAPIC_REDTBL 0x10 /* Redirection Table (24 * 64bit) */

/* Interrupt delivery type */

#define IOAPIC_DT_APIC 0x0 /* APIC serial bus */
#define IOAPIC_DT_FS 0x1   /* Front side bus message*/

/* Version register bits */

#define IOAPIC_MRE_MASK 0x00ff0000 /* Max Red. entry mask */
#define IOAPIC_MRE_POS 16
#define IOAPIC_PRQ 0x00008000      /* this has IRQ reg */
#define IOAPIC_VERSION 0x000000ff  /* version number */

/* Redirection table entry bits: upper 32 bit */

#define IOAPIC_DESTINATION 0xff000000

/* Redirection table entry bits: lower 32 bit */

#define IOAPIC_VEC_MASK 0x000000ff

/* VTD related macros */

#define IOAPIC_VTD_REMAP_FORMAT BIT(16)
/* We care only about the first 14 bits.
 * The 15th bits is in the first 32bits of RTE but since
 * we don't go up to that value, let's ignore it.
 */
#define IOAPIC_VTD_INDEX(index) (index << 17)

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_IOAPIC_PRIV_H_ */
