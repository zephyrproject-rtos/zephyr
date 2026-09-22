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

/** Bit mask for clock module field (3 bits) */
#define ALIF_CLOCK_MODULE_MASK         0x7U
/** Bit shift for clock module field */
#define ALIF_CLOCK_MODULE_SHIFT        0U
/** Bit mask for register offset field (8 bits) */
#define ALIF_CLOCK_REG_MASK            0xFFU
/** Bit shift for register offset field */
#define ALIF_CLOCK_REG_SHIFT           3U
/** Bit mask for enable bit position field (5 bits) */
#define ALIF_CLOCK_EN_BIT_POS_MASK     0x1FU
/** Bit shift for enable bit position field */
#define ALIF_CLOCK_EN_BIT_POS_SHIFT    11U
/** Bit shift for enable mask field */
#define ALIF_CLOCK_EN_MASK_SHIFT       16U
/** Bit mask for clock source selection value (2 bits) */
#define ALIF_CLOCK_SRC_VAL_MASK            0x3U
/** Bit shift for clock source selection value */
#define ALIF_CLOCK_SRC_VAL_SHIFT           17U
/** Bit mask for clock source selection field width (2 bits) */
#define ALIF_CLOCK_SRC_FIELD_WIDTH_MASK    0x3U
/** Bit shift for clock source selection field width */
#define ALIF_CLOCK_SRC_FIELD_WIDTH_SHIFT   19U
/** Bit mask for clock source selection field position (5 bits) */
#define ALIF_CLOCK_SRC_FIELD_POS_MASK      0x1FU
/** Bit shift for clock source selection field position */
#define ALIF_CLOCK_SRC_FIELD_POS_SHIFT     21U
/** Bit mask for parent clock identifier field (5 bits) */
#define ALIF_CLOCK_PARENT_CLK_MASK     0x1FU
/** Bit shift for parent clock identifier field */
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
 * - src_val        [ 17 : 18 ] - Clock source selection value to write (0-3)
 * - src_field_width [ 19 : 20 ] - Clock source field width in bits (0-3, 0=no selection)
 * - src_field_pos   [ 21 : 25 ] - Clock source field bit position (0-31)
 * - parent_clk   [ 26 : 30 ] - Parent clock source identifier (0-31)
 * - reserved     [ 31 ]      - Reserved for future use
 *
 * @param module    Clock module (CGU, CLKCTL_PER_MST, CLKCTL_PER_SLV, etc.)
 * @param reg       Register name (will be expanded to ALIF_<reg>_REG)
 * @param en_bit    Clock enable bit position (0-31)
 * @param en_mask   1 if clock has enable control, 0 if always enabled
 * @param src_val        Clock source value to write (0-3)
 * @param src_field_width Field width for source selection (0=no selection, 1-3=field width in bits)
 * @param src_field_pos   Clock source selection field bit position (0-31)
 * @param parent_clk Parent clock source identifier (0-31, see ALIF_PARENT_CLK_*)
 */
#define ALIF_CLK_CFG(module, reg, en_bit, en_mask, src_val,                                        \
			     src_field_width, src_field_pos, parent_clk)                           \
	((((ALIF_##module##_MODULE) & ALIF_CLOCK_MODULE_MASK) << ALIF_CLOCK_MODULE_SHIFT) |        \
	 (((ALIF_##reg##_REG) & ALIF_CLOCK_REG_MASK) << ALIF_CLOCK_REG_SHIFT) |                    \
	 (((en_bit) & ALIF_CLOCK_EN_BIT_POS_MASK) << ALIF_CLOCK_EN_BIT_POS_SHIFT) |                \
	 (((en_mask) & 0x1U) << ALIF_CLOCK_EN_MASK_SHIFT) |                                        \
	 (((src_val) & ALIF_CLOCK_SRC_VAL_MASK) << ALIF_CLOCK_SRC_VAL_SHIFT) |                     \
	 (((src_field_width) & ALIF_CLOCK_SRC_FIELD_WIDTH_MASK) <<                                 \
	  ALIF_CLOCK_SRC_FIELD_WIDTH_SHIFT) |                                                      \
	 (((src_field_pos) & ALIF_CLOCK_SRC_FIELD_POS_MASK) << ALIF_CLOCK_SRC_FIELD_POS_SHIFT) |   \
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
