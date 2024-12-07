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
 * lan8670/1/2 Rev.C1/C2 PHYs
 */
/* Both Rev.B0 and B1 clause 22 PHYID's are same due to B1 chip limitation */
#define PHY_ID_LAN865X_REVB  0x0007C1B3
#define PHY_ID_LAN867X_REVC1 0x0007C164
#define PHY_ID_LAN867X_REVC2 0x0007C165

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

/* LAN865x Rev.B0/B1 configuration parameters from AN1760
 * As per the Configuration Application Note AN1760 published in the below link,
 * https://www.microchip.com/en-us/application-notes/an1760
 * Revision F (DS60001760G - June 2024)
 *
 * LAN867x Rev.C1/C2 configuration settings described in AN1699 are equal to
 * the first 9 configuration settings and all the sqi fixup settings from
 * LAN865x Rev.B0/B1. So the same fixup registers and values from LAN865x
 * Rev.B0/B1 are used for LAN867x Rev.C1/C2 to avoid duplication.
 * Refer the below link for the AN1699,
 * https://www.microchip.com/en-us/application-notes/an1699
 * Revision E (DS60001699F - June 2024)
 */
static const uint32_t lan865x_revb_fixup_registers[17] = {
	0x00D0, 0x00E0, 0x00E9, 0x00F5, 0x00F4, 0x00F8, 0x00F9, 0x0081, 0x0091,
	0x0043, 0x0044, 0x0045, 0x0053, 0x0054, 0x0055, 0x0040, 0x0050,
};

static const uint16_t lan865x_revb_fixup_values[17] = {
	0x3F31, 0xC000, 0x9E50, 0x1CF8, 0xC020, 0xB900, 0x4E53, 0x0080, 0x9660,
	0x00FF, 0xFFFF, 0x0000, 0x00FF, 0xFFFF, 0x0000, 0x0002, 0x0002,
};

static const uint16_t lan865x_revb_fixup_cfg_regs[2] = {
	0x0084,
	0x008A,
};

static const uint32_t lan865x_revb_sqi_fixup_regs[12] = {
	0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5,
	0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB,
};

static const uint16_t lan865x_revb_sqi_fixup_values[12] = {
	0x0103, 0x0910, 0x1D26, 0x002A, 0x0103, 0x070D,
	0x1720, 0x0027, 0x0509, 0x0E13, 0x1C25, 0x002B,
};

static const uint16_t lan865x_revb_sqi_fixup_cfg_regs[3] = {
	0x00AD,
	0x00AE,
	0x00AF,
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
	const struct device *dev;
	struct phy_link_state state;
	phy_callback_t cb;
	void *cb_data;
	struct k_work_delayable phy_monitor_work;
};

static int phy_mc_t1s_read(const struct device *dev, uint16_t reg, uint32_t *data)
{
	const struct mc_t1s_config *cfg = dev->config;

	/* Make sure excessive bits 16-31 are reset */
	*data = 0U;

	return mdio_read(cfg->mdio, cfg->phy_addr, reg, (uint16_t *)data);
}

static int phy_mc_t1s_write(const struct device *dev, uint16_t reg, uint32_t data)
{
	const struct mc_t1s_config *cfg = dev->config;

	return mdio_write(cfg->mdio, cfg->phy_addr, reg, (uint16_t)data);
}

static int mdio_setup_c45_indirect_access(const struct device *dev, uint16_t devad, uint16_t reg)
{
	const struct mc_t1s_config *cfg = dev->config;
	int ret;

	ret = mdio_write(cfg->mdio, cfg->phy_addr, MII_MMD_ACR, devad);
	if (ret) {
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
	int ret;

	ret = mdio_read_c45(cfg->mdio, cfg->phy_addr, devad, reg, val);
	/* @retval -ENOSYS if read using Clause 45 direct access is not supported */
	if (ret == -ENOSYS) {
		/* Read C45 registers using C22 indirect access registers */
		ret = mdio_setup_c45_indirect_access(dev, devad, reg);
		if (ret) {
			return ret;
		}

		return mdio_read(cfg->mdio, cfg->phy_addr, MII_MMD_AADR, val);
	}

	return ret;
}

static int phy_mc_t1s_c45_write(const struct device *dev, uint8_t devad, uint16_t reg, uint16_t val)
{
	const struct mc_t1s_config *cfg = dev->config;
	int ret;

	ret = mdio_write_c45(cfg->mdio, cfg->phy_addr, devad, reg, val);
	/* @retval -ENOSYS if write using Clause 45 direct access is not supported */
	if (ret == -ENOSYS) {
		/* Write C45 registers using C22 indirect access registers */
		ret = mdio_setup_c45_indirect_access(dev, devad, reg);
		if (ret) {
			return ret;
		}

		return mdio_write(cfg->mdio, cfg->phy_addr, MII_MMD_AADR, val);
	}

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
	if (ret) {
		LOG_ERR("Failed MII_BMSR register read: %d\n", ret);
		return ret;
	}

	state->is_up = value & MII_BMSR_LINK_STATUS;
	state->speed = LINK_HALF_10BASE_T;

	if (memcmp(&old_state, state, sizeof(struct phy_link_state)) != 0) {
		if (state->is_up) {
			LOG_INF("PHY (%d) Link speed 10 Mb, half duplex\n", cfg->phy_addr);
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
static int lan865x_indirect_read(const struct device *dev, uint16_t addr)
{
	uint16_t val;
	int ret;

	ret = phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2, LAN865X_REG_CFGPARAM_ADDR, addr);
	if (ret) {
		return ret;
	}

	ret = phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2, LAN865X_REG_CFGPARAM_CTRL,
				   LAN865X_CFGPARAM_READ_ENABLE);
	if (ret) {
		return ret;
	}

	ret = phy_mc_t1s_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC2, LAN865X_REG_CFGPARAM_DATA, &val);
	if (ret) {
		return ret;
	}

	return val;
}

static int lan865x_generate_cfg_offsets(const struct device *dev, int8_t offsets[])
{
	const uint16_t fixup_regs[2] = {0x0004, 0x0008};
	int ret;

	for (int i = 0; i < ARRAY_SIZE(fixup_regs); i++) {
		ret = lan865x_indirect_read(dev, fixup_regs[i]);
		if (ret < 0) {
			return ret;
		}

		/* 5-bit signed value, sign extend */
		ret &= GENMASK(4, 0);
		if (ret & BIT(4)) {
			offsets[i] = ret | 0xE0;
		} else {
			offsets[i] = ret;
		}
	}

	return 0;
}

static int lan865x_read_cfg_params(const struct device *dev, const uint16_t cfg_regs[],
				   uint16_t cfg_params[], uint8_t count)
{
	uint16_t val;
	int ret;

	for (int i = 0; i < count; i++) {
		ret = phy_mc_t1s_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC2, cfg_regs[i], &val);
		if (ret < 0) {
			return ret;
		}
		cfg_params[i] = (uint16_t)val;
	}

	return 0;
}

static int lan865x_write_cfg_params(const struct device *dev, const uint16_t cfg_regs[],
				    uint16_t cfg_params[], uint8_t count)
{
	int ret;

	for (int i = 0; i < count; i++) {
		ret = phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2, cfg_regs[i],
					   cfg_params[i]);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int lan865x_setup_cfgparam(const struct device *dev, int8_t offsets[])
{
	uint16_t cfg_results[ARRAY_SIZE(lan865x_revb_fixup_cfg_regs)];
	uint16_t cfg_params[ARRAY_SIZE(lan865x_revb_fixup_cfg_regs)];
	int ret;

	ret = lan865x_read_cfg_params(dev, lan865x_revb_fixup_cfg_regs, cfg_params,
				      ARRAY_SIZE(cfg_params));
	if (ret) {
		return ret;
	}

	cfg_results[0] = FIELD_PREP(GENMASK(15, 10), 9 + offsets[0]) |
			 FIELD_PREP(GENMASK(9, 4), 14 + offsets[0]) | 0x03;
	cfg_results[1] = FIELD_PREP(GENMASK(15, 10), 40 + offsets[1]);

	return lan865x_write_cfg_params(dev, lan865x_revb_fixup_cfg_regs, cfg_results,
					ARRAY_SIZE(cfg_results));
}

static int lan865x_setup_sqi_cfgparam(const struct device *dev, int8_t offsets[])
{
	uint16_t cfg_results[ARRAY_SIZE(lan865x_revb_sqi_fixup_cfg_regs)];
	uint16_t cfg_params[ARRAY_SIZE(lan865x_revb_sqi_fixup_cfg_regs)];
	int ret;

	ret = lan865x_read_cfg_params(dev, lan865x_revb_sqi_fixup_cfg_regs, cfg_params,
				      ARRAY_SIZE(cfg_params));
	if (ret) {
		return ret;
	}

	cfg_results[0] = FIELD_PREP(GENMASK(13, 8), 5 + offsets[0]) | (9 + offsets[0]);
	cfg_results[1] = FIELD_PREP(GENMASK(13, 8), 9 + offsets[0]) | (14 + offsets[0]);
	cfg_results[2] = FIELD_PREP(GENMASK(13, 8), 17 + offsets[0]) | (22 + offsets[0]);

	return lan865x_write_cfg_params(dev, lan865x_revb_sqi_fixup_cfg_regs, cfg_results,
					ARRAY_SIZE(cfg_results));
}

static int phy_mc_lan865x_revb_config_init(const struct device *dev)
{
	int8_t offsets[2];
	int ret;

	ret = lan865x_generate_cfg_offsets(dev, offsets);
	if (ret) {
		return ret;
	}

	for (int i = 0; i < ARRAY_SIZE(lan865x_revb_fixup_registers); i++) {
		ret = phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2,
					   lan865x_revb_fixup_registers[i],
					   lan865x_revb_fixup_values[i]);
		if (ret) {
			return ret;
		}

		if (i == 1) {
			ret = lan865x_setup_cfgparam(dev, offsets);
			if (ret) {
				return ret;
			}
		}
	}

	ret = lan865x_setup_sqi_cfgparam(dev, offsets);
	if (ret) {
		return ret;
	}

	for (int i = 0; i < ARRAY_SIZE(lan865x_revb_sqi_fixup_regs); i++) {
		ret = phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2,
					   lan865x_revb_sqi_fixup_regs[i],
					   lan865x_revb_sqi_fixup_values[i]);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int phy_mc_lan867x_revc_config_init(const struct device *dev)
{
	int8_t offsets[2];
	int ret;

	ret = lan865x_generate_cfg_offsets(dev, offsets);
	if (ret) {
		return ret;
	}

	/* LAN867x Rev.C1/C2 configuration settings are equal to the first 9
	 * configuration settings and all the sqi fixup settings from LAN865x
	 * Rev.B0/B1. So the same fixup registers and values from LAN865x
	 * Rev.B0/B1 are used for LAN867x Rev.C1/C2 to avoid duplication.
	 * Refer the below links for the comparison.
	 * https://www.microchip.com/en-us/application-notes/an1760
	 * Revision F (DS60001760G - June 2024)
	 * https://www.microchip.com/en-us/application-notes/an1699
	 * Revision E (DS60001699F - June 2024)
	 */
	for (int i = 0; i < 9; i++) {
		ret = phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2,
					   lan865x_revb_fixup_registers[i],
					   lan865x_revb_fixup_values[i]);
		if (ret) {
			return ret;
		}

		if (i == 1) {
			ret = lan865x_setup_cfgparam(dev, offsets);
			if (ret) {
				return ret;
			}
		}
	}

	ret = lan865x_setup_sqi_cfgparam(dev, offsets);
	if (ret) {
		return ret;
	}

	for (int i = 0; i < ARRAY_SIZE(lan865x_revb_sqi_fixup_regs); i++) {
		ret = phy_mc_t1s_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2,
					   lan865x_revb_sqi_fixup_regs[i],
					   lan865x_revb_sqi_fixup_values[i]);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int lan86xx_config_collision_detection(const struct device *dev, bool plca_enable)
{
	uint16_t val;
	uint16_t new;
	int ret;

	ret = phy_mc_t1s_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC2, LAN86XX_REG_COL_DET_CTRL0, &val);
	if (ret) {
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

static int phy_mc_t1s_cfg_link(const struct device *dev, enum phy_link_speed speeds)
{
	ARG_UNUSED(dev);

	if (speeds & LINK_HALF_10BASE_T) {
		return 0;
	}

	return -ENOTSUP;
}

static int phy_mc_t1s_id(const struct device *dev, uint32_t *phy_id)
{
	uint32_t value;
	int ret;

	ret = phy_mc_t1s_read(dev, MII_PHYID1R, &value);
	if (ret) {
		LOG_ERR("Failed MII_PHYID1R register read: %d\n", ret);
		return ret;
	}

	*phy_id = value << 16;

	ret = phy_mc_t1s_read(dev, MII_PHYID2R, &value);
	if (ret) {
		LOG_ERR("Failed MII_PHYID2R register read: %d\n", ret);
		return ret;
	}

	*phy_id |= value;

	return 0;
}

static int phy_mc_t1s_set_plca_cfg(const struct device *dev, struct phy_plca_cfg *plca_cfg)
{
	int ret;

	ret = genphy_set_plca_cfg(dev, plca_cfg);
	if (ret) {
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
	uint32_t phy_id;
	int ret;

	data->dev = dev;

	ret = phy_mc_t1s_id(dev, &phy_id);
	if (ret) {
		return ret;
	}

	switch (phy_id) {
	case PHY_ID_LAN867X_REVC1:
	case PHY_ID_LAN867X_REVC2:
		ret = phy_mc_lan867x_revc_config_init(dev);
		if (ret) {
			LOG_ERR("PHY initial configuration error: %d\n", ret);
			return ret;
		}
		break;
	case PHY_ID_LAN865X_REVB:
		ret = phy_mc_lan865x_revb_config_init(dev);
		if (ret) {
			LOG_ERR("PHY initial configuration error: %d\n", ret);
			return ret;
		}
		break;
	default:
		LOG_ERR("Unsupported PHY ID: %x\n", phy_id);
		return -ENODEV;
	}

	ret = phy_mc_t1s_set_dt_plca(dev);
	if (ret) {
		return ret;
	}

	k_work_init_delayable(&data->phy_monitor_work, phy_monitor_work_handler);
	phy_monitor_work_handler(&data->phy_monitor_work.work);

	return 0;
}

static const struct ethphy_driver_api mc_t1s_phy_api = {
	.get_link = phy_mc_t1s_get_link,
	.cfg_link = phy_mc_t1s_cfg_link,
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
