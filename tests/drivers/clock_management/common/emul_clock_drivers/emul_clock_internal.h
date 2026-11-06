/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

/* Data structures shared by virtual clock and mux drivers */

struct emul_clock_mux {
	MUX_CLK_SUBSYS_DATA_DEFINE
	uint8_t src_count;
	uint8_t src_sel;
};

int emul_clock_mux_get_parent(const struct clk *clk_hw);
int emul_clock_mux_configure(const struct clk *clk_hw, const void *mux);
int emul_clock_mux_configure_recalc(const struct clk *clk_hw, const void *mux);
int emul_clock_mux_validate_parent(const struct clk *clk_hw, clock_freq_t parent_freq,
				   uint8_t new_idx);
int emul_clock_mux_set_parent(const struct clk *clk_hw, uint8_t new_idx);

struct emul_clock_div {
	STANDARD_CLK_SUBSYS_DATA_DEFINE
	uint8_t div_max;
	uint8_t div_val;
};

int emul_clock_div_configure(const struct clk *clk_hw, const void *div_cfg);
clock_freq_t emul_clock_div_recalc_rate(const struct clk *clk_hw, clock_freq_t parent_rate);
clock_freq_t emul_clock_div_configure_recalc(const struct clk *clk_hw, const void *div_cfg,
					     clock_freq_t parent_rate);
clock_freq_t emul_clock_div_best_rate(const struct clk *clk_hw, clock_freq_t req_rate,
				      clock_freq_t parent_rate, bool commit);
