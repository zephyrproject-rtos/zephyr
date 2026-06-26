/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OpenISA RV32M1 INTMUX devicetree macros
 * @ingroup dt_openisa_intmux
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_OPENISA_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_OPENISA_INTMUX_H_

/**
 * @defgroup dt_openisa_intmux OpenISA RV32M1 INTMUX
 * @brief Devicetree macros for the OpenISA RV32M1 interrupt multiplexer (INTMUX).
 * @ingroup devicetree-interrupt_controller
 *
 * Level 1 IRQ offsets for each INTMUX channel of the <tt>openisa,rv32m1-intmux</tt> compatible
 * interrupt controller. Each channel aggregates a set of level 2 interrupts onto a single level 1
 * line; the @c INTMUX_CHx_IRQ values give the level 1 line number for channel @c x and are set on
 * the corresponding <tt>openisa,rv32m1-intmux-ch</tt> interrupt-controller node.
 *
 * @code{.dts}
 * intmux0_ch0: interrupt-controller@0 {
 *         compatible = "openisa,rv32m1-intmux-ch";
 *         #address-cells = <0>;
 *         #interrupt-cells = <1>;
 *         interrupt-controller;
 *         interrupts = <INTMUX_CH0_IRQ>;
 *         reg = <0x0 0x40>;
 * };
 * @endcode
 * @{
 */

#define INTMUX_CH0_IRQ 24 /**< Level 1 IRQ for INTMUX channel 0 */
#define INTMUX_CH1_IRQ 25 /**< Level 1 IRQ for INTMUX channel 1 */
#define INTMUX_CH2_IRQ 26 /**< Level 1 IRQ for INTMUX channel 2 */
#define INTMUX_CH3_IRQ 27 /**< Level 1 IRQ for INTMUX channel 3 */
#define INTMUX_CH4_IRQ 28 /**< Level 1 IRQ for INTMUX channel 4 */
#define INTMUX_CH5_IRQ 29 /**< Level 1 IRQ for INTMUX channel 5 */
#define INTMUX_CH6_IRQ 30 /**< Level 1 IRQ for INTMUX channel 6 */
#define INTMUX_CH7_IRQ 31 /**< Level 1 IRQ for INTMUX channel 7 */

/** @} */

#endif
