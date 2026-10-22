/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief List pinctrl subsystem IDs for Synaptics SR100 family.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_SYNA_SR100_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_SYNA_SR100_PINCTRL_H_

/**
 * Pin controller id, bit position, mode, mask and offset of pinmux
 *        register, offset of configuration register
 */
/** @brief CTRL field shift */
#define SRXXX_CTRL_SHIFT 0U
/** @brief CTRL field mask */
#define SRXXX_CTRL_MASK  0x07U
/** @brief BIT field shift */
#define SRXXX_BIT_SHIFT  3U
/** @brief BIT field mask */
#define SRXXX_BIT_MASK   0x1FU
/** @brief MODE field shift */
#define SRXXX_MODE_SHIFT 8U
/** @brief MODE field mask */
#define SRXXX_MODE_MASK  0x07U
/** @brief MASK field shift */
#define SRXXX_MASK_SHIFT 11U
/** @brief MASK field mask */
#define SRXXX_MASK_MASK  0x0FU
/** @brief REG field shift */
#define SRXXX_REG_SHIFT  16U
/** @brief REG field mask */
#define SRXXX_REG_MASK   0xFFU
/** @brief CFG field shift */
#define SRXXX_CFG_SHIFT  24U
/** @brief CFG field mask */
#define SRXXX_CFG_MASK   0xFFU

/**
 * @brief Pin configuration bit field. Set mask to 0 for fixed pins (XSPI)

 * Fields:
 *
 * - ctrl  [  0 :  2 ]
 * - bit   [  3 :  7 ]
 * - mode  [  8 : 10 ]
 * - mask  [ 11 : 14 ]
 * - reg   [ 16 : 23 ]
 * - cfg   [ 24 : 31 ]
 *
 * @param ctrl Controller (0 = Global, 1 = AON-Main, 2 = LPS_Gear1, 3 = SWIRE)
 * @param bit  Bit offset inside register (0..27)
 * @param mode Muxing option (0..7)
 * @param mask Bit mask for muxing option (1..7)
 * @param reg  Register offset relative to pinmux base address (0..255)
 * @param cfg  Register offset relative to pincfg base address (0..255)
 */
#define SRXXX_PINMUX(ctrl, reg, bit, mode, mask, cfg)       \
	((((ctrl) & SRXXX_CTRL_MASK) << SRXXX_CTRL_SHIFT) | \
	 (((reg)  & SRXXX_REG_MASK)  << SRXXX_REG_SHIFT)  | \
	 (((bit)  & SRXXX_BIT_MASK)  << SRXXX_BIT_SHIFT)  | \
	 (((mode) & SRXXX_MODE_MASK) << SRXXX_MODE_SHIFT) | \
	 (((mask) & SRXXX_MASK_MASK) << SRXXX_MASK_SHIFT) | \
	 (((cfg)  & SRXXX_CFG_MASK)  << SRXXX_CFG_SHIFT))

/** Macros for pinmux selection */
/** @brief global */
#define SRXXX_GLOBAL_PINMUX(reg, bit, mode, cfg)	SRXXX_PINMUX(0, reg, bit, mode, 7, cfg)
/** @brief AON */
#define SRXXX_AON_PINMUX(reg, bit, mode, cfg)		SRXXX_PINMUX(1, reg, bit, mode, 7, cfg)
/** @brief LPS */
#define SRXXX_LPS_PINMUX(reg, bit, mode, cfg)		SRXXX_PINMUX(2, reg, bit, mode, 7, cfg)
/** @brief Soundwire */
#define SRXXX_SWIRE_PINMUX(bit, mode)			SRXXX_PINMUX(3, 0, bit, mode, 1, 0)

/** @brief Macro for port selection */
#define SRXXX_PORT_PINMUX(reg, bit, mode, mask)		SRXXX_PINMUX(4, reg, bit, mode, mask, 0)
/** @brief Macro for fixed pinmux */
#define SRXXX_FIXED_PINMUX(cfg)				SRXXX_PINMUX(0, 0, 0, 0, 0, cfg)

/** helpers for register fields */
/** @brief CTRL field */
#define SRXXX_PINMUX_CTRL(pinmux)	(((pinmux) >> SRXXX_CTRL_SHIFT) & SRXXX_CTRL_MASK)
/** @brief REG field */
#define SRXXX_PINMUX_REG(pinmux)	(((pinmux) >> SRXXX_REG_SHIFT) & SRXXX_REG_MASK)
/** @brief BIT field */
#define SRXXX_PINMUX_BIT(pinmux)	(((pinmux) >> SRXXX_BIT_SHIFT) & SRXXX_BIT_MASK)
/** @brief MODE field */
#define SRXXX_PINMUX_MODE(pinmux)	(((pinmux) >> SRXXX_MODE_SHIFT) & SRXXX_MODE_MASK)
/** @brief MASK field */
#define SRXXX_PINMUX_MASK(pinmux)	(((pinmux) >> SRXXX_MASK_SHIFT) & SRXXX_MASK_MASK)
/** @brief CFG field */
#define SRXXX_PINMUX_CFG(pinmux)	(((pinmux) >> SRXXX_CFG_SHIFT) & SRXXX_CFG_MASK)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_SYNA_SR100_PINCTRL_H_ */
