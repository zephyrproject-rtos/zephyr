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

int32_t clock_set_pll_rate(int32_t id, uint32_t rate);

uint32_t clock_get_pll_rate(int32_t id);

int32_t clock_set_pll_div(int32_t id, uint32_t pll_div);

int32_t clock_set_clk_ctrl_rate(int32_t id, uint32_t rate);

uint32_t clock_get_clk_ctrl_rate(int32_t id);

int32_t clock_is_peri_enabled(int32_t id);

int32_t clock_enable_peri(int32_t id);

int32_t clock_disable_peri(int32_t id);

uint32_t clock_get_peri_rate(int32_t id);

int32_t clock_set_peri_rate(int32_t id, uint32_t rate);

int32_t clock_is_iobus_pwdn(int32_t id);

int32_t clock_enable_iobus(int32_t id, bool en);

int32_t clock_set_iobus_pwdn(int32_t id, bool en);

int32_t clock_set_sw_reset(int32_t id, bool reset);

#endif /* CLOCK_CONTROL_TCC_CCU_HEADER */
