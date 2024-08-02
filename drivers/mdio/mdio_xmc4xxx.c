/*
 * Copyright (c) 2023 SLB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_mdio

#include <errno.h>

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <xmc_scu.h>
#include <xmc_eth_mac.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_xmc4xxx, CONFIG_MDIO_LOG_LEVEL);

#define MDIO_TRANSFER_TIMEOUT_US 250000

#define MAX_MDC_FREQUENCY 2500000u /* 400ns period */
#define MIN_MDC_FREQUENCY 1000000u /* 1us period */

struct mdio_xmc4xxx_clock_divider {
	uint8_t divider;
	uint8_t reg_val;
};

static const struct mdio_xmc4xxx_clock_divider mdio_clock_divider[] = {
	{.divider = 8, .reg_val = 2},  {.divider = 13, .reg_val = 3},
	{.divider = 21, .reg_val = 0}, {.divider = 31, .reg_val = 1},
	{.divider = 51, .reg_val = 4}, {.divider = 62, .reg_val = 5},
};

struct mdio_xmc4xxx_dev_data {
	struct k_mutex mutex;
	uint32_t reg_value_gmii_address;
};

struct mdio_xmc4xxx_dev_config {
	ETH_GLOBAL_TypeDef *const regs;
	const struct pinctrl_dev_config *pcfg;
	uint8_t mdi_port_ctrl;
};

static int mdio_xmc4xxx_transfer(const struct device *dev, uint8_t phy_addr, uint8_t reg_addr,
			 uint8_t is_write, uint16_t data_write, uint16_t *data_read)
{
	const struct mdio_xmc4xxx_dev_config *const dev_cfg = dev->config;
	ETH_GLOBAL_TypeDef *const regs = dev_cfg->regs;
	struct mdio_xmc4xxx_dev_data *const dev_data = dev->data;
	uint32_t reg;
	int ret = 0;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	if ((regs->GMII_ADDRESS & ETH_GMII_ADDRESS_MB_Msk) != 0) {
		ret = -EBUSY;
		goto finish;
	}

	reg = dev_data->reg_value_gmii_address;
	if (is_write) {
		reg |= ETH_GMII_ADDRESS_MW_Msk;
		regs->GMII_DATA = data_write;
	}

	regs->GMII_ADDRESS = reg | ETH_GMII_ADDRESS_MB_Msk |
			     FIELD_PREP(ETH_GMII_ADDRESS_PA_Msk, phy_addr) |
			     FIELD_PREP(ETH_GMII_ADDRESS_MR_Msk, reg_addr);

	if (!WAIT_FOR((regs->GMII_ADDRESS & ETH_GMII_ADDRESS_MB_Msk) == 0,
		      MDIO_TRANSFER_TIMEOUT_US, k_msleep(5))) {
		LOG_WRN("mdio transfer timedout");
		ret = -ETIMEDOUT;
		goto finish;
	}

	if (!is_write && data_read != NULL) {
		*data_read = regs->GMII_DATA;
	}

finish:
	k_mutex_unlock(&dev_data->mutex);

	return ret;
}

static int mdio_xmc4xxx_read(const struct device *dev, uint8_t phy_addr, uint8_t reg_addr,
			 uint16_t *data)
{
	return mdio_xmc4xxx_transfer(dev, phy_addr, reg_addr, 0, 0, data);
}

static int mdio_xmc4xxx_write(const struct device *dev, uint8_t phy_addr,
			  uint8_t reg_addr, uint16_t data)
{
	return mdio_xmc4xxx_transfer(dev, phy_addr, reg_addr, 1, data, NULL);
}

static void mdio_xmc4xxx_bus_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* this will enable the clock for ETH, which generates to MDIO clk  */
	XMC_ETH_MAC_Enable(NULL);
}

static void mdio_xmc4xxx_bus_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
	XMC_ETH_MAC_Disable(NULL);
}

static int mdio_xmc4xxx_set_clock_divider(const struct device *dev)
{
	struct mdio_xmc4xxx_dev_data *dev_data = dev->data;
	uint32_t eth_mac_clk = XMC_SCU_CLOCK_GetEthernetClockFrequency();

	for (int i = 0; i < ARRAY_SIZE(mdio_clock_divider); i++) {
		uint8_t divider = mdio_clock_divider[i].divider;
		uint8_t reg_val = mdio_clock_divider[i].reg_val;
		uint32_t mdc_clk = eth_mac_clk / divider;

		if (mdc_clk > MIN_MDC_FREQUENCY && mdc_clk < MAX_MDC_FREQUENCY) {
			LOG_DBG("Using MDC clock divider %d", divider);
			LOG_DBG("MDC clock %dHz", mdc_clk);
			dev_data->reg_value_gmii_address =
				FIELD_PREP(ETH_GMII_ADDRESS_CR_Msk, reg_val);
			return 0;
		}
	}

	return -EINVAL;
}

static int mdio_xmc4xxx_initialize(const struct device *dev)
{
	const struct mdio_xmc4xxx_dev_config *dev_cfg = dev->config;
	struct mdio_xmc4xxx_dev_data *dev_data = dev->data;
	XMC_ETH_MAC_PORT_CTRL_t port_ctrl = {0};
	int ret;

	k_mutex_init(&dev_data->mutex);

	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	ret = mdio_xmc4xxx_set_clock_divider(dev);
	if (ret != 0) {
		LOG_ERR("Error setting MDIO clock divider");
		return -EINVAL;
	}

	port_ctrl.mdio = dev_cfg->mdi_port_ctrl;
	ETH0_CON->CON = port_ctrl.raw;

	return ret;
}

static const struct mdio_driver_api mdio_xmc4xxx_driver_api = {
	.read = mdio_xmc4xxx_read,
	.write = mdio_xmc4xxx_write,
	.bus_enable = mdio_xmc4xxx_bus_enable,
	.bus_disable = mdio_xmc4xxx_bus_disable,
};

PINCTRL_DT_INST_DEFINE(0);
static const struct mdio_xmc4xxx_dev_config mdio_xmc4xxx_dev_config_0 = {
	.regs = (ETH_GLOBAL_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(0)),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.mdi_port_ctrl = DT_INST_ENUM_IDX(0, mdi_port_ctrl),
};

static struct mdio_xmc4xxx_dev_data mdio_xmc4xxx_dev_data_0;

DEVICE_DT_INST_DEFINE(0, &mdio_xmc4xxx_initialize, NULL, &mdio_xmc4xxx_dev_data_0,
		      &mdio_xmc4xxx_dev_config_0, POST_KERNEL,
		      CONFIG_MDIO_INIT_PRIORITY, &mdio_xmc4xxx_driver_api);
