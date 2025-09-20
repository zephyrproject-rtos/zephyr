/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pmc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clk_generated, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct clk_generated {
	struct device clk;
	struct device *parents[8];
	pmc_registers_t *pmc;
	struct clk_range range;
	struct k_spinlock *lock;
	uint32_t mux_table[8];
	uint32_t id;
	uint32_t gckdiv;
	const struct clk_pcr_layout *layout;
	uint8_t num_parents;
	uint8_t parent_id;
	int chg_pid;
};

#define to_clk_generated(ptr) CONTAINER_OF(ptr, struct clk_generated, clk)

static struct clk_generated clocks_gck[SOC_NUM_CLOCK_GENERATED];
static uint32_t clocks_gck_idx;

static int clk_generated_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_generated *gck = to_clk_generated(dev);
	uint32_t mask = gck->layout->cmd | PMC_PCR_GCLKEN_Msk;
	uint32_t val = gck->layout->cmd | PMC_PCR_GCLKEN_Msk;
	k_spinlock_key_t key;

	LOG_DBG("gckdiv = %d, parent id = %d\n", gck->gckdiv, gck->parent_id);

	mask |= PMC_PCR_GCLKDIV_Msk | gck->layout->gckcss_mask;
	val |= FIELD_PREP(PMC_PCR_GCLKDIV_Msk, gck->gckdiv);
	val |= FIELD_PREP(gck->layout->gckcss_mask, gck->parent_id);

	key = k_spin_lock(gck->lock);
	regmap_write((void *)gck->pmc, gck->layout->offset, (gck->id & gck->layout->pid_mask));
	regmap_update_bits((void *)gck->pmc, gck->layout->offset, mask, val);
	k_spin_unlock(gck->lock, key);

	return 0;
}

static int clk_generated_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_generated *gck = to_clk_generated(dev);
	k_spinlock_key_t key = k_spin_lock(gck->lock);

	regmap_write((void *)gck->pmc, gck->layout->offset, (gck->id & gck->layout->pid_mask));
	regmap_update_bits((void *)gck->pmc, gck->layout->offset,
			   gck->layout->cmd | PMC_PCR_GCLKEN_Msk,
			   gck->layout->cmd);

	k_spin_unlock(gck->lock, key);

	return 0;
}

static enum clock_control_status clk_generated_get_status(const struct device *dev,
							  clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_generated *gck = to_clk_generated(dev);
	k_spinlock_key_t key = k_spin_lock(gck->lock);
	uint32_t status;

	regmap_write((void *)gck->pmc, gck->layout->offset, (gck->id & gck->layout->pid_mask));
	regmap_read((void *)gck->pmc, gck->layout->offset, &status);

	k_spin_unlock(gck->lock, key);

	if (!!(status & PMC_PCR_GCLKEN_Msk)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static int clk_generated_get_rate(const struct device *dev,
				  clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct clk_generated *gck = to_clk_generated(dev);
	const struct device *parent = NULL;
	uint32_t status, css, i = 0;
	k_spinlock_key_t key;
	int ret;

	if (clk_generated_get_status(dev, sys) == CLOCK_CONTROL_STATUS_OFF) {
		*rate = 0;
		return 0;
	}

	key = k_spin_lock(gck->lock);
	regmap_write((void *)gck->pmc, gck->layout->offset, (gck->id & gck->layout->pid_mask));
	regmap_read((void *)gck->pmc, gck->layout->offset, &status);
	k_spin_unlock(gck->lock, key);

	while (!(gck->layout->gckcss_mask & (1 << i))) {
		i++;
	}
	css = (status & gck->layout->gckcss_mask) >> i;

	for (i = 0; i < gck->num_parents; i++) {
		if (css == gck->mux_table[i]) {
			parent = gck->parents[i];
			break;
		}
	}
	if (!parent) {
		LOG_ERR("find parent clock failed.");
		return -ENXIO;
	}

	ret = clock_control_get_rate(parent, NULL, rate);
	if (ret) {
		LOG_ERR("get parent clock rate failed.");
		return ret;
	}

	gck->gckdiv = FIELD_GET(PMC_PCR_GCLKDIV_Msk, status);
	*rate /= (gck->gckdiv + 1);

	return 0;
}

static DEVICE_API(clock_control, generated_api) = {
	.on = clk_generated_on,
	.off = clk_generated_off,
	.get_rate = clk_generated_get_rate,
	.get_status = clk_generated_get_status,
};

int clk_register_generated(pmc_registers_t *const pmc, struct k_spinlock *lock,
			   const struct clk_pcr_layout *layout,
			   const char *name,
			   const struct device **parents, uint32_t *mux_table,
			   uint8_t num_parents, uint8_t id,
			   const struct clk_range *range, int chg_pid, struct device **clk)
{
	struct clk_generated *gck;

	if (!parents) {
		return -EINVAL;
	}

	gck = &clocks_gck[clocks_gck_idx++];
	if (clocks_gck_idx > ARRAY_SIZE(clocks_gck)) {
		LOG_ERR("Array for generated clock not enough");
		return -ENOMEM;
	}
	if (num_parents > ARRAY_SIZE(gck->parents)) {
		LOG_ERR("Array for parent clock not enough");
		return -ENOMEM;
	}
	if (num_parents > ARRAY_SIZE(gck->mux_table)) {
		LOG_ERR("Array for mux table not enough");
		return -ENOMEM;
	}
	memcpy(gck->parents, parents, sizeof(struct device *) * num_parents);

	*clk = &gck->clk;
	(*clk)->name = name;
	(*clk)->api = &generated_api;
	gck->num_parents = num_parents;
	gck->id = id;
	gck->pmc = pmc;
	gck->lock = lock;
	gck->range = *range;
	gck->chg_pid = chg_pid;
	gck->layout = layout;
	memcpy(gck->mux_table, mux_table, num_parents * sizeof(mux_table));

	return 0;
}
