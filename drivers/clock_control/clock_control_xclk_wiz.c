/*
 * Copyright (C) 2026 Advanced Micro Devices, Inc. (AMD)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

#include "clock_control_xclk_wiz.h"

LOG_MODULE_REGISTER(xlnx_clk_wiz, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT xlnx_clkx5_wiz_1_0

struct xlnx_clk_wiz_data {
	DEVICE_MMIO_RAM;
};

struct xlnx_clk_wiz_config {
	DEVICE_MMIO_ROM;
	uint64_t prim_in_freq_hz;
	uint32_t num_out_clks;
};

struct clk_wiz_divisors {
	uint32_t m;
	uint32_t d;
	uint32_t o;
};

static inline uint32_t clk_wiz_rd(mm_reg_t base, uint32_t off)
{
	return sys_read32(base + off);
}

static inline void clk_wiz_wr(mm_reg_t base, uint32_t off, uint32_t val)
{
	sys_write32(val, base + off);
}

static inline void clk_wiz_setbits(mm_reg_t base, uint32_t off, uint32_t mask)
{
	clk_wiz_wr(base, off, clk_wiz_rd(base, off) | mask);
}

static inline void clk_wiz_clrbits(mm_reg_t base, uint32_t off, uint32_t mask)
{
	clk_wiz_wr(base, off, clk_wiz_rd(base, off) & ~mask);
}

static int clk_wiz_wait_lock(mm_reg_t base)
{
	uint32_t timeout_us = 100U * 1000U;
	uint32_t step_us = 10U;
	uint32_t elapsed = 0U;
	uint32_t status;

	while (elapsed < timeout_us) {
		status = clk_wiz_rd(base, CLK_WIZ_STATUS_OFFSET);
		if (status & CLK_WIZ_LOCKED) {
			return 0;
		}
		k_busy_wait(step_us);
		elapsed += step_us;
	}

	status = clk_wiz_rd(base, CLK_WIZ_STATUS_OFFSET);
	LOG_ERR("MMCM lock timeout after %u ms (status=0x%08x)",
		timeout_us / 1000U, status);
	return -ETIMEDOUT;
}

static uint32_t clk_wiz_clkout_offset(uint32_t clk_id)
{
	uint32_t off;

	if (clk_id < 3U) {
		off = CLK_WIZ_REG3_OFFSET + clk_id * 8U;
	} else {
		off = CLK_WIZ_REG19_OFFSET + clk_id * 8U;
	}
	return off;
}

static int clk_wiz_calc_divisors(uint64_t in_hz, uint64_t target_hz,
				 struct clk_wiz_divisors *divs)
{
	uint64_t best_diff = UINT64_MAX;
	uint32_t m, d, o;

	if (target_hz == 0U || in_hz == 0U) {
		return -EINVAL;
	}

	for (m = CLK_WIZ_M_MIN; m <= CLK_WIZ_M_MAX; m++) {
		for (d = CLK_WIZ_D_MIN; d <= CLK_WIZ_D_MAX; d++) {
			uint64_t fvco = in_hz * m / d;

			if (fvco > CLK_WIZ_VCO_MAX_HZ) {
				continue;
			}
			if (fvco < CLK_WIZ_VCO_MIN_HZ) {
				break;
			}

			for (o = CLK_WIZ_O_MIN; o <= CLK_WIZ_O_MAX; o++) {
				uint64_t freq = fvco / o;
				uint64_t diff = (freq > target_hz) ?
						(freq - target_hz) :
						(target_hz - freq);

				if (diff < best_diff) {
					best_diff = diff;
					divs->m = m;
					divs->d = d;
					divs->o = o;
					if (diff == 0U) {
						return 0;
					}
				}
			}
		}
	}

	if (best_diff == UINT64_MAX) {
		return -EINVAL;
	}

	return 0;
}

static void clk_wiz_update_o(mm_reg_t base, uint32_t clk_id, uint32_t oval)
{
	uint32_t reg_off = clk_wiz_clkout_offset(clk_id);
	uint32_t high_time, div_edge, p5_en, p5_fedge, reg;

	if (oval > CLK_WIZ_O_MAX) {
		oval = CLK_WIZ_O_MAX;
	}

	high_time = oval / 4U;
	div_edge  = (oval % 4U <= 1U) ? 0U : 1U;
	p5_en     = oval % 2U;
	p5_fedge  = oval % 2U;

	reg = CLK_WIZ_REG3_PREDIV2 | CLK_WIZ_REG3_USED | CLK_WIZ_REG3_MX;
	reg |= (div_edge << 8U);
	reg |= (p5_en    << CLK_WIZ_P5EN_SHIFT);
	reg |= (p5_fedge << CLK_WIZ_P5FEDGE_SHIFT);
	clk_wiz_wr(base, reg_off, reg);

	reg = high_time | (high_time << CLK_WIZ_H_SHIFT);
	clk_wiz_wr(base, reg_off + 4U, reg);
}

static void clk_wiz_update_d(mm_reg_t base, uint32_t dval)
{
	uint32_t high_time = dval / 2U;
	uint32_t div_edge  = dval % 2U;
	uint32_t reg;

	reg = div_edge << CLK_WIZ_REG12_EDGE_SHIFT;
	clk_wiz_wr(base, CLK_WIZ_REG12_OFFSET, reg);

	reg = high_time | (high_time << CLK_WIZ_H_SHIFT);
	clk_wiz_wr(base, CLK_WIZ_REG13_OFFSET, reg);
}

static void clk_wiz_cp_res_lookup(uint32_t mval, uint32_t *cp, uint32_t *res)
{
	if (mval == 4U) {
		*cp = 5U;  *res = 15U;
	} else if (mval == 5U) {
		*cp = 6U;  *res = 15U;
	} else if (mval == 6U) {
		*cp = 7U;  *res = 15U;
	} else if (mval == 7U) {
		*cp = 13U; *res = 15U;
	} else if (mval == 8U) {
		*cp = 14U; *res = 15U;
	} else if (mval == 9U) {
		*cp = 15U; *res = 15U;
	} else if (mval == 10U) {
		*cp = 14U; *res = 7U;
	} else if (mval == 11U) {
		*cp = 15U; *res = 7U;
	} else if (mval <= 13U) {
		*cp = 15U; *res = 11U;
	} else if (mval == 14U) {
		*cp = 15U; *res = 13U;
	} else if (mval == 15U) {
		*cp = 15U; *res = 3U;
	} else if (mval <= 17U) {
		*cp = 14U; *res = 5U;
	} else if (mval <= 19U) {
		*cp = 15U; *res = 5U;
	} else if (mval <= 21U) {
		*cp = 15U; *res = 9U;
	} else if (mval <= 23U) {
		*cp = 14U; *res = 14U;
	} else if (mval <= 26U) {
		*cp = 15U; *res = 14U;
	} else if (mval <= 28U) {
		*cp = 14U; *res = 1U;
	} else if (mval <= 33U) {
		*cp = 15U; *res = 1U;
	} else if (mval <= 37U) {
		*cp = 14U; *res = 6U;
	} else if (mval <= 44U) {
		*cp = 15U; *res = 6U;
	} else if (mval <= 57U) {
		*cp = 15U; *res = 10U;
	} else if (mval <= 63U) {
		*cp = 13U; *res = 12U;
	} else if (mval <= 70U) {
		*cp = 14U; *res = 12U;
	} else if (mval <= 86U) {
		*cp = 15U; *res = 12U;
	} else if (mval <= 94U) {
		*cp = 14U; *res = 2U;
	} else if (mval <= 145U) {
		*cp = 15U; *res = 2U;
	} else if (mval <= 163U) {
		*cp = 12U; *res = 4U;
	} else if (mval <= 181U) {
		*cp = 13U; *res = 4U;
	} else if (mval <= 200U) {
		*cp = 14U; *res = 4U;
	} else if (mval <= 273U) {
		*cp = 15U; *res = 4U;
	} else if (mval <= 300U) {
		*cp = 13U; *res = 8U;
	} else if (mval <= 325U) {
		*cp = 14U; *res = 8U;
	} else if (mval <= 432U) {
		*cp = 15U; *res = 8U;
	}
}

static void clk_wiz_lock_lookup(uint32_t mval, uint32_t *lock_cnt,
				uint32_t *lock_ref_dly, uint32_t *lock_fb_dly)
{
	*lock_ref_dly = 16U;
	*lock_fb_dly  = 16U;

	if (mval == 4U) {
		*lock_cnt = 1000U; *lock_ref_dly = 4U; *lock_fb_dly = 4U;
	} else if (mval == 5U) {
		*lock_cnt = 1000U; *lock_ref_dly = 6U; *lock_fb_dly = 6U;
	} else if (mval <= 7U) {
		*lock_cnt = 1000U; *lock_ref_dly = 7U; *lock_fb_dly = 7U;
	} else if (mval == 8U) {
		*lock_cnt = 1000U; *lock_ref_dly = 7U; *lock_fb_dly = 7U;
	} else if (mval <= 12U) {
		*lock_cnt = 1000U; *lock_ref_dly = 8U; *lock_fb_dly = 8U;
	} else if (mval == 13U) {
		*lock_cnt = 1000U; *lock_ref_dly = 10U; *lock_fb_dly = 10U;
	} else if (mval <= 16U) {
		*lock_cnt = 1000U; *lock_ref_dly = 13U; *lock_fb_dly = 13U;
	} else if (mval == 17U) {
		*lock_cnt = 825U;
	} else if (mval == 18U) {
		*lock_cnt = 750U;
	} else if (mval <= 20U) {
		*lock_cnt = 700U;
	} else if (mval == 21U) {
		*lock_cnt = 650U;
	} else if (mval <= 23U) {
		*lock_cnt = 625U;
	} else if (mval == 24U) {
		*lock_cnt = 575U;
	} else if (mval == 25U) {
		*lock_cnt = 550U;
	} else if (mval <= 28U) {
		*lock_cnt = 525U;
	} else if (mval <= 30U) {
		*lock_cnt = 475U;
	} else if (mval == 31U) {
		*lock_cnt = 450U;
	} else if (mval <= 33U) {
		*lock_cnt = 425U;
	} else if (mval <= 36U) {
		*lock_cnt = 400U;
	} else if (mval == 37U) {
		*lock_cnt = 375U;
	} else if (mval <= 40U) {
		*lock_cnt = 350U;
	} else if (mval <= 43U) {
		*lock_cnt = 325U;
	} else if (mval <= 47U) {
		*lock_cnt = 300U;
	} else if (mval <= 51U) {
		*lock_cnt = 275U;
	} else if (mval <= 205U) {
		*lock_cnt = 250U;
	} else if (mval <= 432U) {
		*lock_cnt = 225U;
	}
}

static void clk_wiz_update_m(mm_reg_t base, uint32_t mval)
{
	uint32_t high_time;
	uint32_t div_edge;
	uint32_t cp = 15, res = 15, lock_cnt = 250;
	uint32_t lock_ref_dly = 16, lock_fb_dly = 16;
	uint32_t reg;

	clk_wiz_wr(base, CLK_WIZ_REG25_OFFSET, 0U);
	high_time = mval / 2U;
	div_edge  = mval % 2U;
	reg = high_time | (high_time << CLK_WIZ_H_SHIFT);
	clk_wiz_wr(base, CLK_WIZ_REG2_OFFSET, reg);

	reg = CLK_WIZ_REG1_PREDIV2 | CLK_WIZ_REG1_EN | CLK_WIZ_REG1_MX;
	if (div_edge) {
		reg |= (1U << CLK_WIZ_REG1_EDGE_SHIFT);
	} else {
		reg &= ~(1U << CLK_WIZ_REG1_EDGE_SHIFT);
	}
	clk_wiz_wr(base, CLK_WIZ_REG1_OFFSET, reg);

	clk_wiz_cp_res_lookup(mval, &cp, &res);
	reg = clk_wiz_rd(base, CLK_WIZ_REG11_OFFSET);
	reg &= ~CLK_WIZ_REG11_CP_MASK;
	reg |= (cp & CLK_WIZ_REG11_CP_MASK);
	clk_wiz_wr(base, CLK_WIZ_REG11_OFFSET, reg);

	reg = clk_wiz_rd(base, CLK_WIZ_REG17_OFFSET);
	reg &= ~(CLK_WIZ_REG17_RES_MASK << CLK_WIZ_REG17_RES_SHIFT);
	reg |= ((res & CLK_WIZ_REG17_RES_MASK) << CLK_WIZ_REG17_RES_SHIFT);
	clk_wiz_wr(base, CLK_WIZ_REG17_OFFSET, reg);

	clk_wiz_lock_lookup(mval, &lock_cnt, &lock_ref_dly, &lock_fb_dly);

	reg = lock_cnt | (lock_fb_dly << CLK_WIZ_LOCK_FB_DLY_SHIFT);
	clk_wiz_wr(base, CLK_WIZ_REG15_OFFSET, reg);

	reg = clk_wiz_rd(base, CLK_WIZ_REG16_OFFSET);
	reg &= ~(0x1FU << CLK_WIZ_LOCK_REF_DLY_SHIFT);
	reg |= (lock_ref_dly << CLK_WIZ_LOCK_REF_DLY_SHIFT);
	clk_wiz_wr(base, CLK_WIZ_REG16_OFFSET, reg);
}

static uint64_t clk_wiz_get_vco(mm_reg_t base, uint64_t in_hz)
{
	uint32_t reg;
	uint32_t edge, low, high, mult, div_val;
	uint64_t fvco;

	reg  = clk_wiz_rd(base, CLK_WIZ_REG1_OFFSET);
	edge = !!(reg &  XCLK_WIZ_REG1_EDGE_MASK);
	reg  = clk_wiz_rd(base, CLK_WIZ_REG2_OFFSET);
	low  = reg & CLK_WIZ_L_MASK;
	high = (reg & CLK_WIZ_H_MASK) >> CLK_WIZ_H_SHIFT;
	mult = low + high + edge;

	reg     = clk_wiz_rd(base, CLK_WIZ_REG13_OFFSET);
	low     = reg & CLK_WIZ_L_MASK;
	high    = (reg & CLK_WIZ_H_MASK) >> CLK_WIZ_H_SHIFT;
	reg     = clk_wiz_rd(base, CLK_WIZ_REG12_OFFSET);
	edge    = !!(reg & CLK_WIZ_REG12_EDGE_MASK);
	div_val = low + high + edge;

	if (mult == 0U) {
		mult = 1U;
	}
	if (div_val == 0U) {
		div_val = 1U;
	}

	fvco = in_hz * mult / div_val;
	return fvco;
}

static int xlnx_clk_wiz_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct xlnx_clk_wiz_config *cfg = dev->config;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t clk_id = (uint32_t)(uintptr_t)sys;
	uint32_t reg_off;

	if (clk_id >= cfg->num_out_clks || clk_id > CLK_WIZ_MAX_OUTPUTS) {
		LOG_ERR("Invalid clock ID %u (max %u)", clk_id,
			MIN(cfg->num_out_clks, CLK_WIZ_MAX_OUTPUTS) - 1U);
		return -EINVAL;
	}

	reg_off = clk_wiz_clkout_offset(clk_id);
	clk_wiz_setbits(base, reg_off, CLK_WIZ_REG3_USED);

	LOG_DBG("Clock %u enabled", clk_id);
	return 0;
}

static int xlnx_clk_wiz_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct xlnx_clk_wiz_config *cfg = dev->config;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t clk_id = (uint32_t)(uintptr_t)sys;
	uint32_t reg_off;

	if (clk_id >= cfg->num_out_clks || clk_id > CLK_WIZ_MAX_OUTPUTS) {
		LOG_ERR("Invalid clock ID %u", clk_id);
		return -EINVAL;
	}

	reg_off = clk_wiz_clkout_offset(clk_id);
	clk_wiz_clrbits(base, reg_off, CLK_WIZ_REG3_USED);

	LOG_DBG("Clock %u disabled", clk_id);
	return 0;
}

static int xlnx_clk_wiz_get_rate(const struct device *dev,
				 clock_control_subsys_t sys,
				 uint32_t *rate)
{
	const struct xlnx_clk_wiz_config *cfg = dev->config;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t clk_id = (uint32_t)(uintptr_t)sys;
	uint32_t reg_off, reg;
	uint32_t edge, low, high, leaf, prediv, p5en, div_o;
	uint64_t fvco, freq;

	if (clk_id >= cfg->num_out_clks || clk_id > CLK_WIZ_MAX_OUTPUTS) {
		LOG_ERR("Invalid clock ID %u", clk_id);
		return -EINVAL;
	}

	fvco = clk_wiz_get_vco(base, cfg->prim_in_freq_hz);
	if (fvco == 0U) {
		return -EIO;
	}

	reg_off = clk_wiz_clkout_offset(clk_id);

	reg    = clk_wiz_rd(base, reg_off);
	edge   = !!(reg & CLK_WIZ_REG3_EDGE_MASK);
	p5en   = !!(reg & CLK_WIZ_REG3_P5EN_MASK);
	prediv = !!(reg & CLK_WIZ_REG3_PREDIV2);

	reg  = clk_wiz_rd(base, reg_off + 4U);
	low  = reg & CLK_WIZ_L_MASK;
	high = (reg & CLK_WIZ_H_MASK) >> CLK_WIZ_H_SHIFT;
	leaf = high + low + edge;

	div_o = (prediv + 1U) * leaf + (prediv * p5en);
	if (div_o == 0U) {
		div_o = 1U;
	}

	freq  = fvco / div_o;
	*rate = (uint32_t)MIN(freq, (uint64_t)UINT32_MAX);

	LOG_DBG("Clock %u rate: %u Hz (fvco=%llu div_o=%u)", clk_id, *rate,
		(unsigned long long)fvco, div_o);
	return 0;
}

static int xlnx_clk_wiz_set_rate(const struct device *dev,
				 clock_control_subsys_t sys,
				 clock_control_subsys_rate_t rate)
{
	const struct xlnx_clk_wiz_config *cfg = dev->config;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t clk_id = (uint32_t)(uintptr_t)sys;
	const uint32_t *rate_hz_ptr = (const uint32_t *)rate;
	uint64_t target_hz;
	struct clk_wiz_divisors clk_divs = {0};
	int ret;

	if (clk_id >= cfg->num_out_clks || clk_id > CLK_WIZ_MAX_OUTPUTS) {
		LOG_ERR("Invalid clock ID %u", clk_id);
		return -EINVAL;
	}
	if (!rate_hz_ptr) {
		return -EINVAL;
	}

	target_hz = (uint64_t)*rate_hz_ptr;
	if (target_hz == 0U) {
		return -EINVAL;
	}

	ret = clk_wiz_calc_divisors(cfg->prim_in_freq_hz, target_hz, &clk_divs);
	if (ret != 0) {
		LOG_ERR("No valid M/D/O found for %llu Hz", (unsigned long long)target_hz);
		return ret;
	}

	LOG_DBG("Clock %u set_rate %llu Hz → M=%u D=%u O=%u", clk_id,
		(unsigned long long)target_hz, clk_divs.m, clk_divs.d, clk_divs.o);

	clk_wiz_update_o(base, clk_id, clk_divs.o);

	clk_wiz_update_d(base, clk_divs.d);
	clk_wiz_update_m(base, clk_divs.m);

	LOG_DBG("Clock %u M/D/O programmed, waiting for lock: M=%u D=%u O=%u",
		clk_id, clk_divs.m, clk_divs.d, clk_divs.o);

	clk_wiz_wr(base, CLK_WIZ_RECONFIG_OFFSET,
		   (CLK_WIZ_RECONFIG_SADDR | CLK_WIZ_RECONFIG_LOAD));
	ret = clk_wiz_wait_lock(base);
	return ret;
}

static int xlnx_clk_wiz_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return 0;
}

static DEVICE_API(clock_control, xlnx_clk_wiz_api) = {
	.on       = xlnx_clk_wiz_on,
	.off      = xlnx_clk_wiz_off,
	.get_rate = xlnx_clk_wiz_get_rate,
	.set_rate = xlnx_clk_wiz_set_rate,
};

#define CLK_WIZ_DEVICE_INIT(inst)                                              \
	static struct xlnx_clk_wiz_data xlnx_clk_wiz_data_##inst;              \
	                                                                          \
	static const struct xlnx_clk_wiz_config xlnx_clk_wiz_config_##inst = { \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                           \
		.prim_in_freq_hz = DT_INST_PROP(inst, xlnx_prim_in_freq),          \
		.num_out_clks = DT_INST_PROP(inst, xlnx_num_out_clks),             \
	};                                                                        \
	                                                                          \
	DEVICE_DT_INST_DEFINE(inst, xlnx_clk_wiz_init, NULL,                    \
			      &xlnx_clk_wiz_data_##inst,                         \
			      &xlnx_clk_wiz_config_##inst, POST_KERNEL,          \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                \
			      &xlnx_clk_wiz_api);

DT_INST_FOREACH_STATUS_OKAY(CLK_WIZ_DEVICE_INIT)
