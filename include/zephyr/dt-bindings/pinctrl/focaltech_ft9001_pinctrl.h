/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_FOCALTECH_FT9001_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_FOCALTECH_FT9001_PINCTRL_H_

/**
 * @defgroup focaltech_pinctrl_macros FocalTech IOCTRL Pin Control
 * @brief Pin control macros for FocalTech FT9001 SoC
 * @{
 */

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

/**
 * @defgroup focaltech_pinctrl_regs IOCTRL Register Offsets
 * @brief IOCTRL register offsets
 * @{
 */

#define FOCALTECH_IOCTRL_SPICR_OFFSET    0x00000U
#define FOCALTECH_IOCTRL_I2CCR_OFFSET    0x00008U
#define FOCALTECH_IOCTRL_SCICR_OFFSET    0x0000CU
#define FOCALTECH_IOCTRL_SWAPCR_OFFSET   0x0001CU
#define FOCALTECH_IOCTRL_CLKRSTCR_OFFSET 0x00044U
#define FOCALTECH_IOCTRL_EPORT2CR_OFFSET 0x00054U
#define FOCALTECH_IOCTRL_EPORT3CR_OFFSET 0x00058U
#define FOCALTECH_IOCTRL_EPORT4CR_OFFSET 0x0005CU
#define FOCALTECH_IOCTRL_EPORT5CR_OFFSET 0x00060U
#define FOCALTECH_IOCTRL_EPORT6CR_OFFSET 0x00064U
#define FOCALTECH_IOCTRL_EPORT7CR_OFFSET 0x00068U
#define FOCALTECH_IOCTRL_SWAPCR2_OFFSET  0x0006CU
#define FOCALTECH_IOCTRL_SWAPCR3_OFFSET  0x00070U
#define FOCALTECH_IOCTRL_SWAPCR4_OFFSET  0x00074U
#define FOCALTECH_IOCTRL_SWAPCR5_OFFSET  0x00078U

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_FOCALTECH_FT9001_PINCTRL_H_ */
