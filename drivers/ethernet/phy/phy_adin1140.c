/*
 * Copyright (c) 2024-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief PHY driver for the ADIN1140 10BASE-T1S MAC-PHY.
 *
 * Implements the Zephyr ethernet PHY API for the ADIN1140.
 * The link is always on at 10 Mbps half duplex. Supports
 * PLCA (Physical Layer Collision Avoidance) configuration
 * for 10BASE-T1S multidrop networks via MDIO Clause 45.
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mdio.h>
#include <zephyr/net/mii.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(phy_adin1140, CONFIG_PHY_LOG_LEVEL);

/* ADIN1140 PHY identifier */
#define ADIN1140_PHY_ID 0x0283BE00

/* OA TC6 Registers */
#define PHY_OA_PLCA_CTRL0  0xCA01
#define PHY_OA_PLCA_CTRL1  0xCA02
#define PHY_OA_PLCA_STATUS 0xCA03
#define PHY_OA_PLCA_TOTMR  0xCA04
#define PHY_OA_PLCA_BURST  0xCA05

#define PHY_OA_PLCA_CTRL0_PLCAEN      BIT(15)
#define PHY_OA_PLCA_STATUS_PLCASTATUS BIT(15)

#define OA_PHY_OA_PLCA_CTRL1_NODECOUNT   8U
#define OA_PHY_OA_PLCA_BURST_MAXBURSTCNT 8U

struct phy_adin1140_config {
	const struct device *mdio;
	uint8_t phy_addr;
	struct phy_plca_cfg plca;
};

/** Read a PHY register using MDIO Clause 22. */
static inline int phy_adin1140_c22_read(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct phy_adin1140_config *const cfg = dev->config;

	return mdio_read(cfg->mdio, cfg->phy_addr, reg, val);
}

/** Write a PHY register using MDIO Clause 22. */
static inline int phy_adin1140_c22_write(const struct device *dev, uint16_t reg, uint16_t val)
{
	const struct phy_adin1140_config *const cfg = dev->config;

	return mdio_write(cfg->mdio, cfg->phy_addr, reg, val);
}

/** Read a PHY register using MDIO Clause 45. */
static int phy_adin1140_c45_read(const struct device *dev, uint8_t devad, uint16_t reg,
				 uint16_t *val)
{
	const struct phy_adin1140_config *cfg = dev->config;

	return mdio_read_c45(cfg->mdio, cfg->phy_addr, devad, reg, val);
}

/** Write a PHY register using MDIO Clause 45. */
static int phy_adin1140_c45_write(const struct device *dev, uint8_t devad, uint16_t reg,
				  uint16_t val)
{
	const struct phy_adin1140_config *cfg = dev->config;

	return mdio_write_c45(cfg->mdio, cfg->phy_addr, devad, reg, val);
}

/** PHY API: read a register (Clause 22 wrapper). */
static int phy_adin1140_reg_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	uint16_t val;
	int ret = phy_adin1140_c22_read(dev, reg_addr, &val);
	*data = val;
	return ret;
}

/** PHY API: write a register (Clause 22 wrapper). */
static int phy_adin1140_reg_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	return phy_adin1140_c22_write(dev, reg_addr, (uint16_t)data);
}

/**
 * Read and combine PHY identifier from MII registers.
 */
static int phy_adin1140_id(const struct device *dev, uint32_t *phy_id)
{
	uint16_t val;

	if (phy_adin1140_c22_read(dev, MII_PHYID1R, &val) < 0) {
		return -EIO;
	}

	*phy_id = (val & UINT16_MAX) << 16;

	if (phy_adin1140_c22_read(dev, MII_PHYID2R, &val) < 0) {
		return -EIO;
	}

	*phy_id |= (val & UINT16_MAX);

	return 0;
}

/**
 * Return cached link state. Always up for 10BASE-T1S.
 */
static int phy_adin1140_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	ARG_UNUSED(dev);

	state->is_up = true;
	state->speed = LINK_FULL_10BASE;

	return 0;
}

/**
 * Initialize PHY, verify ID, and apply PLCA configuration from devicetree.
 */
static int phy_adin1140_init(const struct device *dev)
{
	const struct phy_adin1140_config *const cfg = dev->config;
	uint32_t phy_id;
	int ret;

	ret = phy_adin1140_id(dev, &phy_id);
	if (ret < 0) {
		LOG_ERR("Failed to read PHY %u ID, %d", cfg->phy_addr, ret);
		return -ENODEV;
	}

	if (phy_id != ADIN1140_PHY_ID) {
		LOG_ERR("PHY %u unexpected PHY ID %X", cfg->phy_addr, phy_id);
		return -EINVAL;
	}

	LOG_INF("PHY %u ID %X", cfg->phy_addr, phy_id);

	/* configure PLCA from devicetree */
	if (cfg->plca.enable) {
		ret = phy_set_plca_cfg(dev, (struct phy_plca_cfg *)&cfg->plca);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

/**
 * Write PLCA configuration to the PHY via MDIO Clause 45.
 */
static int phy_adin1140_set_plca_cfg(const struct device *dev, struct phy_plca_cfg *plca_cfg)
{
	int ret;

	if (!plca_cfg->enable) {
		ret = phy_adin1140_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2, PHY_OA_PLCA_CTRL0,
					     0x0);
		if (ret == 0) {
			LOG_INF("PLCA: Disabled");
		}
		return ret;
	}

	if (plca_cfg->node_count < 1) {
		LOG_WRN("PLCA Node Count must be > 0. Restoring default value (8).");
		plca_cfg->node_count = 0x8;
	}

	/* Node ID and Node Count */
	ret = phy_adin1140_c45_write(
		dev, MDIO_MMD_VENDOR_SPECIFIC2, PHY_OA_PLCA_CTRL1,
		((uint32_t)plca_cfg->node_count << OA_PHY_OA_PLCA_CTRL1_NODECOUNT) |
			plca_cfg->node_id);
	if (ret < 0) {
		return ret;
	}

	/* TO Timer */
	ret = phy_adin1140_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2, PHY_OA_PLCA_TOTMR,
				     (uint16_t)(plca_cfg->to_timer));
	if (ret < 0) {
		return ret;
	}

	/* Burst Count and Timer */
	ret = phy_adin1140_c45_write(
		dev, MDIO_MMD_VENDOR_SPECIFIC2, PHY_OA_PLCA_BURST,
		((uint32_t)plca_cfg->burst_count << OA_PHY_OA_PLCA_BURST_MAXBURSTCNT) |
			plca_cfg->burst_timer);
	if (ret < 0) {
		return ret;
	}

	/* Enable PLCA */
	ret = phy_adin1140_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2, PHY_OA_PLCA_CTRL0,
				     PHY_OA_PLCA_CTRL0_PLCAEN);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("PLCA: Enabled - Node ID: %d, Node Count: %d, TO: %d", plca_cfg->node_id,
		plca_cfg->node_count, plca_cfg->to_timer);

	return ret;
}

/**
 * Read current PLCA configuration from the PHY.
 */
static int phy_adin1140_get_plca_cfg(const struct device *dev, struct phy_plca_cfg *plca_cfg)
{
	int ret;
	uint16_t val;

	ret = phy_adin1140_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC2, PHY_OA_PLCA_CTRL0, &val);
	if (ret < 0) {
		return ret;
	}
	plca_cfg->enable = (val & PHY_OA_PLCA_CTRL0_PLCAEN) ? true : false;

	ret = phy_adin1140_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC2, PHY_OA_PLCA_CTRL1, &val);
	if (ret < 0) {
		return ret;
	}
	plca_cfg->node_id = FIELD_GET(GENMASK(7, 0), val);
	plca_cfg->node_count = FIELD_GET(GENMASK(15, 8), val);

	ret = phy_adin1140_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC2, PHY_OA_PLCA_BURST, &val);
	if (ret < 0) {
		return ret;
	}
	plca_cfg->burst_count = FIELD_GET(GENMASK(15, 8), val);
	plca_cfg->burst_timer = FIELD_GET(GENMASK(7, 0), val);

	ret = phy_adin1140_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC2, PHY_OA_PLCA_TOTMR, &val);
	if (ret < 0) {
		return ret;
	}
	plca_cfg->to_timer = val;

	LOG_INF("PLCA config: Enabled %d, Node ID: %d, Node Count: %d, TO: %d", plca_cfg->enable,
		plca_cfg->node_id, plca_cfg->node_count, plca_cfg->to_timer);

	return ret;
}

/**
 * Read current PLCA status (active or inactive).
 */
static int phy_adin1140_get_plca_sts(const struct device *dev, bool *plca_sts)
{
	int ret;
	uint16_t val;

	ret = phy_adin1140_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC2, PHY_OA_PLCA_STATUS, &val);
	if (ret < 0) {
		return ret;
	}
	*plca_sts = (val & PHY_OA_PLCA_STATUS_PLCASTATUS) ? true : false;

	return ret;
}

static DEVICE_API(ethphy, phy_adin1140_api) = {
	.get_link = phy_adin1140_get_link_state,
	.read = phy_adin1140_reg_read,
	.write = phy_adin1140_reg_write,
	.read_c45 = phy_adin1140_c45_read,
	.write_c45 = phy_adin1140_c45_write,
#if defined(CONFIG_ETH_PHY_API_PLCA)
	.set_plca_cfg = phy_adin1140_set_plca_cfg,
	.get_plca_cfg = phy_adin1140_get_plca_cfg,
	.get_plca_sts = phy_adin1140_get_plca_sts,
#endif
};

#define ADIN1140_PHY_INITIALIZE(n, model)                                                          \
	static const struct phy_adin1140_config phy_adin##model##_config_##n = {                   \
		.mdio = DEVICE_DT_GET(DT_INST_BUS(n)),                                             \
		.phy_addr = DT_INST_REG_ADDR(n),                                                   \
		.plca =                                                                            \
			{                                                                          \
				.enable = DT_INST_PROP(n, plca_enable),                            \
				.node_id = DT_INST_PROP(n, plca_node_id),                          \
				.node_count = DT_INST_PROP(n, plca_node_count),                    \
				.burst_count = DT_INST_PROP(n, plca_burst_count),                  \
				.burst_timer = DT_INST_PROP(n, plca_burst_timer),                  \
				.to_timer = DT_INST_PROP(n, plca_to_timer),                        \
			},                                                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &phy_adin1140_init, NULL, NULL, &phy_adin##model##_config_##n,    \
			      POST_KERNEL, CONFIG_PHY_INIT_PRIORITY, &phy_adin1140_api);

#define DT_DRV_COMPAT adi_adin1140_phy
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADIN1140_PHY_INITIALIZE, 1140)
#undef DT_DRV_COMPAT
