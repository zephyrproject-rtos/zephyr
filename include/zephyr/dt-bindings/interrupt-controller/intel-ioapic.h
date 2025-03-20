/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_INTEL_IOAPIC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_INTEL_IOAPIC_H_

#define	IRQ_TYPE_LEVEL			0x00008000
#define	IRQ_TYPE_EDGE			0x00000000
#define	IRQ_TYPE_LOW			0x00002000
#define	IRQ_TYPE_HIGH			0x00000000
#define	IRQ_DELIVERY_LOWEST		0x00000100
#define	IRQ_DELIVERY_FIXED		0x00000000

/*
 * for most device interrupts, lowest priority delivery is preferred
 * since this ensures only one CPU gets the interrupt in SMP systems.
 */
#define IRQ_TYPE_LOWEST_EDGE_RISING		(IRQ_DELIVERY_LOWEST | IRQ_TYPE_EDGE | IRQ_TYPE_HIGH)
#define IRQ_TYPE_LOWEST_EDGE_FALLING		(IRQ_DELIVERY_LOWEST | IRQ_TYPE_EDGE | IRQ_TYPE_LOW)
#define IRQ_TYPE_LOWEST_LEVEL_HIGH		(IRQ_DELIVERY_LOWEST | IRQ_TYPE_LEVEL | IRQ_TYPE_HIGH)
#define IRQ_TYPE_LOWEST_LEVEL_LOW		(IRQ_DELIVERY_LOWEST | IRQ_TYPE_LEVEL | IRQ_TYPE_LOW)

/* for interrupts that want to be delivered to all CPUs, e.g. HPET */
#define IRQ_TYPE_FIXED_EDGE_RISING		(IRQ_DELIVERY_FIXED | IRQ_TYPE_EDGE | IRQ_TYPE_HIGH)
#define IRQ_TYPE_FIXED_EDGE_FALLING		(IRQ_DELIVERY_FIXED | IRQ_TYPE_EDGE | IRQ_TYPE_LOW)
#define IRQ_TYPE_FIXED_LEVEL_HIGH		(IRQ_DELIVERY_FIXED | IRQ_TYPE_LEVEL | IRQ_TYPE_HIGH)
#define IRQ_TYPE_FIXED_LEVEL_LOW		(IRQ_DELIVERY_FIXED | IRQ_TYPE_LEVEL | IRQ_TYPE_LOW)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_INTEL_IOAPIC_H_ */
