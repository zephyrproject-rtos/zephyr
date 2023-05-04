/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NXP_S32_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NXP_S32_PINCTRL_H_

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
#define NXP_S32_MSCR_SSS_SHIFT	0U
#define NXP_S32_MSCR_SSS_MASK	BIT_MASK(3)
#define NXP_S32_IMCR_SSS_SHIFT	3U
#define NXP_S32_IMCR_SSS_MASK	BIT_MASK(4)
#define NXP_S32_IMCR_IDX_SHIFT	7U
#define NXP_S32_IMCR_IDX_MASK	BIT_MASK(9)
#define NXP_S32_MSCR_IDX_SHIFT	16U
#define NXP_S32_MSCR_IDX_MASK	BIT_MASK(9)
#define NXP_S32_SIUL2_IDX_SHIFT	25U
#define NXP_S32_SIUL2_IDX_MASK	BIT_MASK(3)

#define NXP_S32_PINMUX_MSCR_SSS(cfg)	\
	(((cfg) & NXP_S32_MSCR_SSS_MASK) << NXP_S32_MSCR_SSS_SHIFT)

#define NXP_S32_PINMUX_IMCR_SSS(cfg)	\
	(((cfg) & NXP_S32_IMCR_SSS_MASK) << NXP_S32_IMCR_SSS_SHIFT)

#define NXP_S32_PINMUX_IMCR_IDX(cfg)	\
	(((cfg) & NXP_S32_IMCR_IDX_MASK) << NXP_S32_IMCR_IDX_SHIFT)

#define NXP_S32_PINMUX_MSCR_IDX(cfg)	\
	(((cfg) & NXP_S32_MSCR_IDX_MASK) << NXP_S32_MSCR_IDX_SHIFT)

#define NXP_S32_PINMUX_SIUL2_IDX(cfg)	\
	(((cfg) & NXP_S32_SIUL2_IDX_MASK) << NXP_S32_SIUL2_IDX_SHIFT)

#define NXP_S32_PINMUX_GET_MSCR_SSS(cfg)	\
	(((cfg) >> NXP_S32_MSCR_SSS_SHIFT) & NXP_S32_MSCR_SSS_MASK)

#define NXP_S32_PINMUX_GET_IMCR_SSS(cfg)	\
	(((cfg) >> NXP_S32_IMCR_SSS_SHIFT) & NXP_S32_IMCR_SSS_MASK)

#define NXP_S32_PINMUX_GET_IMCR_IDX(cfg)	\
	(((cfg) >> NXP_S32_IMCR_IDX_SHIFT) & NXP_S32_IMCR_IDX_MASK)

#define NXP_S32_PINMUX_GET_MSCR_IDX(cfg)	\
	(((cfg) >> NXP_S32_MSCR_IDX_SHIFT) & NXP_S32_MSCR_IDX_MASK)

#define NXP_S32_PINMUX_GET_SIUL2_IDX(cfg)	\
	(((cfg) >> NXP_S32_SIUL2_IDX_SHIFT) & NXP_S32_SIUL2_IDX_MASK)

/**
 * @brief Utility macro to build NXP S32 pinmux property for pinctrl nodes.
 *
 * @param siul2_idx SIUL2 instance index
 * @param mscr_idx Multiplexed Signal Configuration Register (MSCR) index
 * @param mscr_sss Output mux Source Signal Selection (MSCR.SSS)
 * @param imcr_idx Input Multiplexed Signal Configuration Register (IMCR) index
 * @param imcr_sss Input mux Source Signal Selection (IMCR.SSS)
 */
#define NXP_S32_PINMUX(siul2_idx, mscr_idx, mscr_sss, imcr_idx, imcr_sss)		\
	(NXP_S32_PINMUX_SIUL2_IDX(siul2_idx) | NXP_S32_PINMUX_MSCR_IDX(mscr_idx)	\
	 | NXP_S32_PINMUX_MSCR_SSS(mscr_sss) | NXP_S32_PINMUX_IMCR_IDX(imcr_idx)	\
	 | NXP_S32_PINMUX_IMCR_SSS(imcr_sss))

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NXP_NXP_S32_PINCTRL_H_ */
