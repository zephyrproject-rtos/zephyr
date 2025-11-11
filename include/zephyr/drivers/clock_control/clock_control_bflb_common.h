/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Contains short functions relevant to timing and clocks common to all Bouffalolab platforms */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_BFLB_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_BFLB_COMMON_H_

/* Common main clock mux (clock_bflb_set_root_clock) */
/* XCLK is RC32M, main clock is XCLK */
#define BFLB_MAIN_CLOCK_RC32M     0
/* XCLK is Crystal, main clock is XCLK */
#define BFLB_MAIN_CLOCK_XTAL      1
/* XCLK is RC32M, main clock is PLL */
#define BFLB_MAIN_CLOCK_PLL_RC32M 2
/* XCLK is Crystal, main clock is PLL */
#define BFLB_MAIN_CLOCK_PLL_XTAL  3

/* Function that busy waits for a few cycles */
static inline void clock_bflb_settle(void)
{
	__asm__ volatile (".rept 20 ; nop ; .endr");
}

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

static inline uint32_t clock_bflb_get_root_clock(void)
{
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	return ((tmp & HBN_ROOT_CLK_SEL_MSK) >> HBN_ROOT_CLK_SEL_POS);
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_BFLB_COMMON_H_ */
