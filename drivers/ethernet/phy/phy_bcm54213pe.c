/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * Based on phy_realtek_rtl8211f.c, which is:
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm54213pe

#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/sys/util.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_bcm54213pe, CONFIG_ETHERNET_LOG_LEVEL);

#include "phy_mii.h"

#define BRCM_OUI_MSB 0x600DU

/* Auxiliary control register and its shadow registers */
#define BCM54XX_AUX_CTL                   0x18U
#define BCM54XX_AUXCTL_SHDWSEL_MASK       0x0007U
#define BCM54XX_AUXCTL_SHDWSEL_READ_SHIFT 12U
#define BCM54XX_AUXCTL_SHDWSEL_MISC       0x07U
#define BCM54XX_AUXCTL_MISC_WREN          BIT(15)
#define BCM54XX_AUXCTL_MISC_RGMII_SKEW_EN BIT(8)

/* 0x1c shadow registers */
#define BCM54XX_SHD               0x1CU
#define BCM54XX_SHD_WRITE         BIT(15)
#define BCM54XX_SHD_VAL(x)        (((x) & 0x1FU) << 10U)
#define BCM54XX_SHD_DATA_MASK     GENMASK(9, 0)
#define BCM54XX_SHD_CLK_CTL       0x03U
#define BCM54XX_CLK_CTL_GTXCLK_EN BIT(9)

/* 1000BASE-T status bits sit 2 above the matching 1000BASE-T control bits */
#define MII_1KSTSR_OFFSET 2U

#define BCM54213PE_RESET_TIMEOUT_MS 600
#define BCM54213PE_RESET_POLL_MS    10

enum bcm54213pe_rgmii_delay {
	BCM54213PE_RGMII_DELAY_NONE,
	BCM54213PE_RGMII_DELAY_ID,
	BCM54213PE_RGMII_DELAY_RXID,
	BCM54213PE_RGMII_DELAY_TXID,
};

struct bcm54213pe_config {
	uint8_t addr;
	const struct device *mdio_dev;
	enum phy_link_speed default_speeds;
	enum bcm54213pe_rgmii_delay rgmii_delay;
};

struct bcm54213pe_data {
	const struct device *dev;
	struct phy_link_state state;
	phy_callback_t cb;
	void *cb_data;
	struct k_mutex mutex;
	struct k_work_delayable phy_monitor_work;
};

static int phy_bcm54213pe_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	const struct bcm54213pe_config *config = dev->config;
	uint16_t value;
	int ret;

	ret = mdio_read(config->mdio_dev, config->addr, reg_addr, &value);
	if (ret != 0) {
		return ret;
	}

	*data = value;

	return 0;
}

static int phy_bcm54213pe_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	const struct bcm54213pe_config *config = dev->config;

	return mdio_write(config->mdio_dev, config->addr, reg_addr, (uint16_t)data);
}

static int phy_bcm54213pe_auxctl_read(const struct device *dev, uint16_t shadow, uint32_t *val)
{
	int ret;

	/* Select the shadow register in both the write and the read-back selector */
	ret = phy_bcm54213pe_write(dev, BCM54XX_AUX_CTL,
				   BCM54XX_AUXCTL_SHDWSEL_MASK |
					   (shadow << BCM54XX_AUXCTL_SHDWSEL_READ_SHIFT));
	if (ret != 0) {
		return ret;
	}

	return phy_bcm54213pe_read(dev, BCM54XX_AUX_CTL, val);
}

static int phy_bcm54213pe_auxctl_write(const struct device *dev, uint16_t shadow, uint32_t val)
{
	return phy_bcm54213pe_write(dev, BCM54XX_AUX_CTL, shadow | val);
}

static int phy_bcm54213pe_shadow_read(const struct device *dev, uint16_t shadow, uint32_t *val)
{
	int ret;

	ret = phy_bcm54213pe_write(dev, BCM54XX_SHD, BCM54XX_SHD_VAL(shadow));
	if (ret != 0) {
		return ret;
	}

	ret = phy_bcm54213pe_read(dev, BCM54XX_SHD, val);
	if (ret != 0) {
		return ret;
	}

	*val &= BCM54XX_SHD_DATA_MASK;

	return 0;
}

static int phy_bcm54213pe_shadow_write(const struct device *dev, uint16_t shadow, uint32_t val)
{
	return phy_bcm54213pe_write(dev, BCM54XX_SHD,
				    BCM54XX_SHD_WRITE | BCM54XX_SHD_VAL(shadow) |
					    (val & BCM54XX_SHD_DATA_MASK));
}

static int phy_bcm54213pe_reset(const struct device *dev)
{
	const struct bcm54213pe_config *config = dev->config;
	k_timepoint_t timeout = sys_timepoint_calc(K_MSEC(BCM54213PE_RESET_TIMEOUT_MS));
	uint32_t reg_val;
	int ret;

	ret = phy_bcm54213pe_write(dev, MII_BMCR, MII_BMCR_RESET);
	if (ret != 0) {
		LOG_ERR("Error writing phy (%d) basic control register", config->addr);
		return ret;
	}

	/* Wait for the self-clearing reset bit and valid register contents */
	do {
		k_msleep(BCM54213PE_RESET_POLL_MS);

		ret = phy_bcm54213pe_read(dev, MII_BMCR, &reg_val);
		if (ret != 0) {
			LOG_ERR("Error reading phy (%d) basic control register", config->addr);
			return ret;
		}

		if ((reg_val & MII_BMCR_RESET) != 0U) {
			continue;
		}

		ret = phy_bcm54213pe_read(dev, MII_PHYID1R, &reg_val);
		if (ret != 0) {
			LOG_ERR("Error reading phy (%d) identifier register 1", config->addr);
			return ret;
		}

		if (reg_val == BRCM_OUI_MSB) {
			return 0;
		}
	} while (!sys_timepoint_expired(timeout));

	LOG_ERR("phy (%d) reset timed out", config->addr);

	return -ETIMEDOUT;
}

static int phy_bcm54213pe_cfg_rgmii_delay(const struct device *dev)
{
	const struct bcm54213pe_config *config = dev->config;
	bool rx_delay = (config->rgmii_delay == BCM54213PE_RGMII_DELAY_ID) ||
			(config->rgmii_delay == BCM54213PE_RGMII_DELAY_RXID);
	bool tx_delay = (config->rgmii_delay == BCM54213PE_RGMII_DELAY_ID) ||
			(config->rgmii_delay == BCM54213PE_RGMII_DELAY_TXID);
	uint32_t reg_val;
	int ret;

	/* RX clock delay: RXC-RXD skew in the MISC auxiliary control shadow */
	ret = phy_bcm54213pe_auxctl_read(dev, BCM54XX_AUXCTL_SHDWSEL_MISC, &reg_val);
	if (ret != 0) {
		LOG_ERR("Error reading phy (%d) misc control register", config->addr);
		return ret;
	}

	reg_val |= BCM54XX_AUXCTL_MISC_WREN;
	if (rx_delay) {
		reg_val |= BCM54XX_AUXCTL_MISC_RGMII_SKEW_EN;
	} else {
		reg_val &= ~BCM54XX_AUXCTL_MISC_RGMII_SKEW_EN;
	}

	ret = phy_bcm54213pe_auxctl_write(dev, BCM54XX_AUXCTL_SHDWSEL_MISC, reg_val);
	if (ret != 0) {
		LOG_ERR("Error writing phy (%d) misc control register", config->addr);
		return ret;
	}

	/* TX clock delay: GTXCLK delay in the clock alignment control shadow */
	ret = phy_bcm54213pe_shadow_read(dev, BCM54XX_SHD_CLK_CTL, &reg_val);
	if (ret != 0) {
		LOG_ERR("Error reading phy (%d) clock alignment register", config->addr);
		return ret;
	}

	if (tx_delay) {
		reg_val |= BCM54XX_CLK_CTL_GTXCLK_EN;
	} else {
		reg_val &= ~BCM54XX_CLK_CTL_GTXCLK_EN;
	}

	ret = phy_bcm54213pe_shadow_write(dev, BCM54XX_SHD_CLK_CTL, reg_val);
	if (ret != 0) {
		LOG_ERR("Error writing phy (%d) clock alignment register", config->addr);
		return ret;
	}

	LOG_DBG("phy (%d) RGMII RX delay %s, TX delay %s", config->addr, rx_delay ? "on" : "off",
		tx_delay ? "on" : "off");

	return 0;
}

static int phy_bcm54213pe_resolve_speed(const struct device *dev, enum phy_link_speed *speed)
{
	uint32_t anar;
	uint32_t anlpar;
	uint32_t c1kt;
	uint32_t s1kt;
	int ret;

	ret = phy_bcm54213pe_read(dev, MII_ANAR, &anar);
	if (ret != 0) {
		return ret;
	}

	ret = phy_bcm54213pe_read(dev, MII_ANLPAR, &anlpar);
	if (ret != 0) {
		return ret;
	}

	ret = phy_bcm54213pe_read(dev, MII_1KTCR, &c1kt);
	if (ret != 0) {
		return ret;
	}

	ret = phy_bcm54213pe_read(dev, MII_1KSTSR, &s1kt);
	if (ret != 0) {
		return ret;
	}

	s1kt >>= MII_1KSTSR_OFFSET;

	if ((c1kt & s1kt & MII_ADVERTISE_1000_FULL) != 0U) {
		*speed = LINK_FULL_1000BASE;
	} else if ((c1kt & s1kt & MII_ADVERTISE_1000_HALF) != 0U) {
		*speed = LINK_HALF_1000BASE;
	} else if ((anar & anlpar & MII_ADVERTISE_100_FULL) != 0U) {
		*speed = LINK_FULL_100BASE;
	} else if ((anar & anlpar & MII_ADVERTISE_100_HALF) != 0U) {
		*speed = LINK_HALF_100BASE;
	} else if ((anar & anlpar & MII_ADVERTISE_10_FULL) != 0U) {
		*speed = LINK_FULL_10BASE;
	} else {
		*speed = LINK_HALF_10BASE;
	}

	return 0;
}

static int phy_bcm54213pe_get_link(const struct device *dev, struct phy_link_state *state)
{
	const struct bcm54213pe_config *config = dev->config;
	struct bcm54213pe_data *data = dev->data;
	struct phy_link_state old_state = data->state;
	struct phy_link_state new_state = {};
	uint32_t bmsr;
	int ret;

	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("PHY mutex lock error");
		return ret;
	}

	/* Link status is latched low, the second read returns the current state */
	ret = phy_bcm54213pe_read(dev, MII_BMSR, &bmsr);
	if (ret == 0) {
		ret = phy_bcm54213pe_read(dev, MII_BMSR, &bmsr);
	}
	if (ret != 0) {
		LOG_ERR("Error reading phy (%d) basic status register", config->addr);
		goto unlock;
	}

	new_state.is_up =
		((bmsr & MII_BMSR_LINK_STATUS) != 0U) && ((bmsr & MII_BMSR_AUTONEG_COMPLETE) != 0U);
	if (!new_state.is_up) {
		goto unlock;
	}

	ret = phy_bcm54213pe_resolve_speed(dev, &new_state.speed);
	if (ret != 0) {
		LOG_ERR("Error reading phy (%d) autonegotiation registers", config->addr);
	}

unlock:
	(void)k_mutex_unlock(&data->mutex);

	if (ret != 0) {
		return ret;
	}

	if (memcmp(&old_state, &new_state, sizeof(struct phy_link_state)) != 0) {
		LOG_INF("PHY %d is %s", config->addr, new_state.is_up ? "up" : "down");
		if (new_state.is_up) {
			const char *mbps = "10";

			if (PHY_LINK_IS_SPEED_1000M(new_state.speed)) {
				mbps = "1000";
			} else if (PHY_LINK_IS_SPEED_100M(new_state.speed)) {
				mbps = "100";
			}

			LOG_INF("PHY (%d) Link speed %s Mb, %s duplex", config->addr, mbps,
				PHY_LINK_IS_FULL_DUPLEX(new_state.speed) ? "full" : "half");
		}
	}

	memcpy(state, &new_state, sizeof(struct phy_link_state));

	return 0;
}

static int phy_bcm54213pe_cfg_link(const struct device *dev, enum phy_link_speed speeds,
				   enum phy_cfg_link_flag flags)
{
	const struct bcm54213pe_config *config = dev->config;
	struct bcm54213pe_data *data = dev->data;
	int ret;

	if ((flags & PHY_FLAG_AUTO_NEGOTIATION_DISABLED) != 0U) {
		LOG_ERR("Disabling auto-negotiation is not supported by this driver");
		return -ENOTSUP;
	}

	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("PHY mutex lock error");
		return ret;
	}

	/* No monitoring while the PHY is being reconfigured */
	k_work_cancel_delayable(&data->phy_monitor_work);

	ret = phy_mii_cfg_link_autoneg(dev, speeds, true);
	if (ret == -EALREADY) {
		ret = 0;
	} else if (ret != 0) {
		LOG_ERR("Error configuring autonegotiation for phy (%d)", config->addr);
	}

	(void)k_mutex_unlock(&data->mutex);

	k_work_reschedule(&data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));

	return ret;
}

static int phy_bcm54213pe_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct bcm54213pe_data *data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	(void)phy_bcm54213pe_get_link(dev, &data->state);

	if (data->cb != NULL) {
		data->cb(dev, &data->state, data->cb_data);
	}

	return 0;
}

static void phy_bcm54213pe_monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bcm54213pe_data *data =
		CONTAINER_OF(dwork, struct bcm54213pe_data, phy_monitor_work);
	const struct device *dev = data->dev;
	struct phy_link_state state = {};
	int ret;

	ret = phy_bcm54213pe_get_link(dev, &state);

	if ((ret == 0) && (memcmp(&state, &data->state, sizeof(struct phy_link_state)) != 0)) {
		memcpy(&data->state, &state, sizeof(struct phy_link_state));
		if (data->cb != NULL) {
			data->cb(dev, &data->state, data->cb_data);
		}
	}

	k_work_reschedule(&data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int phy_bcm54213pe_init(const struct device *dev)
{
	const struct bcm54213pe_config *config = dev->config;
	struct bcm54213pe_data *data = dev->data;
	int ret;

	data->dev = dev;

	ret = k_mutex_init(&data->mutex);
	if (ret != 0) {
		return ret;
	}

	ret = phy_bcm54213pe_reset(dev);
	if (ret != 0) {
		LOG_ERR("Failed to reset phy (%d)", config->addr);
		return ret;
	}

	ret = phy_bcm54213pe_cfg_rgmii_delay(dev);
	if (ret != 0) {
		return ret;
	}

	k_work_init_delayable(&data->phy_monitor_work, phy_bcm54213pe_monitor_work_handler);

	phy_bcm54213pe_monitor_work_handler(&data->phy_monitor_work.work);

	/* Advertise default speeds */
	(void)phy_bcm54213pe_cfg_link(dev, config->default_speeds, 0);

	return 0;
}

static DEVICE_API(ethphy, bcm54213pe_phy_api) = {
	.get_link = phy_bcm54213pe_get_link,
	.cfg_link = phy_bcm54213pe_cfg_link,
	.link_cb_set = phy_bcm54213pe_link_cb_set,
	.read = phy_bcm54213pe_read,
	.write = phy_bcm54213pe_write,
};

#define BRCM_BCM54213PE_INIT(n)                                                                    \
	static const struct bcm54213pe_config bcm54213pe_##n##_config = {                          \
		.addr = DT_INST_REG_ADDR(n),                                                       \
		.mdio_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                      \
		.default_speeds = PHY_INST_GENERATE_DEFAULT_SPEEDS(n),                             \
		.rgmii_delay = DT_INST_ENUM_IDX(n, brcm_rgmii_delay),                              \
	};                                                                                         \
                                                                                                   \
	static struct bcm54213pe_data bcm54213pe_##n##_data;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &phy_bcm54213pe_init, NULL, &bcm54213pe_##n##_data,               \
			      &bcm54213pe_##n##_config, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,     \
			      &bcm54213pe_phy_api);

DT_INST_FOREACH_STATUS_OKAY(BRCM_BCM54213PE_INIT)
