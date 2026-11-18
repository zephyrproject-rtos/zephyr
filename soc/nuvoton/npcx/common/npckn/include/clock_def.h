/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_CLOCK_DEF_H_
#define _NUVOTON_NPCX_CLOCK_DEF_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <soc_clock.h>

/* FMUL clock */
#if (OFMCLK <= MAX_FMCLK)
#define FMCLK OFMCLK /* FMUL clock = OFMCLK */
#elif ((OFMCLK / 2) <= MAX_FMCLK)
#define FMCLK (OFMCLK / 2) /* FMUL clock = OFMCLK/2 */
#else
#define FMCLK (OFMCLK / 4) /* FMUL clock = OFMCLK/4 */
#endif

/* APBs source clock */
#define APBSRC_CLK OFMCLK

/* AHB6 clock */
#if (CORE_CLK > MAX_AHB6CLK)
#define AHB6DIV_VAL 1 /* AHB6_CLK = CORE_CLK/2 */
#else
#define AHB6DIV_VAL 0 /* AHB6_CLK = CORE_CLK */
#endif

/* FIU clock divider */
#if (CORE_CLK > MAX_FIUCLK)
#define FIUDIV_VAL 1 /* FIU_CLK = CORE_CLK/2 */
#else
#define FIUDIV_VAL 0 /* FIU_CLK = CORE_CLK */
#endif

#if defined(CONFIG_CLOCK_CONTROL_NPCX_SUPP_FIU1)
#if (CORE_CLK > MAX_FIU1CLK)
#define FIU1DIV_VAL 1 /* FIU1_CLK = CORE_CLK/2 */
#else
#define FIU1DIV_VAL 0 /* FIU1_CLK = CORE_CLK */
#endif
#endif /* CONFIG_CLOCK_CONTROL_NPCX_SUPP_FIU1 */

/* I3C clock divider */
#if (OFMCLK == MHZ(120)) /* MCLkD must between 40 mhz to 50 mhz*/
#define MCLKD_SL 2    /* I3C_CLK = (MCLK / 3) */
#elif (OFMCLK <= MHZ(100) && OFMCLK >= MHZ(80))
#define MCLKD_SL 1    /* I3C_CLK = (MCLK / 2) */
#else
#define MCLKD_SL 0    /* I3C_CLK = MCLK */
#endif

/* Get APB clock freq */
#define NPCX_APB_CLOCK(no) (APBSRC_CLK / (APB##no##DIV_VAL + 1))

/*
 * Frequency multiplier M/N value definitions according to the requested
 * OFMCLK (Unit:Hz).
 */
#if (DT_NODE_HAS_PROP(DT_NODELABEL(pcc), hfcg_m_hval) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(pcc), hfcg_m_lval) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(pcc), hfcg_n_val))
/* If M/N values are defined in dts, use them directly. */
#define HFCGMH_VAL DT_PROP(DT_NODELABEL(pcc), hfcg_m_hval)
#define HFCGML_VAL DT_PROP(DT_NODELABEL(pcc), hfcg_m_lval)
#define HFCGN_VAL  DT_PROP(DT_NODELABEL(pcc), hfcg_n_val)
#else
/* Check the requested OFMCLK and calculate HFCG N values accordingly. */
#if (OFMCLK <= MAX_FMCLK)
#define HFCGN_VAL    0x02 /* Set XF_RANGE as 0 */
#elif ((OFMCLK / 2) <= MAX_FMCLK)
#define HFCGN_VAL    0x82 /* Set XF_RANGE as 1 */
#elif ((OFMCLK / 4) <= MAX_FMCLK) && defined(XF_4_RANGE_SUPP)
#define HFCGN_VAL    0x42 /* Set XF_4_RANGE as 1 */
#else
#error "Out of OFMCLK Frequency limitation"
#endif
/* Check the requested OFMCLK and calculate HFCG M values accordingly. */
#if   (OFMCLK == 160000000) && defined(XF_4_RANGE_SUPP)
#define HFCGMH_VAL   0x09
#define HFCGML_VAL   0x89
#elif (OFMCLK == 132000000) && defined(XF_4_RANGE_SUPP)
#define HFCGMH_VAL   0x07
#define HFCGML_VAL   0xDE
#elif (OFMCLK == 100000000)
#define HFCGMH_VAL   0x0B
#define HFCGML_VAL   0xEC
#elif (OFMCLK == 96000000)
#define HFCGMH_VAL   0x0B
#define HFCGML_VAL   0x72
#elif (OFMCLK == 90000000)
#define HFCGMH_VAL   0x0A
#define HFCGML_VAL   0xBA
#elif (OFMCLK == 80000000)
#define HFCGMH_VAL   0x09
#define HFCGML_VAL   0x89
#elif (OFMCLK == 66000000)
#define HFCGMH_VAL   0x07
#define HFCGML_VAL   0xDE
#elif (OFMCLK == 50000000)
#define HFCGMH_VAL   0x0B
#define HFCGML_VAL   0xEC
#elif (OFMCLK == 48000000)
#define HFCGMH_VAL   0x0B
#define HFCGML_VAL   0x72
#else
#error "Unsupported OFMCLK Frequency"
#endif
#endif

#endif /* _NUVOTON_NPCX_CLOCK_DEF_H_ */
