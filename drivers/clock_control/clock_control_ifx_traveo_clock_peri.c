/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <cy_sysclk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_clock_peri_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT infineon_clk_peri

/* peri clock needs to init after pll/hf initialized so increment it to 1 */
#define IFX_PERI_CLOCK_INIT_PRIO UTIL_INC(CONFIG_CLOCK_CONTROL_INIT_PRIORITY)

#define IFX_FRACTION_DIVIDER_VALUE 32U
#define IFX_PERI_NUM_DIV_TYPES     4U
#define IFX_PERI_NUM_INT_TYPES 2U

#define IFX_PERI_DIV_8    0U
#define IFX_PERI_DIV_16   1U
#define IFX_PERI_DIV_16_5 2U
#define IFX_PERI_DIV_24_5 3U

#define IFX_ASSIGNED_INST(v) ((uint8_t)((v) & 0xFFU))
#define IFX_ASSIGNED_TYPE(v) ((uint8_t)(((v) >> 8) & 0x3U))

struct ifx_clock_peri_config {
	const struct device *src_ctlr;
	uint32_t peri_clk_inst;
	uint16_t pclk_count;
	uint8_t  src_clk_id;
	uint8_t  max_dividers[IFX_PERI_NUM_DIV_TYPES];
};

struct ifx_clock_peri_data {
	uint32_t src_clk_freq;
	struct k_mutex lock;
};

struct ifx_div_sel {
	uint8_t  type;
	uint32_t int_reg;
	uint8_t  frac;
};

static inline bool div_is_frac(uint8_t type)
{
	return (type == IFX_PERI_DIV_16_5) || (type == IFX_PERI_DIV_24_5);
}

static inline bool divider_in_range(const struct ifx_clock_peri_config *cfg,
				    uint8_t type, uint8_t inst)
{
	return (type < IFX_PERI_NUM_DIV_TYPES) && (inst < cfg->max_dividers[type]);
}

static inline bool divider_active(const struct ifx_clock_peri_config *cfg,
				  en_clk_dst_t ip_base, uint8_t type, uint8_t inst)
{
	return divider_in_range(cfg, type, inst) &&
	       Cy_SysClk_PeriPclkGetDividerEnabled(ip_base,
					   (cy_en_divider_types_t)type, inst);
}

static inline uint32_t div_int_max(uint8_t type)
{
	switch (type) {
	case IFX_PERI_DIV_8:
		return (1U << 8);
	case IFX_PERI_DIV_16:
	case IFX_PERI_DIV_16_5:
		return (1U << 16);
	case IFX_PERI_DIV_24_5:
		return (1U << 24);
	default:
		return 0U;
	}
}

static uint32_t calc_for_type(uint32_t src, uint32_t freq, uint8_t type,
			      uint32_t *int_reg, uint8_t *frac)
{
	uint32_t idiv;
	uint8_t frac_val = 0U;

	if (div_is_frac(type)) {
		uint64_t rem;

		idiv = src / freq;
		rem = (uint64_t)(src - (idiv * freq)) * IFX_FRACTION_DIVIDER_VALUE;
		frac_val = (uint8_t)(rem / freq);
		if (((rem % freq) * 2U) >= freq) {
			frac_val++;
		}

		if (frac_val >= IFX_FRACTION_DIVIDER_VALUE) {
			frac_val = 0U;
			idiv++;
		}
	} else {
		idiv = DIV_ROUND_CLOSEST(src, freq);
	}

	if ((idiv == 0U) || (idiv > div_int_max(type))) {
		return UINT32_MAX;
	}

	uint64_t denom = ((uint64_t)idiv * IFX_FRACTION_DIVIDER_VALUE) + frac_val;
	uint32_t actual =
		(uint32_t)(((uint64_t)src * IFX_FRACTION_DIVIDER_VALUE) / denom);

	*int_reg = idiv - 1U;
	*frac = frac_val;

	return (actual > freq) ? (actual - freq) : (freq - actual);
}

static int choose_divider(const struct ifx_clock_peri_config *cfg, uint32_t src,
			  uint32_t freq, struct ifx_div_sel *out, bool integer_only)
{
	static const uint8_t types[] = {IFX_PERI_DIV_8, IFX_PERI_DIV_16,
					IFX_PERI_DIV_16_5, IFX_PERI_DIV_24_5};
	uint32_t int_reg;
	uint8_t frac;
	uint8_t iter = integer_only ? IFX_PERI_NUM_INT_TYPES : ARRAY_SIZE(types);
	uint32_t best = UINT32_MAX;

	for (uint8_t i = 0U; i < iter; i++) {
		uint8_t type = types[i];
		uint32_t err;

		if (cfg->max_dividers[type] == 0U) {
			continue;
		}

		err = calc_for_type(src, freq, type, &int_reg, &frac);
		if (err == UINT32_MAX) {
			continue;
		}

		if (err < best) {
			best = err;
			out->type = type;
			out->int_reg = int_reg;
			out->frac = frac;
		}
	}

	if (best == UINT32_MAX) {
		LOG_ERR("no representable divider for %u Hz (src=%u)", freq, src);
		return -ERANGE;
	}

	return 0;
}

static bool div_matches(en_clk_dst_t ip, const struct ifx_div_sel *d, uint8_t inst)
{
	if (div_is_frac(d->type)) {
		uint32_t int_div;
		uint32_t frac_div;

		Cy_SysClk_PeriPclkGetFracDivider(ip, (cy_en_divider_types_t)d->type,
						 inst, &int_div, &frac_div);

		return (int_div == d->int_reg) && ((uint8_t)frac_div == d->frac);
	}

	return Cy_SysClk_PeriPclkGetDivider(ip, (cy_en_divider_types_t)d->type,
					    inst) == d->int_reg;
}

static int find_sharer(en_clk_dst_t ip_base,
		       const struct ifx_clock_peri_config *cfg,
		       const struct ifx_div_sel *d)
{
	for (uint8_t inst = 0U; inst < cfg->max_dividers[d->type]; inst++) {
		if (!Cy_SysClk_PeriPclkGetDividerEnabled(
			    ip_base, (cy_en_divider_types_t)d->type, inst)) {
			continue;
		}

		if (div_matches(ip_base, d, inst)) {
			return inst;
		}
	}

	return -1;
}

static int find_free(en_clk_dst_t ip_base,
		     const struct ifx_clock_peri_config *cfg, uint8_t type)
{
	for (uint8_t inst = 0U; inst < cfg->max_dividers[type]; inst++) {
		if (!Cy_SysClk_PeriPclkGetDividerEnabled(
			    ip_base, (cy_en_divider_types_t)type, inst)) {
			return inst;
		}
	}

	return -1;
}

static bool divider_still_used(const struct ifx_clock_peri_config *cfg,
			       uint16_t self_pclk, uint8_t type, uint8_t inst)
{
	en_clk_dst_t ip_base = (en_clk_dst_t)cfg->peri_clk_inst;

	for (uint16_t p = 0U; p < cfg->pclk_count; p++) {
		uint32_t assigned;

		if (p == self_pclk) {
			continue;
		}

		assigned = Cy_SysClk_PeriPclkGetAssignedDivider(
			(en_clk_dst_t)(cfg->peri_clk_inst | p));

		if ((IFX_ASSIGNED_TYPE(assigned) == type) &&
		    (IFX_ASSIGNED_INST(assigned) == inst) &&
		    divider_active(cfg, ip_base, type, inst)) {
			return true;
		}
	}

	return false;
}

static int program_divider(en_clk_dst_t ip_base, const struct ifx_div_sel *d,
			   uint8_t inst)
{
	cy_en_sysclk_status_t status;

	Cy_SysClk_PeriPclkDisableDivider(ip_base,
					 (cy_en_divider_types_t)d->type, inst);

	if (div_is_frac(d->type)) {
		status = Cy_SysClk_PeriPclkSetFracDivider(ip_base,
						  (cy_en_divider_types_t)d->type,
						  inst, d->int_reg, d->frac);
	} else {
		status = Cy_SysClk_PeriPclkSetDivider(ip_base,
						(cy_en_divider_types_t)d->type,
						inst, d->int_reg);
	}

	if (status != CY_SYSCLK_SUCCESS) {
		return -EIO;
	}

	if (Cy_SysClk_PeriPclkEnableDivider(ip_base, (cy_en_divider_types_t)d->type,
					    inst) != CY_SYSCLK_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int place_divider(en_clk_dst_t ip_base,
			 const struct ifx_clock_peri_config *cfg,
			 struct ifx_div_sel *d, bool integer_only)
{
	int inst;
	int ret;

	inst  = find_sharer(ip_base, cfg, d);
	if (inst >= 0) {
		return inst;
	}

	if (integer_only && d->type == IFX_PERI_DIV_8) {
		d->type = IFX_PERI_DIV_16;

		inst  = find_sharer(ip_base, cfg, d);
		if (inst >= 0) {
			return inst;
		}

		d->type = IFX_PERI_DIV_8;
	}

	inst = find_free(ip_base, cfg, d->type);
	if ((inst < 0) && integer_only && (d->type == IFX_PERI_DIV_8) &&
	    (cfg->max_dividers[IFX_PERI_DIV_16] > 0U)) {
		d->type = IFX_PERI_DIV_16;
		inst = find_free(ip_base, cfg, d->type);
	}

	if (inst < 0) {
		return -ENOSPC;
	}

	ret = program_divider(ip_base, d, (uint8_t)inst);
	if (ret != 0) {
		return ret;
	}

	return inst;
}

static void log_div_result(en_clk_dst_t ip_base, uint8_t type, uint8_t inst,
			   uint16_t pclk_id, uint32_t req)
{
	uint32_t actual = Cy_SysClk_PeriPclkGetFrequency(
		ip_base, (cy_en_divider_types_t)type, inst);
	uint32_t err = (actual > req) ? (actual - req) : (req - actual);
	uint32_t ppm =
		(req != 0U) ? (uint32_t)(((uint64_t)err * 1000000U) / req) : 0U;

	if (div_is_frac(type)) {
		uint32_t int_div = 0U;
		uint32_t frac_div = 0U;

		Cy_SysClk_PeriPclkGetFracDivider(ip_base,
						 (cy_en_divider_types_t)type, inst,
						 &int_div, &frac_div);
		LOG_INF("pclk %u div type=%u inst=%u int=%u frac=%u/32 "
			"req=%u act=%u err=%u ppm=%u",
			pclk_id, type, inst, int_div, frac_div, req, actual,
			err, ppm);
	} else {
		uint32_t int_div = Cy_SysClk_PeriPclkGetDivider(
			ip_base, (cy_en_divider_types_t)type, inst);

		LOG_INF("pclk %u div type=%u inst=%u int=%u req=%u act=%u "
			"err=%u ppm=%u",
			pclk_id, type, inst, int_div, req, actual, err, ppm);
	}
}

static int clock_control_ifx_clock_peri_set_rate(const struct device *dev,
						 clock_control_subsys_t sys,
						 clock_control_subsys_rate_t rate)
{
	struct ifx_clk_peri *clk = (struct ifx_clk_peri *)sys;
	const struct ifx_clock_peri_config *cfg = dev->config;
	struct ifx_clock_peri_data *data = dev->data;
	uint32_t freq = (rate != NULL) ? *(uint32_t *)rate : 0U;
	uint32_t src = data->src_clk_freq;
	en_clk_dst_t ip_base;
	en_clk_dst_t ip_pclk;
	struct ifx_div_sel d;
	uint32_t assigned;
	uint8_t cur_type;
	uint8_t cur_inst;
	bool cur_valid;
	int inst;
	int ret;

	if ((clk == NULL) || (clk->pclk_id >= cfg->pclk_count) ||
	    (freq == 0U) || (src == 0U) || (freq > src)) {
		LOG_ERR("invalid request: pclk=%u freq=%u src=%u", clk ?
				clk->pclk_id : 0xFFFF, freq, src);
		return -EINVAL;
	}

	ret = choose_divider(cfg, src, freq, &d, clk->integer_only);
	if (ret != 0) {
		return ret;
	}

	ip_base = (en_clk_dst_t)cfg->peri_clk_inst;
	ip_pclk = (en_clk_dst_t)(cfg->peri_clk_inst | clk->pclk_id);

	k_mutex_lock(&data->lock, K_FOREVER);

	assigned = Cy_SysClk_PeriPclkGetAssignedDivider(ip_pclk);
	cur_type = IFX_ASSIGNED_TYPE(assigned);
	cur_inst = IFX_ASSIGNED_INST(assigned);
	cur_valid = divider_active(cfg, ip_base, cur_type, cur_inst);

	/* check whether exiting divider matches required frequency */
	if (cur_valid && (cur_type == d.type) &&
	    div_matches(ip_base, &d, cur_inst)) {
		log_div_result(ip_base, cur_type, cur_inst, clk->pclk_id, freq);
		ret = 0;
		goto out;
	}

	inst = place_divider(ip_base, cfg, &d, clk->integer_only);
	if (inst < 0) {
		if (inst == -ENOSPC) {
			LOG_ERR("no free peripheral divider in group");
		}
		ret = inst;
		goto out;
	}

	if (Cy_SysClk_PeriPclkAssignDivider(ip_pclk, (cy_en_divider_types_t)d.type,
					    (uint32_t)inst) != CY_SYSCLK_SUCCESS) {
		ret = -EIO;
		goto out;
	}

	log_div_result(ip_base, d.type, (uint8_t)inst, clk->pclk_id, freq);

	bool same_divider = (cur_type == d.type) && (cur_inst == (uint8_t)inst);

	/* disable if the previously configured divider not in use */
	if (cur_valid && !same_divider &&
	    !divider_still_used(cfg, clk->pclk_id, cur_type, cur_inst)) {
		Cy_SysClk_PeriPclkDisableDivider(
			ip_base, (cy_en_divider_types_t)cur_type, cur_inst);
	}

	ret = 0;
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int clock_control_ifx_clock_peri_get_rate(const struct device *dev,
						 clock_control_subsys_t sys,
						 uint32_t *rate)
{
	struct ifx_clk_peri *clk = (struct ifx_clk_peri *)sys;
	const struct ifx_clock_peri_config *cfg = dev->config;
	struct ifx_clock_peri_data *data = dev->data;
	en_clk_dst_t ip_base;
	bool ret;
	uint32_t assigned;
	uint8_t type;
	uint8_t inst;

	if ((rate == NULL) || (clk == NULL) ||
	    (clk->pclk_id >= cfg->pclk_count)) {
		return -EINVAL;
	}

	ip_base = (en_clk_dst_t)cfg->peri_clk_inst;

	k_mutex_lock(&data->lock, K_FOREVER);
	assigned = Cy_SysClk_PeriPclkGetAssignedDivider(
		(en_clk_dst_t)(cfg->peri_clk_inst | clk->pclk_id));
	type = IFX_ASSIGNED_TYPE(assigned);
	inst = IFX_ASSIGNED_INST(assigned);

	ret = divider_active(cfg, ip_base, type, inst);
	if (ret) {
		*rate = Cy_SysClk_PeriPclkGetFrequency(
			ip_base, (cy_en_divider_types_t)type, inst);
	}

	k_mutex_unlock(&data->lock);

	return ret ? 0 : -EIO;
}

static int clock_control_ifx_clock_peri_init(const struct device *dev)
{
	struct ifx_clock_peri_data *data = dev->data;
	const struct ifx_clock_peri_config *cfg = dev->config;
	struct ifx_clk clk = {
		.clk = IFX_CLK_HF,
		.clk_id = cfg->src_clk_id,
	};
	int ret;

	k_mutex_init(&data->lock);

	ret = clock_control_get_rate(cfg->src_ctlr, (clock_control_subsys_t)&clk,
				     &data->src_clk_freq);
	if (ret != 0 || (data->src_clk_freq == 0U)) {
		LOG_ERR("Invalid source CLK_HF%u (ret=%d)",
			cfg->src_clk_id, ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(clock_control, ifx_clock_peri_control_api) = {
	.get_rate = clock_control_ifx_clock_peri_get_rate,
	.set_rate = clock_control_ifx_clock_peri_set_rate,
};

#define IFX_MAXDIV(inst, idx, prop)						\
	.max_dividers[idx] = DT_INST_PROP_OR(inst, prop, 0)

#define FILL_DIVIDER_INST(inst)							\
	IFX_MAXDIV(inst, IFX_PERI_DIV_8, ifx_8_bit_max_divider),		\
	IFX_MAXDIV(inst, IFX_PERI_DIV_16, ifx_16_bit_max_divider),		\
	IFX_MAXDIV(inst, IFX_PERI_DIV_16_5, ifx_16_5_bit_max_divider),		\
	IFX_MAXDIV(inst, IFX_PERI_DIV_24_5, ifx_24_5_bit_max_divider),

#define IFX_CLOCK_PERI_INIT(inst)						\
	static const struct ifx_clock_peri_config clk_cfg_##inst = {		\
		.src_ctlr = DEVICE_DT_GET(DT_NODELABEL(clocks)),		\
		.src_clk_id = DT_REG_ADDR(DT_INST_PHANDLE(inst, clocks)),	\
		.peri_clk_inst = DT_INST_PROP_OR(inst, ifx_peri_clk_inst, 0),	\
		.pclk_count = DT_INST_PROP_OR(inst, ifx_pclk_count, 0),		\
		FILL_DIVIDER_INST(inst)};					\
										\
	static struct ifx_clock_peri_data clk_data_##inst;			\
										\
	DEVICE_DT_INST_DEFINE(inst, clock_control_ifx_clock_peri_init,		\
			      NULL, &clk_data_##inst, &clk_cfg_##inst,		\
			      PRE_KERNEL_1, IFX_PERI_CLOCK_INIT_PRIO,		\
			      &ifx_clock_peri_control_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_CLOCK_PERI_INIT)
