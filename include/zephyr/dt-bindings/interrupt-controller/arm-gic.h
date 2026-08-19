/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Arm GIC interrupt controller devicetree macros
 * @ingroup dt_arm_gic
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ARM_GIC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ARM_GIC_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @defgroup dt_arm_gic Arm GIC interrupt controller
 * @brief Devicetree macros for the Arm Generic Interrupt Controller (GIC).
 * @ingroup devicetree-interrupt_controller
 *
 * Macros for encoding interrupt cells for the <tt>arm,gic</tt> and <tt>arm,gic-v3</tt> compatible
 * interrupt controllers. An interrupt is described by three cells: the interrupt group (@ref
 * GIC_SPI or @ref GIC_PPI), the interrupt number within that group, and the trigger type (@ref
 * IRQ_TYPE_LEVEL or @ref IRQ_TYPE_EDGE).
 *
 * @code{.dts}
 * &i2c0 {
 *         interrupts = <GIC_SPI 25 IRQ_TYPE_LEVEL>;
 * };
 * @endcode
 * @{
 */

/**
 * @name CPU interrupt numbers
 *
 * Private peripheral interrupt (PPI) numbers for the Arm CPU interface.
 * @{
 */
#define	GIC_INT_VIRT_MAINT		25 /**< Virtual maintenance interrupt */
#define	GIC_INT_HYP_TIMER		26 /**< Hypervisor timer interrupt */
#define	GIC_INT_VIRT_TIMER		27 /**< Virtual timer interrupt */
#define	GIC_INT_LEGACY_FIQ		28 /**< Legacy nFIQ signal */
#define	GIC_INT_PHYS_TIMER		29 /**< Secure physical timer interrupt */
#define	GIC_INT_NS_PHYS_TIMER		30 /**< Non-secure physical timer interrupt (alias) */
#define	GIC_INT_LEGACY_IRQ		31 /**< Legacy nIRQ signal */
/** @} */

/* BIT(0) reserved for IRQ_ZERO_LATENCY */
#define	IRQ_TYPE_LEVEL		BIT(1) /**< Level-triggered interrupt */
#define	IRQ_TYPE_EDGE		BIT(2) /**< Edge-triggered interrupt */

#define	GIC_SPI			0x0 /**< Shared Peripheral Interrupt group */
#define	GIC_PPI			0x1 /**< Private Peripheral Interrupt group */

#define IRQ_DEFAULT_PRIORITY	0xa0 /**< Default interrupt priority */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ARM_GIC_H_ */
