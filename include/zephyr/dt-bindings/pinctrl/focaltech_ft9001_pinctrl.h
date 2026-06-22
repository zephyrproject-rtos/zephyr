/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree pin control helpers for FocalTech FT9001
 * @ingroup focaltech_ft9001_pinctrl
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_FOCALTECH_FT9001_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_FOCALTECH_FT9001_PINCTRL_H_

/**
 * @defgroup focaltech_pinctrl FocalTech pin control helpers
 * @ingroup devicetree-pinctrl
 */

/**
 * @defgroup focaltech_ft9001_pinctrl FocalTech FT9001 pin control helpers
 * @brief Macros for pin multiplexing configuration on FocalTech FT9001
 * @ingroup focaltech_pinctrl
 *
 * These helpers describe IO mux selections for the devicetree node compatible with
 * @c focaltech,ft9001-pinctrl. Each entry in a @c pinmux property is formed with
 * @ref FOCALTECH_PINMUX, passing one of the IO controller register offsets from
 * @ref focaltech_pinctrl_regs, the bit index within that register that controls the pad or signal,
 * and the mux select value for that field (0 = default, 1 = alternate).
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/pinctrl/focaltech_ft9001_pinctrl.h>
 *
 * &pinctrl {
 *     usart3_default: usart3_default {
 *         group1 {
 *             pinmux = <FOCALTECH_PINMUX(FOCALTECH_IOCTRL_SCICR_OFFSET, 25, 1)>,
 *                      <FOCALTECH_PINMUX(FOCALTECH_IOCTRL_SCICR_OFFSET, 26, 1)>;
 *         };
 *     };
 * };
 * @endcode
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/*
 * Pinmux encoding:
 *   bit[0:0]   - VALUE (0 = default, 1 = alternate)
 *   bit[5:1]   - BIT position in register
 *   bit[25:6]  - Register offset
 */

#define FOCALTECH_IOCTRL_BASE 0x40000000U

#define FOCALTECH_PINCTRL_VALUE_POS 0U
#define FOCALTECH_PINCTRL_BIT_POS   1U
#define FOCALTECH_PINCTRL_REG_POS   6U

#define FOCALTECH_PINCTRL_VALUE_MASK 0x1U
#define FOCALTECH_PINCTRL_BIT_MASK   0x1FU
#define FOCALTECH_PINCTRL_REG_MASK   0xFFFFFU

#define FOCALTECH_PINMUX(reg, bit, value)                                                          \
	(((reg) << FOCALTECH_PINCTRL_REG_POS) | ((bit) << FOCALTECH_PINCTRL_BIT_POS) |             \
	 ((value) << FOCALTECH_PINCTRL_VALUE_POS))

#define FOCALTECH_PINCTRL_REG_GET(pinmux)                                                          \
	((((pinmux) >> FOCALTECH_PINCTRL_REG_POS) & FOCALTECH_PINCTRL_REG_MASK) |                  \
	 FOCALTECH_IOCTRL_BASE)

#define FOCALTECH_PINCTRL_BIT_GET(pinmux)                                                          \
	(((pinmux) >> FOCALTECH_PINCTRL_BIT_POS) & FOCALTECH_PINCTRL_BIT_MASK)

#define FOCALTECH_PINCTRL_VALUE_GET(pinmux)                                                        \
	(((pinmux) >> FOCALTECH_PINCTRL_VALUE_POS) & FOCALTECH_PINCTRL_VALUE_MASK)

/** @endcond */

/**
 * @brief Encode one @c pinmux cell for FT9001 pinctrl devicetree properties.
 *
 * @param reg IO mux register offset (use @c FOCALTECH_IOCTRL_*_OFFSET).
 * @param bit Bit index inside @a reg for this mux control field.
 * @param value Mux selection written to that field.
 */
#define FOCALTECH_PINMUX(reg, bit, value)                                                          \
	(((reg) << FOCALTECH_PINCTRL_REG_POS) | ((bit) << FOCALTECH_PINCTRL_BIT_POS) |             \
	 ((value) << FOCALTECH_PINCTRL_VALUE_POS))

/**
 * @defgroup focaltech_pinctrl_regs FT9001 IOCTRL register offsets
 * @brief Register offsets used as @a reg arguments to @ref FOCALTECH_PINMUX
 * @{
 */

/** SPI control register */
#define FOCALTECH_IOCTRL_SPICR_OFFSET    0x00000U
/** I2C control register */
#define FOCALTECH_IOCTRL_I2CCR_OFFSET    0x00008U
/** SCI (UART) control register */
#define FOCALTECH_IOCTRL_SCICR_OFFSET    0x0000CU
/** Pad swap control register */
#define FOCALTECH_IOCTRL_SWAPCR_OFFSET   0x0001CU
/** Clock / reset control register */
#define FOCALTECH_IOCTRL_CLKRSTCR_OFFSET 0x00044U
/** EPORT2 control register */
#define FOCALTECH_IOCTRL_EPORT2CR_OFFSET 0x00054U
/** EPORT3 control register */
#define FOCALTECH_IOCTRL_EPORT3CR_OFFSET 0x00058U
/** EPORT4 control register */
#define FOCALTECH_IOCTRL_EPORT4CR_OFFSET 0x0005CU
/** EPORT5 control register */
#define FOCALTECH_IOCTRL_EPORT5CR_OFFSET 0x00060U
/** EPORT6 control register */
#define FOCALTECH_IOCTRL_EPORT6CR_OFFSET 0x00064U
/** EPORT7 control register */
#define FOCALTECH_IOCTRL_EPORT7CR_OFFSET 0x00068U
/** Pad swap control register 2 */
#define FOCALTECH_IOCTRL_SWAPCR2_OFFSET  0x0006CU
/** Pad swap control register 3 */
#define FOCALTECH_IOCTRL_SWAPCR3_OFFSET  0x00070U
/** Pad swap control register 4 */
#define FOCALTECH_IOCTRL_SWAPCR4_OFFSET  0x00074U
/** Pad swap control register 5 */
#define FOCALTECH_IOCTRL_SWAPCR5_OFFSET  0x00078U

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_FOCALTECH_FT9001_PINCTRL_H_ */
