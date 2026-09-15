/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MPFS_PLL_H_
/** @brief Constant value for ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MPFS_PLL_H_. */
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MPFS_PLL_H_

#include <zephyr/sys/util.h>

/**
 * @file
 * @brief Register definitions for the Microchip MPFS PLL clock controller.
 *
 * This header provides register offsets and field masks used by the
 * MPFS PLL clock-control drivers.
 */

/* -------------------------------------------------------------------------- */
/* Register offsets                                                           */
/* -------------------------------------------------------------------------- */
/** @brief Offset for MPFS_PLL_SOFT_RESET. */
#define MPFS_PLL_SOFT_RESET_OFFSET  0x00u
/** @brief Offset for MPFS_PLL_PLL_CTRL. */
#define MPFS_PLL_PLL_CTRL_OFFSET    0x04u
/** @brief Offset for MPFS_PLL_PLL_REF_FB. */
#define MPFS_PLL_PLL_REF_FB_OFFSET  0x08u
/** @brief Offset for MPFS_PLL_PLL_FRACN. */
#define MPFS_PLL_PLL_FRACN_OFFSET   0x0Cu
/** @brief Offset for MPFS_PLL_PLL_DIV_0_1. */
#define MPFS_PLL_PLL_DIV_0_1_OFFSET 0x10u
/** @brief Offset for MPFS_PLL_PLL_DIV_2_3. */
#define MPFS_PLL_PLL_DIV_2_3_OFFSET 0x14u
/** @brief Offset for MPFS_PLL_PLL_CTRL2. */
#define MPFS_PLL_PLL_CTRL2_OFFSET   0x18u
/** @brief Offset for MPFS_PLL_PLL_CAL. */
#define MPFS_PLL_PLL_CAL_OFFSET     0x1Cu
/** @brief Offset for MPFS_PLL_PLL_PHADJ. */
#define MPFS_PLL_PLL_PHADJ_OFFSET   0x20u
/** @brief Offset for MPFS_PLL_SSCG_REG_0. */
#define MPFS_PLL_SSCG_REG_0_OFFSET  0x24u
/** @brief Offset for MPFS_PLL_SSCG_REG_1. */
#define MPFS_PLL_SSCG_REG_1_OFFSET  0x28u
/** @brief Offset for MPFS_PLL_SSCG_REG_2. */
#define MPFS_PLL_SSCG_REG_2_OFFSET  0x2Cu
/** @brief Offset for MPFS_PLL_SSCG_REG_3. */
#define MPFS_PLL_SSCG_REG_3_OFFSET  0x30u

/* -------------------------------------------------------------------------- */
/* SOFT_RESET fields                                                          */
/* -------------------------------------------------------------------------- */
/** @brief Functional reset of the block. */
#define MPFS_PLL_SOFT_RESET_PERIPH_MASK  BIT(8)
/** @brief Bit shift for MPFS_PLL_SOFT_RESET_PERIPH. */
#define MPFS_PLL_SOFT_RESET_PERIPH_SHIFT 8U

/** @brief Reset all volatile register bits (W1P). */
#define MPFS_PLL_SOFT_RESET_V_MAP_MASK  BIT(1)
/** @brief Bit shift for MPFS_PLL_SOFT_RESET_V_MAP. */
#define MPFS_PLL_SOFT_RESET_V_MAP_SHIFT 1U

/** @brief Reset all non-volatile register bits (W1P). */
#define MPFS_PLL_SOFT_RESET_NV_MAP_MASK  BIT(0)
/** @brief Bit shift for MPFS_PLL_SOFT_RESET_NV_MAP. */
#define MPFS_PLL_SOFT_RESET_NV_MAP_SHIFT 0U

/* -------------------------------------------------------------------------- */
/* PLL_CTRL fields                                                            */
/* -------------------------------------------------------------------------- */
/** @brief Inversion of LOCK. */
#define MPFS_PLL_PLL_CTRL_LOCK_B_MASK  BIT(31)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_LOCK_B. */
#define MPFS_PLL_PLL_CTRL_LOCK_B_SHIFT 31U

/** @brief PLL unlock interrupt status (W1C). */
#define MPFS_PLL_PLL_CTRL_UNLOCK_INT_MASK  BIT(29)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_UNLOCK_INT. */
#define MPFS_PLL_PLL_CTRL_UNLOCK_INT_SHIFT 29U

/** @brief PLL lock interrupt status (W1C). */
#define MPFS_PLL_PLL_CTRL_LOCK_INT_MASK  BIT(28)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_LOCK_INT. */
#define MPFS_PLL_PLL_CTRL_LOCK_INT_SHIFT 28U

/** @brief Enable PLL unlock interrupt. */
#define MPFS_PLL_PLL_CTRL_UNLOCK_INT_EN_MASK  BIT(27)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_UNLOCK_INT_EN. */
#define MPFS_PLL_PLL_CTRL_UNLOCK_INT_EN_SHIFT 27U

/** @brief Enable PLL lock interrupt. */
#define MPFS_PLL_PLL_CTRL_LOCK_INT_EN_MASK  BIT(26)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_LOCK_INT_EN. */
#define MPFS_PLL_PLL_CTRL_LOCK_INT_EN_SHIFT 26U

/** @brief PLL lock detect status. */
#define MPFS_PLL_PLL_CTRL_LOCK_MASK  BIT(25)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_LOCK. */
#define MPFS_PLL_PLL_CTRL_LOCK_SHIFT 25U

/** @brief Low-power mode requires lock when set. */
#define MPFS_PLL_PLL_CTRL_LP_REQUIRES_LOCK_MASK BIT(24)

/** @brief Bypass mux control for post divider output */
#define MPFS_PLL_PLL_CTRL_LP_REQUIRES_LOCK_SHIFT 24U

/** @brief Bypass mux control for post-divider outputs. */
#define MPFS_PLL_PLL_CTRL_REG_BYPASSPOST_MASK  GENMASK(23, 20)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_REG_BYPASSPOST. */
#define MPFS_PLL_PLL_CTRL_REG_BYPASSPOST_SHIFT 20U

/** @brief Bypass mux control for post-divider inputs. */
#define MPFS_PLL_PLL_CTRL_REG_BYPASSPRE_MASK  GENMASK(19, 16)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_REG_BYPASSPRE. */
#define MPFS_PLL_PLL_CTRL_REG_BYPASSPRE_SHIFT 16U

/** @brief Enable bypass controls (active low). */
#define MPFS_PLL_PLL_CTRL_REG_BYPASS_GO_B_MASK  BIT(12)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_REG_BYPASS_GO_B. */
#define MPFS_PLL_PLL_CTRL_REG_BYPASS_GO_B_SHIFT 12U

/** @brief Select RFCLK/FBCLK for bypass mode. */
#define MPFS_PLL_PLL_CTRL_BYPCK_SEL_MASK  GENMASK(11, 8)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_BYPCK_SEL. */
#define MPFS_PLL_PLL_CTRL_BYPCK_SEL_SHIFT 8U

/** @brief Hold outputs low until LOCK and reset on lock edge. */
#define MPFS_PLL_PLL_CTRL_RESETONLOCK_MASK  BIT(7)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_RESETONLOCK. */
#define MPFS_PLL_PLL_CTRL_RESETONLOCK_SHIFT 7U

/** @brief Select reference clock input. */
#define MPFS_PLL_PLL_CTRL_REG_RFCLK_SEL_MASK  BIT(6)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_REG_RFCLK_SEL. */
#define MPFS_PLL_PLL_CTRL_REG_RFCLK_SEL_SHIFT 6U

/** @brief Enable post-divider 3 output (glitchless). */
#define MPFS_PLL_PLL_CTRL_REG_DIVQ3_EN_MASK  BIT(5)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_REG_DIVQ3_EN. */
#define MPFS_PLL_PLL_CTRL_REG_DIVQ3_EN_SHIFT 5U

/** @brief Enable post-divider 2 output (glitchless). */
#define MPFS_PLL_PLL_CTRL_REG_DIVQ2_EN_MASK  BIT(4)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_REG_DIVQ2_EN. */
#define MPFS_PLL_PLL_CTRL_REG_DIVQ2_EN_SHIFT 4U

/** @brief Enable post-divider 1 output (glitchless). */
#define MPFS_PLL_PLL_CTRL_REG_DIVQ1_EN_MASK  BIT(3)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_REG_DIVQ1_EN. */
#define MPFS_PLL_PLL_CTRL_REG_DIVQ1_EN_SHIFT 3U

/** @brief Enable post-divider 0 output (glitchless). */
#define MPFS_PLL_PLL_CTRL_REG_DIVQ0_EN_MASK  BIT(2)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_REG_DIVQ0_EN. */
#define MPFS_PLL_PLL_CTRL_REG_DIVQ0_EN_SHIFT 2U

/** @brief Enable reference divider. */
#define MPFS_PLL_PLL_CTRL_REG_RFDIV_EN_MASK  BIT(1)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_REG_RFDIV_EN. */
#define MPFS_PLL_PLL_CTRL_REG_RFDIV_EN_SHIFT 1U

/** @brief PLL core enable (active high). */
#define MPFS_PLL_PLL_CTRL_REG_POWERDOWN_B_MASK  BIT(0)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_REG_POWERDOWN_B. */
#define MPFS_PLL_PLL_CTRL_REG_POWERDOWN_B_SHIFT 0U

/** @brief Aggregate DIVQ enable bits [Q3..Q0]. */
#define MPFS_PLL_PLL_CTRL_REG_DIVQ_EN_MASK  GENMASK(5, 2)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL_REG_DIVQ_EN. */
#define MPFS_PLL_PLL_CTRL_REG_DIVQ_EN_SHIFT 2U

/* -------------------------------------------------------------------------- */
/* PLL_REF_FB fields                                                          */
/* -------------------------------------------------------------------------- */
/** @brief Reference divide value (1..63). */
#define MPFS_PLL_PLL_REF_FB_RFDIV_MASK  GENMASK(13, 8)
/** @brief Bit shift for MPFS_PLL_PLL_REF_FB_RFDIV. */
#define MPFS_PLL_PLL_REF_FB_RFDIV_SHIFT 8U

/** @brief Enable feedback source mux path. */
#define MPFS_PLL_PLL_REF_FB_FOUTFB_SELMUX_EN_MASK  BIT(3)
/** @brief Bit shift for MPFS_PLL_PLL_REF_FB_FOUTFB_SELMUX_EN. */
#define MPFS_PLL_PLL_REF_FB_FOUTFB_SELMUX_EN_SHIFT 3U

/** @brief Select feedback source from post-dividers. */
#define MPFS_PLL_PLL_REF_FB_FBCK_SEL_MASK  GENMASK(2, 1)
/** @brief Bit shift for MPFS_PLL_PLL_REF_FB_FBCK_SEL. */
#define MPFS_PLL_PLL_REF_FB_FBCK_SEL_SHIFT 1U

/** @brief Deskew mode select: internal/external feedback. */
#define MPFS_PLL_PLL_REF_FB_FSE_B_MASK  BIT(0)
/** @brief Bit shift for MPFS_PLL_PLL_REF_FB_FSE_B. */
#define MPFS_PLL_PLL_REF_FB_FSE_B_SHIFT 0U

/* -------------------------------------------------------------------------- */
/* PLL_FRACN fields                                                           */
/* -------------------------------------------------------------------------- */
/** @brief Fractional noise-cancellation DAC enable. */
#define MPFS_PLL_PLL_FRACN_FRACN_DAC_EN_MASK  BIT(1)
/** @brief Bit shift for MPFS_PLL_PLL_FRACN_FRACN_DAC_EN. */
#define MPFS_PLL_PLL_FRACN_FRACN_DAC_EN_SHIFT 1U

/** @brief Delta-sigma fractional mode enable. */
#define MPFS_PLL_PLL_FRACN_FRACN_EN_MASK  BIT(0)
/** @brief Bit shift for MPFS_PLL_PLL_FRACN_FRACN_EN. */
#define MPFS_PLL_PLL_FRACN_FRACN_EN_SHIFT 0U

/* -------------------------------------------------------------------------- */
/* PLL_DIV_0_1 fields                                                         */
/* -------------------------------------------------------------------------- */
/** @brief Post-divider value for PLL_OUT[1] (1..127). */
#define MPFS_PLL_PLL_DIV_0_1_POST1DIV_MASK  GENMASK(30, 24)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_0_1_POST1DIV. */
#define MPFS_PLL_PLL_DIV_0_1_POST1DIV_SHIFT 24U

/** @brief Start delay (VCO cycles) for divider 1. */
#define MPFS_PLL_PLL_DIV_0_1_DIV1_START_MASK  GENMASK(21, 19)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_0_1_DIV1_START. */
#define MPFS_PLL_PLL_DIV_0_1_DIV1_START_SHIFT 19U

/** @brief VCO phase select for divider 1. */
#define MPFS_PLL_PLL_DIV_0_1_VCO1PH_SEL_MASK  GENMASK(18, 16)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_0_1_VCO1PH_SEL. */
#define MPFS_PLL_PLL_DIV_0_1_VCO1PH_SEL_SHIFT 16U

/** @brief Post-divider value for PLL_OUT[0] (1..127). */
#define MPFS_PLL_PLL_DIV_0_1_POST0DIV_MASK  GENMASK(14, 8)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_0_1_POST0DIV. */
#define MPFS_PLL_PLL_DIV_0_1_POST0DIV_SHIFT 8U

/** @brief Start delay (VCO cycles) for divider 0. */
#define MPFS_PLL_PLL_DIV_0_1_DIV0_START_MASK  GENMASK(5, 3)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_0_1_DIV0_START. */
#define MPFS_PLL_PLL_DIV_0_1_DIV0_START_SHIFT 3U

/** @brief VCO phase select for divider 0. */
#define MPFS_PLL_PLL_DIV_0_1_VCO0PH_SEL_MASK  GENMASK(2, 0)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_0_1_VCO0PH_SEL. */
#define MPFS_PLL_PLL_DIV_0_1_VCO0PH_SEL_SHIFT 0U

/* -------------------------------------------------------------------------- */
/* PLL_DIV_2_3 fields                                                         */
/* -------------------------------------------------------------------------- */
/** @brief Select post-divider 3 input source (direct/cascaded). */
#define MPFS_PLL_PLL_DIV_2_3_CKPOST3_SEL_MASK  BIT(31)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_2_3_CKPOST3_SEL. */
#define MPFS_PLL_PLL_DIV_2_3_CKPOST3_SEL_SHIFT 31U

/** @brief Post-divider value for PLL_OUT[3] (1..127). */
#define MPFS_PLL_PLL_DIV_2_3_POST3DIV_MASK  GENMASK(30, 24)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_2_3_POST3DIV. */
#define MPFS_PLL_PLL_DIV_2_3_POST3DIV_SHIFT 24U

/** @brief Start delay (VCO cycles) for divider 3. */
#define MPFS_PLL_PLL_DIV_2_3_DIV3_START_MASK  GENMASK(21, 19)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_2_3_DIV3_START. */
#define MPFS_PLL_PLL_DIV_2_3_DIV3_START_SHIFT 19U

/** @brief VCO phase select for divider 3. */
#define MPFS_PLL_PLL_DIV_2_3_VCO3PH_SEL_MASK  GENMASK(18, 16)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_2_3_VCO3PH_SEL. */
#define MPFS_PLL_PLL_DIV_2_3_VCO3PH_SEL_SHIFT 16U

/** @brief Post-divider value for PLL_OUT[2] (1..127). */
#define MPFS_PLL_PLL_DIV_2_3_POST2DIV_MASK  GENMASK(14, 8)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_2_3_POST2DIV. */
#define MPFS_PLL_PLL_DIV_2_3_POST2DIV_SHIFT 8U

/** @brief Start delay (VCO cycles) for divider 2. */
#define MPFS_PLL_PLL_DIV_2_3_DIV2_START_MASK  GENMASK(5, 3)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_2_3_DIV2_START. */
#define MPFS_PLL_PLL_DIV_2_3_DIV2_START_SHIFT 3U

/** @brief VCO phase select for divider 2. */
#define MPFS_PLL_PLL_DIV_2_3_VCO2PH_SEL_MASK  GENMASK(2, 0)
/** @brief Bit shift for MPFS_PLL_PLL_DIV_2_3_VCO2PH_SEL. */
#define MPFS_PLL_PLL_DIV_2_3_VCO2PH_SEL_SHIFT 0U

/* -------------------------------------------------------------------------- */
/* PLL_CTRL2 fields                                                           */
/* -------------------------------------------------------------------------- */
/** @brief Analog test mux select. */
#define MPFS_PLL_PLL_CTRL2_ATEST_SEL_MASK  GENMASK(20, 18)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL2_ATEST_SEL. */
#define MPFS_PLL_PLL_CTRL2_ATEST_SEL_SHIFT 18U

/** @brief Analog test mux enable. */
#define MPFS_PLL_PLL_CTRL2_ATEST_EN_MASK  BIT(17)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL2_ATEST_EN. */
#define MPFS_PLL_PLL_CTRL2_ATEST_EN_SHIFT 17U

/** @brief Lock count selector (2^LOCKCOUNTSEL cycles). */
#define MPFS_PLL_PLL_CTRL2_LOCKCOUNTSEL_MASK  GENMASK(12, 9)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL2_LOCKCOUNTSEL. */
#define MPFS_PLL_PLL_CTRL2_LOCKCOUNTSEL_SHIFT 9U

/** @brief Force output toggle with stopped reference. */
#define MPFS_PLL_PLL_CTRL2_IREF_TOGGLE_MASK  BIT(5)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL2_IREF_TOGGLE. */
#define MPFS_PLL_PLL_CTRL2_IREF_TOGGLE_SHIFT 5U

/** @brief Enable IREF injection to VCO. */
#define MPFS_PLL_PLL_CTRL2_IREF_EN_MASK  BIT(4)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL2_IREF_EN. */
#define MPFS_PLL_PLL_CTRL2_IREF_EN_SHIFT 4U

/** @brief Proportional loop bandwidth control. */
#define MPFS_PLL_PLL_CTRL2_BWP_MASK  GENMASK(3, 2)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL2_BWP. */
#define MPFS_PLL_PLL_CTRL2_BWP_SHIFT 2U

/** @brief Integral loop bandwidth control. */
#define MPFS_PLL_PLL_CTRL2_BWI_MASK  GENMASK(1, 0)
/** @brief Bit shift for MPFS_PLL_PLL_CTRL2_BWI. */
#define MPFS_PLL_PLL_CTRL2_BWI_SHIFT 0U

/* -------------------------------------------------------------------------- */
/* PLL_CAL fields                                                             */
/* -------------------------------------------------------------------------- */
/** @brief Deskew calibration output value. */
#define MPFS_PLL_PLL_CAL_DSKEWCALOUT_MASK  GENMASK(22, 16)
/** @brief Bit shift for MPFS_PLL_PLL_CAL_DSKEWCALOUT. */
#define MPFS_PLL_PLL_CAL_DSKEWCALOUT_SHIFT 16U

/** @brief Deskew calibration override input value. */
#define MPFS_PLL_PLL_CAL_DSKEWCALIN_MASK  GENMASK(14, 8)
/** @brief Bit shift for MPFS_PLL_PLL_CAL_DSKEWCALIN. */
#define MPFS_PLL_PLL_CAL_DSKEWCALIN_SHIFT 8U

/** @brief Deskew calibration bypass select. */
#define MPFS_PLL_PLL_CAL_DSKEWCALBYP_MASK  BIT(4)
/** @brief Bit shift for MPFS_PLL_PLL_CAL_DSKEWCALBYP. */
#define MPFS_PLL_PLL_CAL_DSKEWCALBYP_SHIFT 4U

/** @brief Deskew calibration enable. */
#define MPFS_PLL_PLL_CAL_DSKEWCAL_EN_MASK  BIT(3)
/** @brief Bit shift for MPFS_PLL_PLL_CAL_DSKEWCAL_EN. */
#define MPFS_PLL_PLL_CAL_DSKEWCAL_EN_SHIFT 3U

/** @brief Deskew calibration loop wait counter. */
#define MPFS_PLL_PLL_CAL_DSKEWCALCNT_MASK  GENMASK(2, 0)
/** @brief Bit shift for MPFS_PLL_PLL_CAL_DSKEWCALCNT. */
#define MPFS_PLL_PLL_CAL_DSKEWCALCNT_SHIFT 0U

/* -------------------------------------------------------------------------- */
/* PLL_PHADJ fields                                                           */
/* -------------------------------------------------------------------------- */
/** @brief Load originally programmed output phases (active low control). */
#define MPFS_PLL_PLL_PHADJ_REG_LOADPHS_B_MASK  BIT(14)
/** @brief Bit shift for MPFS_PLL_PLL_PHADJ_REG_LOADPHS_B. */
#define MPFS_PLL_PLL_PHADJ_REG_LOADPHS_B_SHIFT 14U

/** @brief Initial phase for output 3. */
#define MPFS_PLL_PLL_PHADJ_REG_OUT3_PHSINIT_MASK  GENMASK(13, 11)
/** @brief Bit shift for MPFS_PLL_PLL_PHADJ_REG_OUT3_PHSINIT. */
#define MPFS_PLL_PLL_PHADJ_REG_OUT3_PHSINIT_SHIFT 11U

/** @brief Initial phase for output 2. */
#define MPFS_PLL_PLL_PHADJ_REG_OUT2_PHSINIT_MASK  GENMASK(10, 8)
/** @brief Bit shift for MPFS_PLL_PLL_PHADJ_REG_OUT2_PHSINIT. */
#define MPFS_PLL_PLL_PHADJ_REG_OUT2_PHSINIT_SHIFT 8U

/** @brief Initial phase for output 1. */
#define MPFS_PLL_PLL_PHADJ_REG_OUT1_PHSINIT_MASK  GENMASK(7, 5)
/** @brief Bit shift for MPFS_PLL_PLL_PHADJ_REG_OUT1_PHSINIT. */
#define MPFS_PLL_PLL_PHADJ_REG_OUT1_PHSINIT_SHIFT 5U

/** @brief Initial phase for output 0. */
#define MPFS_PLL_PLL_PHADJ_REG_OUT0_PHSINIT_MASK  GENMASK(4, 2)
/** @brief Bit shift for MPFS_PLL_PLL_PHADJ_REG_OUT0_PHSINIT. */
#define MPFS_PLL_PLL_PHADJ_REG_OUT0_PHSINIT_SHIFT 2U

/** @brief Enable synchronous reset of reference dividers. */
#define MPFS_PLL_PLL_PHADJ_SYNCREFDIV_RST_EN_MASK  BIT(1)
/** @brief Bit shift for MPFS_PLL_PLL_PHADJ_SYNCREFDIV_RST_EN. */
#define MPFS_PLL_PLL_PHADJ_SYNCREFDIV_RST_EN_SHIFT 1U

/** @brief Enable synchronizing reference dividers. */
#define MPFS_PLL_PLL_PHADJ_SYNCREFDIV_EN_MASK  BIT(0)
/** @brief Bit shift for MPFS_PLL_PLL_PHADJ_SYNCREFDIV_EN. */
#define MPFS_PLL_PLL_PHADJ_SYNCREFDIV_EN_SHIFT 0U

/* -------------------------------------------------------------------------- */
/* SSCG_REG_0 fields                                                          */
/* -------------------------------------------------------------------------- */
/** @brief Input fractional divide value. */
#define MPFS_PLL_SSCG_REG_0_FRACIN_MASK  GENMASK(29, 6)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_0_FRACIN. */
#define MPFS_PLL_SSCG_REG_0_FRACIN_SHIFT 6U

/** @brief SSCG modulation frequency divider. */
#define MPFS_PLL_SSCG_REG_0_DIVVAL_MASK  GENMASK(5, 0)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_0_DIVVAL. */
#define MPFS_PLL_SSCG_REG_0_DIVVAL_SHIFT 0U

/* -------------------------------------------------------------------------- */
/* SSCG_REG_1 fields                                                          */
/* -------------------------------------------------------------------------- */
/** @brief Modulated fractional divide value (status). */
#define MPFS_PLL_SSCG_REG_1_FRACMOD_MASK  GENMASK(29, 6)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_1_FRACMOD. */
#define MPFS_PLL_SSCG_REG_1_FRACMOD_SHIFT 6U

/** @brief Spread-spectrum modulation depth. */
#define MPFS_PLL_SSCG_REG_1_SSMD_MASK  GENMASK(5, 1)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_1_SSMD. */
#define MPFS_PLL_SSCG_REG_1_SSMD_SHIFT 1U

/** @brief Select center-spread or down-spread mode. */
#define MPFS_PLL_SSCG_REG_1_DOWNSPREAD_MASK  BIT(0)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_1_DOWNSPREAD. */
#define MPFS_PLL_SSCG_REG_1_DOWNSPREAD_SHIFT 0U

/* -------------------------------------------------------------------------- */
/* SSCG_REG_2 fields                                                          */
/* -------------------------------------------------------------------------- */
/** @brief Modulated integer divide value (status). */
#define MPFS_PLL_SSCG_REG_2_INTMOD_MASK  GENMASK(23, 12)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_2_INTMOD. */
#define MPFS_PLL_SSCG_REG_2_INTMOD_SHIFT 12U

/** @brief Input integer divide value. */
#define MPFS_PLL_SSCG_REG_2_INTIN_MASK  GENMASK(11, 0)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_2_INTIN. */
#define MPFS_PLL_SSCG_REG_2_INTIN_SHIFT 0U

/* -------------------------------------------------------------------------- */
/* SSCG_REG_3 fields                                                          */
/* -------------------------------------------------------------------------- */
/** @brief Pseudo-noise pattern selector. */
#define MPFS_PLL_SSCG_REG_3_RANDOM_SEL_MASK  GENMASK(21, 20)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_3_RANDOM_SEL. */
#define MPFS_PLL_SSCG_REG_3_RANDOM_SEL_SHIFT 20U

/** @brief High-pass filter enable for pseudo-noise source. */
#define MPFS_PLL_SSCG_REG_3_RANDOM_FILTER_MASK  BIT(19)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_3_RANDOM_FILTER. */
#define MPFS_PLL_SSCG_REG_3_RANDOM_FILTER_SHIFT 19U

/** @brief External wave table address output (status). */
#define MPFS_PLL_SSCG_REG_3_TBLADDR_MASK  GENMASK(18, 11)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_3_TBLADDR. */
#define MPFS_PLL_SSCG_REG_3_TBLADDR_SHIFT 11U

/** @brief External wave table maximum address input. */
#define MPFS_PLL_SSCG_REG_3_EXT_MAXADDR_MASK  GENMASK(10, 3)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_3_EXT_MAXADDR. */
#define MPFS_PLL_SSCG_REG_3_EXT_MAXADDR_SHIFT 3U

/** @brief Select internal table / external table / pseudo-random source. */
#define MPFS_PLL_SSCG_REG_3_SEL_EXTWAVE_MASK  GENMASK(2, 1)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_3_SEL_EXTWAVE. */
#define MPFS_PLL_SSCG_REG_3_SEL_EXTWAVE_SHIFT 1U

/** @brief Spread-spectrum bypass control. */
#define MPFS_PLL_SSCG_REG_3_SSE_B_MASK  BIT(0)
/** @brief Bit shift for MPFS_PLL_SSCG_REG_3_SSE_B. */
#define MPFS_PLL_SSCG_REG_3_SSE_B_SHIFT 0U

/*
 * FRACIN is a Q24 fractional component in the feedback divider math:
 * feedback_div_scaled = (INTIN << Q24) + FRACIN, then divide by 2^Q24.
 */
/** @brief Bit shift for MPFS_PLL_FRACIN_Q24. */
#define MPFS_PLL_FRACIN_Q24_SHIFT 24U
/** @brief Constant value for MPFS_PLL_FIXED_DIV. */
#define MPFS_PLL_FIXED_DIV        4U

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MPFS_PLL_H_ */
