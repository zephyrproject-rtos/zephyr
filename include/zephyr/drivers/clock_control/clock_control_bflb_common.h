/*
 * Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Contains short functions relevant to timing and clocks common to all Bouffalolab platforms */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_BFLB_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_BFLB_COMMON_H_

/* Function that busy waits for a few cycles */
static inline void clock_bflb_settle(void)
{
	__asm__ volatile (".rept 20 ; nop ; .endr");
}

/* Common main clock mux
 *
 * 32 Mhz Oscillator: 0 (using XCLK)
 * crystal: 1 (using XCLK)
 * PLL and 32M: 2 (using PLL mux, XCLK is 32M)
 * PLL and crystal: 3 (using PLL mux, XCLK is crystal)
 */
static inline void clock_bflb_set_root_clock(uint32_t clock)
{
	uint32_t tmp;

	/* invalid value, fallback to internal 32M */
	if (clock > 3) {
		clock = 0;
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
	return (((tmp & HBN_ROOT_CLK_SEL_MSK) >> HBN_ROOT_CLK_SEL_POS) & 0x3);
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_BFLB_COMMON_H_ */
