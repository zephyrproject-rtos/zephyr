/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXL_CLOCK_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXL_CLOCK_COMMON_H_

/* Fallback for direct C inclusion outside devicetree.h. */
#ifndef DT_FREQ_M
#define DT_FREQ_M(x) ((x) * 1000000)
#endif

/**
 * @file
 * @brief Shared 32-bit clock-specifier layout for NXP MCXL clock controllers.
 *
 * Bit layout (MSB to LSB):
 *   [31:26] gate_idx  symbolic gate index      (0 = none)
 *   [25:20] sel_idx   symbolic selector index  (0 = none)
 *   [19:16] src       mux value for the selector
 *   [15:10] div_idx   symbolic divider index   (0 = none)
 *   [9:2]   div_val   literal divisor passed to CLOCK_SetClockDiv()
 *   [1:0]   flags     see MCXL_CLOCK_FLAG_* bits below
 *
 * Fields carry Zephyr symbolic indices, not raw HAL enum values.
 * The driver translates indices to HAL enums via bounds-checked lookup tables.
 */

/* Shared field shifts and masks. */
#define MCXL_CLOCK_GATE_IDX_SHIFT 26U
#define MCXL_CLOCK_GATE_IDX_MASK  0x3FU
#define MCXL_CLOCK_SEL_IDX_SHIFT  20U
#define MCXL_CLOCK_SEL_IDX_MASK   0x3FU
#define MCXL_CLOCK_SRC_SHIFT      16U
#define MCXL_CLOCK_SRC_MASK       0xFU
#define MCXL_CLOCK_DIV_IDX_SHIFT  10U
#define MCXL_CLOCK_DIV_IDX_MASK   0x3FU
#define MCXL_CLOCK_DIV_VAL_SHIFT  2U
#define MCXL_CLOCK_DIV_VAL_MASK   0xFFU
#define MCXL_CLOCK_FLAGS_SHIFT    0U
#define MCXL_CLOCK_FLAGS_MASK     0x3U

#define MCXL_CLOCK_FLAG_NONE         0U   /* No special clock behaviour. */
#define MCXL_CLOCK_FLAG_FIXED_BIT    0x1U /* Clock is non-gateable; skip enable/disable. */
#define MCXL_CLOCK_FLAG_DIV_HALT_BIT 0x2U /* Halt/bypass the divider instead of setting it. */

/* Divider values */
#define MCXL_CLOCK_DIV_BY_1 0U
#define MCXL_CLOCK_DIV_BY_2 1U
#define MCXL_CLOCK_DIV_BY_3 2U
#define MCXL_CLOCK_DIV_BY_4 3U
#define MCXL_CLOCK_DIV_BY_5 4U
#define MCXL_CLOCK_DIV_BY_6 5U
#define MCXL_CLOCK_DIV_BY_7 6U
#define MCXL_CLOCK_DIV_BY_8 7U

/* Pack all six fields into one 32-bit DT cell. */
#define MCXL_CLOCK_ENCODE(gate_idx, sel_idx, src, div_idx, div_val, flags) \
	((((gate_idx) & MCXL_CLOCK_GATE_IDX_MASK) << MCXL_CLOCK_GATE_IDX_SHIFT) | \
	 (((sel_idx) & MCXL_CLOCK_SEL_IDX_MASK) << MCXL_CLOCK_SEL_IDX_SHIFT) | \
	 (((src) & MCXL_CLOCK_SRC_MASK) << MCXL_CLOCK_SRC_SHIFT) | \
	 (((div_idx) & MCXL_CLOCK_DIV_IDX_MASK) << MCXL_CLOCK_DIV_IDX_SHIFT) | \
	 (((div_val) & MCXL_CLOCK_DIV_VAL_MASK) << MCXL_CLOCK_DIV_VAL_SHIFT) | \
	 (((flags) & MCXL_CLOCK_FLAGS_MASK) << MCXL_CLOCK_FLAGS_SHIFT))

/* Field extraction macros. */
#define MCXL_CLOCK_EXTRACT_GATE_IDX(spec) \
	(((spec) >> MCXL_CLOCK_GATE_IDX_SHIFT) & MCXL_CLOCK_GATE_IDX_MASK)
#define MCXL_CLOCK_EXTRACT_SEL_IDX(spec) \
	(((spec) >> MCXL_CLOCK_SEL_IDX_SHIFT) & MCXL_CLOCK_SEL_IDX_MASK)
#define MCXL_CLOCK_EXTRACT_SRC(spec) \
	(((spec) >> MCXL_CLOCK_SRC_SHIFT) & MCXL_CLOCK_SRC_MASK)
#define MCXL_CLOCK_EXTRACT_DIV_IDX(spec) \
	(((spec) >> MCXL_CLOCK_DIV_IDX_SHIFT) & MCXL_CLOCK_DIV_IDX_MASK)
#define MCXL_CLOCK_EXTRACT_DIV_VAL(spec) \
	(((spec) >> MCXL_CLOCK_DIV_VAL_SHIFT) & MCXL_CLOCK_DIV_VAL_MASK)
#define MCXL_CLOCK_EXTRACT_FLAGS(spec) \
	(((spec) >> MCXL_CLOCK_FLAGS_SHIFT) & MCXL_CLOCK_FLAGS_MASK)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXL_CLOCK_COMMON_H_ */
