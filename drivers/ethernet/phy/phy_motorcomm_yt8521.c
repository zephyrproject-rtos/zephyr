/*
 * Copyright 2025 NXP
 *
 * Inspiration from phy_mii.c, which is:
 * Copyright (c) 2021 IP-Logix Inc.
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT motorcomm_yt8521

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mdio.h>
#include <zephyr/net/mii.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_motorcomm_yt8521, CONFIG_PHY_LOG_LEVEL);

#define PHY_ID_YT8521 (0x0000011A)

/* PHY Specific Status Register */
#define SPEC_STATUS_REG_DUPLEX_MASK (1U << 13)
#define PHY_DUPLEX_HALF             (0U << 13)
#define PHY_DUPLEX_FULL             (1U << 13)

#define SPEC_STATUS_REG_SPEED_MASK (0x3U << 14)
#define PHY_SPEED_10M              (0U << 14)
#define PHY_SPEED_100M             (1U << 14)
#define PHY_SPEED_1000M            (2U << 14)

/* Specific Status Register */
#define YTPHY_SPECIFIC_STATUS_REG 0x11
#define YTPHY_SSR_LINK            BIT(10)

/* Extended Register's Address Offset Register */
#define YTPHY_PAGE_SELECT 0x1E
/* Extended Register's Data Register */
#define YTPHY_PAGE_DATA   0x1F

#define YT8521_REG_SPACE_SELECT_REG 0xA000

#define YT8521_CHIP_CONFIG_REG 0xA001
#define YT8521_CCR_RXC_DLY_EN  BIT(8)

#define YT8521_EXTREG_SLEEP_CONTROL1_REG 0x27
#define YT8521_ESC1R_SLEEP_SW            BIT(15)

#define YT8521_RSSR_UTP_SPACE      (0x0 << 1)
#define YT8521_RC1R_RGMII_1_950_NS 13

#define YT8521_RGMII_CONFIG1_REG     0xA003
#define YT8521_RC1R_RX_DELAY_MASK    GENMASK(13, 10)
#define YT8521_RC1R_GE_TX_DELAY_MASK GENMASK(3, 0)

#define YTPHY_WOL_CONFIG_REG 0xA00A
#define YTPHY_WCR_ENABLE     BIT(3)

#define YTPHY_SYNCE_CFG_REG     0xA012
#define YT8521_SCR_SYNCE_ENABLE BIT(5)

#define BIT_SET(t, v, b)                                                                           \
	do {                                                                                       \
		if (t) {                                                                           \
			(v) |= (b);                                                                \
		}                                                                                  \
	} while (0)
#define BIT_SET_CLR(t, v, b)                                                                       \
	do {                                                                                       \
		if (t) {                                                                           \
			(v) |= (b);                                                                \
		} else {                                                                           \
			(v) &= ~(b);                                                               \
		}                                                                                  \
	} while (0)

static int mc_ytphy_get_link_state(const struct device *dev, struct phy_link_state *state);

const static int speed_to_phy_link_speed[] = {
	LINK_HALF_10BASE_T,  LINK_FULL_10BASE_T,   LINK_HALF_100BASE_T,
	LINK_FULL_100BASE_T, LINK_HALF_1000BASE_T, LINK_FULL_1000BASE_T,
};

struct mc_ytphy_config {
	uint8_t addr;
	bool fixed_link;
	int fixed_speed;
	const struct device *mdio_dev;
};

struct mc_ytphy_data {
	const struct device *dev;
	phy_callback_t cb;
	void *cb_data;
	struct k_work_delayable monitor_work;
	struct phy_link_state state;
	uint32_t phy_feature;
	struct k_sem sem;
	bool autoneg_done;
	int autoneg;
};

static int mc_ytphy_read(const struct device *dev, uint16_t reg, uint32_t *data)
{
	const struct mc_ytphy_config *config = dev->config;

	/* Make sure excessive bits 16-31 are reset */
	*data = 0U;

	/* Read the PHY register */
	return mdio_read(config->mdio_dev, config->addr, reg, (uint16_t *)data);
}

static int mc_ytphy_write(const struct device *dev, uint16_t reg, uint32_t data)
{
	const struct mc_ytphy_config *config = dev->config;

	return mdio_write(config->mdio_dev, config->addr, reg, (uint16_t)data);
}

static int mc_ytphy_modify(const struct device *dev, uint16_t reg, uint16_t mask, uint16_t set)
{
	uint32_t data = 0;
	uint32_t new = 0;
	int ret;

	ret = mc_ytphy_read(dev, reg, &data);
	if (ret) {
		return ret;
	}

	new = (data & ~mask) | set;
	if (new == data) {
		return 0;
	}

	return mc_ytphy_write(dev, reg, new);
}

static int mc_ytphy_read_ext(const struct device *dev, uint16_t reg, uint32_t *data)
{
	int ret;

	ret = mc_ytphy_write(dev, YTPHY_PAGE_SELECT, reg);
	if (ret) {
		return ret;
	}

	return mc_ytphy_read(dev, YTPHY_PAGE_DATA, data);
}

static int mc_ytphy_write_ext(const struct device *dev, uint16_t reg, uint32_t data)
{
	int ret;

	ret = mc_ytphy_write(dev, YTPHY_PAGE_SELECT, reg);
	if (ret) {
		return ret;
	}

	return mc_ytphy_write(dev, YTPHY_PAGE_DATA, data);
}

static int mc_ytphy_modify_ext(const struct device *dev, uint16_t reg, uint16_t mask, uint16_t set)
{
	int ret;

	ret = mc_ytphy_write(dev, YTPHY_PAGE_SELECT, reg);
	if (ret) {
		return ret;
	}

	return mc_ytphy_modify(dev, YTPHY_PAGE_DATA, mask, set);
}

static int mc_ytphy_soft_reset(const struct device *dev)
{
	int max_cnt = 500; /* max time of reset ~500ms */
	uint32_t data;
	int ret;

	ret = mc_ytphy_modify(dev, MII_BMCR, 0, MII_BMCR_RESET);
	if (ret) {
		return ret;
	}

	while (max_cnt--) {
		k_msleep(1);

		ret = mc_ytphy_read(dev, MII_BMCR, &data);
		if (ret) {
			break;
		}

		if (!(data & MII_BMCR_RESET)) {
			ret = 0;
			break;
		}
	}

	return ret;
}

static int mc_ytphy_cfg_clock_delay(const struct device *dev)
{
	uint32_t delay = YT8521_RC1R_RGMII_1_950_NS;
	uint16_t mask, val = 0;
	int ret;

	ret = mc_ytphy_modify_ext(dev, YT8521_CHIP_CONFIG_REG, YT8521_CCR_RXC_DLY_EN, 0);
	if (ret) {
		return ret;
	}

	mask = YT8521_RC1R_RX_DELAY_MASK | YT8521_RC1R_GE_TX_DELAY_MASK;

	val = FIELD_PREP(YT8521_RC1R_RX_DELAY_MASK, delay);
	val |= FIELD_PREP(YT8521_RC1R_GE_TX_DELAY_MASK, delay);

	return mc_ytphy_modify_ext(dev, YT8521_RGMII_CONFIG1_REG, mask, val);
}

static int mc_ytphy_resume(const struct device *dev)
{
	uint32_t wol_config;
	int ret;

	/* disable auto sleep */
	ret = mc_ytphy_modify_ext(dev, YT8521_EXTREG_SLEEP_CONTROL1_REG, YT8521_ESC1R_SLEEP_SW, 0);
	if (ret) {
		return ret;
	}

	ret = mc_ytphy_read_ext(dev, YTPHY_WOL_CONFIG_REG, &wol_config);
	if (ret) {
		return ret;
	}

	/* if wol enable, do nothing */
	if (wol_config & YTPHY_WCR_ENABLE) {
		return 0;
	}

	return mc_ytphy_modify(dev, MII_BMCR, MII_BMCR_POWER_DOWN, 0);
}

static int mc_ytphy_get_features(const struct device *dev)
{
	struct mc_ytphy_data *data = dev->data;
	uint32_t val = 0;
	int ret;

	ret = mc_ytphy_read(dev, MII_BMSR, &val);
	if (ret) {
		return ret;
	}

	data->autoneg = (val & MII_BMSR_AUTONEG_ABILITY) ? 1 : 0;

	BIT_SET(val & MII_BMSR_100BASE_X_FULL, data->phy_feature, LINK_FULL_100BASE_T);
	BIT_SET(val & MII_BMSR_100BASE_X_HALF, data->phy_feature, LINK_HALF_100BASE_T);
	BIT_SET(val & MII_BMSR_10_FULL, data->phy_feature, LINK_FULL_10BASE_T);
	BIT_SET(val & MII_BMSR_10_HALF, data->phy_feature, LINK_HALF_10BASE_T);

	if ((val & MII_BMSR_EXTEND_STATUS) == 0) {
		return 0;
	}

	ret = mc_ytphy_read(dev, MII_ESTAT, &val);
	if (ret) {
		return ret;
	}

	BIT_SET(val & MII_ESTAT_1000BASE_T_FULL, data->phy_feature, LINK_FULL_1000BASE_T);
	BIT_SET(val & MII_ESTAT_1000BASE_T_HALF, data->phy_feature, LINK_HALF_1000BASE_T);

	return 0;
}

static inline uint16_t mii_bmcr_encode_fixed(enum phy_link_speed speed)
{
	uint16_t bmcr;

	if (PHY_LINK_IS_SPEED_1000M(speed)) {
		bmcr = MII_BMCR_SPEED_1000;
	} else if (PHY_LINK_IS_SPEED_100M(speed)) {
		bmcr = MII_BMCR_SPEED_100;
	} else {
		bmcr = MII_BMCR_SPEED_10;
	}

	if (PHY_LINK_IS_FULL_DUPLEX(speed)) {
		bmcr |= MII_BMCR_DUPLEX_MODE;
	}

	return bmcr;
}

static int mc_ytphy_update_link_state(const struct device *dev)
{
	const struct mc_ytphy_config *const cfg = dev->config;
	struct mc_ytphy_data *const data = dev->data;
	char *pduplex = "unknown";
	char *pspeed = "unknown";
	uint16_t speed, duplex;
	uint32_t stat_reg;
	uint32_t bmsr_reg;
	bool autoneg_done;
	bool link_up;

	if (mc_ytphy_read(dev, YTPHY_SPECIFIC_STATUS_REG, &stat_reg) < 0) {
		return -EIO;
	}

	link_up = (uint16_t)stat_reg & YTPHY_SSR_LINK;

	/* If there is no change in link state don't proceed. */
	if (link_up == data->state.is_up) {
		return -EAGAIN;
	}

	if (data->state.is_up && !link_up) {
		LOG_WRN("PHY (%d) Link is down", cfg->addr);
	}
	data->state.is_up = link_up;

	/* If link is down, there is nothing more to be done */
	if (!link_up) {
		return 0;
	}

	if (mc_ytphy_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	autoneg_done = data->autoneg_done;
	data->autoneg_done = bmsr_reg & MII_BMSR_AUTONEG_COMPLETE ? 1 : 0;
	if (data->autoneg_done && !autoneg_done) {
		LOG_INF("PHY (%d) Link autoneg is done.\n", cfg->addr);
	}

	speed = stat_reg & SPEC_STATUS_REG_SPEED_MASK;
	duplex = stat_reg & SPEC_STATUS_REG_DUPLEX_MASK;

	switch (speed | duplex) {
	case PHY_SPEED_10M | PHY_DUPLEX_FULL:
		data->state.speed = LINK_FULL_10BASE_T;
		break;
	case PHY_SPEED_10M | PHY_DUPLEX_HALF:
		data->state.speed = LINK_HALF_10BASE_T;
		break;
	case PHY_SPEED_100M | PHY_DUPLEX_FULL:
		data->state.speed = LINK_FULL_100BASE_T;
		break;
	case PHY_SPEED_100M | PHY_DUPLEX_HALF:
		data->state.speed = LINK_HALF_100BASE_T;
		break;
	case PHY_SPEED_1000M | PHY_DUPLEX_FULL:
		data->state.speed = LINK_FULL_1000BASE_T;
		break;
	case PHY_SPEED_1000M | PHY_DUPLEX_HALF:
		data->state.speed = LINK_HALF_1000BASE_T;
		break;
	}

	if (PHY_LINK_IS_SPEED_1000M(data->state.speed)) {
		pspeed = "1000";
	} else if (PHY_LINK_IS_SPEED_100M(data->state.speed)) {
		pspeed = "100";
	} else {
		pspeed = "10";
	}

	if (PHY_LINK_IS_FULL_DUPLEX(data->state.speed)) {
		pduplex = "full";
	} else {
		pduplex = "half";
	}

	LOG_INF("PHY (%d) Link speed %s Mb, %s duplex", cfg->addr, pspeed, pduplex);

	return 0;
}

static void invoke_link_cb(const struct device *dev)
{
	struct mc_ytphy_data *const data = dev->data;
	struct phy_link_state state;

	if (data->cb == NULL) {
		return;
	}

	mc_ytphy_get_link_state(dev, &state);

	data->cb(data->dev, &state, data->cb_data);
}

static void monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mc_ytphy_data *data = CONTAINER_OF(dwork, struct mc_ytphy_data, monitor_work);
	const struct device *dev = data->dev;
	int ret;

	k_sem_take(&data->sem, K_FOREVER);

	ret = mc_ytphy_update_link_state(dev);

	k_sem_give(&data->sem);

	/* If link state has changed and a callback is set, invoke callback */
	if (ret == 0) {
		invoke_link_cb(dev);
	}

	/* Submit delayed work */
	k_work_reschedule(&data->monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int mc_ytphy_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds)
{
	uint32_t anar_reg;
	uint32_t bmcr_reg;
	uint32_t bmsr_reg;
	uint32_t c1kt_reg;

	if (mc_ytphy_read(dev, MII_ANAR, &anar_reg) < 0) {
		return -EIO;
	}

	if (mc_ytphy_read(dev, MII_BMCR, &bmcr_reg) < 0) {
		return -EIO;
	}

	if (mc_ytphy_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	if (bmsr_reg & MII_BMSR_EXTEND_STATUS) {
		if (mc_ytphy_read(dev, MII_1KTCR, &c1kt_reg) < 0) {
			return -EIO;
		}
	}

	BIT_SET_CLR(adv_speeds & LINK_HALF_10BASE_T, anar_reg, MII_ADVERTISE_10_HALF);
	BIT_SET_CLR(adv_speeds & LINK_FULL_10BASE_T, anar_reg, MII_ADVERTISE_10_FULL);

	BIT_SET_CLR(adv_speeds & LINK_HALF_100BASE_T, anar_reg, MII_ADVERTISE_100_HALF);
	BIT_SET_CLR(adv_speeds & LINK_FULL_100BASE_T, anar_reg, MII_ADVERTISE_100_FULL);

	if (bmsr_reg & MII_BMSR_EXTEND_STATUS) {
		BIT_SET_CLR(adv_speeds & LINK_HALF_1000BASE_T, c1kt_reg, MII_ADVERTISE_1000_HALF);
		BIT_SET_CLR(adv_speeds & LINK_FULL_1000BASE_T, c1kt_reg, MII_ADVERTISE_1000_FULL);

		if (mc_ytphy_write(dev, MII_1KTCR, c1kt_reg) < 0) {
			return -EIO;
		}
	}

	if (mc_ytphy_write(dev, MII_ANAR, anar_reg) < 0) {
		return -EIO;
	}

	bmcr_reg |= MII_BMCR_AUTONEG_ENABLE | MII_BMCR_AUTONEG_RESTART;
	if (mc_ytphy_write(dev, MII_BMCR, bmcr_reg) < 0) {
		return -EIO;
	}

	return 0;
}

static int mc_ytphy_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	struct mc_ytphy_data *const data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);

	memcpy(state, &data->state, sizeof(struct phy_link_state));

	k_sem_give(&data->sem);

	return 0;
}

static int mc_ytphy_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct mc_ytphy_data *const data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	/**
	 * Immediately invoke the callback to notify the caller of the
	 * current link status.
	 */
	invoke_link_cb(dev);

	return 0;
}

static int mc_ytphy_get_id(const struct device *dev, uint32_t *phy_id)
{
	const struct mc_ytphy_config *const config = dev->config;
	uint32_t cnt = 1000;
	uint32_t val = 0;

	do {
		if (mc_ytphy_read(dev, MII_PHYID2R, &val) < 0) {
			k_msleep(1);
		}
	} while (val != PHY_ID_YT8521 && --cnt > 0);

	if (cnt == 0U) {
		LOG_ERR("PHY (%d) timeout to get PHY ID", config->addr);
		return -EIO;
	}

	if (val != PHY_ID_YT8521) {
		LOG_ERR("PHY (%d) can't get PHY ID, value:0x%X", config->addr, val);
		return -EIO;
	}

	if (phy_id) {
		*phy_id = val;
	} else {
		LOG_INF("PHY (%d) ID:0x%X", config->addr, val);
	}

	return 0;
}

static int mc_ytphy_init(const struct device *dev)
{
	const struct mc_ytphy_config *const config = dev->config;
	struct mc_ytphy_data *const data = dev->data;
	uint16_t mask, val = 0;
	int ret;

	k_sem_init(&data->sem, 1, 1);

	mdio_bus_enable(config->mdio_dev);

	data->autoneg_done = false;
	data->state.is_up = false;
	data->dev = dev;
	data->cb = NULL;

	if (mc_ytphy_get_id(dev, NULL)) {
		return -EIO;
	}

	/* set default reg space */
	mc_ytphy_write_ext(dev, YT8521_REG_SPACE_SELECT_REG, YT8521_RSSR_UTP_SPACE);

	mc_ytphy_modify_ext(dev, YTPHY_SYNCE_CFG_REG, YT8521_SCR_SYNCE_ENABLE, 0);

	ret = mc_ytphy_get_features(dev);
	if (ret) {
		return -EIO;
	}

	/* Reset PHY */
	ret = mc_ytphy_soft_reset(dev);
	if (ret) {
		return -EIO;
	}

	/* Enable clock delay */
	mc_ytphy_cfg_clock_delay(dev);

	mc_ytphy_resume(dev);

	if (config->fixed_link) {
		data->state.speed = speed_to_phy_link_speed[config->fixed_speed];
		val = mii_bmcr_encode_fixed(data->state.speed);

		LOG_INF("fixed_link: speed:%d-0x%X\n", config->fixed_speed, data->state.speed);
		mask = ~(MII_BMCR_LOOPBACK | MII_BMCR_ISOLATE | MII_BMCR_POWER_DOWN);
		mc_ytphy_modify(dev, MII_BMCR, mask, val);

		data->autoneg_done = true;
		data->state.is_up = true;
	} else {
		/* Auto negotiation, advertise all supported speeds */
		mc_ytphy_cfg_link(dev, data->phy_feature);

		k_work_init_delayable(&data->monitor_work, monitor_work_handler);

		monitor_work_handler(&data->monitor_work.work);
	}

	return 0;
}

static DEVICE_API(ethphy, mc_ytphy_driver_api) = {
	.get_link = mc_ytphy_get_link_state,
	.cfg_link = mc_ytphy_cfg_link,
	.link_cb_set = mc_ytphy_link_cb_set,
	.read = mc_ytphy_read,
	.write = mc_ytphy_write,
};

#define MC_YTPHY_CONFIG(n)                                                                         \
	static const struct mc_ytphy_config mc_ytphy_config_##n = {                                \
		.addr = DT_INST_REG_ADDR(n),                                                       \
		.fixed_link = DT_INST_NODE_HAS_PROP(n, fixed_link),                                \
		.fixed_speed = DT_INST_ENUM_IDX_OR(n, fixed_link, 0),                              \
		.mdio_dev = DEVICE_DT_GET(DT_INST_BUS(n)),                                         \
	};

#define MC_YTPHY_DEVICE(n)                                                                         \
	MC_YTPHY_CONFIG(n);                                                                        \
	static struct mc_ytphy_data mc_ytphy_data_##n;                                             \
	DEVICE_DT_INST_DEFINE(n, &mc_ytphy_init, NULL, &mc_ytphy_data_##n, &mc_ytphy_config_##n,   \
			      POST_KERNEL, CONFIG_PHY_INIT_PRIORITY, &mc_ytphy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MC_YTPHY_DEVICE)
