/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ALIF_CLOCKS_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ALIF_CLOCKS_COMMON_H_

/**
 * @file
 * @brief Common clock definitions for Alif Semiconductor SoC families
 *
 * This header defines the clock ID encoding scheme and common definitions
 * shared across all Alif SoC families.
 *
 * The encoding scheme is generic and supports:
 * - Up to 8 clock modules (3-bit field)
 * - Up to 32 input clock sources (5-bit field)
 * - Per clock parent clock source selection
 * - Per-clock enable/disable control
 */

/**
 * @name Clock ID encoding bit field definitions
 * @{
 */

#define ALIF_CLOCK_MODULE_MASK         0x7U
#define ALIF_CLOCK_MODULE_SHIFT        0U
#define ALIF_CLOCK_REG_MASK            0xFFU
#define ALIF_CLOCK_REG_SHIFT           3U
#define ALIF_CLOCK_EN_BIT_POS_MASK     0x1FU
#define ALIF_CLOCK_EN_BIT_POS_SHIFT    11U
#define ALIF_CLOCK_EN_MASK_SHIFT       16U
#define ALIF_CLOCK_SRC_MASK            0x3U
#define ALIF_CLOCK_SRC_SHIFT           17U
#define ALIF_CLOCK_SRC_MASK_MASK       0x3U
#define ALIF_CLOCK_SRC_MASK_SHIFT      19U
#define ALIF_CLOCK_SRC_BIT_POS_MASK    0x1FU
#define ALIF_CLOCK_SRC_BIT_POS_SHIFT   21U
#define ALIF_CLOCK_PARENT_CLK_MASK     0x1FU
#define ALIF_CLOCK_PARENT_CLK_SHIFT    26U

/** @} */

/**
 * @brief ALIF clock configuration bit field encoding
 *
 * This macro encodes all clock control information into a single 32-bit value
 * that can be passed through device tree to the clock driver.
 *
 * Bit field layout:
 * - module       [ 0 : 2 ]   - Clock module ID (0-7, see ALIF_*_MODULE)
 * - reg          [ 3 : 10 ]  - Register offset within module (0-255)
 * - en_bit       [ 11 : 15 ] - Bit position for enable control (0-31)
 * - en_mask      [ 16 ]      - 1 if enable control exists, 0 if always-on
 * - src          [ 17 : 18 ] - Clock source selection value (0-3)
 * - src_mask     [ 19 : 20 ] - Mask for source selection (0-3, 0=no selection)
 * - src_bit      [ 21 : 25 ] - Bit position for source selection (0-31)
 * - parent_clk   [ 26 : 30 ] - Parent clock source identifier (0-31)
 * - reserved     [ 31 ]      - Reserved for future use
 *
 * @param module    Clock module (CGU, CLKCTL_PER_MST, CLKCTL_PER_SLV, etc.)
 * @param reg       Register name (will be expanded to ALIF_<reg>_REG)
 * @param en_bit    Clock enable bit position (0-31)
 * @param en_mask   1 if clock has enable control, 0 if always enabled
 * @param src       Clock source value to write (0-3)
 * @param src_mask  Mask for source field (0=no source select, 1-3=field width)
 * @param src_bit   Clock source selection bit position (0-31)
 * @param parent_clk Parent clock source identifier (0-31, see ALIF_PARENT_CLK_*)
 */
#define ALIF_CLK_CFG(module, reg, en_bit, en_mask, src, src_mask, src_bit, parent_clk)      \
	((((ALIF_##module##_MODULE) & ALIF_CLOCK_MODULE_MASK) << ALIF_CLOCK_MODULE_SHIFT) |        \
	 (((ALIF_##reg##_REG) & ALIF_CLOCK_REG_MASK) << ALIF_CLOCK_REG_SHIFT) |                    \
	 (((en_bit) & ALIF_CLOCK_EN_BIT_POS_MASK) << ALIF_CLOCK_EN_BIT_POS_SHIFT) |                \
	 (((en_mask) & 0x1U) << ALIF_CLOCK_EN_MASK_SHIFT) |                                        \
	 (((src) & ALIF_CLOCK_SRC_MASK) << ALIF_CLOCK_SRC_SHIFT) |                                 \
	 (((src_mask) & ALIF_CLOCK_SRC_MASK_MASK) << ALIF_CLOCK_SRC_MASK_SHIFT) |                  \
	 (((src_bit) & ALIF_CLOCK_SRC_BIT_POS_MASK) << ALIF_CLOCK_SRC_BIT_POS_SHIFT) |             \
	 (((parent_clk) & ALIF_CLOCK_PARENT_CLK_MASK) << ALIF_CLOCK_PARENT_CLK_SHIFT))

/**
 * @name Parent clock source identifiers
 * @{
 */

/** System AXI clock (SYST_ACLK) */
#define ALIF_PARENT_CLK_SYST_ACLK   0x0U
/** System AHB clock (SYST_HCLK) */
#define ALIF_PARENT_CLK_SYST_HCLK   0x1U
/** System APB clock (SYST_PCLK) */
#define ALIF_PARENT_CLK_SYST_PCLK   0x2U

/** @} */

/**
 * @name Clock module identifiers
 *
 * These module IDs are common across Alif SoC families. Each module
 * represents a distinct clock control register block.
 *
 * @{
 */

#define ALIF_CGU_MODULE                 0x0U  /**< Clock Generation Unit */
#define ALIF_CLKCTL_PER_MST_MODULE      0x1U  /**< Peripheral Clock Control (Master) */
#define ALIF_CLKCTL_PER_SLV_MODULE      0x2U  /**< Peripheral Clock Control (Slave) */
#define ALIF_AON_MODULE                 0x3U  /**< Always-On Clock Control */
#define ALIF_VBAT_MODULE                0x4U  /**< VBAT Clock Control */
#define ALIF_M55HE_CFG_MODULE           0x5U  /**< M55 High Efficiency Config */
#define ALIF_M55HP_CFG_MODULE           0x6U  /**< M55 High Performance Config */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ALIF_CLOCKS_COMMON_H_ */
