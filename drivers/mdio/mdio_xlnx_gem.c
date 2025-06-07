/*
 * Copyright (c) 2021 Weidmueller Interface GmbH & Co. KG
 * Copyright (c) 2025 Immo Birnbaum
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(xlnx_gem_mdio, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT xlnx_gem_mdio

/*
 * Subset of register offsets and control bits / masks required for MDIO:
 *
 * Register offsets within the respective GEM's address space:
 * NWCTRL   = gem.net_ctrl   Network Control       register
 * NWCFG    = gem.net_cfg    Network Configuration register
 * NWSR     = gem.net_status Network Status        register
 * PHYMNTNC = gem.phy_maint  PHY maintenance       register
 *
 * gem.net_ctrl:
 * [04]       Enable MDIO port
 * gem.net_cfg:
 * [20 .. 18] MDC clock division setting
 * gem.net_status:
 * [02]       PHY management idle bit
 * [01]       MDIO input status
 * gem.phy_maint:
 * [31 .. 30] constant values
 * [17 .. 16] constant values
 * [29]       Read operation control bit
 * [28]       Write operation control bit
 * [27 .. 23] PHY address
 * [22 .. 18] Register address
 * [15 .. 00] 16-bit data word
 */
#define ETH_XLNX_GEM_NWCTRL_OFFSET   0x00000000
#define ETH_XLNX_GEM_NWCTRL_MDEN_BIT BIT(4)

#define ETH_XLNX_GEM_NWCFG_OFFSET    0x00000004
#define ETH_XLNX_GEM_NWCFG_MDC_MASK  0x7
#define ETH_XLNX_GEM_NWCFG_MDC_SHIFT 18

#define ETH_XLNX_GEM_NWSR_OFFSET        0x00000008
#define ETH_XLNX_GEM_NWSR_MDIO_IDLE_BIT BIT(2)

#define ETH_XLNX_GEM_PHY_MAINTENANCE_OFFSET      0x00000034
#define ETH_XLNX_GEM_PHY_MAINT_CONST_BITS        0x40020000
#define ETH_XLNX_GEM_PHY_MAINT_READ_OP_BIT       BIT(29)
#define ETH_XLNX_GEM_PHY_MAINT_WRITE_OP_BIT      BIT(28)
#define ETH_XLNX_GEM_PHY_MAINT_PHY_ADDRESS_MASK  0x0000001F
#define ETH_XLNX_GEM_PHY_MAINT_PHY_ADDRESS_SHIFT 23
#define ETH_XLNX_GEM_PHY_MAINT_REGISTER_ID_MASK  0x0000001F
#define ETH_XLNX_GEM_PHY_MAINT_REGISTER_ID_SHIFT 18
#define ETH_XLNX_GEM_PHY_MAINT_DATA_MASK         0x0000FFFF

/**
 * @brief MDC clock divider configuration enumeration type.
 *
 * Enumeration type containing the supported clock divider values
 * used to generate the MDIO interface clock (MDC) from either the
 * cpu_1x clock (Zynq-7000) or the LPD LSBUS clock (ZynqMP).
 * This is a configuration item in the controller's net_cfg register.
 */
enum eth_xlnx_mdc_clock_divider {
	MDC_DIVIDER_8 = 0,
	MDC_DIVIDER_16,
	MDC_DIVIDER_32,
	MDC_DIVIDER_48,
	MDC_DIVIDER_64,
	MDC_DIVIDER_96,
	MDC_DIVIDER_128,
	MDC_DIVIDER_224
};

/**
 * @brief Constant device configuration data structure.
 *
 * This struct contains all device configuration data for a GEM
 * MDIO interface instance which is constant.
 */
struct xlnx_gem_mdio_config {
	uint32_t gem_base_addr;
};

/**
 * @brief GEM MDIO interface data read function
 *
 * @param dev   Pointer to the GEM MDIO device
 * @param prtad MDIO address of the PHY to be accessed
 * @param regad Index of the PHY register to be read
 * @param data  Read data output pointer
 * @return      0 in case of success, -ETIMEDOUT if the read operation
 *              timed out (idle bit not set as expected)
 */
static int xlnx_gem_mdio_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			      uint16_t *data)
{
	const struct xlnx_gem_mdio_config *const dev_conf = dev->config;

	uint32_t reg_val;
	uint32_t poll_cnt = 0;

	/*
	 * MDIO read operation as described in Zynq-7000 TRM,
	 * chapter 16.3.4, p. 517.
	 */

	/*
	 * Wait until gem.net_status[phy_mgmt_idle] == 1 before issuing the
	 * current command.
	 */
	do {
		if (poll_cnt++ > 0) {
			k_busy_wait(CONFIG_MDIO_XLNX_GEM_POLL_DELAY);
		}
		reg_val = sys_read32(dev_conf->gem_base_addr + ETH_XLNX_GEM_NWSR_OFFSET);
	} while ((reg_val & ETH_XLNX_GEM_NWSR_MDIO_IDLE_BIT) == 0 &&
		 poll_cnt < CONFIG_MDIO_XLNX_GEM_MAX_POLL_RETRIES);
	if (poll_cnt == CONFIG_MDIO_XLNX_GEM_MAX_POLL_RETRIES) {
		LOG_ERR("%s: read from PHY address %hhu, register address %hhu timed out",
			dev->name, prtad, regad);
		return -ETIMEDOUT;
	}

	/* Assemble & write the read command to the gem.phy_maint register */

	/* Set the bits constant for any operation */
	reg_val = ETH_XLNX_GEM_PHY_MAINT_CONST_BITS;
	/* Indicate a read operation */
	reg_val |= ETH_XLNX_GEM_PHY_MAINT_READ_OP_BIT;
	/* PHY address */
	reg_val |= (((uint32_t)prtad & ETH_XLNX_GEM_PHY_MAINT_PHY_ADDRESS_MASK)
		    << ETH_XLNX_GEM_PHY_MAINT_PHY_ADDRESS_SHIFT);
	/* Register address */
	reg_val |= (((uint32_t)regad & ETH_XLNX_GEM_PHY_MAINT_REGISTER_ID_MASK)
		    << ETH_XLNX_GEM_PHY_MAINT_REGISTER_ID_SHIFT);

	sys_write32(reg_val, dev_conf->gem_base_addr + ETH_XLNX_GEM_PHY_MAINTENANCE_OFFSET);

	/*
	 * Wait until gem.net_status[phy_mgmt_idle] == 1 -> current command
	 * completed.
	 */
	poll_cnt = 0;
	do {
		if (poll_cnt++ > 0) {
			k_busy_wait(CONFIG_MDIO_XLNX_GEM_POLL_DELAY);
		}
		reg_val = sys_read32(dev_conf->gem_base_addr + ETH_XLNX_GEM_NWSR_OFFSET);
	} while ((reg_val & ETH_XLNX_GEM_NWSR_MDIO_IDLE_BIT) == 0 &&
		 poll_cnt < CONFIG_MDIO_XLNX_GEM_MAX_POLL_RETRIES);
	if (poll_cnt == CONFIG_MDIO_XLNX_GEM_MAX_POLL_RETRIES) {
		LOG_ERR("%s: read from PHY address %hhu, register address %hhu timed out",
			dev->name, prtad, regad);
		return -ETIMEDOUT;
	}

	/*
	 * Read the data returned by the PHY -> lower 16 bits of the PHY main-
	 * tenance register
	 */
	reg_val = sys_read32(dev_conf->gem_base_addr + ETH_XLNX_GEM_PHY_MAINTENANCE_OFFSET);

	*data = (uint16_t)reg_val;
	return 0;
}

/**
 * @brief GEM MDIO interface data write function
 *
 * @param dev   Pointer to the GEM MDIO device
 * @param prtad MDIO address of the PHY to be accessed
 * @param regad Index of the PHY register to write to
 * @param data  Data word to be written to the target register
 * @return      0 in case of success, -ETIMEDOUT if the read operation
 *              timed out (idle bit not set as expected)
 */
static int xlnx_gem_mdio_write(const struct device *dev, uint8_t prtad, uint8_t regad,
			       uint16_t data)
{
	const struct xlnx_gem_mdio_config *const dev_conf = dev->config;

	uint32_t reg_val;
	uint32_t poll_cnt = 0;

	/*
	 * MDIO write operation as described in Zynq-7000 TRM,
	 * chapter 16.3.4, p. 517.
	 */

	/*
	 * Wait until gem.net_status[phy_mgmt_idle] == 1 before issuing the
	 * current command.
	 */
	do {
		if (poll_cnt++ > 0) {
			k_busy_wait(CONFIG_MDIO_XLNX_GEM_POLL_DELAY);
		}
		reg_val = sys_read32(dev_conf->gem_base_addr + ETH_XLNX_GEM_NWSR_OFFSET);
	} while ((reg_val & ETH_XLNX_GEM_NWSR_MDIO_IDLE_BIT) == 0 &&
		 poll_cnt < CONFIG_MDIO_XLNX_GEM_MAX_POLL_RETRIES);
	if (poll_cnt == CONFIG_MDIO_XLNX_GEM_MAX_POLL_RETRIES) {
		LOG_ERR("%s: write to PHY address %hhu, register address %hhu timed out", dev->name,
			prtad, regad);
		return -ETIMEDOUT;
	}

	/* Assemble & write the read command to the gem.phy_maint register */

	/* Set the bits constant for any operation */
	reg_val = ETH_XLNX_GEM_PHY_MAINT_CONST_BITS;
	/* Indicate a read operation */
	reg_val |= ETH_XLNX_GEM_PHY_MAINT_WRITE_OP_BIT;
	/* PHY address */
	reg_val |= (((uint32_t)prtad & ETH_XLNX_GEM_PHY_MAINT_PHY_ADDRESS_MASK)
		    << ETH_XLNX_GEM_PHY_MAINT_PHY_ADDRESS_SHIFT);
	/* Register address */
	reg_val |= (((uint32_t)regad & ETH_XLNX_GEM_PHY_MAINT_REGISTER_ID_MASK)
		    << ETH_XLNX_GEM_PHY_MAINT_REGISTER_ID_SHIFT);
	/* 16 bits of data for the destination register */
	reg_val |= ((uint32_t)data & ETH_XLNX_GEM_PHY_MAINT_DATA_MASK);

	sys_write32(reg_val, dev_conf->gem_base_addr + ETH_XLNX_GEM_PHY_MAINTENANCE_OFFSET);

	/*
	 * Wait until gem.net_status[phy_mgmt_idle] == 1 -> current command
	 * completed.
	 */
	poll_cnt = 0;
	do {
		if (poll_cnt++ > 0) {
			k_busy_wait(CONFIG_MDIO_XLNX_GEM_POLL_DELAY);
		}
		reg_val = sys_read32(dev_conf->gem_base_addr + ETH_XLNX_GEM_NWSR_OFFSET);
	} while ((reg_val & ETH_XLNX_GEM_NWSR_MDIO_IDLE_BIT) == 0 &&
		 poll_cnt < CONFIG_MDIO_XLNX_GEM_MAX_POLL_RETRIES);
	if (poll_cnt == CONFIG_MDIO_XLNX_GEM_MAX_POLL_RETRIES) {
		LOG_ERR("%s: write to PHY address %hhu, register address %hhu timed out", dev->name,
			prtad, regad);
		return -ETIMEDOUT;
	}

	return 0;
}

/**
 * @brief GEM MDIO interface initialization function
 * Configures the MDC clock divider in the associated GEM instance's
 * net_config (NWCFG) register and sets the MDIO enable bit in the
 * net_control (NWCTRL) register.
 *
 * @param dev Pointer to the GEM MDIO device
 * @return    always returns 0
 */
static int xlnx_gem_mdio_initialize(const struct device *dev)
{
	const struct xlnx_gem_mdio_config *const dev_conf = dev->config;

	uint32_t reg_val;
	uint32_t mdc_divider = (uint32_t)MDC_DIVIDER_224;

	/* Set the MDC divider in gem.net_config */
	reg_val = sys_read32(dev_conf->gem_base_addr + ETH_XLNX_GEM_NWCFG_OFFSET);
	reg_val &= ~(ETH_XLNX_GEM_NWCFG_MDC_MASK << ETH_XLNX_GEM_NWCFG_MDC_SHIFT);
	reg_val |= ((mdc_divider & ETH_XLNX_GEM_NWCFG_MDC_MASK) << ETH_XLNX_GEM_NWCFG_MDC_SHIFT);
	sys_write32(reg_val, dev_conf->gem_base_addr + ETH_XLNX_GEM_NWCFG_OFFSET);

	/* Enable the MDIO interface */
	reg_val = sys_read32(dev_conf->gem_base_addr + ETH_XLNX_GEM_NWCTRL_OFFSET);
	reg_val |= ETH_XLNX_GEM_NWCTRL_MDEN_BIT;
	sys_write32(reg_val, dev_conf->gem_base_addr + ETH_XLNX_GEM_NWCTRL_OFFSET);

	LOG_DBG("%s: initialized", dev->name);
	return 0;
}

static DEVICE_API(mdio, xlnx_gem_mdio_api) = {
	.read = xlnx_gem_mdio_read,
	.write = xlnx_gem_mdio_write,
};

#define XLNX_GEM_MDIO_DEV_CONFIG(port)                                                             \
	static const struct xlnx_gem_mdio_config xlnx_gem##port##_mdio_cfg = {                     \
		.gem_base_addr = DT_REG_ADDR_BY_IDX(DT_INST(port, xlnx_gem), 0),                   \
	};

#define XLNX_GEM_MDIO_DEV_INIT(port)                                                               \
	DEVICE_DT_INST_DEFINE(port, &xlnx_gem_mdio_initialize, NULL, NULL,                         \
			      &xlnx_gem##port##_mdio_cfg, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,  \
			      &xlnx_gem_mdio_api);

#define XLNX_GEM_MDIO_INITIALIZE(port)                                                             \
	XLNX_GEM_MDIO_DEV_CONFIG(port);                                                            \
	XLNX_GEM_MDIO_DEV_INIT(port);

DT_INST_FOREACH_STATUS_OKAY(XLNX_GEM_MDIO_INITIALIZE)
