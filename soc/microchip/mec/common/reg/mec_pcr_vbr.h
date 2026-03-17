/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_MICROCHIP_MEC_COMMON_REG_MEC_PCR_VBR_H
#define __SOC_MICROCHIP_MEC_COMMON_REG_MEC_PCR_VBR_H

#include <zephyr/arch/cpu.h>

/* PCR hardware registers for MEC SoC */
#define XEC_CC_PCR_MAX_SCR 5

#define XEC_CC_SLP_CR_OFS              0
#define XEC_CC_SLP_CR_MODE_DS_POS      0 /* if set enter deep sleep when sleep is triggered */
#define XEC_CC_SLP_CR_SLP_ALL_POS      3 /* trigger sleep */
#define XEC_CC_SLP_CR_ALLOW_PLL_NL_POS 8 /* allow sleep entry when PLL is not locked */

#define XEC_CC_PCLK_CR_OFS        4u
#define XEC_CC_PCLK_CR_DIV_POS    0
#define XEC_CC_PCLK_CR_DIV_MSK    GENMASK(7, 0)
#define XEC_CC_PCLK_CR_DIV_SET(d) FIELD_PREP(XEC_CC_PCLK_CR_DIV_MSK, (d))
#define XEC_CC_PCLK_CR_DIV_GET(r) FIELD_GET(XEC_CC_PCLK_CR_DIV_MSK, (r))

#define XEC_CC_SCLK_CR_OFS        8u
#define XEC_CC_SCLK_CR_DIV_POS    0
#define XEC_CC_SCLK_CR_DIV_MSK    GENMASK(9, 0)
#define XEC_CC_SCLK_CR_DIV_SET(d) FIELD_PREP(XEC_CC_SCLK_CR_DIV_MSK, (d))
#define XEC_CC_SCLK_CR_DIV_GET(r) FIELD_GET(XEC_CC_SCLK_CR_DIV_MSK, (r))

#define XEC_CC_OSC_ID_OFS          0xcu /* read-only */
#define XEC_CC_OSC_ID_PLL_LOCK_POS 8

#define XEC_CC_PWR_SR_OFS              0x10u
#define XEC_CC_PWR_SR_MSK              (GENMASK(8, 1) | GENMASK(11, 10))
#define XEC_CC_PWR_SR_VCC_PWRGD2_POS   1 /* RO MEC172x/4x/5x only */
#define XEC_CC_PWR_SR_VCC_PWRGD_POS    2 /* RO */
#define XEC_CC_PWR_SR_RST_HOST_POS     3 /* RO */
#define XEC_CC_PWR_SR_RST_VTR_POS      4
#define XEC_CC_PWR_SR_RST_VBAT_POS     5
#define XEC_CC_PWR_SR_RST_SYS_POS      6
#define XEC_CC_PWR_SR_RST_JTAG_POS     7 /* RO */
#define XEC_CC_PWR_SR_WDT_EV_POS       8
#define XEC_CC_PWR_SR_32K_ACT_POS      10 /* RO */
#define XEC_CC_PWR_SR_ESPI_CLK_ACT_POS 11 /* RO */

#define XEC_CC_PERIPH_RST_LOCK_OFS 0x84u
#define XEC_CC_PERIPH_RST_UNLOCK   0xa6382d4cu
#define XEC_CC_PERIPH_RST_LOCK     0xa6382d4du

#define XEC_CC_SLP_EN_OFS(n)  (0x30u + ((uint32_t)(n) * 4u))
#define XEC_CC_CLK_REQ_OFS(n) (0x50u + ((uint32_t)(n) * 4u))
#define XEC_CC_RST_EN_OFS(n)  (0x70u + ((uint32_t)(n) * 4u))

#ifdef CONFIG_SOC_SERIES_MEC15XX
#define XEC_CC_PCR3_CRYPTO_MASK (BIT(26) | BIT(27) | BIT(28))
#else /* MEC172x/4x/5x only */
#define XEC_CC_PCR3_CRYPTO_MASK BIT(26)

#define XEC_CC_TCLK_OFS         0x1cu
#define XEC_CC_TCLK_FAST_EN_POS 2

#define XEC_CC_VBAT_CR_OFS      0x88u
#define XEC_CC_VBAT_CR_SRST_POS 0

#define XEC_CC_VTR_CS_OFS             0x8cu
#define XEC_CC_VTR_CS_PLL_SEL_POS     0
#define XEC_CC_VTR_CS_PLL_SEL_MSK     GENMASK(1, 0)
#define XEC_CC_VTR_CS_PLL_SEL_SI      0u /* internal silicon oscillator */
#define XEC_CC_VTR_CS_PLL_SEL_XTAL    1u /* external crystal */
#define XEC_CC_VTR_CS_PLL_SEL_32K_PIN 2u /* 32KHZ_IN pin */
#define XEC_CC_VTR_CS_PLL_SEL_OFF     3u /* PLL held in reset. Use internal ring osc for clocks */
#define XEC_CC_VTR_CS_PLL_SEL_SET(p)  FIELD_PREP(XEC_CC_VTR_CS_PLL_SEL_MSK, (p))
#define XEC_CC_VTR_CS_PLL_SEL_GET(r)  FIELD_GET(XEC_CC_VTR_CS_PLL_SEL_MSK, (r))
#define XEC_CC_VTR_CS_PLL_LOCKED_POS  8 /* PLL reference clock selection is locked */

#define XEC_CC_32K_PER_CNT_OFS 0xc0u /* read-only */
#define XEC_CC_32K_PER_CNT_MSK GENMASK(15, 0)

#define XEC_CC_32K_HP_CNT_OFS 0xc4u /* read-only */
#define XEC_CC_32K_HP_CNT_MSK GENMASK(15, 0)

#define XEC_CC_32K_MIN_PER_CNT_OFS 0xc8u
#define XEC_CC_32K_MIN_PER_CNT_MSK GENMASK(15, 0)

#define XEC_CC_32K_MAX_PER_CNT_OFS 0xccu
#define XEC_CC_32K_MAX_PER_CNT_MSK GENMASK(15, 0)

#define XEC_CC_32K_DC_VAR_CNT_OFS 0xd0u
#define XEC_CC_32K_DC_VAR_CNT_MSK GENMASK(15, 0)

#define XEC_CC_32K_MAX_DC_VAR_OFS 0xd4u
#define XEC_CC_32K_MAX_DC_VAR_MSK GENMASK(15, 0)

/* Number of consecutive valid periods and pulse widths */
#define XEC_CC_32K_VAL_CNT_OFS    0xd8u /* read-only */
#define XEC_CC_32K_VAL_CNT_MSK    GENMASK(7, 0)

#define XEC_CC_32K_MIN_VAL_CNT_OFS 0xdcu
#define XEC_CC_32K_MIN_VAL_CNT_MSK GENMASK(7, 0)

#define XEC_CC_32K_CCR_OFS            0xe0u /* 32K count control */
#define XEC_CC_32K_CCR_PER_CNT_EN_POS 0
#define XEC_CC_32K_CCR_DC_CNT_EN_POS  1
#define XEC_CC_32K_VAL_CNT_EN_POS     2
#define XEC_CC_32K_CCR_SRC_SOCS_POS   4  /* 32K source is internal silicon oscillator else XTAL */
#define XEC_CC_32K_CCR_CLR_POS        24 /* clear all counters */

#define XEC_CC_32K_SR_OFS           0xe4u /* R/W1C */
#define XEC_CC_32K_IER_OFS          0xe8u
#define XEC_CC_32K_SR_IER_ALL_MSK   GENMASK(6, 0)
#define XEC_CC_32K_SR_PULSE_RDY_POS 0
#define XEC_CC_32K_SR_PER_PASS_POS  1
#define XEC_CC_32K_SR_DC_PASS_POS   2
#define XEC_CC_32K_SR_FAIL_POS      3
#define XEC_CC_32K_SR_PER_OVFL_POS  4
#define XEC_CC_32K_SR_VALID_POS     5
#define XEC_CC_32K_SR_UNWELL_POS    6

#endif /* CONFIG_SOC_SERIES_MEC15XX */

/* VBAT register bank */
#define XEC_VBR_PFR_SR_OFS            0 /* power-fail-reset-status. R/W1C */
#define XEC_VBR_PFR_SR_MSK            0xf4u
#define XEC_VBR_PFR_SR_SOFT_RST_POS   2
#define XEC_VBR_PFR_SR_RSTI_POS       4
#define XEC_VBR_PFR_SR_WDT_STS_POS    5
#define XEC_VBR_PFR_SR_SYSRR_STS_POS  6
#define XEC_VBR_PFR_SR_VB_RST_STS_POS 7

#define XEC_VBR_CS_OFS 8u
#ifdef CONFIG_SOC_SERIES_MEC15XX
#define XEC_VBR_CS_MSK                GENMASK(3, 1)
#define XEC_VBR_CS_EXT_32K_IN_PIN_POS 1
#define XEC_VBR_CS_SEL_XTAL_POS       2
#define XEC_VBR_CS_XTAL_SE_POS        3
#define XEC_VBR_CS_USE_SIL_OSC        0
#define XEC_VBR_CS_USE_32KIN_PIN      BIT(XEC_VBR_CS_EXT_32K_IN_PIN_POS)
#define XEC_VBR_CS_USE_PAR_XTAL       BIT(XEC_VBR_CS_SEL_XTAL_POS)
#define XEC_VBR_CS_USE_SE_XTAL        (BIT(XEC_VBR_CS_SEL_XTAL_POS) | BIT(XEC_VBR_CS_XTAL_SE_POS))
#else
#define XEC_VBR_CS_SO_EN_POS      0
#define XEC_VBR_CS_SO_LOCK_POS    7
#define XEC_VBR_CS_XSTA_POS       8
#define XEC_VBR_CS_XSE_POS        9
#define XEC_VBR_CS_XDHSC_POS      10
#define XEC_VBR_CS_XG_POS         11
#define XEC_VBR_CS_XG_MSK         GENMASK(12, 11)
#define XEC_VBR_CS_XG_4X          0
#define XEC_VBR_CS_XG_3X          1u
#define XEC_VBR_CS_XG_2X          2u
#define XEC_VBR_CS_XG_1X          3u
#define XEC_VBR_CS_XG_SET(g)      FIELD_PREP(XEC_VBR_CS_XG_MSK, (g))
#define XEC_VBR_CS_XG_GET(r)      FIELD_GET(XEC_VBR_CS_XG_MSK, (r))
#define XEC_VBR_CS_PCS_POS        16
#define XEC_VBR_CS_PCS_MSK        GENMASK(17, 16)
#define XEC_VBR_CS_PCS_SI         0
#define XEC_VBR_CS_PCS_XTAL       1u
#define XEC_VBR_CS_PCS_PIN_SI     2u
#define XEC_VBR_CS_PCS_PIN_XTAL   3u
#define XEC_VBR_CS_PCS_SET(s)     FIELD_PREP(XEC_VBR_CS_PCS_MSK, (s))
#define XEC_VBR_CS_PCS_GET(r)     FIELD_GET(XEC_VBR_CS_PCS_MSK, (r))
#define XEC_VBR_VTR_SI_OFF_EN_POS 18u
#endif /* CONFIG_SOC_SERIES_MEC15XX */

#define XEC_VBR_OT_OFS 0x14u
#define XEC_VBR_OT_MSK GENMASK(7, 0)

#define XEC_VBR_TCR_OFS          0x1cu
#define XEC_VBR_TCR_OVR_MODE_POS 0
#define XEC_VBR_TCR_OVR_EN_POS   1
#define XEC_VBR_TCR_STA_POS      2
#ifdef CONFIG_SOC_SERIES_MEC15XX
#define XEC_VBR_TCR_MSK GENMASK(2, 0)
#else
#define XEC_VBR_TCR_MSK       GENMASK(3, 0)
#define XEC_VBR_TCR_CG_EN_POS 3
#endif

#endif /* __SOC_MICROCHIP_MEC_COMMON_REG_MEC_PCR_VBR_H */
