/*
 * Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef CLOCK_CONTROL_TCC_CCU_HEADER
#define CLOCK_CONTROL_TCC_CCU_HEADER

/*
 * DEFINITIONS
 *
 */

#define CKC_MHZ(x) (uint32_t)((x) * 1000000)

/*
 * FUNCTION PROTOTYPES
 *
 */

void vcp_clock_init(void);

long clock_set_pll_rate(signed long id, uint32_t rate);

uint32_t clock_get_pll_rate(signed long id);

long clock_set_pll_div(signed long id, uint32_t pll_div);

long clock_set_clk_ctrl_rate(signed long id, uint32_t rate);

uint32_t clock_get_clk_ctrl_rate(signed long id);

long clock_is_peri_enabled(signed long id);

long clock_enable_peri(signed long id);

long clock_disable_peri(signed long id);

uint32_t clock_get_peri_rate(signed long id);

long clock_set_peri_rate(signed long id, uint32_t rate);

long clock_is_iobus_pwdn(signed long id);

long clock_enable_iobus(signed long id, unsigned char en);

long clock_set_iobus_pwdn(signed long id, unsigned char en);

long clock_set_sw_reset(signed long id, unsigned char reset);

#endif /* CLOCK_CONTROL_TCC_CCU_HEADER */
