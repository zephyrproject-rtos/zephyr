/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_MCHP_PCR_H_
#define _SOC_MCHP_PCR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* slp_idx = [0, 4], bitpos = [0, 31] refer above */
#define MCHP_XEC_PCR_SCR_ENCODE(slp_idx, bitpos)			\
	(((uint16_t)(slp_idx) & 0x7u) | (((uint16_t)bitpos & 0x1fu) << 3))

#define MCHP_XEC_PCR_SCR_GET_IDX(e)	((e) & 0x7u)
#define MCHP_XEC_PCR_SCR_GET_BITPOS(e)	(((e) & 0xf8u) >> 3)

/* cpu clock divider */
#define MCHP_XEC_CLK_CPU_MASK		GENMASK(7, 0)
#define MCHP_XEC_CLK_CPU_CLK_DIV_1	1u
#define MCHP_XEC_CLK_CPU_CLK_DIV_2	2u
#define MCHP_XEC_CLK_CPU_CLK_DIV_4	4u
#define MCHP_XEC_CLK_CPU_CLK_DIV_8	8u
#define MCHP_XEC_CLK_CPU_CLK_DIV_16	16u
#define MCHP_XEC_CLK_CPU_CLK_DIV_48	48u

/* slow clock divider */
#define MCHP_XEC_CLK_SLOW_MASK		GENMASK(8, 0)
#define MCHP_XEC_CLK_SLOW_CLK_DIV_100K	480u

#define MCHP_XEC_CLK_SRC_POS		24
#define MCHP_XEC_CLK_SRC_MASK		GENMASK(31, 24)

#define MCHP_XEC_CLK_SRC_GET(n)		\
	(((n) & MCHP_XEC_CLK_SRC_MASK) >> MCHP_XEC_CLK_SRC_POS)

#define MCHP_XEC_CLK_SRC_SET(v, c)	(((v) & ~MCHP_XEC_CLK_SRC_MASK) |\
	(((c) << MCHP_XEC_CLK_SRC_POS) & MCHP_XEC_CLK_SRC_MASK))

/*
 * b[31:24] = clock source
 * b[23:0] = clock source specific format
 */
struct mchp_xec_pcr_clk_ctrl {
	uint32_t pcr_info;
};

#ifdef __cplusplus
}
#endif

#endif /* _SOC_MCHP_PCR_H_ */
