/*
 * Copyright (c) 2023 PHOENIX CONTACT Electronics GmbH
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
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
#include "phy_adin2111_priv.h"

LOG_MODULE_REGISTER(phy_adin, CONFIG_PHY_LOG_LEVEL);

/* PHYs out of reset check retry delay */
#define ADIN2111_PHY_AWAIT_DELAY_POLL_US			15U
/*
 * Number of retries for PHYs out of reset check,
 * rmii variants as ADIN11XX need 70ms maximum after hw reset to be up,
 * so the increasing the count for that, as default 25ms (sw reset) + 45.
 */
#define ADIN2111_PHY_AWAIT_RETRY_COUNT				3000U

/* PHY's software powerdown check retry delay */
#define ADIN2111_PHY_SFT_PD_DELAY_POLL_US			15U
/* Number of retries for PHY's software powerdown check */
#define ADIN2111_PHY_SFT_PD_RETRY_COUNT				200U

/* Software reset, CLK_25 disabled time*/
#define ADIN1100_PHY_SFT_RESET_MS				25U

/* PHYs autonegotiation complete timeout */
#define ADIN2111_AN_COMPLETE_AWAIT_TIMEOUT_MS			3000U

/* ADIN2111 PHY identifier */
#define ADIN2111_PHY_ID						0x0283BCA1U
#define ADIN1110_PHY_ID						0x0283BC91U
#define ADIN1100_PHY_ID						0x0283BC81U

/* System Interrupt Mask Register */
#define ADIN2111_PHY_CRSM_IRQ_MASK				0x0020U
/* System Interrupt Status Register */
#define ADIN2111_PHY_CRSM_IRQ_STATUS				0x0010U
/**
 * Mask of reserved interrupts that indicates a fatal error in the system.
 *
 * There is inconsistency between RM and ADI driver example:
 *   - RM mask 0x6FFF
 *   - ADI driver example mask 0x2BFF
 *
 * The value from the example doesn't include reserved bits 10 and 14.
 * The tests show that PHY is still functioning when bit 10 is raised.
 *
 * Here the value from ADI driver example is used instead of RM.
 */
#define ADIN2111_PHY_CRSM_IRQ_STATUS_FATAL_ERR			0x2BFFU

/* PHY Subsystem Interrupt Mask Register */
#define ADIN2111_PHY_SUBSYS_IRQ_MASK				0x0021U
/* PHY Subsystem Interrupt Status Register */
#define ADIN2111_PHY_SUBSYS_IRQ_STATUS				0x0011U
/* Link Status Change */
#define ADIN2111_PHY_SUBSYS_IRQ_STATUS_LINK_STAT_CHNG_LH	BIT(1)

/* Software Power-down Control Register */
#define ADIN2111_PHY_CRSM_SFT_PD_CNTRL				0x8812U
/* System Status Register */
#define ADIN2111_PHY_CRSM_STAT					0x8818U
/* Software Power-down Status */
#define ADIN2111_CRSM_STAT_CRSM_SFT_PD_RDY			BIT(1)

/* LED Control Register */
#define ADIN2111_PHY_LED_CNTRL					0x8C82U
/* LED 1 Enable */
#define ADIN2111_PHY_LED_CNTRL_LED1_EN				BIT(15)
/* LED 0 Enable */
#define ADIN2111_PHY_LED_CNTRL_LED0_EN				BIT(7)

/* MMD bridge regs */
#define ADIN1100_MMD_ACCESS_CNTRL				0x0DU
#define ADIN1100_MMD_ACCESS					0x0EU

struct phy_adin2111_config {
	const struct device *mdio;
	uint8_t phy_addr;
	bool led0_en;
	bool led1_en;
	bool tx_24v;
	bool mii;
};

struct phy_adin2111_data {
	const struct device *dev;
	struct phy_link_state state;
	struct k_sem sem;
	struct k_work_delayable monitor_work;
	phy_callback_t cb;
	void *cb_data;
};

static inline int phy_adin2111_c22_read(const struct device *dev, uint16_t reg,
					uint16_t *val)
{
	const struct phy_adin2111_config *const cfg = dev->config;

	return mdio_read(cfg->mdio, cfg->phy_addr, reg, val);
}

static inline int phy_adin2111_c22_write(const struct device *dev, uint16_t reg,
					 uint16_t val)
{
	const struct phy_adin2111_config *const cfg = dev->config;

	return mdio_write(cfg->mdio, cfg->phy_addr, reg, val);
}

static int phy_adin2111_c45_setup_dev_reg(const struct device *dev, uint16_t devad,
					  uint16_t reg)
{
	const struct phy_adin2111_config *cfg = dev->config;
	int rval;

	rval = mdio_write(cfg->mdio, cfg->phy_addr, ADIN1100_MMD_ACCESS_CNTRL, devad);
	if (rval < 0) {
		return rval;
	}
	rval = mdio_write(cfg->mdio, cfg->phy_addr, ADIN1100_MMD_ACCESS, reg);
	if (rval < 0) {
		return rval;
	}

	return mdio_write(cfg->mdio, cfg->phy_addr, ADIN1100_MMD_ACCESS_CNTRL, devad | BIT(14));
}

static int phy_adin2111_c45_read(const struct device *dev, uint16_t devad,
				 uint16_t reg, uint16_t *val)
{
	const struct phy_adin2111_config *cfg = dev->config;
	int rval;

	if (cfg->mii) {
		/* Using C22 -> devad bridge */
		rval = phy_adin2111_c45_setup_dev_reg(dev, devad, reg);
		if (rval < 0) {
			return rval;
		}

		return mdio_read(cfg->mdio, cfg->phy_addr, ADIN1100_MMD_ACCESS, val);
	}

	return mdio_read_c45(cfg->mdio, cfg->phy_addr, devad, reg, val);
}

static int phy_adin2111_c45_write(const struct device *dev, uint16_t devad,
				  uint16_t reg, uint16_t val)
{
	const struct phy_adin2111_config *cfg = dev->config;
	int rval;

	if (cfg->mii) {
		/* Using C22 -> devad bridge */
		rval = phy_adin2111_c45_setup_dev_reg(dev, devad, reg);
		if (rval < 0) {
			return rval;
		}

		return mdio_write(cfg->mdio, cfg->phy_addr, ADIN1100_MMD_ACCESS, val);
	}

	return mdio_write_c45(cfg->mdio, cfg->phy_addr, devad, reg, val);
}

static int phy_adin2111_reg_read(const struct device *dev, uint16_t reg_addr,
				 uint32_t *data)
{
	const struct phy_adin2111_config *cfg = dev->config;
	int ret;

	mdio_bus_enable(cfg->mdio);

	ret = phy_adin2111_c22_read(dev, reg_addr, (uint16_t *) data);

	mdio_bus_disable(cfg->mdio);

	return ret;
}

static int phy_adin2111_reg_write(const struct device *dev, uint16_t reg_addr,
				  uint32_t data)
{
	const struct phy_adin2111_config *cfg = dev->config;
	int ret;

	mdio_bus_enable(cfg->mdio);

	ret = phy_adin2111_c22_write(dev, reg_addr, (uint16_t) data);

	mdio_bus_disable(cfg->mdio);

	return ret;
}

static int phy_adin2111_await_phy(const struct device *dev)
{
	int ret;
	uint32_t count;
	uint16_t val;

	/**
	 * Port 2 PHY comes out of reset after Port 1 PHY,
	 * wait until both are out of reset.
	 * Reading Port 2 PHY registers returns 0s until
	 * it comes out from reset.
	 */
	for (count = 0U; count < ADIN2111_PHY_AWAIT_RETRY_COUNT; ++count) {
		ret = phy_adin2111_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC1,
					    ADIN2111_PHY_CRSM_IRQ_MASK, &val);
		if (ret >= 0) {
			if (val != 0U) {
				break;
			}
			ret = -ETIMEDOUT;
		}
		k_sleep(K_USEC(ADIN2111_PHY_AWAIT_DELAY_POLL_US));
	}

	return ret;
}

static int phy_adin2111_an_state_read(const struct device *dev)
{
	struct phy_adin2111_data *const data = dev->data;
	uint16_t bmsr;
	int ret;

	/* read twice to get actual link status, latch low */
	ret = phy_adin2111_c22_read(dev, MII_BMSR, &bmsr);
	if (ret < 0) {
		return ret;
	}
	ret = phy_adin2111_c22_read(dev, MII_BMSR, &bmsr);
	if (ret < 0) {
		return ret;
	}

	data->state.is_up = !!(bmsr & MII_BMSR_LINK_STATUS);

	return 0;
}

int phy_adin2111_handle_phy_irq(const struct device *dev,
				struct phy_link_state *state)
{
	struct phy_adin2111_data *const data = dev->data;
	uint16_t subsys_status;
	int ret;

	ret = phy_adin2111_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC2,
				    ADIN2111_PHY_SUBSYS_IRQ_STATUS,
				    &subsys_status);
	if (ret < 0) {
		return ret;
	}

	if ((subsys_status & ADIN2111_PHY_SUBSYS_IRQ_STATUS_LINK_STAT_CHNG_LH) == 0U) {
		/* nothing to process */
		return -EAGAIN;
	}

	k_sem_take(&data->sem, K_FOREVER);

	ret = phy_adin2111_an_state_read(dev);

	memcpy(state, &data->state, sizeof(struct phy_link_state));

	k_sem_give(&data->sem);

	return ret;
}

static int phy_adin2111_sft_pd(const struct device *dev, bool enter)
{
	int ret;
	uint32_t count;
	const uint16_t expected = enter ? ADIN2111_CRSM_STAT_CRSM_SFT_PD_RDY : 0U;
	uint16_t val;

	ret = phy_adin2111_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC1,
				     ADIN2111_PHY_CRSM_SFT_PD_CNTRL,
				     enter ? 1U : 0U);
	if (ret < 0) {
		return ret;
	}

	for (count = 0U; count < ADIN2111_PHY_SFT_PD_RETRY_COUNT; ++count) {
		ret = phy_adin2111_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC1,
					    ADIN2111_PHY_CRSM_STAT, &val);
		if (ret >= 0) {
			if ((val & ADIN2111_CRSM_STAT_CRSM_SFT_PD_RDY) == expected) {
				break;
			}
			ret = -ETIMEDOUT;
		}
		k_sleep(K_USEC(ADIN2111_PHY_SFT_PD_DELAY_POLL_US));
	}

	return ret;
}

static int phy_adin2111_id(const struct device *dev, uint32_t *phy_id)
{
	uint16_t val;

	if (phy_adin2111_c22_read(dev, MII_PHYID1R, &val) < 0) {
		return -EIO;
	}

	*phy_id = (val & UINT16_MAX) << 16;

	if (phy_adin2111_c22_read(dev, MII_PHYID2R, &val) < 0) {
		return -EIO;
	}

	*phy_id |= (val & UINT16_MAX);

	return 0;
}

static int phy_adin2111_get_link_state(const struct device *dev,
				       struct phy_link_state *state)
{
	struct phy_adin2111_data *const data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);

	memcpy(state, &data->state, sizeof(struct phy_link_state));

	k_sem_give(&data->sem);

	return 0;
}

static int phy_adin2111_cfg_link(const struct device *dev,
				 enum phy_link_speed adv_speeds)
{
	ARG_UNUSED(dev);

	if (!!(adv_speeds & LINK_FULL_10BASE_T)) {
		return 0;
	}

	return -ENOTSUP;
}

static int phy_adin2111_reset(const struct device *dev)
{
	int ret;

	ret = phy_adin2111_c22_write(dev, MII_BMCR, MII_BMCR_RESET);
	if (ret < 0) {
		return ret;
	}

	k_msleep(ADIN1100_PHY_SFT_RESET_MS);

	return 0;
}

static void invoke_link_cb(const struct device *dev)
{
	struct phy_adin2111_data *const data = dev->data;
	struct phy_link_state state;

	if (data->cb == NULL) {
		return;
	}

	data->cb(dev, &state, data->cb_data);
}

static int update_link_state(const struct device *dev)
{
	struct phy_adin2111_data *const data = dev->data;
	const struct phy_adin2111_config *config = dev->config;
	struct phy_link_state old_state;
	uint16_t bmsr;
	int ret;

	ret = phy_adin2111_c22_read(dev, MII_BMSR, &bmsr);
	if (ret < 0) {
		return ret;
	}

	old_state = data->state;
	data->state.is_up = !!(bmsr & MII_BMSR_LINK_STATUS);

	if (old_state.speed != data->state.speed || old_state.is_up != data->state.is_up) {

		LOG_INF("PHY (%d) Link is %s", config->phy_addr, data->state.is_up ? "up" : "down");

		if (data->state.is_up == false) {
			return 0;
		}

		invoke_link_cb(dev);

		LOG_INF("PHY (%d) Link speed %s Mb, %s duplex\n", config->phy_addr,
			(PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100" : "10"),
			PHY_LINK_IS_FULL_DUPLEX(data->state.speed) ? "full" : "half");
	}

	return 0;
}

static void monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct phy_adin2111_data *const data =
		CONTAINER_OF(dwork, struct phy_adin2111_data, monitor_work);
	const struct device *dev = data->dev;
	int rc;

	k_sem_take(&data->sem, K_FOREVER);

	rc = update_link_state(dev);

	k_sem_give(&data->sem);

	/* Submit delayed work */
	k_work_reschedule(&data->monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int phy_adin2111_init(const struct device *dev)
{
	const struct phy_adin2111_config *const cfg = dev->config;
	struct phy_adin2111_data *const data = dev->data;
	uint32_t phy_id;
	uint16_t val;
	bool tx_24v_supported = false;
	int ret;

	data->dev = dev;
	data->state.is_up = false;
	data->state.speed = LINK_FULL_10BASE_T;

	/*
	 * For adin1100 and further mii stuff,
	 * reset may not be performed from the mac layer, doing a clean reset here.
	 */
	if (cfg->mii) {
		ret = phy_adin2111_reset(dev);
		if (ret < 0) {
			return ret;
		}
	}

	ret = phy_adin2111_await_phy(dev);
	if (ret < 0) {
		LOG_ERR("PHY %u didn't come out of reset, %d",
			cfg->phy_addr, ret);
		return -ENODEV;
	}

	ret = phy_adin2111_id(dev, &phy_id);
	if (ret < 0) {
		LOG_ERR("Failed to read PHY %u ID, %d",
			cfg->phy_addr, ret);
		return -ENODEV;
	}

	if (phy_id != ADIN2111_PHY_ID && phy_id != ADIN1110_PHY_ID && phy_id != ADIN1100_PHY_ID) {
		LOG_ERR("PHY %u unexpected PHY ID %X", cfg->phy_addr, phy_id);
		return -EINVAL;
	}

	LOG_INF("PHY %u ID %X", cfg->phy_addr, phy_id);

	/* enter software powerdown */
	ret = phy_adin2111_sft_pd(dev, true);
	if (ret < 0) {
		return ret;
	}

	/* disable interrupts */
	ret = phy_adin2111_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC1,
				     ADIN2111_PHY_CRSM_IRQ_MASK, 0U);
	if (ret < 0) {
		return ret;
	}

	/* enable link status change irq */
	ret = phy_adin2111_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC2,
				     ADIN2111_PHY_SUBSYS_IRQ_MASK,
				     ADIN2111_PHY_SUBSYS_IRQ_STATUS_LINK_STAT_CHNG_LH);
	if (ret < 0) {
		return ret;
	}

	/* clear PHY IRQ status before enabling ADIN IRQs */
	ret = phy_adin2111_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC1,
				    ADIN2111_PHY_CRSM_IRQ_STATUS, &val);
	if (ret < 0) {
		return ret;
	}

	if (val & ADIN2111_PHY_CRSM_IRQ_STATUS_FATAL_ERR) {
		LOG_ERR("PHY %u CRSM reports fatal system error", cfg->phy_addr);
		return -ENODEV;
	}

	ret = phy_adin2111_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC2,
				    ADIN2111_PHY_SUBSYS_IRQ_STATUS, &val);
	if (ret < 0) {
		return ret;
	}

	if (!cfg->led0_en || !cfg->led1_en) {
		ret = phy_adin2111_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC1,
					    ADIN2111_PHY_LED_CNTRL, &val);
		if (ret < 0) {
			return ret;
		}
		if (!cfg->led0_en) {
			val &= ~(ADIN2111_PHY_LED_CNTRL_LED0_EN);
		}
		if (!cfg->led1_en) {
			val &= ~(ADIN2111_PHY_LED_CNTRL_LED1_EN);
		}
		ret = phy_adin2111_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC1,
					     ADIN2111_PHY_LED_CNTRL, val);
		if (ret < 0) {
			return ret;
		}
	}

	/* check 2.4V support */
	ret = phy_adin2111_c45_read(dev, MDIO_MMD_PMAPMD, MDIO_PMA_B10L_STAT, &val);
	if (ret < 0) {
		return ret;
	}

	tx_24v_supported = !!(val & MDIO_PMA_B10L_STAT_2V4_ABLE);

	LOG_INF("PHY %u 2.4V mode %s", cfg->phy_addr,
		tx_24v_supported ? "supported" : "not supported");

	if (!cfg->tx_24v & tx_24v_supported) {
		LOG_ERR("PHY %u 2.4V mode supported, but not enabled",
			cfg->phy_addr);
	}

	/* config 2.4V auto-negotiation */
	ret = phy_adin2111_c45_read(dev, MDIO_MMD_AN, MDIO_AN_T1_ADV_H, &val);
	if (ret < 0) {
		return ret;
	}

	if (tx_24v_supported) {
		val |= MDIO_AN_T1_ADV_H_10L_TX_HI;
	} else {
		val &= ~MDIO_AN_T1_ADV_H_10L_TX_HI;
	}

	if (cfg->tx_24v) {
		if (!tx_24v_supported) {
			LOG_ERR("PHY %u 2.4V mode enabled, but not supported",
				cfg->phy_addr);
			return -EINVAL;
		}

		val |= MDIO_AN_T1_ADV_H_10L_TX_HI_REQ;
	} else {
		val &= ~MDIO_AN_T1_ADV_H_10L_TX_HI_REQ;
	}

	ret = phy_adin2111_c45_write(dev, MDIO_MMD_AN, MDIO_AN_T1_ADV_H, val);
	if (ret < 0) {
		return ret;
	}

	/* enable auto-negotiation */
	ret = phy_adin2111_c45_write(dev, MDIO_MMD_AN, MDIO_AN_T1_CTRL,
				     MDIO_AN_T1_CTRL_EN);
	if (ret < 0) {
		return ret;
	}

	if (cfg->mii) {
		k_work_init_delayable(&data->monitor_work, monitor_work_handler);
		monitor_work_handler(&data->monitor_work.work);
	}

	/**
	 * done, PHY is in software powerdown (SFT PD)
	 * exit software powerdown, PHY 1 has to exit before PHY 2
	 * correct PHY order is expected to be in DTS to guarantee that
	 */
	return phy_adin2111_sft_pd(dev, false);
}

static int phy_adin2111_link_cb_set(const struct device *dev, phy_callback_t cb,
				    void *user_data)
{
	struct phy_adin2111_data *const data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	/* Invoke the callback to notify the caller of the current
	 * link status.
	 */
	invoke_link_cb(dev);

	return 0;
}

static DEVICE_API(ethphy, phy_adin2111_api) = {
	.get_link = phy_adin2111_get_link_state,
	.cfg_link = phy_adin2111_cfg_link,
	.link_cb_set = phy_adin2111_link_cb_set,
	.read = phy_adin2111_reg_read,
	.write = phy_adin2111_reg_write,
};

#define ADIN2111_PHY_INITIALIZE(n, model)						\
	static const struct phy_adin2111_config phy_adin##model##_config_##n = {	\
		.mdio = DEVICE_DT_GET(DT_INST_BUS(n)),					\
		.phy_addr = DT_INST_REG_ADDR(n),					\
		.led0_en = DT_INST_PROP(n, led0_en),					\
		.led1_en = DT_INST_PROP(n, led1_en),					\
		.tx_24v = !(DT_INST_PROP(n, disable_tx_mode_24v)),			\
		IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(adi_adin1100_phy),			\
		(.mii = 1))								\
	};										\
	static struct phy_adin2111_data phy_adin##model##_data_##n = {			\
		.sem = Z_SEM_INITIALIZER(phy_adin##model##_data_##n.sem, 1, 1),		\
	};										\
	DEVICE_DT_INST_DEFINE(n, &phy_adin2111_init, NULL,				\
			      &phy_adin##model##_data_##n,				\
			      &phy_adin##model##_config_##n,				\
			      POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,			\
			      &phy_adin2111_api);

#define DT_DRV_COMPAT adi_adin2111_phy
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADIN2111_PHY_INITIALIZE, 2111)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT adi_adin1100_phy
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADIN2111_PHY_INITIALIZE, 1100)
#undef DT_DRV_COMPAT
