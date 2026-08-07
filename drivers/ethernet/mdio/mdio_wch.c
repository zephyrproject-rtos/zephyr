/*
 * Copyright (c) 2025 James Bennion-Pedley <james@bojit.org>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_mdio

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_wch, CONFIG_MDIO_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/kernel.h>

#include <hal_ch32fun.h>

#define ETH_MACMIIAR_PA_POS 11
#define ETH_MACMIIAR_MR_POS 6

#define PHY_RESPONSE_TIMEOUT_MS 20

struct mdio_wch_config {
	ETH_TypeDef *regs;
	const struct device *clk_dev;
	uint8_t clk_id;
	const struct pinctrl_dev_config *pin_cfg;
};

struct mdio_wch_data {
	struct k_sem sem;
};

static int mdio_wch_transfer(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t *data,
			     bool write)
{
	struct mdio_wch_data *const dev_data = dev->data;
	const struct mdio_wch_config *const config = dev->config;
	k_timepoint_t timeout = sys_timepoint_calc(K_MSEC(PHY_RESPONSE_TIMEOUT_MS));
	uint32_t tmpreg = 0;

	k_sem_take(&dev_data->sem, K_FOREVER);

	tmpreg = config->regs->MACMIIAR;
	tmpreg &= ~MACMIIAR_CR_MASK;
	tmpreg |= (((uint32_t)prtad << ETH_MACMIIAR_PA_POS) & ETH_MACMIIAR_PA);
	tmpreg |= (((uint32_t)regad << ETH_MACMIIAR_MR_POS) & ETH_MACMIIAR_MR);

	if (write) {
		tmpreg |= ETH_MACMIIAR_MW;
		config->regs->MACMIIDR = *data;
	} else {
		tmpreg &= ~ETH_MACMIIAR_MW;
	}

	tmpreg |= ETH_MACMIIAR_MB;
	config->regs->MACMIIAR = tmpreg;

	do {
		if (sys_timepoint_expired(timeout)) {
			k_sem_give(&dev_data->sem);
			LOG_ERR("Transfer: MDIO Timeout!");
			return -EIO;
		}

		k_msleep(10);
	} while (config->regs->MACMIIAR & ETH_MACMIIAR_MB);

	if (!write) {
		*data = (uint16_t)(config->regs->MACMIIDR);
	}

	k_sem_give(&dev_data->sem);

	return 0;
}

static int mdio_wch_read(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t *data)
{
	int ret = mdio_wch_transfer(dev, prtad, regad, data, false);

	LOG_DBG("Fetch %u, %u", regad, *data);

	return ret;
}

static int mdio_wch_write(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t data)
{
	LOG_DBG("Write %u, %u", regad, data);

	return mdio_wch_transfer(dev, prtad, regad, &data, true);
}

static int mdio_wch_init(const struct device *dev)
{
	struct mdio_wch_data *const dev_data = dev->data;
	const struct mdio_wch_config *const config = dev->config;

	/* enable clock */
	clock_control_subsys_t clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clk_id;
	int ret = clock_control_on(config->clk_dev, clock_sys);

	if (ret < 0) {
		LOG_ERR("Failed to enable ethernet clock needed for MDIO (%d)", ret);
		return ret;
	}

	/* configure pinmux */
	ret = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	k_sem_init(&dev_data->sem, 1, 1);

	return 0;
}

static DEVICE_API(mdio, mdio_wch_api) = {
	.read = mdio_wch_read,
	.write = mdio_wch_write,
};

#define MDIO_WCH_DEVICE(inst)                                                                      \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct mdio_wch_data mdio_wch_data_##inst;                                          \
	static const struct mdio_wch_config mdio_wch_config_##inst = {                             \
		.regs = (ETH_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(inst)),                          \
		.clk_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(DT_INST_PARENT(inst), mac)),       \
		.clk_id = DT_CLOCKS_CELL_BY_NAME(DT_INST_PARENT(inst), mac, id),                   \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &mdio_wch_init, NULL, &mdio_wch_data_##inst,                   \
			      &mdio_wch_config_##inst, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,     \
			      &mdio_wch_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_WCH_DEVICE)
