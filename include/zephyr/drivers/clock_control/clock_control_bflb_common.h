/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Timing and clock helpers common to all Bouffalo Lab platforms.
 * @ingroup clock_control_bflb
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_BFLB_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_BFLB_COMMON_H_

/**
 * @defgroup clock_control_bflb Bouffalo Lab
 * @ingroup clock_control_interface_ext
 * @{
 */

/** @name Main clock mux selections (see clock_bflb_set_root_clock()) */
/** @{ */
/** XCLK is RC32M, main clock is XCLK. */
#define BFLB_MAIN_CLOCK_RC32M     0
/** XCLK is the crystal, main clock is XCLK. */
#define BFLB_MAIN_CLOCK_XTAL      1
/** XCLK is RC32M, main clock is the PLL. */
#define BFLB_MAIN_CLOCK_PLL_RC32M 2
/** XCLK is the crystal, main clock is the PLL. */
#define BFLB_MAIN_CLOCK_PLL_XTAL  3
/** @} */

/** @brief Busy-wait for a few CPU cycles to let a clock change settle. */
static inline void clock_bflb_settle(void)
{
	__asm__ volatile (".rept 20 ; nop ; .endr");
}

/**
 * @brief Select the root (main) clock source.
 *
 * @param clock One of the @c BFLB_MAIN_CLOCK_* selections. Out-of-range values fall back to
 *              @ref BFLB_MAIN_CLOCK_RC32M.
 */
static inline void clock_bflb_set_root_clock(uint32_t clock)
{
	uint32_t tmp;

	/* invalid value, fallback to internal 32M */
	if (clock > BFLB_MAIN_CLOCK_PLL_XTAL) {
		clock = BFLB_MAIN_CLOCK_RC32M;
	}
	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp = (tmp & HBN_ROOT_CLK_SEL_UMSK) | (clock << HBN_ROOT_CLK_SEL_POS);
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);

	clock_bflb_settle();
}

/**
 * @brief Get the currently selected root (main) clock source.
 *
 * @return One of the @c BFLB_MAIN_CLOCK_* selections.
 */
static inline uint32_t clock_bflb_get_root_clock(void)
{
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	return ((tmp & HBN_ROOT_CLK_SEL_MSK) >> HBN_ROOT_CLK_SEL_POS);
}


/** @cond INTERNAL_HIDDEN */

/* Avoid overflow when calculating multipliers */
#define BFLB_MUL_CLK(_value, _top, _base) \
	(((uint64_t)(_value) * (uint64_t)(_top)) / (uint64_t)(_base))

/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_BFLB_COMMON_H_ */
