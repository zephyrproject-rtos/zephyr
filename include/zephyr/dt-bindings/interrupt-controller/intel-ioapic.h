/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Intel I/O APIC interrupt controller devicetree macros
 * @ingroup dt_intel_ioapic
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_INTEL_IOAPIC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_INTEL_IOAPIC_H_

/**
 * @defgroup dt_intel_ioapic Intel I/O APIC interrupt controller
 * @brief Devicetree flags for the Intel I/O APIC interrupt controller.
 * @ingroup devicetree-interrupt_controller
 *
 * Interrupt trigger, polarity and delivery-mode flags for the <tt>intel,ioapic</tt> compatible
 * interrupt controller. The individual flags are OR-ed together to form the interrupt sense field;
 * the @c IRQ_TYPE_LOWEST_* and @c IRQ_TYPE_FIXED_* macros provide ready-made combinations for the
 * common cases.
 *
 * @code{.dts}
 * &some_device {
 *         interrupts = <17 IRQ_TYPE_LOWEST_LEVEL_LOW 3>;
 * };
 * @endcode
 * @{
 */

#define	IRQ_TYPE_LEVEL			0x00008000 /**< Level-triggered interrupt */
#define	IRQ_TYPE_EDGE			0x00000000 /**< Edge-triggered interrupt */
#define	IRQ_TYPE_LOW			0x00002000 /**< Active-low polarity */
#define	IRQ_TYPE_HIGH			0x00000000 /**< Active-high polarity */
#define	IRQ_DELIVERY_LOWEST		0x00000100 /**< Lowest-priority delivery mode */
#define	IRQ_DELIVERY_FIXED		0x00000000 /**< Fixed delivery mode (all CPUs) */

/*
 * for most device interrupts, lowest priority delivery is preferred
 * since this ensures only one CPU gets the interrupt in SMP systems.
 */
/** Lowest-priority delivery, edge-triggered, rising edge. */
#define IRQ_TYPE_LOWEST_EDGE_RISING		(IRQ_DELIVERY_LOWEST | IRQ_TYPE_EDGE | IRQ_TYPE_HIGH)
/** Lowest-priority delivery, edge-triggered, falling edge. */
#define IRQ_TYPE_LOWEST_EDGE_FALLING		(IRQ_DELIVERY_LOWEST | IRQ_TYPE_EDGE | IRQ_TYPE_LOW)
/** Lowest-priority delivery, level-triggered, active high. */
#define IRQ_TYPE_LOWEST_LEVEL_HIGH		(IRQ_DELIVERY_LOWEST | IRQ_TYPE_LEVEL | IRQ_TYPE_HIGH)
/** Lowest-priority delivery, level-triggered, active low. */
#define IRQ_TYPE_LOWEST_LEVEL_LOW		(IRQ_DELIVERY_LOWEST | IRQ_TYPE_LEVEL | IRQ_TYPE_LOW)

/* for interrupts that want to be delivered to all CPUs, e.g. HPET */
/** Fixed delivery, edge-triggered, rising edge. */
#define IRQ_TYPE_FIXED_EDGE_RISING		(IRQ_DELIVERY_FIXED | IRQ_TYPE_EDGE | IRQ_TYPE_HIGH)
/** Fixed delivery, edge-triggered, falling edge. */
#define IRQ_TYPE_FIXED_EDGE_FALLING		(IRQ_DELIVERY_FIXED | IRQ_TYPE_EDGE | IRQ_TYPE_LOW)
/** Fixed delivery, level-triggered, active high. */
#define IRQ_TYPE_FIXED_LEVEL_HIGH		(IRQ_DELIVERY_FIXED | IRQ_TYPE_LEVEL | IRQ_TYPE_HIGH)
/** Fixed delivery, level-triggered, active low. */
#define IRQ_TYPE_FIXED_LEVEL_LOW		(IRQ_DELIVERY_FIXED | IRQ_TYPE_LEVEL | IRQ_TYPE_LOW)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_INTEL_IOAPIC_H_ */
