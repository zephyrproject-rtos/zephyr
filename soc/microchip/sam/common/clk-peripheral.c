/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <pmc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clk_peripheral, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define PERIPHERAL_ID_MIN	2

struct clk_peripheral {
	pmc_registers_t *pmc;
	struct device clk;
	struct device *parent;
	struct clk_range range;
	struct k_spinlock *lock;
	uint32_t id;
	const struct clk_pcr_layout *layout;
};

static struct clk_peripheral clocks_periph[SOC_NUM_CLOCK_PERIPHERAL];
static uint32_t clocks_periph_idx;

#define to_clk_peripheral(ptr) CONTAINER_OF(ptr, struct clk_peripheral, clk)

static void clk_peripheral_set(struct clk_peripheral *periph,
			       uint32_t enable)
{
	k_spinlock_key_t key = k_spin_lock(periph->lock);

	LOG_DBG("id %d, enable %d", periph->id, enable);
	enable = enable ? PMC_PCR_EN_Msk : 0;
	regmap_write((void *)periph->pmc, periph->layout->offset,
		     periph->id & periph->layout->pid_mask);
	regmap_update_bits((void *)periph->pmc, periph->layout->offset,
			   periph->layout->cmd | enable,
			   periph->layout->cmd | enable);

	k_spin_unlock(periph->lock, key);
}

static int clk_peripheral_on(const struct device *clk,
			     clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	clk_peripheral_set(to_clk_peripheral(clk), 1);

	return 0;
}

static int clk_peripheral_off(const struct device *clk,
			      clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	clk_peripheral_set(to_clk_peripheral(clk), 0);

	return 0;
}

static enum clock_control_status clk_peripheral_get_status(const struct device *dev,
							   clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_peripheral *periph = to_clk_peripheral(dev);
	k_spinlock_key_t key;
	uint32_t status;

	if (periph->id < PERIPHERAL_ID_MIN) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	key = k_spin_lock(periph->lock);
	regmap_write((void *)periph->pmc, periph->layout->offset,
		     (periph->id & periph->layout->pid_mask));
	regmap_read((void *)periph->pmc, periph->layout->offset, &status);
	k_spin_unlock(periph->lock, key);

	if (!!(status & PMC_PCR_EN_Msk)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static int clk_peripheral_get_rate(const struct device *clk,
				   clock_control_subsys_t sys,
				   uint32_t *rate)
{
	ARG_UNUSED(sys);

	uint32_t retval = 0;
	uint32_t status;

	struct clk_peripheral *periph = to_clk_peripheral(clk);

	regmap_write((void *)periph->pmc, periph->layout->offset,
		     (periph->id & periph->layout->pid_mask));
	regmap_read((void *)periph->pmc, periph->layout->offset, &status);

	if (status & PMC_PCR_EN_Msk) {
		retval = clock_control_get_rate(periph->parent, NULL, rate);
	} else {
		*rate = 0;
	}
	LOG_DBG("id %d, rate %d", periph->id, *rate);

	return retval;
}

static DEVICE_API(clock_control, clk_peripheral_api) = {
	.on = clk_peripheral_on,
	.off = clk_peripheral_off,
	.get_rate = clk_peripheral_get_rate,
	.get_status = clk_peripheral_get_status,
};

int clk_register_peripheral(pmc_registers_t *const pmc,
			    struct k_spinlock *lock,
			    const struct clk_pcr_layout *layout,
			    const char *name,
			    struct device *parent,
			    uint32_t id,
			    const struct clk_range *range,
			    struct device **clk)
{
	struct clk_peripheral *periph;

	if (!name) {
		return -EINVAL;
	}

	periph = &clocks_periph[clocks_periph_idx++];
	if (clocks_periph_idx > ARRAY_SIZE(clocks_periph)) {
		LOG_ERR("Array for peripheral clock not enough");
		return -ENOMEM;
	}

	*clk = &periph->clk;
	periph->clk.name = name;
	periph->clk.api = &clk_peripheral_api;
	periph->parent = parent;
	periph->id = id;
	periph->pmc = pmc;
	periph->lock = lock;
	periph->layout = layout;
	periph->range = *range;

	return 0;
}
