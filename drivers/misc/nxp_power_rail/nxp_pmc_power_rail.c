/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pmc_power_rail_controller

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/misc/nxp_power_rail/nxp_power_rail.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/sys_io.h>

/* PMC register map (offsets from DT reg base). */
#define NXP_PMC_STATUS_OFFSET      0x4U
#define NXP_PMC_CTRL_OFFSET        0xCU
#define NXP_PMC_PDRUNCFG0_OFFSET   0xA0U

#define NXP_PMC_STATUS_BUSY_MASK   BIT(0)
#define NXP_PMC_CTRL_APPLYCFG_MASK BIT(0)

/* The PDRUNCFG array provides PDRUNCFG0..PDRUNCFG5. */
#define NXP_PMC_NUM_PDRUNCFG       6U
#define NXP_PMC_BITS_PER_PDRUNCFG  32U
#define NXP_PMC_NUM_BITS           (NXP_PMC_NUM_PDRUNCFG * NXP_PMC_BITS_PER_PDRUNCFG)

struct nxp_pmc_power_rail_config {
	mm_reg_t base;
};

struct nxp_pmc_power_rail_data {
	struct k_spinlock lock;
	uint8_t refcount[NXP_PMC_NUM_BITS];
};

static inline mm_reg_t nxp_pmc_pdruncfg_addr(mm_reg_t base, uint16_t id)
{
	return base + NXP_PMC_PDRUNCFG0_OFFSET + ((id / NXP_PMC_BITS_PER_PDRUNCFG) * 4U);
}

static inline uint32_t nxp_pmc_bit_mask(uint16_t id)
{
	return BIT(id % NXP_PMC_BITS_PER_PDRUNCFG);
}

static void nxp_pmc_apply(mm_reg_t base)
{
	/* Cannot set APPLYCFG while BUSY is asserted. */
	while ((sys_read32(base + NXP_PMC_STATUS_OFFSET) & NXP_PMC_STATUS_BUSY_MASK) != 0U) {
	}
	sys_set_bits(base + NXP_PMC_CTRL_OFFSET, NXP_PMC_CTRL_APPLYCFG_MASK);
	while ((sys_read32(base + NXP_PMC_STATUS_OFFSET) & NXP_PMC_STATUS_BUSY_MASK) != 0U) {
	}
}

static int nxp_pmc_power_rail_request(const struct device *dev, uint16_t id)
{
	const struct nxp_pmc_power_rail_config *config = dev->config;
	struct nxp_pmc_power_rail_data *data = dev->data;
	k_spinlock_key_t key;

	if (id >= NXP_PMC_NUM_BITS) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (data->refcount[id]++ == 0U) {
		sys_clear_bits(nxp_pmc_pdruncfg_addr(config->base, id), nxp_pmc_bit_mask(id));
		nxp_pmc_apply(config->base);
	}
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int nxp_pmc_power_rail_release(const struct device *dev, uint16_t id)
{
	const struct nxp_pmc_power_rail_config *config = dev->config;
	struct nxp_pmc_power_rail_data *data = dev->data;
	k_spinlock_key_t key;

	if (id >= NXP_PMC_NUM_BITS) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	__ASSERT(data->refcount[id] > 0U,
		 "nxp_pmc_power_rail: id %u released without outstanding request", id);
	if ((data->refcount[id] > 0U) && (--data->refcount[id] == 0U)) {
		sys_set_bits(nxp_pmc_pdruncfg_addr(config->base, id), nxp_pmc_bit_mask(id));
		nxp_pmc_apply(config->base);
	}
	k_spin_unlock(&data->lock, key);

	return 0;
}

static DEVICE_API(nxp_power_rail, nxp_pmc_power_rail_api) = {
	.request = nxp_pmc_power_rail_request,
	.release = nxp_pmc_power_rail_release,
};

static int nxp_pmc_power_rail_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#define NXP_PMC_POWER_RAIL_DEFINE(inst)                                                            \
	static const struct nxp_pmc_power_rail_config nxp_pmc_power_rail_config_##inst = {         \
		.base = DT_INST_REG_ADDR(inst),                                                    \
	};                                                                                         \
	static struct nxp_pmc_power_rail_data nxp_pmc_power_rail_data_##inst;                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, nxp_pmc_power_rail_init, NULL, &nxp_pmc_power_rail_data_##inst,\
			      &nxp_pmc_power_rail_config_##inst, PRE_KERNEL_1,                     \
			      CONFIG_NXP_POWER_RAIL_INIT_PRIORITY, &nxp_pmc_power_rail_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_PMC_POWER_RAIL_DEFINE)
