/*
 * Copyright (c) 2022 Schlumberger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Infineon XMC4xxx interrupt controller devicetree macros
 * @ingroup dt_infineon_xmc4xxx_intc
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_INFINEON_XMC4XXX_INTC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_INFINEON_XMC4XXX_INTC_H_

/**
 * @defgroup dt_infineon_xmc4xxx_intc Infineon XMC4xxx interrupt controller
 * @brief Devicetree macros for the Infineon XMC4xxx interrupt controller.
 * @ingroup devicetree-interrupt_controller
 *
 * Macros for encoding the ERU (Event Request Unit) line mapping cells used with the
 * <tt>infineon,xmc4xxx-intc</tt> compatible interrupt controller. @ref XMC4XXX_INTC_SET_LINE_MAP()
 * packs the GPIO @c port and @c pin, the ERU input source @c eru_src and the ERU output @c line
 * into a single mapping value; the matching @c XMC4XXX_INTC_GET_* accessors extract each field.
 *
 * @code{.dts}
 * &some_device {
 *         interrupt-names = "rx";
 *         interrupts = <XMC4XXX_INTC_SET_LINE_MAP(0, 1, 0, 0) 0>;
 * };
 * @endcode
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#define XMC4XXX_INTC_PORT_POS		0
#define XMC4XXX_INTC_PORT_MASK		0xf

#define XMC4XXX_INTC_PIN_POS		4
#define XMC4XXX_INTC_PIN_MASK		0xf

#define XMC4XXX_INTC_LINE_POS		8
#define XMC4XXX_INTC_LINE_MASK		0x7

#define XMC4XXX_INTC_ERU_SRC_POS	11
#define XMC4XXX_INTC_ERU_SRC_MASK	0x7

/** @endcond */

/** @brief Extract the GPIO port from a line mapping value. */
#define XMC4XXX_INTC_GET_PORT(mx)    ((mx >> XMC4XXX_INTC_PORT_POS) & XMC4XXX_INTC_PORT_MASK)
/** @brief Extract the GPIO pin from a line mapping value. */
#define XMC4XXX_INTC_GET_PIN(mx)     ((mx >> XMC4XXX_INTC_PIN_POS) & XMC4XXX_INTC_PIN_MASK)
/** @brief Extract the ERU output line from a line mapping value. */
#define XMC4XXX_INTC_GET_LINE(mx)    ((mx >> XMC4XXX_INTC_LINE_POS) & XMC4XXX_INTC_LINE_MASK)
/** @brief Extract the ERU input source from a line mapping value. */
#define XMC4XXX_INTC_GET_ERU_SRC(mx) ((mx >> XMC4XXX_INTC_ERU_SRC_POS) & XMC4XXX_INTC_ERU_SRC_MASK)

/**
 * @brief Encode an ERU line mapping value.
 *
 * @param port    GPIO port number.
 * @param pin     GPIO pin number.
 * @param eru_src ERU input source.
 * @param line    ERU output line.
 */
#define XMC4XXX_INTC_SET_LINE_MAP(port, pin, eru_src, line)                                        \
	((port) << XMC4XXX_INTC_PORT_POS | (pin) << XMC4XXX_INTC_PIN_POS |                         \
	 (eru_src) << XMC4XXX_INTC_ERU_SRC_POS | (line) << XMC4XXX_INTC_LINE_POS)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_INFINEON_XMC4XXX_INTC_H_ */
