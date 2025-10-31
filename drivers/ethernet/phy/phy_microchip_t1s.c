/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_t1s_phy

#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/net/mdio.h>
#include <zephyr/drivers/mdio.h>

#define LOG_MODULE_NAME phy_mc_t1s
#define LOG_LEVEL       CONFIG_PHY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* Support: Microchip Phys:
 * lan8650/1 Rev.B0/B1 Internal PHYs
 * lan8670/1/2 Rev.C1/C2/D0 PHYs
 */
/* Both Rev.B0 and B1 clause 22 PHYID's are same due to B1 chip limitation */
#define PHY_ID_LAN865X_REVB  0x0007C1B3
#define PHY_ID_LAN867X_REVC1 0x0007C164
#define PHY_ID_LAN867X_REVC2 0x0007C165
#define PHY_ID_LAN867X_REVD0 0x0007C166

/* Configuration param registers */
#define LAN865X_REG_CFGPARAM_ADDR    0x00D8
#define LAN865X_REG_CFGPARAM_DATA    0x00D9
#define LAN865X_REG_CFGPARAM_CTRL    0x00DA
#define LAN865X_REG_STS2             0x0019
#define LAN865X_CFGPARAM_READ_ENABLE BIT(1)

/* Collision detection enable/disable registers */
#define LAN86XX_DISABLE_COL_DET   0x0000
#define LAN86XX_ENABLE_COL_DET    0x8000
#define LAN86XX_COL_DET_MASK      0x8000
#define LAN86XX_REG_COL_DET_CTRL0 0x0087

/* LAN8670/1/2 Rev.D0 Link Status Selection Register */
#define LAN867X_REG_LINK_STATUS_CTRL	0x0012
#define LINK_STATUS_CONFIGURATION	GENMASK(12, 11)
#define LINK_STATUS_SEMAPHORE		BIT(0)

/* Link Status Configuration */
#define LINK_STATUS_CONFIG_PLCA_STATUS	0x1
#define LINK_STATUS_CONFIG_SEMAPHORE	0x2

#define LINK_STATUS_SEMAPHORE_SET	0x1

/* Structure holding configuration register address and value */
typedef struct {
	uint32_t address;
	uint16_t value;
} lan865x_config;

/* LAN865x Rev.B0/B1 configuration parameters from AN1760
 * As per the Configuration Application Note AN1760 published in the below link,
 * https://www.microchip.com/en-us/application-notes/an1760
 * Revision F (DS60001760G - June 2024)
 * Addresses 0x0084, 0x008A, 0x00AD, 0x00AE and 0x00AF will be updated with cfgparam1, cfgparam2,
 * cfgparam3, cfgparam4 and cfgparam5 respectively.
 *
 * LAN867x Rev.C1/C2 configuration settings described in AN1699 are equal to
 * the first 11 configuration settings and all the sqi fixup settings from
 * LAN865x Rev.B0/B1. So the same fixup registers and values from LAN865x
 * Rev.B0/B1 are used for LAN867x Rev.C1/C2 to avoid duplication.
 * Refer the below link for the AN1699,
 * https://www.microchip.com/en-us/application-notes/an1699
 * Revision E (DS60001699F - June 2024)
 */
static lan865x_config lan865x_revb_config[] = {
	{.address = 0x00D0, .value = 0x3F31}, {.address = 0x00E0, .value = 0xC000},
	{.address = 0x0084, .value = 0x0000}, {.address = 0x008A, .value = 0x0000},
	{.address = 0x00E9, .value = 0x9E50}, {.address = 0x00F5, .value = 0x1CF8},
	{.address = 0x00F4, .value = 0xC020}, {.address = 0x00F8, .value = 0xB900},
	{.address = 0x00F9, .value = 0x4E53}, {.address = 0x0081, .value = 0x0080},
	{.address = 0x0091, .value = 0x9660}, {.address = 0x0043, .value = 0x00FF},
	{.address = 0x0044, .value = 0xFFFF}, {.address = 0x0045, .value = 0x0000},
	{.address = 0x0053, .value = 0x00FF}, {.address = 0x0054, .value = 0xFFFF},
	{.address = 0x0055, .value = 0x0000}, {.address = 0x0040, .value = 0x0002},
	{.address = 0x0050, .value = 0x0002}, {.address = 0x00AD, .value = 0x0000},
	{.address = 0x00AE, .value = 0x0000}, {.address = 0x00AF, .value = 0x0000},
	{.address = 0x00B0, .value = 0x0103}, {.address = 0x00B1, .value = 0x0910},
	{.address = 0x00B2, .value = 0x1D26}, {.address = 0x00B3, .value = 0x002A},
	{.address = 0x00B4, .value = 0x0103}, {.address = 0x00B5, .value = 0x070D},
	{.address = 0x00B6, .value = 0x1720}, {.address = 0x00B7, .value = 0x0027},
	{.address = 0x00B8, .value = 0x0509}, {.address = 0x00B9, .value = 0x0E13},
	{.address = 0x00BA, .value = 0x1C25}, {.address = 0x00BB, .value = 0x002B},
};

/* LAN867x Rev.D0 configuration parameters from AN1699
 * As per the Configuration Application Note AN1699 published in the below link,
 * https://www.microchip.com/en-us/application-notes/an1699
 * Revision G (DS60001699G - October 2025)
 */
static const lan865x_config lan867x_revd0_config[] = {
	{.address = 0x0037, .value = 0x0800},
	{.address = 0x008A, .value = 0xBFC0},
	{.address = 0x0118, .value = 0x029C},
	{.address = 0x00D6, .value = 0x1001},
	{.address = 0x0082, .value = 0x001C},
	{.address = 0x00FD, .value = 0x0C0B},
	{.address = 0x00FD, .value = 0x8C07},
	{.address = 0x0091, .value = 0x9660},
};

struct mc_t1s_plca_config {
	bool enable;
	uint8_t node_id;
	uint8_t node_count;
	uint8_t burst_count;
	uint8_t burst_timer;
	uint8_t to_timer;
};

struct mc_t1s_config {
	uint8_t phy_addr;
	const struct device *mdio;
	struct mc_t1s_plca_config *plca;
};

struct mc_t1s_data {
	uint32_t phy_id;
	const struct device *dev;
	struct phy_link_state state;
	phy_callback_t cb;
	void *cb_data;
	struct k_work_delayable phy_monitor_work;
};

static int phy_mc_t1s_read(const struct device *dev, uint16_t reg, uint32_t *data)
{
	const struct mc_t1s_config *cfg = dev->config;
	int ret;

	/* Make sure excessive bits 16-31 are reset */
	*data = 0U;

	mdio_bus_enable(cfg->mdio);

	ret = mdio_read(cfg->mdio, cfg->phy_addr, reg, (uint16_t *)data);

	mdio_bus_disable(cfg->mdio);

	return ret;
}

static int phy_mc_t1s_write(const struct device *dev, uint16_t reg, uint32_t data)
{
	const struct mc_t1s_config *cfg = dev->config;
	int ret;

	mdio_bus_enable(cfg->mdio);

	ret = mdio_write(cfg->mdio, cfg->phy_addr, reg, (uint16_t)data);

	mdio_bus_disable(cfg->mdio);

	return ret;
}

static int mdio_setup_c45_indirect_access(const struct device *dev, uint16_t devad, uint16_t reg)
{
	const struct mc_t1s_config *cfg = dev->config;
	int ret;

	ret = mdio_write(cfg->mdio, cfg->phy_addr, MII_MMD_ACR, devad);
	if (ret < 0) {
		return ret;
	}

	ret = mdio_write(cfg->mdio, cfg->phy_addr, MII_MMD_AADR, reg);
	if (ret < 0) {
		return ret;
	}

	return mdio_write(cfg->mdio, cfg->phy_addr, MII_MMD_ACR, devad | BIT(14));
}

static int phy_mc_t1s_c45_read(const struct device *dev, uint8_t devad, uint16_t reg, uint16_t *val)
{
	const struct mc_t1s_config *cfg = dev->config;
	struct mc_t1s_data *data = dev->data;
	int ret;

	/* C45 direct read access is only supported by LAN865x internal PHY */
	if (data->phy_id == PHY_ID_LAN865X_REVB) {
		return mdio_read_c45(cfg->mdio, cfg->phy_addr, devad, reg, val);
	}

	mdio_bus_enable(cfg->mdio);

	/* Read C45 registers using C22 indirect access registers */
	ret = mdio_setup_c45_indirect_access(dev, devad, reg);
	if (ret < 0) {
		mdio_bus_disable(cfg->mdio);
		return ret;
	}

	ret = mdio_read(cfg->mdio, cfg->phy_addr, MII_MMD_AADR, val);

	mdio_bus_disable(cfg->mdio);

	return ret;
}

static int phy_mc_t1s_c45_write(const struct device *dev, uint8_t devad, uint16_t reg, uint16_t val)
{
	const struct mc_t1s_config *cfg = dev->config;
	struct mc_t1s_data *data = dev->data;
	int ret;

	/* C45 direct write access is only supported by LAN865x internal PHY */
	if (data->phy_id == PHY_ID_LAN865X_REVB) {
		return mdio_write_c45(cfg->mdio, cfg->phy_addr, devad, reg, val);
	}

	mdio_bus_enable(cfg->mdio);

	/* Write C45 registers using C22 indirect access registers */
	ret = mdio_setup_c45_indirect_access(dev, devad, reg);
	if (ret < 0) {
		mdio_bus_disable(cfg->mdio);
		return ret;
	}

	ret = mdio_write(cfg->mdio, cfg->phy_addr, MII_MMD_AADR, val);

	mdio_bus_disable(cfg->mdio);

	return ret;
}

static int phy_mc_t1s_get_link(const struct device *dev, struct phy_link_state *state)
{
	const struct mc_t1s_config *cfg = dev->config;
	struct mc_t1s_data *data = dev->data;
	struct phy_link_state old_state = data->state;
	uint32_t value = 0;
	int ret;

	ret = phy_mc_t1s_read(dev, MII_BMSR, &value);
	if (ret < 0) {
		LOG_ERR("Failed MII_BMSR register read: %d\n", ret);
		return ret;
	}

	state->is_up = value & MII_BMSR_LINK_STATUS;
	state->speed = LINK_HALF_10BASE;

	if (memcmp(&old_state, state, sizeof(struct phy_link_state)) != 0) {
		if (state->is_up) {
			LOG_INF("PHY (%d) Link speed 10 Mbps, half duplex\n", cfg->phy_addr);
		}
	}

	return 0;
}

static int phy_mc_t1s_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct mc_t1s_data *data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	if (data->cb) {
		data->cb(dev, &data->state, data->cb_data);
	}

	return 0;
}

static void phy_monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mc_t1s_data *const data = CONTAINER_OF(dwork, struct mc_t1s_data, phy_monitor_work);
	const struct device *dev = data->dev;

	if (!data->cb) {
		k_work_reschedule(&data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
		return;
	}

	phy_mc_t1s_get_link(dev, &data->state);

	data->cb(dev, &data->state, data->cb_data);

	/* Submit delayed work */
	k_work_reschedule(&data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

/* Pulled from AN1760 describing 'indirect read'
 *
 * write_register(0x4, 0x00D8, addr)
 * write_register(0x4, 0x00DA, 0x2)
 * return (int8)(read_register(0x4, 0x00D9))
 *
 * 0x4 refers to memory map selector 4, which maps to MDIO_MMD_VENDOR_SPECIFIC2
 */
static int lan865x_indirect_read(const struct device *dev, uint16_t addr, uint16_t *value)
{
	int ret;

	ret = phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2, LAN865X_REG_CFGPARAM_ADDR, addr);
	if (ret < 0) {
		return ret;
	}

	ret = phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2, LAN865X_REG_CFGPARAM_CTRL,
				   LAN865X_CFGPARAM_READ_ENABLE);
	if (ret < 0) {
		return ret;
	}

	return phy_mc_t1s_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC2, LAN865X_REG_CFGPARAM_DATA,
				   value);
}

static int lan865x_calculate_offset(const struct device *dev, uint16_t address, int8_t *offset)
{
	uint16_t value;
	int ret;

	ret = lan865x_indirect_read(dev, address, &value);
	if (ret < 0) {
		return ret;
	}

	/* 5-bit signed value, sign extend */
	value &= GENMASK(4, 0);
	if (value & BIT(4)) {
		*offset = (int8_t)((uint8_t)value - 0x20);
	} else {
		*offset = (int8_t)value;
	}

	return 0;
}

static void lan865x_update_cfgparam(uint32_t address, uint16_t cfgparam)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(lan865x_revb_config); i++) {
		if (lan865x_revb_config[i].address == address) {
			lan865x_revb_config[i].value = cfgparam;
		}
	}
}

static int lan865x_calculate_update_cfgparams(const struct device *dev)
{
	uint16_t cfgparam;
	int8_t offset1;
	int8_t offset2;
	int ret;

	/* Calculate offset1 */
	ret = lan865x_calculate_offset(dev, 0x04, &offset1);
	if (ret < 0) {
		return ret;
	}

	/* Calculate offset2 */
	ret = lan865x_calculate_offset(dev, 0x08, &offset2);
	if (ret < 0) {
		return ret;
	}

	/* Calculate & update cfgparam1 for configuration */
	cfgparam = (uint16_t)(((9 + offset1) & 0x3F) << 10) |
		   (uint16_t)(((14 + offset1) & 0x3F) << 4) | 0x03;
	lan865x_update_cfgparam(0x0084, cfgparam);

	/* Calculate & update cfgparam2 for configuration */
	cfgparam = (uint16_t)(((40 + offset2) & 0x3F) << 10);
	lan865x_update_cfgparam(0x008A, cfgparam);

	/* Calculate & update cfgparam3 for configuration */
	cfgparam = (uint16_t)(((5 + offset1) & 0x3F) << 8) | (uint16_t)((9 + offset1) & 0x3F);
	lan865x_update_cfgparam(0x00AD, cfgparam);

	/* Calculate & update cfgparam4 for configuration */
	cfgparam = (uint16_t)(((9 + offset1) & 0x3F) << 8) | (uint16_t)((14 + offset1) & 0x3F);
	lan865x_update_cfgparam(0x00AE, cfgparam);

	/* Calculate & update cfgparam5 for configuration */
	cfgparam = (uint16_t)(((17 + offset1) & 0x3F) << 8) | (uint16_t)((22 + offset1) & 0x3F);
	lan865x_update_cfgparam(0x00AF, cfgparam);

	return 0;
}

static int phy_mc_lan865x_revb_config_init(const struct device *dev)
{
	int ret;

	ret = lan865x_calculate_update_cfgparams(dev);
	if (ret < 0) {
		return ret;
	}

	/* Configure lan865x initial settings */
	for (int i = 0; i < ARRAY_SIZE(lan865x_revb_config); i++) {
		ret = phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2,
					   lan865x_revb_config[i].address,
					   lan865x_revb_config[i].value);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

/* LAN867x Rev.C1/C2 configuration settings are equal to the first 11 configuration settings and all
 * the sqi fixup settings from LAN865x Rev.B0/B1. So the same fixup registers and values from
 * LAN865x Rev.B0/B1 are used for LAN867x Rev.C1/C2 to avoid duplication.
 * Refer the below links for the comparison.
 * https://www.microchip.com/en-us/application-notes/an1760
 * Revision F (DS60001760G - June 2024)
 * https://www.microchip.com/en-us/application-notes/an1699
 * Revision E (DS60001699F - June 2024)
 */
static int phy_mc_lan867x_revc_config_init(const struct device *dev)
{
	int ret;

	ret = lan865x_calculate_update_cfgparams(dev);
	if (ret < 0) {
		return ret;
	}

	for (int i = 0; i < ARRAY_SIZE(lan865x_revb_config); i++) {
		ret = phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2,
					   lan865x_revb_config[i].address,
					   lan865x_revb_config[i].value);
		if (ret < 0) {
			return ret;
		}

		/* LAN867x Rev.C1/C2 configuration settings are equal to the first 11 configuration
		 * settings and all the sqi fixup settings from LAN865x Rev.B0/B1. So the 8
		 * inbetween configuration settings are skipped.
		 */
		if (i == 10) {
			i += 8;
		}
	}

	return 0;
}

static int phy_mc_lan867x_revd0_link_status_selection(const struct device *dev, bool plca_enabled)
{
	uint16_t value;

	if (plca_enabled) {
		/* 0x1 - When PLCA is enabled: link status reflects plca_status.
		 */
		value = FIELD_PREP(LINK_STATUS_CONFIGURATION, LINK_STATUS_CONFIG_PLCA_STATUS);
	} else {
		/* 0x2 - Link status is controlled by the value written into the
		 * LINK_STATUS_SEMAPHORE bit written. Here the link semaphore
		 * bit is written with 0x1 to set the link always active in
		 * CSMA/CD mode as it doesn't support autoneg.
		 */
		value = FIELD_PREP(LINK_STATUS_CONFIGURATION, LINK_STATUS_CONFIG_SEMAPHORE) |
			FIELD_PREP(LINK_STATUS_SEMAPHORE, LINK_STATUS_SEMAPHORE_SET);
	}

	return phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2, LAN867X_REG_LINK_STATUS_CTRL,
				    value);
}

static int phy_mc_lan867x_revd0_config_init(const struct device *dev)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(lan867x_revd0_config); i++) {
		ret = phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2,
					   lan867x_revd0_config[i].address,
					   lan867x_revd0_config[i].value);
		if (ret < 0) {
			return ret;
		}
	}

	/* Initially the PHY will be in CSMA/CD mode by default. So it is
	 * required to set the link always active as it doesn't support
	 * autoneg.
	 */
	return phy_mc_lan867x_revd0_link_status_selection(dev, false);
}

static int lan86xx_config_collision_detection(const struct device *dev, bool plca_enable)
{
	uint16_t val;
	uint16_t new;
	int ret;

	ret = phy_mc_t1s_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC2, LAN86XX_REG_COL_DET_CTRL0, &val);
	if (ret < 0) {
		return ret;
	}

	if (plca_enable) {
		new = (val & ~LAN86XX_COL_DET_MASK) | LAN86XX_DISABLE_COL_DET;
	} else {
		new = (val & ~LAN86XX_COL_DET_MASK) | LAN86XX_ENABLE_COL_DET;
	}

	if (new == val) {
		return 0;
	}

	return phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2, LAN86XX_REG_COL_DET_CTRL0, new);
}

static int phy_mc_t1s_id(const struct device *dev, uint32_t *phy_id)
{
	uint32_t value;
	int ret;

	ret = phy_mc_t1s_read(dev, MII_PHYID1R, &value);
	if (ret < 0) {
		LOG_ERR("Failed MII_PHYID1R register read: %d\n", ret);
		return ret;
	}

	*phy_id = value << 16;

	ret = phy_mc_t1s_read(dev, MII_PHYID2R, &value);
	if (ret < 0) {
		LOG_ERR("Failed MII_PHYID2R register read: %d\n", ret);
		return ret;
	}

	*phy_id |= value;

	return 0;
}

static int phy_mc_t1s_set_plca_cfg(const struct device *dev, struct phy_plca_cfg *plca_cfg)
{
	struct mc_t1s_data *data = dev->data;
	int ret;

	/* Link status selection must be configured for LAN8670/1/2 Rev.D0 */
	if (data->phy_id == PHY_ID_LAN867X_REVD0) {
		ret = phy_mc_lan867x_revd0_link_status_selection(dev, plca_cfg->enable);
		if (ret < 0) {
			return ret;
		}
	}

	ret = genphy_set_plca_cfg(dev, plca_cfg);
	if (ret < 0) {
		return ret;
	}

	return lan86xx_config_collision_detection(dev, plca_cfg->enable);
}

static int phy_mc_t1s_set_dt_plca(const struct device *dev)
{
	const struct mc_t1s_config *cfg = dev->config;
	struct phy_plca_cfg plca_cfg;

	if (!cfg->plca->enable) {
		return 0;
	}

	plca_cfg.enable = cfg->plca->enable;
	plca_cfg.node_id = cfg->plca->node_id;
	plca_cfg.node_count = cfg->plca->node_count;
	plca_cfg.burst_count = cfg->plca->burst_count;
	plca_cfg.burst_timer = cfg->plca->burst_timer;
	plca_cfg.to_timer = cfg->plca->to_timer;

	return phy_mc_t1s_set_plca_cfg(dev, &plca_cfg);
}

static int phy_mc_t1s_init(const struct device *dev)
{
	struct mc_t1s_data *data = dev->data;
	int ret;

	data->dev = dev;

	ret = phy_mc_t1s_id(dev, &data->phy_id);
	if (ret < 0) {
		return ret;
	}

	switch (data->phy_id) {
	case PHY_ID_LAN867X_REVC1:
	case PHY_ID_LAN867X_REVC2:
		ret = phy_mc_lan867x_revc_config_init(dev);
		break;
	case PHY_ID_LAN865X_REVB:
		ret = phy_mc_lan865x_revb_config_init(dev);
		break;
	case PHY_ID_LAN867X_REVD0:
		ret = phy_mc_lan867x_revd0_config_init(dev);
		break;
	default:
		LOG_ERR("Unsupported PHY ID: %x\n", data->phy_id);
		return -ENODEV;
	}

	if (ret < 0) {
		LOG_ERR("PHY initial configuration error: %d\n", ret);
		return ret;
	}

	ret = phy_mc_t1s_set_dt_plca(dev);
	if (ret < 0) {
		return ret;
	}

	k_work_init_delayable(&data->phy_monitor_work, phy_monitor_work_handler);
	phy_monitor_work_handler(&data->phy_monitor_work.work);

	return 0;
}

static DEVICE_API(ethphy, mc_t1s_phy_api) = {
	.get_link = phy_mc_t1s_get_link,
	.link_cb_set = phy_mc_t1s_link_cb_set,
	.set_plca_cfg = phy_mc_t1s_set_plca_cfg,
	.get_plca_cfg = genphy_get_plca_cfg,
	.get_plca_sts = genphy_get_plca_sts,
	.read = phy_mc_t1s_read,
	.write = phy_mc_t1s_write,
	.read_c45 = phy_mc_t1s_c45_read,
	.write_c45 = phy_mc_t1s_c45_write,
};

#define MICROCHIP_T1S_PHY_INIT(n)                                                                  \
	static struct mc_t1s_plca_config mc_t1s_plca_##n##_config = {                              \
		.enable = DT_INST_PROP(n, plca_enable),                                            \
		.node_id = DT_INST_PROP(n, plca_node_id),                                          \
		.node_count = DT_INST_PROP(n, plca_node_count),                                    \
		.burst_count = DT_INST_PROP(n, plca_burst_count),                                  \
		.burst_timer = DT_INST_PROP(n, plca_burst_timer),                                  \
		.to_timer = DT_INST_PROP(n, plca_to_timer),                                        \
	};                                                                                         \
                                                                                                   \
	static const struct mc_t1s_config mc_t1s_##n##_config = {                                  \
		.phy_addr = DT_INST_REG_ADDR(n),                                                   \
		.mdio = DEVICE_DT_GET(DT_INST_PARENT(n)),                                          \
		.plca = &mc_t1s_plca_##n##_config,                                                 \
	};                                                                                         \
                                                                                                   \
	static struct mc_t1s_data mc_t1s_##n##_data;                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &phy_mc_t1s_init, NULL, &mc_t1s_##n##_data, &mc_t1s_##n##_config, \
			      POST_KERNEL, CONFIG_PHY_INIT_PRIORITY, &mc_t1s_phy_api);

DT_INST_FOREACH_STATUS_OKAY(MICROCHIP_T1S_PHY_INIT);
