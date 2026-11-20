/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT alif_phy_regulator

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* PHY switch control bits, relative to en-offset */
#define PHY_PWR_MASK BIT(0)
#define PHY_ISO      BIT(1)

/* D-PHY 1.8 V supply control bits, relative to en-offset */
#define LDO_BYP_EN  BIT(0)
#define LDO_BYP_VAL BIT(1)

struct regulator_alif_phy_config {
	struct regulator_common_config common;
	DEVICE_MMIO_NAMED_ROM(regs);
	uint8_t en_offset;
};

struct regulator_alif_phy_data {
	struct regulator_common_data common;
	DEVICE_MMIO_NAMED_RAM(regs);
};

#define DEV_DATA(dev) ((struct regulator_alif_phy_data *)((dev)->data))
#define DEV_CFG(dev)  ((struct regulator_alif_phy_config *)((dev)->config))

/* PWR_CTRL is shared by all switches, so serialize read-modify-write cycles */
static struct k_spinlock pwr_ctrl_lock;

static uint32_t regulator_alif_phy_read(const struct device *dev)
{
	const struct regulator_alif_phy_config *config = dev->config;
	mem_addr_t base = DEVICE_MMIO_NAMED_GET(dev, regs);

	return (sys_read32(base) >> config->en_offset) & BIT_MASK(2);
}

static void regulator_alif_phy_update(const struct device *dev, uint32_t clear, uint32_t set)
{
	const struct regulator_alif_phy_config *config = dev->config;
	k_spinlock_key_t key = k_spin_lock(&pwr_ctrl_lock);
	mem_addr_t base = DEVICE_MMIO_NAMED_GET(dev, regs);
	uint32_t val = sys_read32(base);

	val &= ~(clear << config->en_offset);
	val |= set << config->en_offset;
	sys_write32(val, base);

	k_spin_unlock(&pwr_ctrl_lock, key);
}

static int regulator_alif_phy_enable(const struct device *dev)
{
	regulator_alif_phy_update(dev, PHY_PWR_MASK | PHY_ISO, 0U);

	return 0;
}

static int regulator_alif_phy_disable(const struct device *dev)
{
	regulator_alif_phy_update(dev, 0U, PHY_ISO);

	return 0;
}

static DEVICE_API(regulator, regulator_alif_phy_api) = {
	.enable = regulator_alif_phy_enable,
	.disable = regulator_alif_phy_disable,
};

static int regulator_alif_phy_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, regs, K_MEM_CACHE_NONE);

	regulator_common_data_init(dev);

	return regulator_common_init(dev, regulator_alif_phy_read(dev) == 0U);
}

static int regulator_alif_ldo_enable(const struct device *dev)
{
	/* With bypass disabled the supply follows the PD-6 power state */
	regulator_alif_phy_update(dev, LDO_BYP_EN | LDO_BYP_VAL, 0U);

	return 0;
}

static int regulator_alif_ldo_disable(const struct device *dev)
{
	regulator_alif_phy_update(dev, LDO_BYP_VAL, LDO_BYP_EN);

	return 0;
}

static DEVICE_API(regulator, regulator_alif_ldo_api) = {
	.enable = regulator_alif_ldo_enable,
	.disable = regulator_alif_ldo_disable,
};

static int regulator_alif_ldo_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, regs, K_MEM_CACHE_NONE);
	uint32_t val = regulator_alif_phy_read(dev);

	regulator_common_data_init(dev);

	return regulator_common_init(dev, ((val & LDO_BYP_EN) == 0U) ||
						  ((val & LDO_BYP_VAL) != 0U));
}

#define REGULATOR_ALIF_PHY_DEFINE(node_id, api, init_fn)                                           \
	BUILD_ASSERT(DT_PROP(node_id, en_offset) < 31,                                             \
		     "en-offset must leave room for two bits in PWR_CTRL");                        \
                                                                                                   \
	static struct regulator_alif_phy_data data_##node_id;                                      \
                                                                                                   \
	static const struct regulator_alif_phy_config config_##node_id = {                         \
		DEVICE_MMIO_NAMED_ROM_INIT(regs, DT_PARENT(node_id)),                              \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.en_offset = DT_PROP(node_id, en_offset),                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, init_fn, NULL, &data_##node_id, &config_##node_id, POST_KERNEL,  \
			 CONFIG_REGULATOR_ALIF_PHY_INIT_PRIORITY, &api);

#define REGULATOR_ALIF_PHY_DEFINE_IF_OKAY(node_id, api, init_fn)                                  \
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(node_id),                                              \
		    (REGULATOR_ALIF_PHY_DEFINE(node_id, api, init_fn)), ())

#define REGULATOR_ALIF_PHY_DEFINE_CHILD(inst, child, api, init_fn)                                 \
	REGULATOR_ALIF_PHY_DEFINE_IF_OKAY(DT_INST_CHILD(inst, child), api, init_fn)

#define REGULATOR_ALIF_PHY_DEFINE_ALL(inst)                                                        \
	REGULATOR_ALIF_PHY_DEFINE_CHILD(inst, mipi_tx_dphy, regulator_alif_phy_api,                \
					regulator_alif_phy_init)                                   \
	REGULATOR_ALIF_PHY_DEFINE_CHILD(inst, mipi_rx_dphy, regulator_alif_phy_api,                \
					regulator_alif_phy_init)                                   \
	REGULATOR_ALIF_PHY_DEFINE_CHILD(inst, mipi_pll_dphy, regulator_alif_phy_api,               \
					regulator_alif_phy_init)                                   \
	REGULATOR_ALIF_PHY_DEFINE_CHILD(inst, usb_phy, regulator_alif_phy_api,                     \
					regulator_alif_phy_init)                                   \
	REGULATOR_ALIF_PHY_DEFINE_CHILD(inst, ldo_phy, regulator_alif_ldo_api,                     \
					regulator_alif_ldo_init)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_ALIF_PHY_DEFINE_ALL)
