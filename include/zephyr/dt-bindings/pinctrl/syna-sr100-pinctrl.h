/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_SYNA_SR100_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_SYNA_SR100_PINCTRL_H_

/**
 * @brief Pin controller id, bit position, mode, mask and offset of pinmux
 *        register, offset of configuration register
 */
#define SRXXX_CTRL_SHIFT 0U
#define SRXXX_CTRL_MASK  0x07U
#define SRXXX_BIT_SHIFT  3U
#define SRXXX_BIT_MASK   0x1FU
#define SRXXX_MODE_SHIFT 8U
#define SRXXX_MODE_MASK  0x07U
#define SRXXX_MASK_SHIFT 11U
#define SRXXX_MASK_MASK  0x0FU
#define SRXXX_REG_SHIFT  16U
#define SRXXX_REG_MASK   0xFFU
#define SRXXX_CFG_SHIFT  24U
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

#define SRXXX_GLOBAL_PINMUX(reg, bit, mode, cfg)	SRXXX_PINMUX(0, reg, bit, mode, 7, cfg)
#define SRXXX_AON_PINMUX(reg, bit, mode, cfg)		SRXXX_PINMUX(1, reg, bit, mode, 7, cfg)
#define SRXXX_LPS_PINMUX(reg, bit, mode, cfg)		SRXXX_PINMUX(2, reg, bit, mode, 7, cfg)
#define SRXXX_SWIRE_PINMUX(bit, mode)			SRXXX_PINMUX(3, 0, bit, mode, 1, 0)
#define SRXXX_PORT_PINMUX(reg, bit, mode, mask)		SRXXX_PINMUX(4, reg, bit, mode, mask, 0)
#define SRXXX_FIXED_PINMUX(cfg)				SRXXX_PINMUX(0, 0, 0, 0, 0, cfg)

#define SRXXX_PINMUX_CTRL(pinmux)	(((pinmux) >> SRXXX_CTRL_SHIFT) & SRXXX_CTRL_MASK)
#define SRXXX_PINMUX_REG(pinmux)	(((pinmux) >> SRXXX_REG_SHIFT) & SRXXX_REG_MASK)
#define SRXXX_PINMUX_BIT(pinmux)	(((pinmux) >> SRXXX_BIT_SHIFT) & SRXXX_BIT_MASK)
#define SRXXX_PINMUX_MODE(pinmux)	(((pinmux) >> SRXXX_MODE_SHIFT) & SRXXX_MODE_MASK)
#define SRXXX_PINMUX_MASK(pinmux)	(((pinmux) >> SRXXX_MASK_SHIFT) & SRXXX_MASK_MASK)
#define SRXXX_PINMUX_CFG(pinmux)	(((pinmux) >> SRXXX_CFG_SHIFT) & SRXXX_CFG_MASK)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_SYNA_SR100_PINCTRL_H_ */
