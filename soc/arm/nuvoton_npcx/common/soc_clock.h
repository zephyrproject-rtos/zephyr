/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_CLOCK_H_
#define _NUVOTON_NPCX_SOC_CLOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Common clock control device name for all NPCX series */
#define NPCX_CLK_CTRL_NAME "npcx-cc"

/**
 * @brief NPCX clock configuration structure
 *
 * Used to indicate the device's clock bus type and corresponding PWDWN_CTL
 * register/bit to turn on/off its source clock.
 */
struct npcx_clk_cfg {
	uint16_t bus:8;
	uint16_t ctrl:5;
	uint16_t bit:3;
};

/*
 * NPCX7 and later series clock tree macros:
 * (Please refer Figure 58. for more information.)
 *
 * Suggestion:
 * - OSC_CLK >= 80MHz, XF_RANGE should be 1, else 0.
 * - CORE_CLK > 66MHz, AHB6DIV should be 1, else 0.
 * - CORE_CLK > 50MHz, FIUDIV should be 1, else 0.
 */

/* Target OSC_CLK freq */
#define OSC_CLK   CONFIG_CLOCK_NPCX_OSC_CYCLES_PER_SEC
/* Core domain clock */
#define CORE_CLK  CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
/* Low Frequency clock */
#define LFCLK     32768
/* Core clock prescaler */
#define FPRED_VAL ((OSC_CLK / CORE_CLK) - 1)

/* FMUL clock */
#if (OSC_CLK >= 80000000)
#define FMCLK (OSC_CLK / 2) /* FMUL clock = OSC_CLK/2 if OSC_CLK >= 80MHz */
#else
#define FMCLK OSC_CLK /* FMUL clock = OSC_CLK */
#endif

/* APBs source clock */
#define APBSRC_CLK OSC_CLK
/* APB1 clock divider, default value (APB1 clock = OSC_CLK/6) */
#define APB1DIV_VAL (CONFIG_CLOCK_NPCX_APB1_PRESCALER - 1)
/* APB2 clock divider, default value (APB2 clock = OSC_CLK/6) */
#define APB2DIV_VAL (CONFIG_CLOCK_NPCX_APB2_PRESCALER - 1)
/* APB3 clock divider, default value (APB3 clock = OSC_CLK/6) */
#define APB3DIV_VAL (CONFIG_CLOCK_NPCX_APB3_PRESCALER - 1)

/* AHB6 clock */
#if (CORE_CLK > 66000000)
#define AHB6DIV_VAL 1 /* AHB6_CLK = CORE_CLK/2 if CORE_CLK > 66MHz */
#else
#define AHB6DIV_VAL 0 /* AHB6_CLK = CORE_CLK */
#endif
/* FIU clock divider */
#if (CORE_CLK > 50000000)
#define FIUDIV_VAL 1 /* FIU_CLK = CORE_CLK/2 */
#else
#define FIUDIV_VAL 0 /* FIU_CLK = CORE_CLK */
#endif

/* Get APB clock freq */
#define NPCX_APB_CLOCK(no) (APBSRC_CLK / (APB##no##DIV_VAL + 1))

/*
 * Frequency multiplier M/N value definitions according to the requested
 * OSC_CLK (Unit:Hz).
 */
#if (OSC_CLK > 80000000)
#define HFCGN_VAL    0x82 /* Set XF_RANGE as 1 if OSC_CLK >= 80MHz */
#else
#define HFCGN_VAL    0x02
#endif
#if   (OSC_CLK == 100000000)
#define HFCGMH_VAL   0x0B
#define HFCGML_VAL   0xEC
#elif (OSC_CLK == 90000000)
#define HFCGMH_VAL   0x0A
#define HFCGML_VAL   0xBA
#elif (OSC_CLK == 80000000)
#define HFCGMH_VAL   0x09
#define HFCGML_VAL   0x89
#elif (OSC_CLK == 66000000)
#define HFCGMH_VAL   0x0F
#define HFCGML_VAL   0xBC
#elif (OSC_CLK == 50000000)
#define HFCGMH_VAL   0x0B
#define HFCGML_VAL   0xEC
#elif (OSC_CLK == 48000000)
#define HFCGMH_VAL   0x0B
#define HFCGML_VAL   0x72
#elif (OSC_CLK == 40000000)
#define HFCGMH_VAL   0x09
#define HFCGML_VAL   0x89
#elif (OSC_CLK == 33000000)
#define HFCGMH_VAL   0x07
#define HFCGML_VAL   0xDE
#elif (OSC_CLK == 30000000)
#define HFCGMH_VAL   0x07
#define HFCGML_VAL   0x27
#elif (OSC_CLK == 26000000)
#define HFCGMH_VAL   0x06
#define HFCGML_VAL   0x33
#else
#error "Unsupported OSC_CLK Frequency"
#endif

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NPCX_SOC_CLOCK_H_ */
