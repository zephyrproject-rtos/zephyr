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
LOG_MODULE_REGISTER(clk_mck, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define MASTER_PRES_MASK	0x7
#define MASTER_PRES_MAX		MASTER_PRES_MASK
#define MASTER_DIV_SHIFT	8
#define MASTER_DIV_MASK		0x7

#define PMC_MCR_CSS_SHIFT	16

#define MASTER_MAX_ID		(SOC_NUM_CLOCK_MASTER - 1)

#define to_clk_master(ptr) CONTAINER_OF(ptr, struct clk_master, clk)

struct clk_master {
	pmc_registers_t *pmc;
	struct device clk;
	struct device *parents[6];
	struct k_spinlock *lock;
	const struct clk_master_layout *layout;
	const struct clk_master_characteristics *characteristics;
	uint32_t mux_table[6];
	uint32_t mckr;
	uint8_t id;
	uint8_t num_parents;
	uint8_t parent;
	uint8_t div;
};

static struct clk_master clocks_master[SOC_NUM_CLOCK_MASTER];
static uint32_t clocks_master_idx;

static inline bool clk_master_ready(struct clk_master *master)
{
	uint32_t bit = master->id ? PMC_SR_MCKXRDY_Msk : PMC_SR_MCKRDY_Msk;
	uint32_t status;

	status = master->pmc->PMC_SR;

	return !!(status & bit);
}

static int clk_master_div_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_master *master = to_clk_master(dev);
	k_spinlock_key_t key;

	key = k_spin_lock(master->lock);

	while (!clk_master_ready(master)) {
		k_busy_wait(1);
	}

	k_spin_unlock(master->lock, key);

	return 0;
}

static enum clock_control_status clk_master_div_get_status(const struct device *dev,
							   clock_control_subsys_t sys)

{
	ARG_UNUSED(sys);

	struct clk_master *master = to_clk_master(dev);
	k_spinlock_key_t key;
	bool status;

	key = k_spin_lock(master->lock);
	status = clk_master_ready(master);
	k_spin_unlock(master->lock, key);

	return status ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
}

static int clk_master_div_get_rate(const struct device *dev,
				   clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct clk_master *master = to_clk_master(dev);
	uint32_t retval = 0;

	retval = clock_control_get_rate(master->parents[master->parent], NULL, rate);
	if (!retval) {
		*rate /= master->div;
	}

	LOG_DBG("%s, rate %d", dev->name, *rate);

	return retval;
}

static DEVICE_API(clock_control, master_div_api) = {
	.on = clk_master_div_on,
	.get_rate = clk_master_div_get_rate,
	.get_status = clk_master_div_get_status,
};

int clk_register_master_div(pmc_registers_t *const pmc, const char *name,
			    const struct device *parent, const struct clk_master_layout *layout,
			    const struct clk_master_characteristics *characteristics,
			    struct k_spinlock *lock, uint32_t safe_div, struct device **clk)
{
	struct clk_master *master;
	k_spinlock_key_t key;
	uint32_t mckr;

	if (!name || !lock) {
		return -EINVAL;
	}

	master = &clocks_master[clocks_master_idx++];
	if (clocks_master_idx > ARRAY_SIZE(clocks_master)) {
		LOG_ERR("Array for master clock not enough");
		return -ENOMEM;
	}

	*clk = &master->clk;
	(*clk)->name = name;
	(*clk)->api = &master_div_api;
	memcpy(master->parents, &parent, sizeof(struct device *) * 1);
	master->num_parents = 1;
	master->parent = 0;
	master->layout = layout;
	master->characteristics = characteristics;
	master->pmc = pmc;
	master->lock = lock;

	key = k_spin_lock(master->lock);
	regmap_read((void *)master->pmc, master->layout->offset, &mckr);
	k_spin_unlock(master->lock, key);

	mckr &= layout->mask;
	mckr = (mckr >> MASTER_DIV_SHIFT) & MASTER_DIV_MASK;
	master->div = characteristics->divisors[mckr];

	return 0;
}

static int clk_mck_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct clk_master *master = to_clk_master(dev);
	int retval = 0;
	uint32_t mcr;

	const struct device *parent;
	k_spinlock_key_t key;
	uint8_t i;

	for (i = 0; ; i++) {
		if (i >= master->num_parents) {
			LOG_ERR("clk %s: source %d not found", dev->name, i);
			return ESRCH;
		}
		if (master->mux_table[i] == master->parent) {
			parent = master->parents[i];
			break;
		}
	}

	key = k_spin_lock(master->lock);
	master->pmc->PMC_MCR = master->id;
	mcr = master->pmc->PMC_MCR;
	master->div = FIELD_GET(PMC_MCR_DIV_Msk, mcr);
	k_spin_unlock(master->lock, key);

	retval = clock_control_get_rate(parent, NULL, rate);
	if (retval) {
		LOG_ERR("get parent clock rate failed.");
	} else {
		if (master->div == MASTER_PRES_MAX) {
			*rate /= 3;
		} else {
			*rate /= 1 << master->div;
		}
	}
	LOG_DBG("clk %s: rate %d", dev->name, *rate);

	return retval;
}

static int clk_mck_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_master *master = to_clk_master(dev);
	k_spinlock_key_t key;
	uint32_t val, cparent;
	uint32_t enable = PMC_MCR_EN_Msk;
	uint32_t parent = master->parent << PMC_MCR_CSS_SHIFT;
	uint32_t div = master->div << MASTER_DIV_SHIFT;

	key = k_spin_lock(master->lock);

	master->pmc->PMC_MCR = PMC_MCR_ID(master->id);
	val = master->pmc->PMC_MCR;
	reg_update_bits(master->pmc->PMC_MCR,
			enable | PMC_MCR_CSS_Msk | PMC_MCR_DIV_Msk |
			PMC_MCR_CMD_Msk | PMC_MCR_ID_Msk,
			enable | parent | div | PMC_MCR_CMD_Msk |
			PMC_MCR_ID(master->id));

	cparent = (val & PMC_MCR_CSS_Msk) >> PMC_MCR_CSS_SHIFT;

	/* Wait here only if parent is being changed. */
	while ((cparent != master->parent) && !clk_master_ready(master)) {
		k_busy_wait(1);
	}

	k_spin_unlock(master->lock, key);

	return 0;
}

static int clk_mck_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_master *master = to_clk_master(dev);
	k_spinlock_key_t key;

	key = k_spin_lock(master->lock);
	master->pmc->PMC_MCR = master->id;
	reg_update_bits(master->pmc->PMC_MCR,
			PMC_MCR_EN_Msk | PMC_MCR_CMD_Msk |
			PMC_MCR_ID_Msk,
			PMC_MCR_CMD_Msk |
			PMC_MCR_ID(master->id));
	k_spin_unlock(master->lock, key);

	return 0;
}

static enum clock_control_status clk_mck_get_status(const struct device *dev,
						    clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_master *master = to_clk_master(dev);
	k_spinlock_key_t key;
	uint32_t val;

	key = k_spin_lock(master->lock);

	master->pmc->PMC_MCR = master->id;
	val = master->pmc->PMC_MCR;

	k_spin_unlock(master->lock, key);

	if (!!(val & PMC_MCR_EN_Msk)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static DEVICE_API(clock_control, sama7g5_master_api) = {
	.on = clk_mck_on,
	.off = clk_mck_off,
	.get_rate = clk_mck_get_rate,
	.get_status = clk_mck_get_status,
};

int clk_register_master(pmc_registers_t *const pmc,
			const char *name, int num_parents,
			const struct device **parents,
			uint32_t *mux_table,
			struct k_spinlock *lock, uint8_t id,
			struct device **clk)
{
	struct clk_master *master;
	k_spinlock_key_t key;
	unsigned int val;

	if (!num_parents || !parents || !mux_table || !lock || id > MASTER_MAX_ID) {
		return -EINVAL;
	}

	master = &clocks_master[clocks_master_idx++];
	if (clocks_master_idx > ARRAY_SIZE(clocks_master)) {
		LOG_ERR("Array for master clock not enough");
		return -ENOMEM;
	}
	if (num_parents > ARRAY_SIZE(master->parents)) {
		LOG_ERR("Array for parent clock not enough");
		return -ENOMEM;
	}
	if (num_parents > ARRAY_SIZE(master->mux_table)) {
		LOG_ERR("Array for mux table not enough");
		return -ENOMEM;
	}
	memcpy(master->parents, parents, sizeof(struct device *) * num_parents);

	*clk = &master->clk;
	(*clk)->name = name;
	(*clk)->api = &sama7g5_master_api;

	master->num_parents = num_parents;
	master->pmc = pmc;
	master->id = id;
	master->lock = lock;
	memcpy(master->mux_table, mux_table, num_parents * sizeof(mux_table));

	key = k_spin_lock(master->lock);
	pmc->PMC_MCR = master->id;
	val = pmc->PMC_MCR;
	master->parent = (val & PMC_MCR_CSS_Msk) >> PMC_MCR_CSS_SHIFT;
	master->div = (val & PMC_MCR_DIV_Msk) >> MASTER_DIV_SHIFT;
	k_spin_unlock(master->lock, key);

	return 0;
}
