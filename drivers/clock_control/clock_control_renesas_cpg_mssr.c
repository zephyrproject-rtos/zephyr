/*
 * Copyright (c) 2020-2022 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_cpg_mssr.h>
#include <zephyr/dt-bindings/clock/renesas_cpg_mssr.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include "clock_control_renesas_cpg_mssr.h"
#include <stdlib.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_rcar);

static void rcar_cpg_reset(uint32_t base_address, uint32_t reg, uint32_t bit)
{
	rcar_cpg_write(base_address, srcr[reg], BIT(bit));
	rcar_cpg_write(base_address, SRSTCLR(reg), BIT(bit));
}

void rcar_cpg_write(uint32_t base_address, uint32_t reg, uint32_t val)
{
	sys_write32(~val, base_address + CPGWPR);
	sys_write32(val, base_address + reg);
	/* Wait for at least one cycle of the RCLK clock (@ ca. 32 kHz) */
	k_sleep(K_USEC(35));
}

int rcar_cpg_mstp_clock_endisable(uint32_t base_address, uint32_t module, bool enable)
{
	uint32_t reg = module / 100;
	uint32_t bit = module % 100;
	uint32_t bitmask = BIT(bit);
	uint32_t reg_val;

	__ASSERT((bit < 32) && reg < ARRAY_SIZE(mstpcr), "Invalid module number for cpg clock: %d",
		 module);

	reg_val = sys_read32(base_address + mstpcr[reg]);
	if (enable) {
		reg_val &= ~bitmask;
	} else {
		reg_val |= bitmask;
	}

	sys_write32(reg_val, base_address + mstpcr[reg]);
	if (!enable) {
		rcar_cpg_reset(base_address, reg, bit);
	}

	return 0;
}

static int cmp_cpg_clk_info_table_items(const void *key, const void *element)
{
	const struct cpg_clk_info_table *e = element;
	uint32_t module = (uintptr_t)key;

	if (e->module == module) {
		return 0;
	} else if (e->module < module) {
		return 1;
	} else {
		return -1;
	}
}

struct cpg_clk_info_table *
rcar_cpg_find_clk_info_by_module_id(const struct device *dev, uint32_t domain, uint32_t id)
{
	struct rcar_cpg_mssr_data *data = dev->data;
	struct cpg_clk_info_table *item;
	struct cpg_clk_info_table *table = data->clk_info_table[domain];
	uint32_t table_size = data->clk_info_table_size[domain];
	uintptr_t uintptr_id = id;

	item = bsearch((void *)uintptr_id, table, table_size, sizeof(*item),
		       cmp_cpg_clk_info_table_items);
	if (!item) {
		LOG_ERR("%s: can't find clk info (domain %u module %u)", dev->name, domain, id);
	}

	return item;
}

static uint32_t rcar_cpg_get_divider(const struct device *dev, struct cpg_clk_info_table *clk_info)
{
	mem_addr_t reg_addr;
	mm_reg_t reg_val;
	uint32_t divider = RCAR_CPG_NONE;
	struct rcar_cpg_mssr_data *data = dev->data;

	if (clk_info->domain == CPG_MOD) {
		return 1;
	}

	reg_addr = clk_info->offset;
	if (reg_addr == RCAR_CPG_NONE) {
		/* if we don't have valid offset, in is equal to out */
		return 1;
	}

	reg_addr += DEVICE_MMIO_GET(dev);
	reg_val = sys_read32(reg_addr);

	if (data->get_div_helper) {
		divider = data->get_div_helper(reg_val, clk_info->module);
	}

	if (!divider) {
		return RCAR_CPG_NONE;
	}

	return divider;
}

static int rcar_cpg_update_out_freq(const struct device *dev, struct cpg_clk_info_table *clk_info)
{
	uint32_t divider = rcar_cpg_get_divider(dev, clk_info);

	if (divider == RCAR_CPG_NONE) {
		return -EINVAL;
	}

	clk_info->out_freq = clk_info->in_freq / divider;
	return 0;
}

static int64_t rcar_cpg_get_in_update_out_freq(const struct device *dev,
					   struct cpg_clk_info_table *clk_info)
{
	int64_t freq = -ENOTSUP;
	struct cpg_clk_info_table *parent_clk;

	if (!clk_info) {
		return freq;
	}

	if (clk_info->in_freq != RCAR_CPG_NONE) {
		if (clk_info->out_freq == RCAR_CPG_NONE) {
			if (rcar_cpg_update_out_freq(dev, clk_info) < 0) {
				return freq;
			}
		}
		return clk_info->in_freq;
	}

	parent_clk = clk_info->parent;

	freq = rcar_cpg_get_in_update_out_freq(dev, parent_clk);
	if (freq < 0) {
		return freq;
	}

	clk_info->in_freq = parent_clk->out_freq;

	freq = rcar_cpg_update_out_freq(dev, clk_info);
	if (freq < 0) {
		return freq;
	}

	return clk_info->in_freq;
}

static int64_t rcar_cpg_get_out_freq(const struct device *dev, struct cpg_clk_info_table *clk_info)
{
	int64_t freq;

	if (clk_info->out_freq != RCAR_CPG_NONE) {
		return clk_info->out_freq;
	}

	freq = rcar_cpg_get_in_update_out_freq(dev, clk_info);
	if (freq < 0) {
		return freq;
	}

	return clk_info->out_freq;
}

static void rcar_cpg_change_children_in_out_freq(const struct device *dev,
						 struct cpg_clk_info_table *parent)
{
	struct cpg_clk_info_table *children_list = parent->children_list;

	while (children_list) {
		children_list->in_freq = parent->out_freq;

		if (rcar_cpg_update_out_freq(dev, children_list) < 0) {
			/*
			 * Why it can happen:
			 * - divider is zero (with current implementation of board specific
			 *   divider helper function it is impossible);
			 * - we don't have board specific implementation of get divider helper
			 *   function;
			 * - we don't have this module in a table (for some of call chains of
			 *   this function it is impossible);
			 * - impossible value is set in clock register divider bits.
			 */
			LOG_ERR("%s: error during getting divider from clock register, domain %u "
				"module %u! Please, revise logic related to obtaining divider or "
				"check presentence of clock inside appropriate clk_info_table",
				dev->name, children_list->domain, children_list->module);
			k_panic();
			return;
		}

		/* child can have children */
		rcar_cpg_change_children_in_out_freq(dev, children_list);
		children_list = children_list->next_sibling;
	}
}

int rcar_cpg_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	int64_t ret;
	struct rcar_cpg_mssr_data *data;
	struct rcar_cpg_clk *clk = (struct rcar_cpg_clk *)sys;
	k_spinlock_key_t key;

	struct cpg_clk_info_table *clk_info;

	if (!dev || !sys || !rate) {
		LOG_ERR("%s: received null ptr input arg(s) dev %p sys %p rate %p",
			__func__, dev, sys, rate);
		return -EINVAL;
	}

	clk_info = rcar_cpg_find_clk_info_by_module_id(dev, clk->domain, clk->module);
	if (clk_info == NULL) {
		return -EINVAL;
	}

	data = dev->data;

	key = k_spin_lock(&data->lock);
	ret = rcar_cpg_get_out_freq(dev, clk_info);
	k_spin_unlock(&data->lock, key);

	if (ret < 0) {
		LOG_ERR("%s: clk (domain %u module %u) error (%lld) during getting out frequency",
			dev->name, clk->domain, clk->module, ret);
		return -EINVAL;
	} else if (ret > UINT_MAX) {
		LOG_ERR("%s: clk (domain %u module %u) frequency bigger then max uint value",
			dev->name, clk->domain, clk->module);
		return -EINVAL;
	}

	*rate = ret;
	return 0;
}

int rcar_cpg_set_rate(const struct device *dev, clock_control_subsys_t sys,
		      clock_control_subsys_rate_t rate)
{
	int ret = -ENOTSUP;
	k_spinlock_key_t key;
	struct cpg_clk_info_table *clk_info;
	struct rcar_cpg_clk *clk = (struct rcar_cpg_clk *)sys;
	struct rcar_cpg_mssr_data *data;
	int64_t in_freq;
	uint32_t divider;
	uint32_t div_mask;
	uint32_t module;
	uintptr_t u_rate = (uintptr_t)rate;

	if (!dev || !sys || !rate) {
		LOG_ERR("%s: received null ptr input arg(s) dev %p sys %p rate %p",
			__func__, dev, sys, rate);
		return -EINVAL;
	}

	clk_info = rcar_cpg_find_clk_info_by_module_id(dev, clk->domain, clk->module);
	if (clk_info == NULL) {
		return -EINVAL;
	}

	if (clk_info->domain == CPG_MOD) {
		if (!clk_info->parent) {
			LOG_ERR("%s: parent isn't present for module clock, module id %u",
				dev->name, clk_info->module);
			k_panic();
		}
		clk_info = clk_info->parent;
	}

	module = clk_info->module;
	data = dev->data;

	key = k_spin_lock(&data->lock);
	in_freq = rcar_cpg_get_in_update_out_freq(dev, clk_info);
	if (in_freq < 0) {
		ret = in_freq;
		goto unlock;
	}

	divider = in_freq / u_rate;
	if (divider * u_rate != in_freq) {
		ret = -EINVAL;
		goto unlock;
	}

	if (!data->set_rate_helper) {
		ret = -ENOTSUP;
		goto unlock;
	}

	ret = data->set_rate_helper(module, &divider, &div_mask);
	if (!ret) {
		int64_t out_rate;
		uint32_t reg = sys_read32(clk_info->offset + DEVICE_MMIO_GET(dev));

		reg &= ~div_mask;
		rcar_cpg_write(DEVICE_MMIO_GET(dev), clk_info->offset, reg | divider);

		clk_info->out_freq = RCAR_CPG_NONE;

		out_rate = rcar_cpg_get_out_freq(dev, clk_info);
		if (out_rate < 0 || out_rate != u_rate) {
			ret = -EINVAL;
			LOG_ERR("%s: clock (domain %u module %u) register cfg freq (%lld) "
				"isn't equal to requested %lu",
				dev->name, clk->domain, clk->module, out_rate, u_rate);
			goto unlock;
		}

		rcar_cpg_change_children_in_out_freq(dev, clk_info);
	}

unlock:
	k_spin_unlock(&data->lock, key);
	return ret;
}

void rcar_cpg_build_clock_relationship(const struct device *dev)
{
	uint32_t domain;
	k_spinlock_key_t key;
	struct rcar_cpg_mssr_data *data = dev->data;

	if (!data) {
		return;
	}

	key = k_spin_lock(&data->lock);
	for (domain = 0; domain < CPG_NUM_DOMAINS; domain++) {
		uint32_t idx;
		uint32_t prev_mod_id = 0;
		struct cpg_clk_info_table *item = data->clk_info_table[domain];

		for (idx = 0; idx < data->clk_info_table_size[domain]; idx++, item++) {
			struct cpg_clk_info_table *parent;

			/* check if an array is sorted by module id or not */
			if (prev_mod_id >= item->module) {
				LOG_ERR("%s: clocks have to be sorted inside clock table in "
					"ascending order by module id field, domain %u "
					"module id %u",
					dev->name, item->domain, item->module);
				k_panic();
			}

			prev_mod_id = item->module;

			if (item->parent_id == RCAR_CPG_NONE) {
				continue;
			}

			parent = rcar_cpg_find_clk_info_by_module_id(dev, CPG_CORE,
								     item->parent_id);
			if (!parent) {
				LOG_ERR("%s: can't find parent for clock with valid parent id, "
					"domain %u module id %u",
					dev->name, item->domain, item->module);
				k_panic();
			}

			if (item->parent != NULL) {
				LOG_ERR("%s: trying to set another parent for a clock, domain %u "
					"module id %u, parent for the clock has been already set",
					dev->name, item->domain, item->module);
				k_panic();
			}

			item->parent = parent;

			/* insert in the head of the children list of the parent */
			item->next_sibling = parent->children_list;
			parent->children_list = item;
		}
	}
	k_spin_unlock(&data->lock, key);
}

void rcar_cpg_update_all_in_out_freq(const struct device *dev)
{
	uint32_t domain;
	k_spinlock_key_t key;
	struct rcar_cpg_mssr_data *data = dev->data;

	if (!data) {
		return;
	}

	key = k_spin_lock(&data->lock);
	for (domain = 0; domain < CPG_NUM_DOMAINS; domain++) {
		uint32_t idx;
		struct cpg_clk_info_table *item = data->clk_info_table[domain];

		for (idx = 0; idx < data->clk_info_table_size[domain]; idx++, item++) {
			if (rcar_cpg_get_in_update_out_freq(dev, item) < 0) {
				LOG_ERR("%s: can't update in/out freq for clock during init, "
					"domain %u module %u! Please, review correctness of data "
					"inside clk_info_table",
					dev->name, item->domain, item->module);
				k_panic();
			}
		}
	}
	k_spin_unlock(&data->lock, key);
}
