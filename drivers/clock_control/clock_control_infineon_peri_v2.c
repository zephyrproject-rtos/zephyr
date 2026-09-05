/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_peri_clk

#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <cy_sysclk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(infineon_peri_v2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define IFX_PERI_INIT_PRIO     UTIL_INC(CONFIG_CLOCK_CONTROL_INIT_PRIORITY)

#define IFX_PERI_DIV_8         0U
#define IFX_PERI_DIV_16        1U
#define IFX_PERI_DIV_16_5      2U
#define IFX_PERI_DIV_24_5      3U

#define IFX_FRAC_STEPS         32U

struct ifx_peri_div_info {
	uint32_t clock_div;
	uint32_t div_ord;
	uint8_t div_type;
	uint8_t div_inst;
	uint8_t frac;
};

struct ifx_peri_config {
	const struct device *src_ctlr;
	const struct ifx_peri_div_info *dividers;
	uint32_t peri_clk_inst;
	uint16_t num_dividers;
	uint8_t src_clk_id;
};

struct ifx_peri_data {
	uint32_t src_clk_freq;
	struct k_spinlock lock;
};

static inline bool div_is_frac(uint8_t type)
{
	return (type == IFX_PERI_DIV_16_5) || (type == IFX_PERI_DIV_24_5);
}

static int check_divider_range(uint32_t int_div, uint8_t type)
{
	uint32_t max;

	switch (type) {
	case IFX_PERI_DIV_8:
		max = (1U << 8);
		break;
	case IFX_PERI_DIV_16:
	case IFX_PERI_DIV_16_5:
		max = (1U << 16);
		break;
	case IFX_PERI_DIV_24_5:
		max = (1U << 24);
		break;
	default:
		return -EINVAL;
	}

	return ((int_div == 0U) || (int_div > max)) ? -ERANGE : 0;
}

static const struct ifx_peri_div_info *
find_divider(const struct ifx_peri_config *cfg, uint32_t div_ord)
{
	for (uint16_t i = 0U; i < cfg->num_dividers; i++) {
		const struct ifx_peri_div_info *d = &cfg->dividers[i];

		if (d->div_ord == div_ord) {
			return d;
		}
	}

	return NULL;
}

static int program_divider(en_clk_dst_t clock_id, uint8_t type, uint8_t inst,
			   uint32_t int_div, uint8_t frac)
{
	int ret;

	ret = check_divider_range(int_div, type);
	if (ret != 0) {
		return ret;
	}

	Cy_SysClk_PeriPclkDisableDivider(clock_id, (cy_en_divider_types_t)type, inst);
	if (div_is_frac(type)) {
		if (Cy_SysClk_PeriPclkSetFracDivider(clock_id, (cy_en_divider_types_t)type,
						     inst, int_div - 1U, frac) !=
		    CY_SYSCLK_SUCCESS) {
			return -EIO;
		}
	} else {
		if (Cy_SysClk_PeriPclkSetDivider(clock_id, (cy_en_divider_types_t)type,
						 inst, int_div - 1U) != CY_SYSCLK_SUCCESS) {
			return -EIO;
		}
	}

	if (Cy_SysClk_PeriPclkEnableDivider(clock_id, (cy_en_divider_types_t)type,
					    inst) != CY_SYSCLK_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int ifx_peri_set_rate(const struct device *dev,
			     clock_control_subsys_t sys,
			     clock_control_subsys_rate_t rate)
{
	struct ifx_clk_peri *clk = (struct ifx_clk_peri *)sys;
	const struct ifx_peri_config *cfg = dev->config;
	struct ifx_peri_data *data = dev->data;
	const uint32_t *req = (const uint32_t *)rate;
	const struct ifx_peri_div_info *d;
	en_clk_dst_t clock_id_base;
	en_clk_dst_t clock_id_pclk;
	k_spinlock_key_t key;
	uint32_t int_div;
	uint8_t frac = 0U;
	int ret;

	if (clk == NULL) {
		return -EINVAL;
	}

	d = find_divider(cfg, clk->div_ord);
	if (d == NULL) {
		return -EINVAL;
	}

	clock_id_base = (en_clk_dst_t)cfg->peri_clk_inst;
	clock_id_pclk = (en_clk_dst_t)(cfg->peri_clk_inst | clk->rootclk_id);

	key = k_spin_lock(&data->lock);

	if (d->clock_div != 0U) {
		/* divider already programmed at init: only connect the
		 * peripheral to it.
		 */
		ret = (Cy_SysClk_PeriPclkAssignDivider(clock_id_pclk,
						(cy_en_divider_types_t)d->div_type,
						d->div_inst) == CY_SYSCLK_SUCCESS)
			      ? 0
			      : -EIO;
		goto out;
	}

	if ((req == NULL) || (*req == 0U) || (*req > data->src_clk_freq)) {
		ret = -EINVAL;
		goto out;
	}

	int_div = data->src_clk_freq / *req;
	if (div_is_frac(d->div_type)) {
		frac = (uint8_t)(((uint64_t)(data->src_clk_freq - (int_div * *req)) *
				  IFX_FRAC_STEPS) / *req);
	}

	ret = program_divider(clock_id_base, d->div_type, d->div_inst, int_div, frac);
	if (ret == 0) {
		if (Cy_SysClk_PeriPclkAssignDivider(clock_id_pclk,
						(cy_en_divider_types_t)d->div_type,
						d->div_inst) != CY_SYSCLK_SUCCESS) {
			ret = -EIO;
		}
	}

out:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ifx_peri_get_rate(const struct device *dev,
			     clock_control_subsys_t sys, uint32_t *rate)
{
	struct ifx_clk_peri *clk = (struct ifx_clk_peri *)sys;
	const struct ifx_peri_config *cfg = dev->config;
	struct ifx_peri_data *data = dev->data;
	const struct ifx_peri_div_info *d;

	if ((rate == NULL) || (clk == NULL)) {
		return -EINVAL;
	}

	d = find_divider(cfg, clk->div_ord);
	if (d == NULL) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		*rate = Cy_SysClk_PeriPclkGetFrequency((en_clk_dst_t)cfg->peri_clk_inst,
					       (cy_en_divider_types_t)d->div_type,
					       d->div_inst);
	}

	return 0;
}

static int ifx_peri_init(const struct device *dev)
{
	const struct ifx_peri_config *cfg = dev->config;
	struct ifx_peri_data *data = dev->data;
	struct ifx_clk src = {
		.clk = IFX_CLK_HF,
		.clk_id = cfg->src_clk_id,
	};
	int ret;

	ret = clock_control_get_rate(cfg->src_ctlr, (clock_control_subsys_t)&src,
				     &data->src_clk_freq);
	if ((ret != 0) || (data->src_clk_freq == 0U)) {
		LOG_ERR("source CLK_HF%u rate unavailable (ret=%d)", cfg->src_clk_id, ret);
		return (ret != 0) ? ret : -EINVAL;
	}

	/* Program the dividers that has a divider value. Dividers without
	 * a value are left for the peripheral to configure via set_rate().
	 */
	for (uint16_t i = 0U; i < cfg->num_dividers; i++) {
		const struct ifx_peri_div_info *d = &cfg->dividers[i];

		if (d->clock_div == 0U) {
			continue;
		}

		ret = program_divider((en_clk_dst_t)cfg->peri_clk_inst, d->div_type,
				      d->div_inst, d->clock_div, d->frac);
		if (ret != 0) {
			LOG_ERR("preset divider type=%u inst=%u div=%u failed (%d)",
				d->div_type, d->div_inst, d->clock_div, ret);
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(clock_control, ifx_peri_api) = {
	.get_rate = ifx_peri_get_rate,
	.set_rate = ifx_peri_set_rate,
};

#define IFX_PERI_DIV_ENTRY(child)						\
	{									\
		.div_ord = DT_DEP_ORD(child),					\
		.div_type = DT_PROP(child, div_type),				\
		.div_inst = DT_PROP(child, channel),				\
		.clock_div = DT_PROP_OR(child, clock_div, 0),			\
		.frac = DT_PROP_OR(child, div_frac_value, 0),			\
	},

#define IFX_PERI_INIT(inst)							\
	static const struct ifx_peri_div_info dividers_##inst[] = {		\
		DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, IFX_PERI_DIV_ENTRY) {0},\
	};									\
										\
	static const struct ifx_peri_config cfg_##inst = {			\
		.src_ctlr = DEVICE_DT_GET(DT_NODELABEL(clocks)),		\
		.src_clk_id = DT_REG_ADDR(DT_INST_PHANDLE(inst, clocks)),	\
		.peri_clk_inst = DT_INST_PROP(inst, ifx_peri_clk_inst),		\
		.dividers = dividers_##inst,					\
		.num_dividers = ARRAY_SIZE(dividers_##inst),			\
	};									\
										\
	static struct ifx_peri_data data_##inst;				\
										\
	DEVICE_DT_INST_DEFINE(inst, ifx_peri_init, NULL, &data_##inst,		\
			      &cfg_##inst, PRE_KERNEL_1, IFX_PERI_INIT_PRIO,	\
			      &ifx_peri_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_PERI_INIT)
