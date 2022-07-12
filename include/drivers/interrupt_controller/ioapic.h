/* ioapic.h - public IOAPIC APIs */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileComment: IEC-61508-SIL3
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_IOAPIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_IOAPIC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Redirection table entry bits: lower 32 bit
 * Used as flags argument in ioapic_irq_set
 */

#define IOAPIC_INT_MASK 0x00010000U
#define IOAPIC_TRIGGER_MASK 0x00008000U
#define IOAPIC_LEVEL 0x00008000U
#define IOAPIC_EDGE 0x00000000U
#define IOAPIC_REMOTE 0x00004000U
#define IOAPIC_LOW 0x00002000U
#define IOAPIC_HIGH 0x00000000U
#define IOAPIC_LOGICAL 0x00000800U
#define IOAPIC_PHYSICAL 0x00000000U
#define IOAPIC_FIXED 0x00000000U
#define IOAPIC_LOWEST 0x00000100U
#define IOAPIC_SMI 0x00000200U
#define IOAPIC_NMI 0x00000400U
#define IOAPIC_INIT 0x00000500U
#define IOAPIC_EXTINT 0x00000700U

#ifndef _ASMLANGUAGE
uint32_t z_ioapic_num_rtes(void);
void z_ioapic_irq_enable(unsigned int irq);
void z_ioapic_irq_disable(unsigned int irq);
void z_ioapic_int_vec_set(unsigned int irq, unsigned int vector);
void z_ioapic_irq_set(unsigned int irq, unsigned int vector, uint32_t flags);
#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_IOAPIC_H_ */
