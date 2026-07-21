/*
 * Copyright (C) 2026 Advanced Micro Devices, Inc. (AMD)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "clock_control_xclk_wiz.h"

LOG_MODULE_REGISTER(xlnx_clk_wiz, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT xlnx_clkx5_wiz_1_0

struct xlnx_clk_wiz_data {
	DEVICE_MMIO_RAM;
	uint64_t prim_in_freq_hz;
	struct k_mutex lock;
};

struct xlnx_clk_wiz_config {
	DEVICE_MMIO_ROM;
	const struct device *parent_clk;
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
	uint32_t timeout_ms = 100U;

	if (WAIT_FOR((clk_wiz_rd(base, CLK_WIZ_STATUS_OFFSET) & CLK_WIZ_LOCKED),
		     timeout_ms * 1000U, k_busy_wait(100U))) {
		return 0;
	}

	uint32_t status = clk_wiz_rd(base, CLK_WIZ_STATUS_OFFSET);

	LOG_ERR("MMCM lock timeout after %u ms (status=0x%08x)",
		timeout_ms, status);
	return -ETIMEDOUT;
}

static uint32_t clk_wiz_clkout_offset(uint32_t clk_id)
{
	uint32_t off;

	if (clk_id < 4U) {
		off = CLK_WIZ_REG3_OFFSET + clk_id * 8U;
	} else {
		off = CLK_WIZ_REG19_OFFSET + (clk_id - 4U) * 8U;
	}
	return off;
}

static bool clk_wiz_update_best(uint64_t diff, uint64_t *best_diff,
				struct clk_wiz_divisors *divs,
				uint32_t m, uint32_t d, uint32_t o)
{
	if (diff >= *best_diff) {
		return false;
	}

	*best_diff = diff;
	divs->m = m;
	divs->d = d;
	divs->o = o;

	return (diff == 0U);
}

static int clk_wiz_calc_divisors(uint64_t in_hz, uint64_t target_hz,
				 struct clk_wiz_divisors *divs)
{
	uint64_t best_diff = UINT64_MAX;

	if (target_hz == 0U || in_hz == 0U) {
		return -EINVAL;
	}

	for (uint32_t m = CLK_WIZ_M_MIN; m <= CLK_WIZ_M_MAX; m++) {
		for (uint32_t d = CLK_WIZ_D_MIN; d <= CLK_WIZ_D_MAX; d++) {
			uint64_t fvco = in_hz * m / d;

			if (fvco > CLK_WIZ_VCO_MAX_HZ) {
				continue;
			}
			if (fvco < CLK_WIZ_VCO_MIN_HZ) {
				break;
			}

			for (uint32_t o = CLK_WIZ_O_MIN; o <= CLK_WIZ_O_MAX; o++) {
				uint64_t freq = fvco / o;
				uint64_t diff = (freq > target_hz) ?
						(freq - target_hz) :
						(target_hz - freq);

				if (clk_wiz_update_best(diff, &best_diff, divs, m, d, o)) {
					return 0;
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
	static const struct {
		uint32_t max_m;
		uint32_t cp;
		uint32_t res;
	} cp_res_tbl[] = {
		{4U,   5U,  15U},
		{5U,   6U,  15U},
		{6U,   7U,  15U},
		{7U,   13U, 15U},
		{8U,   14U, 15U},
		{9U,   15U, 15U},
		{10U,  14U, 7U},
		{11U,  15U, 7U},
		{13U,  15U, 11U},
		{14U,  15U, 13U},
		{15U,  15U, 3U},
		{17U,  14U, 5U},
		{19U,  15U, 5U},
		{21U,  15U, 9U},
		{23U,  14U, 14U},
		{26U,  15U, 14U},
		{28U,  14U, 1U},
		{33U,  15U, 1U},
		{37U,  14U, 6U},
		{44U,  15U, 6U},
		{57U,  15U, 10U},
		{63U,  13U, 12U},
		{70U,  14U, 12U},
		{86U,  15U, 12U},
		{94U,  14U, 2U},
		{145U, 15U, 2U},
		{163U, 12U, 4U},
		{181U, 13U, 4U},
		{200U, 14U, 4U},
		{273U, 15U, 4U},
		{300U, 13U, 8U},
		{325U, 14U, 8U},
		{432U, 15U, 8U},
	};

	for (size_t i = 0U; i < ARRAY_SIZE(cp_res_tbl); i++) {
		if (mval <= cp_res_tbl[i].max_m) {
			*cp = cp_res_tbl[i].cp;
			*res = cp_res_tbl[i].res;
			return;
		}
	}
}

static void clk_wiz_lock_lookup(uint32_t mval, uint32_t *lock_cnt,
				uint32_t *lock_ref_dly, uint32_t *lock_fb_dly)
{
	static const struct {
		uint32_t max_m;
		uint32_t lock_cnt;
		uint32_t ref_dly;
		uint32_t fb_dly;
	} lock_tbl[] = {
		{4U,   1000U, 4U,  4U},
		{5U,   1000U, 6U,  6U},
		{7U,   1000U, 7U,  7U},
		{8U,   1000U, 7U,  7U},
		{12U,  1000U, 8U,  8U},
		{13U,  1000U, 10U, 10U},
		{16U,  1000U, 13U, 13U},
		{17U,  825U,  16U, 16U},
		{18U,  750U,  16U, 16U},
		{20U,  700U,  16U, 16U},
		{21U,  650U,  16U, 16U},
		{23U,  625U,  16U, 16U},
		{24U,  575U,  16U, 16U},
		{25U,  550U,  16U, 16U},
		{28U,  525U,  16U, 16U},
		{30U,  475U,  16U, 16U},
		{31U,  450U,  16U, 16U},
		{33U,  425U,  16U, 16U},
		{36U,  400U,  16U, 16U},
		{37U,  375U,  16U, 16U},
		{40U,  350U,  16U, 16U},
		{43U,  325U,  16U, 16U},
		{47U,  300U,  16U, 16U},
		{51U,  275U,  16U, 16U},
		{205U, 250U,  16U, 16U},
		{432U, 225U,  16U, 16U},
	};

	*lock_ref_dly = 16U;
	*lock_fb_dly = 16U;

	for (size_t i = 0U; i < ARRAY_SIZE(lock_tbl); i++) {
		if (mval <= lock_tbl[i].max_m) {
			*lock_cnt = lock_tbl[i].lock_cnt;
			*lock_ref_dly = lock_tbl[i].ref_dly;
			*lock_fb_dly = lock_tbl[i].fb_dly;
			return;
		}
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
	edge = !!(reg &  CLK_WIZ_REG1_EDGE_MASK);
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
	struct xlnx_clk_wiz_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t clk_id = (uint32_t)(uintptr_t)sys;
	uint32_t reg_off;

	if (clk_id >= cfg->num_out_clks || clk_id >= CLK_WIZ_MAX_OUTPUTS) {
		LOG_ERR("Invalid clock ID %u (max %u)", clk_id,
			MIN(cfg->num_out_clks, CLK_WIZ_MAX_OUTPUTS) - 1U);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	reg_off = clk_wiz_clkout_offset(clk_id);
	clk_wiz_setbits(base, reg_off, CLK_WIZ_REG3_USED);
	k_mutex_unlock(&data->lock);

	LOG_DBG("Clock %u enabled", clk_id);
	return 0;
}

static int xlnx_clk_wiz_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct xlnx_clk_wiz_config *cfg = dev->config;
	struct xlnx_clk_wiz_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t clk_id = (uint32_t)(uintptr_t)sys;
	uint32_t reg_off;

	if (clk_id >= cfg->num_out_clks || clk_id >= CLK_WIZ_MAX_OUTPUTS) {
		LOG_ERR("Invalid clock ID %u", clk_id);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	reg_off = clk_wiz_clkout_offset(clk_id);
	clk_wiz_clrbits(base, reg_off, CLK_WIZ_REG3_USED);
	k_mutex_unlock(&data->lock);

	LOG_DBG("Clock %u disabled", clk_id);
	return 0;
}

static int xlnx_clk_wiz_get_rate(const struct device *dev,
				 clock_control_subsys_t sys,
				 uint32_t *rate)
{
	const struct xlnx_clk_wiz_config *cfg = dev->config;
	const struct xlnx_clk_wiz_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t clk_id = (uint32_t)(uintptr_t)sys;
	uint32_t reg_off, reg;
	uint32_t edge, low, high, leaf, prediv, p5en, div_o;
	uint64_t fvco, freq;

	if (clk_id >= cfg->num_out_clks || clk_id >= CLK_WIZ_MAX_OUTPUTS) {
		LOG_ERR("Invalid clock ID %u", clk_id);
		return -EINVAL;
	}

	fvco = clk_wiz_get_vco(base, data->prim_in_freq_hz);
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
	struct xlnx_clk_wiz_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t clk_id = (uint32_t)(uintptr_t)sys;
	const uint32_t *rate_hz_ptr = (const uint32_t *)rate;
	uint64_t target_hz;
	struct clk_wiz_divisors clk_divs = {0};
	int ret;

	if (clk_id >= cfg->num_out_clks || clk_id >= CLK_WIZ_MAX_OUTPUTS) {
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

	ret = clk_wiz_calc_divisors(data->prim_in_freq_hz, target_hz, &clk_divs);
	if (ret != 0) {
		LOG_ERR("No valid M/D/O found for %llu Hz", (unsigned long long)target_hz);
		return ret;
	}

	LOG_DBG("Clock %u set_rate %llu Hz → M=%u D=%u O=%u", clk_id,
		(unsigned long long)target_hz, clk_divs.m, clk_divs.d, clk_divs.o);

	k_mutex_lock(&data->lock, K_FOREVER);

	clk_wiz_update_o(base, clk_id, clk_divs.o);

	clk_wiz_update_d(base, clk_divs.d);
	clk_wiz_update_m(base, clk_divs.m);

	LOG_DBG("Clock %u M/D/O programmed, waiting for lock: M=%u D=%u O=%u",
		clk_id, clk_divs.m, clk_divs.d, clk_divs.o);

	clk_wiz_wr(base, CLK_WIZ_RECONFIG_OFFSET,
		   (CLK_WIZ_RECONFIG_SADDR | CLK_WIZ_RECONFIG_LOAD));
	ret = clk_wiz_wait_lock(base);

	k_mutex_unlock(&data->lock);
	return ret;
}

static int xlnx_clk_wiz_init(const struct device *dev)
{
	const struct xlnx_clk_wiz_config *cfg = dev->config;
	struct xlnx_clk_wiz_data *data = dev->data;
	uint32_t rate;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = clock_control_get_rate(cfg->parent_clk, NULL, &rate);
	if (ret != 0) {
		LOG_ERR("Failed to get input clock rate: %d", ret);
		return ret;
	}

	k_mutex_init(&data->lock);
	data->prim_in_freq_hz = rate;
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
		.parent_clk = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),            \
		.num_out_clks = DT_INST_PROP(inst, xlnx_num_out_clks),             \
	};                                                                        \
	                                                                          \
	DEVICE_DT_INST_DEFINE(inst, xlnx_clk_wiz_init, NULL,                    \
			      &xlnx_clk_wiz_data_##inst,                         \
			      &xlnx_clk_wiz_config_##inst, POST_KERNEL,          \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                \
			      &xlnx_clk_wiz_api);

DT_INST_FOREACH_STATUS_OKAY(CLK_WIZ_DEVICE_INIT)
