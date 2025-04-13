/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
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
#if (OFMCLK > (MAX_OFMCLK / 2))
#define FMCLK (OFMCLK / 2) /* FMUL clock = OFMCLK/2 */
#else
#define FMCLK OFMCLK /* FMUL clock = OFMCLK */
#endif

/* APBs source clock */
#define APBSRC_CLK OFMCLK

/* AHB6 clock */
#if (CORE_CLK > (MAX_OFMCLK / 2))
#define AHB6DIV_VAL 1 /* AHB6_CLK = CORE_CLK/2 */
#else
#define AHB6DIV_VAL 0 /* AHB6_CLK = CORE_CLK */
#endif

/* FIU clock divider */
#if (CORE_CLK > (MAX_OFMCLK / 2))
#define FIUDIV_VAL 1 /* FIU_CLK = CORE_CLK/2 */
#else
#define FIUDIV_VAL 0 /* FIU_CLK = CORE_CLK */
#endif

#if defined(CONFIG_CLOCK_CONTROL_NPCX_SUPP_FIU1)
#if (CORE_CLK > (MAX_OFMCLK / 2))
#define FIU1DIV_VAL 1 /* FIU1_CLK = CORE_CLK/2 */
#else
#define FIU1DIV_VAL 0 /* FIU1_CLK = CORE_CLK */
#endif
#endif /* CONFIG_CLOCK_CONTROL_NPCX_SUPP_FIU1 */

/* I3C clock divider */
#if (OFMCLK == MHZ(120)) /* MCLkD must between 40 mhz to 50 mhz*/
#define MCLKD_SL 2       /* I3C_CLK = (MCLK / 3) */
#elif (OFMCLK <= MHZ(100) && OFMCLK >= MHZ(80))
#define MCLKD_SL 1 /* I3C_CLK = (MCLK / 2) */
#else
#define MCLKD_SL 0 /* I3C_CLK = MCLK */
#endif

/* Get APB clock freq */
#define NPCX_APB_CLOCK(no) (APBSRC_CLK / (APB##no##DIV_VAL + 1))

/*
 * Frequency multiplier M/N value definitions according to the requested
 * OFMCLK (Unit:Hz).
 */
#if (OFMCLK > (MAX_OFMCLK / 2))
#define HFCGN_VAL 0x82 /* Set XF_RANGE as 1 */
#else
#define HFCGN_VAL 0x02
#endif
#if (OFMCLK == 120000000)
#define HFCGMH_VAL 0x0E
#define HFCGML_VAL 0x4E
#elif (OFMCLK == 100000000)
#define HFCGMH_VAL 0x0B
#define HFCGML_VAL 0xEC
#elif (OFMCLK == 96000000)
#define HFCGMH_VAL 0x0B
#define HFCGML_VAL 0x72
#elif (OFMCLK == 90000000)
#define HFCGMH_VAL 0x0A
#define HFCGML_VAL 0xBA
#elif (OFMCLK == 80000000)
#define HFCGMH_VAL 0x09
#define HFCGML_VAL 0x89
#elif (OFMCLK == 66000000)
#define HFCGMH_VAL 0x07
#define HFCGML_VAL 0xDE
#elif (OFMCLK == 50000000)
#define HFCGMH_VAL 0x0B
#define HFCGML_VAL 0xEC
#elif (OFMCLK == 48000000)
#define HFCGMH_VAL 0x0B
#define HFCGML_VAL 0x72
#else
#error "Unsupported OFMCLK Frequency"
#endif

/* Clock prescaler configurations in different series */
#define VAL_HFCGP ((FPRED_VAL << 4) | AHB6DIV_VAL)
#if defined(FIU1DIV_VAL)
#define VAL_HFCBCD ((FIU1DIV_VAL << 4) | (FIUDIV_VAL << 2))
#else
#define VAL_HFCBCD (FIUDIV_VAL << 4)
#endif /* FIU1DIV_VAL */
#define VAL_HFCBCD1 (APB1DIV_VAL | (APB2DIV_VAL << 4))
#if defined(APB4DIV_VAL)
#define VAL_HFCBCD2 (APB3DIV_VAL | (APB4DIV_VAL << 4))
#else
#define VAL_HFCBCD2 APB3DIV_VAL
#endif /* APB4DIV_VAL */
/* I3C1~I3C3 share the same configuration */
#define VAL_HFCBCD3 MCLKD_SL

#endif /* _NUVOTON_NPCX_CLOCK_DEF_H_ */
