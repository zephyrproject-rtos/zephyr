/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_S32_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_S32_PINCTRL_H_

#include <zephyr/sys/util_macro.h>

/*
 * The NXP S32 pinmux configuration is encoded in a 32-bit field value as follows:
 *
 * - 0..2:   Output mux Source Signal Selection (MSCR.SSS)
 * - 3..6:   Input mux Source Signal Selection (IMCR.SSS)
 * - 7..15:  Input Multiplexed Signal Configuration Register (IMCR) index
 * - 16..24: Multiplexed Signal Configuration Register (MSCR) index
 * - 25..27: SIUL2 instance index (0..7)
 * - 28..31: Reserved for future use
 */
#define S32_MSCR_SSS_SHIFT	0U
#define S32_MSCR_SSS_MASK	BIT_MASK(3)
#define S32_IMCR_SSS_SHIFT	3U
#define S32_IMCR_SSS_MASK	BIT_MASK(4)
#define S32_IMCR_IDX_SHIFT	7U
#define S32_IMCR_IDX_MASK	BIT_MASK(9)
#define S32_MSCR_IDX_SHIFT	16U
#define S32_MSCR_IDX_MASK	BIT_MASK(9)
#define S32_SIUL2_IDX_SHIFT	25U
#define S32_SIUL2_IDX_MASK	BIT_MASK(3)

#define S32_PINMUX_MSCR_SSS(cfg)	\
	(((cfg) & S32_MSCR_SSS_MASK) << S32_MSCR_SSS_SHIFT)

#define S32_PINMUX_IMCR_SSS(cfg)	\
	(((cfg) & S32_IMCR_SSS_MASK) << S32_IMCR_SSS_SHIFT)

#define S32_PINMUX_IMCR_IDX(cfg)	\
	(((cfg) & S32_IMCR_IDX_MASK) << S32_IMCR_IDX_SHIFT)

#define S32_PINMUX_MSCR_IDX(cfg)	\
	(((cfg) & S32_MSCR_IDX_MASK) << S32_MSCR_IDX_SHIFT)

#define S32_PINMUX_SIUL2_IDX(cfg)	\
	(((cfg) & S32_SIUL2_IDX_MASK) << S32_SIUL2_IDX_SHIFT)

#define S32_PINMUX_GET_MSCR_SSS(cfg)	\
	(((cfg) >> S32_MSCR_SSS_SHIFT) & S32_MSCR_SSS_MASK)

#define S32_PINMUX_GET_IMCR_SSS(cfg)	\
	(((cfg) >> S32_IMCR_SSS_SHIFT) & S32_IMCR_SSS_MASK)

#define S32_PINMUX_GET_IMCR_IDX(cfg)	\
	(((cfg) >> S32_IMCR_IDX_SHIFT) & S32_IMCR_IDX_MASK)

#define S32_PINMUX_GET_MSCR_IDX(cfg)	\
	(((cfg) >> S32_MSCR_IDX_SHIFT) & S32_MSCR_IDX_MASK)

#define S32_PINMUX_GET_SIUL2_IDX(cfg)	\
	(((cfg) >> S32_SIUL2_IDX_SHIFT) & S32_SIUL2_IDX_MASK)

/**
 * @brief Utility macro to build NXP S32 pinmux property for pinctrl nodes.
 *
 * @param siul2_idx SIUL2 instance index
 * @param mscr_idx Multiplexed Signal Configuration Register (MSCR) index
 * @param mscr_sss Output mux Source Signal Selection (MSCR.SSS)
 * @param imcr_idx Input Multiplexed Signal Configuration Register (IMCR) index
 * @param imcr_sss Input mux Source Signal Selection (IMCR.SSS)
 */
#define S32_PINMUX(siul2_idx, mscr_idx, mscr_sss, imcr_idx, imcr_sss)		\
	(S32_PINMUX_SIUL2_IDX(siul2_idx) | S32_PINMUX_MSCR_IDX(mscr_idx)	\
	 | S32_PINMUX_MSCR_SSS(mscr_sss) | S32_PINMUX_IMCR_IDX(imcr_idx)	\
	 | S32_PINMUX_IMCR_SSS(imcr_sss))

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_S32_PINCTRL_H_ */
