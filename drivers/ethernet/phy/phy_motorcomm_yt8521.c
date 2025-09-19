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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_motorcomm_yt8521, CONFIG_PHY_LOG_LEVEL);

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mdio.h>
#include <zephyr/net/mii.h>

#include "phy_mii.h"

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

#define YT8521_RSSR_UTP_SPACE (0x0 << 1)

#define YT8521_RGMII_CONFIG1_REG  0xA003
#define YT8521_RC1R_RX_DELAY_MASK GENMASK(13, 10)
#define YT8521_RC1R_TX_DELAY_MASK GENMASK(3, 0)

#define YTPHY_WOL_CONFIG_REG 0xA00A
#define YTPHY_WCR_ENABLE     BIT(3)

#define YTPHY_SYNCE_CFG_REG     0xA012
#define YT8521_SCR_SYNCE_ENABLE BIT(5)

static int mc_ytphy_get_link_state(const struct device *dev, struct phy_link_state *state);

struct mc_ytphy_config {
	uint8_t phy_addr;
	const struct device *mdio;
	uint8_t rx_delay_sel;
	uint8_t tx_delay_sel;
	enum phy_link_speed default_speeds;
};

struct mc_ytphy_data {
	const struct device *dev;
	phy_callback_t cb;
	void *cb_data;
	struct phy_link_state state;
	struct k_sem sem;
	struct k_work_delayable monitor_work;
	bool autoneg_in_progress;
	k_timepoint_t autoneg_timeout;
};

/* How often to poll auto-negotiation status while waiting for it to complete */
#define MII_AUTONEG_POLL_INTERVAL_MS 100

static int mc_ytphy_read(const struct device *dev, uint16_t reg, uint32_t *data)
{
	const struct mc_ytphy_config *config = dev->config;

	/* Make sure excessive bits 16-31 are reset */
	*data = 0U;

	/* Read the PHY register */
	return mdio_read(config->mdio, config->phy_addr, reg, (uint16_t *)data);
}

static int mc_ytphy_write(const struct device *dev, uint16_t reg, uint32_t data)
{
	const struct mc_ytphy_config *config = dev->config;

	return mdio_write(config->mdio, config->phy_addr, reg, (uint16_t)data);
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
	const struct mc_ytphy_config *const cfg = dev->config;
	uint16_t mask, val = 0;
	int ret;

	ret = mc_ytphy_modify_ext(dev, YT8521_CHIP_CONFIG_REG, YT8521_CCR_RXC_DLY_EN, 0);
	if (ret) {
		return ret;
	}

	mask = YT8521_RC1R_RX_DELAY_MASK | YT8521_RC1R_TX_DELAY_MASK;

	val |= FIELD_PREP(YT8521_RC1R_RX_DELAY_MASK, cfg->rx_delay_sel);
	val |= FIELD_PREP(YT8521_RC1R_TX_DELAY_MASK, cfg->tx_delay_sel);

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

static inline enum phy_link_speed mc_ytphy_get_link_speed_stat_reg(const struct device *dev,
								   uint16_t stat_reg)
{
	enum phy_link_speed speed;

	switch (stat_reg & (SPEC_STATUS_REG_SPEED_MASK | SPEC_STATUS_REG_DUPLEX_MASK)) {
	case PHY_SPEED_10M | PHY_DUPLEX_FULL:
		speed = LINK_FULL_10BASE;
		break;
	case PHY_SPEED_10M | PHY_DUPLEX_HALF:
		speed = LINK_HALF_10BASE;
		break;
	case PHY_SPEED_100M | PHY_DUPLEX_FULL:
		speed = LINK_FULL_100BASE;
		break;
	case PHY_SPEED_100M | PHY_DUPLEX_HALF:
		speed = LINK_HALF_100BASE;
		break;
	case PHY_SPEED_1000M | PHY_DUPLEX_FULL:
		speed = LINK_FULL_1000BASE;
		break;
	case PHY_SPEED_1000M | PHY_DUPLEX_HALF:
		speed = LINK_HALF_1000BASE;
		break;
	default:
		speed = 0;
		break;
	}

	return speed;
}

static int update_link_state(const struct device *dev)
{
	const struct mc_ytphy_config *const cfg = dev->config;
	struct mc_ytphy_data *const data = dev->data;
	uint32_t stat_reg;
	uint32_t bmcr_reg;
	bool link_up;

	if (mc_ytphy_read(dev, YTPHY_SPECIFIC_STATUS_REG, &stat_reg) < 0) {
		return -EIO;
	}

	link_up = (uint16_t)stat_reg & YTPHY_SSR_LINK;

	/* If link is down, we can stop here. */
	if (!link_up) {
		data->state.speed = 0;
		if (link_up != data->state.is_up) {
			data->state.is_up = false;
			LOG_INF("PHY (%d) is down", cfg->phy_addr);
			return 0;
		}
		return -EAGAIN;
	}

	if (mc_ytphy_read(dev, MII_BMCR, &bmcr_reg) < 0) {
		return -EIO;
	}

	/* If auto-negotiation is not enabled, we only need to check the link speed */
	if ((bmcr_reg & MII_BMCR_AUTONEG_ENABLE) == 0U) {
		enum phy_link_speed new_speed = mc_ytphy_get_link_speed_stat_reg(dev, stat_reg);

		if ((data->state.speed != new_speed) || !data->state.is_up) {
			data->state.is_up = true;
			data->state.speed = new_speed;

			LOG_INF("PHY (%d) Link speed %s Mb, %s duplex", cfg->phy_addr,
				PHY_LINK_IS_SPEED_1000M(data->state.speed) ? "1000"
					: (PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100"
										     : "10"),
				PHY_LINK_IS_FULL_DUPLEX(data->state.speed) ? "full" : "half");

			return 0;
		}
		return -EAGAIN;
	}

	/* If auto-negotiation is enabled and the link was already up last time we checked,
	 * we can return immediately, as the link state has not changed.
	 * If the link was down, we will start the auto-negotiation sequence.
	 */
	if (data->state.is_up) {
		return -EAGAIN;
	}

	data->state.is_up = true;

	LOG_DBG("PHY (%d) Starting MII PHY auto-negotiate sequence", cfg->phy_addr);

	data->autoneg_timeout = sys_timepoint_calc(K_MSEC(CONFIG_PHY_AUTONEG_TIMEOUT_MS));
	return -EINPROGRESS;
}

static int check_autonegotiation_completion(const struct device *dev)
{
	const struct mc_ytphy_config *const cfg = dev->config;
	struct mc_ytphy_data *const data = dev->data;

	uint32_t stat_reg = 0;
	uint32_t bmsr_reg = 0;

	/* On some PHY chips, the BMSR bits are latched, so the first read may
	 * show incorrect status. A second read ensures correct values.
	 */
	if (mc_ytphy_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	/* Second read, clears the latched bits and gives the correct status */
	if (mc_ytphy_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	if ((bmsr_reg & MII_BMSR_AUTONEG_COMPLETE) == 0U) {
		if (sys_timepoint_expired(data->autoneg_timeout)) {
			LOG_DBG("PHY (%d) auto-negotiate timeout", cfg->phy_addr);
			return -ETIMEDOUT;
		}
		return -EINPROGRESS;
	}

	LOG_DBG("PHY (%d) auto-negotiate sequence completed", cfg->phy_addr);

	if (mc_ytphy_read(dev, YTPHY_SPECIFIC_STATUS_REG, &stat_reg) < 0) {
		return -EIO;
	}

	data->state.speed = mc_ytphy_get_link_speed_stat_reg(dev, stat_reg);

	data->state.is_up = (bmsr_reg & MII_BMSR_LINK_STATUS) != 0U;

	LOG_INF("PHY (%d) Link speed %s Mb, %s duplex", cfg->phy_addr,
		PHY_LINK_IS_SPEED_1000M(data->state.speed)
			? "1000"
			: (PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100" : "10"),
		PHY_LINK_IS_FULL_DUPLEX(data->state.speed) ? "full" : "half");

	return 0;
}

static void monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mc_ytphy_data *const data = CONTAINER_OF(dwork, struct mc_ytphy_data, monitor_work);
	const struct device *dev = data->dev;
	int rc;

	if (k_sem_take(&data->sem, K_NO_WAIT) == 0) {
		if (data->autoneg_in_progress) {
			rc = check_autonegotiation_completion(dev);
		} else {
			/* If autonegotiation is not in progress, just update the link state */
			rc = update_link_state(dev);
		}

		data->autoneg_in_progress = (rc == -EINPROGRESS);

		k_sem_give(&data->sem);

		/* If link state has changed and a callback is set, invoke callback */
		if (rc == 0) {
			invoke_link_cb(dev);
		}
	}

	k_work_reschedule(&data->monitor_work, data->autoneg_in_progress
						       ? K_MSEC(MII_AUTONEG_POLL_INTERVAL_MS)
						       : K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int mc_ytphy_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds,
			     enum phy_cfg_link_flag flags)
{
	struct mc_ytphy_data *const data = dev->data;
	const struct mc_ytphy_config *const cfg = dev->config;
	int ret = 0;

	k_sem_take(&data->sem, K_FOREVER);

	if ((flags & PHY_FLAG_AUTO_NEGOTIATION_DISABLED) != 0U) {
		ret = phy_mii_set_bmcr_reg_autoneg_disabled(dev, adv_speeds);
		if (ret >= 0) {
			data->autoneg_in_progress = false;
			k_work_reschedule(&data->monitor_work, K_NO_WAIT);
		}
	} else {
		ret = phy_mii_cfg_link_autoneg(dev, adv_speeds, true);
		if (ret >= 0) {
			LOG_DBG("PHY (%d) Starting MII PHY auto-negotiate sequence", cfg->phy_addr);
			data->autoneg_in_progress = true;
			data->autoneg_timeout =
				sys_timepoint_calc(K_MSEC(CONFIG_PHY_AUTONEG_TIMEOUT_MS));
			k_work_reschedule(&data->monitor_work,
					  K_MSEC(MII_AUTONEG_POLL_INTERVAL_MS));
		}
	}

	if (ret == -EALREADY) {
		LOG_DBG("PHY (%d) Link already configured", cfg->phy_addr);
	}

	k_sem_give(&data->sem);

	return ret;

}

static int mc_ytphy_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	struct mc_ytphy_data *const data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);

	memcpy(state, &data->state, sizeof(struct phy_link_state));

	if (state->speed == 0) {
		/* If speed is 0, then link is also down, happens when autonegotiation is in
		 * progress
		 */
		state->is_up = false;
	}

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
		LOG_ERR("PHY (%d) timeout to get PHY ID", config->phy_addr);
		return -EIO;
	}

	if (val != PHY_ID_YT8521) {
		LOG_ERR("PHY (%d) can't get PHY ID, value:0x%X", config->phy_addr, val);
		return -EIO;
	}

	if (phy_id) {
		*phy_id = val;
	} else {
		LOG_INF("PHY (%d) ID:0x%X", config->phy_addr, val);
	}

	return 0;
}

static int mc_ytphy_init(const struct device *dev)
{
	const struct mc_ytphy_config *const config = dev->config;
	struct mc_ytphy_data *const data = dev->data;
	int ret;

	k_sem_init(&data->sem, 1, 1);

	mdio_bus_enable(config->mdio);

	data->state.is_up = false;
	data->dev = dev;
	data->cb = NULL;

	if (mc_ytphy_get_id(dev, NULL)) {
		return -EIO;
	}

	/* set default reg space */
	mc_ytphy_write_ext(dev, YT8521_REG_SPACE_SELECT_REG, YT8521_RSSR_UTP_SPACE);

	mc_ytphy_modify_ext(dev, YTPHY_SYNCE_CFG_REG, YT8521_SCR_SYNCE_ENABLE, 0);

	/* Reset PHY */
	ret = mc_ytphy_soft_reset(dev);
	if (ret) {
		return -EIO;
	}

	/* Enable clock delay */
	mc_ytphy_cfg_clock_delay(dev);

	mc_ytphy_resume(dev);

	return 0;
}

static int mc_ytphy_initialize_dynamic_link(const struct device *dev)
{
	const struct mc_ytphy_config *const config = dev->config;
	struct mc_ytphy_data *const data = dev->data;
	int ret = 0;

	ret = mc_ytphy_init(dev);
	if (ret < 0) {
		return ret;
	}

	data->state.is_up = false;

	k_work_init_delayable(&data->monitor_work, monitor_work_handler);

	/* Advertise default speeds */
	mc_ytphy_cfg_link(dev, config->default_speeds, 0);

	/* This will schedule the monitor work, if not already scheduled by mc_ytphy_cfg_link(). */
	k_work_schedule(&data->monitor_work, K_NO_WAIT);

	return 0;
}

static DEVICE_API(ethphy, mc_ytphy_driver_api) = {
	.get_link = mc_ytphy_get_link_state,
	.link_cb_set = mc_ytphy_link_cb_set,
	.cfg_link = mc_ytphy_cfg_link,
	.read = mc_ytphy_read,
	.write = mc_ytphy_write,
};

#define MC_YTPHY_CONFIG(n)                                                                         \
	static const struct mc_ytphy_config mc_ytphy_config_##n = {                                \
		.phy_addr = DT_INST_REG_ADDR(n),                                                   \
		.mdio = DEVICE_DT_GET(DT_INST_BUS(n)),                                             \
		.rx_delay_sel = DT_INST_PROP_OR(n, motorcomm_rx_delay_sel, 0),                     \
		.tx_delay_sel = DT_INST_PROP_OR(n, motorcomm_tx_delay_sel, 0),                     \
		.default_speeds = PHY_INST_GENERATE_DEFAULT_SPEEDS(n),                             \
	};

#define MC_YTPHY_DATA(n)                                                                           \
	static struct mc_ytphy_data mc_ytphy_data_##n = {                                          \
		.dev = DEVICE_DT_INST_GET(n),                                                      \
		.cb = NULL,                                                                        \
		.sem = Z_SEM_INITIALIZER(mc_ytphy_data_##n.sem, 1, 1),                             \
	};

#define MC_YTPHY_INIT &mc_ytphy_initialize_dynamic_link

#define MC_YTPHY_DEVICE(n)                                                                         \
	MC_YTPHY_CONFIG(n)                                                                         \
	MC_YTPHY_DATA(n)                                                                           \
	DEVICE_DT_INST_DEFINE(n, MC_YTPHY_INIT, NULL, &mc_ytphy_data_##n, &mc_ytphy_config_##n,    \
			      POST_KERNEL, CONFIG_PHY_INIT_PRIORITY, &mc_ytphy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MC_YTPHY_DEVICE)
