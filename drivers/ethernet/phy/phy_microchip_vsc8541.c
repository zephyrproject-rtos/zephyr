/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 sensry.io
 */

#define DT_DRV_COMPAT microchip_vsc8541

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(microchip_vsc8541, CONFIG_PHY_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/drivers/mdio.h>
#include <string.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/gpio.h>

#include "phy_mii.h"

/* phy page selectors */
#define PHY_PAGE_0 0x00 /* main registers space active */
#define PHY_PAGE_1 0x01 /* reg 16 - 30 will be redirected to ext. register space 1 */
#define PHY_PAGE_2 0x02 /* reg 16 - 30 will be redirected to ext. register space 2 */
#define PHY_PAGE_3 0x03 /* reg 0 - 30 will be redirected to gpio register space */

/* virtual register, treat higher byte as page selector and lower byte is register */
#define PHY_REG(page, reg) ((page << 8) | (reg << 0))

/* Generic Register */
#define PHY_REG_PAGE0_STAT100         PHY_REG(PHY_PAGE_0, 0x10)
#define PHY_REG_PAGE0_STAT1000_EXT2   PHY_REG(PHY_PAGE_0, 0x11)
#define PHY_REG_AUX_CTRL              0x12
#define PHY_REG_PAGE0_ERROR_COUNTER_1 PHY_REG(0, 0x13)
#define PHY_REG_PAGE0_ERROR_COUNTER_2 PHY_REG(0, 0x14)
#define PHY_REG_PAGE0_EXT_CTRL_STAT   PHY_REG(PHY_PAGE_0, 0x16)
#define PHY_REG_PAGE0_EXT_CONTROL_1   PHY_REG(PHY_PAGE_0, 0x17)
#define PHY_REG_PAGE0_EXT_DEV_AUX     PHY_REG(PHY_PAGE_0, 0x1C)
#define PHY_REG_LED_MODE              0x1d

#define PHY_REG_PAGE_SELECTOR 0x1F

/* Extended Register */
#define PHY_REG_PAGE1_EXT_MODE_CTRL  PHY_REG(PHY_PAGE_1, 0x13)
#define PHY_REG_PAGE2_RGMII_CONTROL  PHY_REG(PHY_PAGE_2, 0x14)
#define PHY_REG_PAGE2_MAC_IF_CONTROL PHY_REG(PHY_PAGE_2, 0x1b)

#define PHY_REG_PAGE0_EXT_DEV_AUX_DUPLEX BIT(5)

enum vsc8541_interface {
	VSC8541_MII,
	VSC8541_RMII,
	VSC8541_GMII,
	VSC8541_RGMII,
};

/* Thread stack size */
#define STACK_SIZE 512

/* Thread priority */
#define THREAD_PRIORITY 7

struct mc_vsc8541_config {
	uint8_t addr;
	const struct device *mdio_dev;
	enum vsc8541_interface microchip_interface_type;
	enum phy_link_speed default_speeds;
	uint8_t rgmii_rx_clk_delay;
	uint8_t rgmii_tx_clk_delay;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	const struct gpio_dt_spec reset_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct gpio_dt_spec interrupt_gpio;
#endif
};

struct mc_vsc8541_data {
	const struct device *dev;

	struct phy_link_state state;
	int active_page;

	struct k_mutex mutex;

	phy_callback_t cb;
	void *cb_data;

	struct k_thread link_monitor_thread;
	uint8_t link_monitor_thread_stack[STACK_SIZE];
};

static int phy_mc_vsc8541_read(const struct device *dev, uint16_t reg_addr, uint16_t *data);
static int phy_mc_vsc8541_write(const struct device *dev, uint16_t reg_addr, uint16_t data);
static void phy_mc_vsc8541_link_monitor(void *arg1, void *arg2, void *arg3);

#if CONFIG_PHY_VERIFY_DEVICE_IDENTIFICATION
/**
 * @brief Reads the phy manufacture id and compares to designed, known model version
 */
static int phy_mc_vsc8541_verify_phy_id(const struct device *dev)
{
	uint16_t phy_id_1;
	uint16_t phy_id_2;

	if (phy_mc_vsc8541_read(dev, MII_PHYID1R, &phy_id_1) < 0) {
		return -EINVAL;
	}

	if (phy_mc_vsc8541_read(dev, MII_PHYID2R, &phy_id_2) < 0) {
		return -EINVAL;
	}

	if (phy_id_1 == 0x0007) {
		if (phy_id_2 == 0x0771) {
			LOG_INF("model vsc8541-01 rev b");
			return 0;
		}
		if (phy_id_2 == 0x0772) {
			LOG_INF("model vsc8541-02/-05 rev c");
			return 0;
		}
	}

	LOG_INF("phy id is %#.4x - %#.4x", phy_id_1, phy_id_2);
	return -EINVAL;
}
#endif

/**
 * @brief Low level reset procedure that toggles the reset gpio
 */
static int phy_mc_vsc8541_reset(const struct device *dev)
{
	const struct mc_vsc8541_config *cfg = dev->config;
	int ret;
	uint16_t reg = 0U;
	int count = 0;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)

	if (!cfg->reset_gpio.port) {
		LOG_WRN("missing reset port definition");
		return -EINVAL;
	}

	/* configure the reset pin */
	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	for (uint32_t i = 0; i < 2; i++) {
		/* Start reset */
		ret = gpio_pin_set_dt(&cfg->reset_gpio, 1);
		if (ret < 0) {
			LOG_WRN("failed to set reset gpio");
			return -EINVAL;
		}

		/* Wait as specified by datasheet */
		k_sleep(K_MSEC(200));

		/* Reset over */
		gpio_pin_set_dt(&cfg->reset_gpio, 0);

		/* After de-asserting reset, must wait before using the config interface */
		k_sleep(K_MSEC(200));
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios) */

	/* According to IEEE 802.3, Section 2, Subsection 22.2.4.1.1,
	 * a PHY reset may take up to 0.5 s.
	 */
	k_sleep(K_MSEC(500));

#if CONFIG_PHY_VERIFY_DEVICE_IDENTIFICATION
	/* confirm phy organizationally unique identifier, if enabled */
	if (phy_mc_vsc8541_verify_phy_id(dev) < 0) {
		LOG_ERR("failed to verify phy id");
		return -EINVAL;
	}
#endif

	/* set RGMII mode (must be executed BEFORE software reset -- see datasheet) */
	if (cfg->microchip_interface_type == VSC8541_RGMII) {
		ret = phy_mc_vsc8541_write(dev, PHY_REG_PAGE0_EXT_CONTROL_1,
					   (0x0 << 13) | (0x2 << 11));
		if (ret < 0) {
			return ret;
		}
	}

	/* software reset */
	ret = phy_mc_vsc8541_write(dev, MII_BMCR, MII_BMCR_RESET);
	if (ret < 0) {
		return ret;
	}

	/* wait for phy finished software reset */
	do {
		ret = phy_mc_vsc8541_read(dev, MII_BMCR, &reg);
		if (ret < 0) {
			return ret;
		}
		if (count++ > 1000) {
			LOG_ERR("phy reset timed out");
			return -ETIMEDOUT;
		}
	} while ((reg & MII_BMCR_RESET) != 0U);

	/* configure the RGMII clk delay */
	reg = 0x0;
	/* RX_CLK delay */
	reg |= (cfg->rgmii_rx_clk_delay << 4);
	/* TX_CLK delay */
	reg |= (cfg->rgmii_tx_clk_delay << 0);

	return phy_mc_vsc8541_write(dev, PHY_REG_PAGE2_RGMII_CONTROL, reg);
}

/**
 * @brief Update the phy state with current speed given by readings of the phy
 *
 * @param dev device structure of the phy
 * @param state being updated (we only update speed here)
 */
static int phy_mc_vsc8541_get_speed(const struct device *dev, struct phy_link_state *state)
{
	int ret;
	uint16_t aux_status;
	uint16_t link10_status;
	uint16_t link100_status;
	uint16_t link1000_status;
	bool is_duplex;

	ret = phy_mc_vsc8541_read(dev, PHY_REG_PAGE0_EXT_DEV_AUX, &aux_status);
	if (ret < 0) {
		return ret;
	}

	is_duplex = (aux_status & PHY_REG_PAGE0_EXT_DEV_AUX_DUPLEX) != 0U;

	ret = phy_mc_vsc8541_read(dev, PHY_REG_PAGE0_STAT1000_EXT2, &link1000_status);
	if (ret < 0) {
		return ret;
	}

	if ((link1000_status & BIT(12)) != 0U) {
		state->speed = is_duplex ? LINK_FULL_1000BASE : LINK_HALF_1000BASE;
		return 0; /* no need to check lower speeds */
	}

	ret = phy_mc_vsc8541_read(dev, PHY_REG_PAGE0_STAT100, &link100_status);
	if (ret < 0) {
		return ret;
	}

	if ((link100_status & BIT(12)) != 0U) {
		state->speed = is_duplex ? LINK_FULL_100BASE : LINK_HALF_100BASE;
		return 0; /* no need to check lower speeds */
	}

	ret = phy_mc_vsc8541_read(dev, PHY_REG_PAGE0_EXT_CTRL_STAT, &link10_status);
	if (ret < 0) {
		return ret;
	}

	if ((link10_status & BIT(6)) != 0U) {
		state->speed = is_duplex ? LINK_FULL_10BASE : LINK_HALF_10BASE;
	} else {
		state->speed = 0; /* no link */
	}

	return 0;
}

static int phy_mc_vsc8541_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds,
				   enum phy_cfg_link_flag flags)
{
	int ret;

	if ((flags & PHY_FLAG_AUTO_NEGOTIATION_DISABLED) != 0U) {
		ret = phy_mii_set_bmcr_reg_autoneg_disabled(dev, adv_speeds);
	} else {
		ret = phy_mii_cfg_link_autoneg(dev, adv_speeds, true);
	}

	return ret;
}

/**
 * @brief Initializes the phy and starts the link monitor
 *
 */
static int phy_mc_vsc8541_init(const struct device *dev)
{
	struct mc_vsc8541_data *data = dev->data;
	const struct mc_vsc8541_config *cfg = dev->config;
	int ret;

	data->active_page = -1;

	k_mutex_init(&data->mutex);

	/* Reset PHY */
	ret = phy_mc_vsc8541_reset(dev);
	if (ret < 0) {
		LOG_ERR("initialize failed");
		return ret;
	}

	/* setup thread to watch link state */
	k_thread_create(&data->link_monitor_thread,
			(k_thread_stack_t *)data->link_monitor_thread_stack, STACK_SIZE,
			phy_mc_vsc8541_link_monitor, (void *)dev, NULL, NULL, THREAD_PRIORITY, 0,
			K_NO_WAIT);

	k_thread_name_set(&data->link_monitor_thread, "phy-link-mon");

	phy_mc_vsc8541_cfg_link(dev, cfg->default_speeds, 0);

	return 0;
}

/**
 * @brief Update the link status with readings from phy
 *
 * @param dev device structure to phy
 * @param state which is being updated
 */
static int phy_mc_vsc8541_get_link(const struct device *dev, struct phy_link_state *state)
{
	int ret;
	uint16_t reg_sr;
	uint16_t reg_cr;
	bool hasLink;
	bool auto_negotiation_finished = true;

	ret = phy_mc_vsc8541_read(dev, MII_BMSR, &reg_sr);
	if (ret < 0) {
		return ret;
	}

	ret = phy_mc_vsc8541_read(dev, MII_BMCR, &reg_cr);
	if (ret < 0) {
		return ret;
	}

	hasLink = (reg_sr & MII_BMSR_LINK_STATUS) != 0;

	if ((reg_cr & MII_BMCR_AUTONEG_ENABLE) != 0U) {
		/* auto negotiation active; update status */
		auto_negotiation_finished = (reg_sr & MII_BMSR_AUTONEG_COMPLETE) != 0;
	}

	if (hasLink && auto_negotiation_finished) {
		state->is_up = true;
		ret = phy_mc_vsc8541_get_speed(dev, state);
		if (ret < 0) {
			return ret;
		}
	} else {
		state->is_up = false;
		state->speed = 0;
	}

	return 0;
}

/**
 * @brief Set callback which is used to announce link status changes
 *
 * @param dev device structure to phy device
 * @param cb
 * @param user_data
 */
static int phy_mc_vsc8541_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct mc_vsc8541_data *data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	data->cb(dev, &data->state, data->cb_data);

	return 0;
}

/**
 * @brief Monitor thread to check the link state and announce if changed
 *
 * @param arg1 provides a pointer to device structure
 * @param arg2 not used
 * @param arg3 not used
 */
void phy_mc_vsc8541_link_monitor(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	struct mc_vsc8541_data *data = dev->data;

	struct phy_link_state new_state;

	while (1) {
		k_sleep(K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
		phy_mc_vsc8541_get_link(dev, &new_state);

		if ((new_state.is_up != data->state.is_up) ||
		    (new_state.speed != data->state.speed)) {
			/* state changed */
			data->state.is_up = new_state.is_up;
			data->state.speed = new_state.speed;

			if (data->cb) {
				/* announce new state */
				data->cb(dev, &data->state, data->cb_data);
			}
		}
	}
}

/**
 * @brief Reading of phy register content at given address via mdio interface
 *
 * - high byte of register address defines page
 * - low byte of register address defines the address within selected page
 * - to speed up, we store the last used page and only swap page if needed
 *
 */
static int phy_mc_vsc8541_read(const struct device *dev, uint16_t reg_addr, uint16_t *data)
{
	const struct mc_vsc8541_config *cfg = dev->config;
	struct mc_vsc8541_data *dev_data = dev->data;
	int ret;

	*data = 0U;

	/* decode page */
	uint16_t page = reg_addr >> 8;

	/* mask out lower byte */
	reg_addr &= 0x00ff;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);
	mdio_bus_enable(cfg->mdio_dev);

	/* select page, given by register upper byte */
	if (dev_data->active_page != page) {
		ret = mdio_write(cfg->mdio_dev, cfg->addr, PHY_REG_PAGE_SELECTOR, page);
		if (ret < 0) {
			goto read_end;
		}
		dev_data->active_page = (int)page;
	}

	/* select register, given by register lower byte */
	ret = mdio_read(cfg->mdio_dev, cfg->addr, reg_addr, data);

read_end:
	mdio_bus_disable(cfg->mdio_dev);
	k_mutex_unlock(&dev_data->mutex);

	return ret;
}

/**
 * @brief Reading of phy register content at given address via mdio interface
 * For external API using uint32_t as data type
 */
static int phy_mc_vsc8541_read_ext(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	return phy_mc_vsc8541_read(dev, reg_addr, (uint16_t *)data);
}

/**
 * @brief Writing of new value to phy register at given address via mdio interface
 *
 * - high byte of register address defines page
 * - low byte of register address defines the address within selected page
 * - to speed up, we store the last used page and only swap page if needed
 *
 */
static int phy_mc_vsc8541_write(const struct device *dev, uint16_t reg_addr, uint16_t data)
{
	const struct mc_vsc8541_config *cfg = dev->config;
	struct mc_vsc8541_data *dev_data = dev->data;
	int ret;

	/* decode page */
	uint16_t page = reg_addr >> 8;

	/* mask out lower byte */
	reg_addr &= 0x00ff;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);
	mdio_bus_enable(cfg->mdio_dev);

	/* select page, given by register upper byte */
	if (dev_data->active_page != page) {
		ret = mdio_write(cfg->mdio_dev, cfg->addr, PHY_REG_PAGE_SELECTOR, (uint16_t)page);
		if (ret < 0) {
			goto write_end;
		}
		dev_data->active_page = (int)page;
	}

	/* write register, given by lower byte */
	ret = mdio_write(cfg->mdio_dev, cfg->addr, reg_addr, data);

write_end:
	mdio_bus_disable(cfg->mdio_dev);
	k_mutex_unlock(&dev_data->mutex);

	return ret;
}

/**
 * @brief Writing of new value to phy register at given address via mdio interface
 * For external API using uint32_t as data type
 */
static int phy_mc_vsc8541_write_ext(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	return phy_mc_vsc8541_write(dev, reg_addr, (uint16_t)data);
}

static DEVICE_API(ethphy, mc_vsc8541_phy_api) = {
	.get_link = phy_mc_vsc8541_get_link,
	.cfg_link = phy_mc_vsc8541_cfg_link,
	.link_cb_set = phy_mc_vsc8541_link_cb_set,
	.read = phy_mc_vsc8541_read_ext,
	.write = phy_mc_vsc8541_write_ext,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define RESET_GPIO(n) .reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),
#else
#define RESET_GPIO(n)
#endif /* reset gpio */

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
#define INTERRUPT_GPIO(n) .interrupt_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, {0}),
#else
#define INTERRUPT_GPIO(n)
#endif /* interrupt gpio */

#define MICROCHIP_VSC8541_INIT(n)                                                                  \
	static const struct mc_vsc8541_config mc_vsc8541_##n##_config = {                          \
		.addr = DT_INST_REG_ADDR(n),                                                       \
		.mdio_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                      \
		.microchip_interface_type = DT_INST_ENUM_IDX(n, microchip_interface_type),         \
		.rgmii_rx_clk_delay = DT_INST_PROP(n, microchip_rgmii_rx_clk_delay),               \
		.rgmii_tx_clk_delay = DT_INST_PROP(n, microchip_rgmii_tx_clk_delay),               \
		.default_speeds = PHY_INST_GENERATE_DEFAULT_SPEEDS(n),				   \
		RESET_GPIO(n) INTERRUPT_GPIO(n)};                                                  \
                                                                                                   \
	static struct mc_vsc8541_data mc_vsc8541_##n##_data;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &phy_mc_vsc8541_init, NULL, &mc_vsc8541_##n##_data,               \
			      &mc_vsc8541_##n##_config, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,     \
			      &mc_vsc8541_phy_api);

DT_INST_FOREACH_STATUS_OKAY(MICROCHIP_VSC8541_INIT)
