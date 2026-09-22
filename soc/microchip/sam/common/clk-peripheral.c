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
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clk_peripheral, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define PERIPHERAL_ID_MIN	2

struct clk_peripheral {
	mem_addr_t pmc;
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
	mem_addr_t pcr_addr = periph->pmc + periph->layout->offset;
	k_spinlock_key_t key = k_spin_lock(periph->lock);

	LOG_DBG("id %d, enable %d", periph->id, enable);
	enable = enable ? PMC_PCR_EN_Msk : 0;
	sys_write32(periph->id & periph->layout->pid_mask, pcr_addr);
	sys_write32((sys_read32(pcr_addr) & ~PMC_PCR_EN_Msk) | (periph->layout->cmd | enable),
		    pcr_addr);

	/*
	 * For SAMA5D2, PMC Peripheral Clock Enable/Disable Register 0/1
	 * enables and disables the peripheral clocks.
	 */
#if defined(PMC_PCER0_REG_OFST)
	if (enable != 0) {
		if (periph->id < 32) {
			sys_set_bit(periph->pmc + PMC_PCER0_REG_OFST, periph->id);
		} else {
			sys_set_bit(periph->pmc + PMC_PCER1_REG_OFST, periph->id % 32);
		}
	} else {
		if (periph->id < 32) {
			sys_set_bit(periph->pmc + PMC_PCDR0_REG_OFST, periph->id);
		} else {
			sys_set_bit(periph->pmc + PMC_PCDR1_REG_OFST, periph->id % 32);
		}
	}
#endif

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
	sys_write32(periph->id & periph->layout->pid_mask, periph->pmc + periph->layout->offset);
	status = sys_read32(periph->pmc + periph->layout->offset);
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

	sys_write32(periph->id & periph->layout->pid_mask, periph->pmc + periph->layout->offset);
	status = sys_read32(periph->pmc + periph->layout->offset);

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
	periph->pmc = (mem_addr_t)pmc;
	periph->lock = lock;
	periph->layout = layout;
	if (range != NULL) {
		periph->range = *range;
	}

	return 0;
}
