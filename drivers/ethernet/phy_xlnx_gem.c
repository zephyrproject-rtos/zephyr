/*
 * Xilinx Processor System Gigabit Ethernet controller (GEM) driver
 *
 * PHY management interface implementation
 * Models currently supported:
 * - Marvell Alaska 88E1111 (QEMU simulated PHY)
 * - Marvell Alaska 88E1510/88E1518/88E1512/88E1514 (Zedboard)
 * - Texas Instruments TLK105
 * - Texas Instruments DP83822
 *
 * Copyright (c) 2021, Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>

#include "eth_xlnx_gem_priv.h"

#define LOG_MODULE_NAME phy_xlnx_gem
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* Basic MDIO read / write functions for PHY access */

/**
 * @brief Read PHY data via the MDIO interface
 * Reads data from a PHY attached to the respective GEM's MDIO interface
 *
 * @param base_addr Base address of the GEM's register space
 * @param phy_addr  MDIO address of the PHY to be accessed
 * @param reg_addr  Index of the PHY register to be read
 * @return          16-bit data word received from the PHY
 */
static uint16_t phy_xlnx_gem_mdio_read(
	uint32_t base_addr, uint8_t phy_addr,
	uint8_t reg_addr)
{
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
			k_busy_wait(100);
		}
		reg_val = sys_read32(base_addr + ETH_XLNX_GEM_NWSR_OFFSET);
	} while ((reg_val & ETH_XLNX_GEM_MDIO_IDLE_BIT) == 0 && poll_cnt < 10);
	if (poll_cnt == 10) {
		LOG_ERR("GEM@0x%08X read from PHY address %hhu, "
			"register address %hhu timed out",
			base_addr, phy_addr, reg_addr);
		return 0;
	}

	/* Assemble & write the read command to the gem.phy_maint register */

	/* Set the bits constant for any operation */
	reg_val  = ETH_XLNX_GEM_PHY_MAINT_CONST_BITS;
	/* Indicate a read operation */
	reg_val |= ETH_XLNX_GEM_PHY_MAINT_READ_OP_BIT;
	/* PHY address */
	reg_val |= (((uint32_t)phy_addr & ETH_XLNX_GEM_PHY_MAINT_PHY_ADDRESS_MASK) <<
		   ETH_XLNX_GEM_PHY_MAINT_PHY_ADDRESS_SHIFT);
	/* Register address */
	reg_val |= (((uint32_t)reg_addr & ETH_XLNX_GEM_PHY_MAINT_REGISTER_ID_MASK) <<
		   ETH_XLNX_GEM_PHY_MAINT_REGISTER_ID_SHIFT);

	sys_write32(reg_val, base_addr + ETH_XLNX_GEM_PHY_MAINTENANCE_OFFSET);

	/*
	 * Wait until gem.net_status[phy_mgmt_idle] == 1 -> current command
	 * completed.
	 */
	poll_cnt = 0;
	do {
		if (poll_cnt++ > 0) {
			k_busy_wait(100);
		}
		reg_val = sys_read32(base_addr + ETH_XLNX_GEM_NWSR_OFFSET);
	} while ((reg_val & ETH_XLNX_GEM_MDIO_IDLE_BIT) == 0 && poll_cnt < 10);
	if (poll_cnt == 10) {
		LOG_ERR("GEM@0x%08X read from PHY address %hhu, "
			"register address %hhu timed out",
			base_addr, phy_addr, reg_addr);
		return 0;
	}

	/*
	 * Read the data returned by the PHY -> lower 16 bits of the PHY main-
	 * tenance register
	 */
	reg_val = sys_read32(base_addr + ETH_XLNX_GEM_PHY_MAINTENANCE_OFFSET);
	return (uint16_t)reg_val;
}

/**
 * @brief Writes PHY data via the MDIO interface
 * Writes data to a PHY attached to the respective GEM's MDIO interface
 *
 * @param base_addr Base address of the GEM's register space
 * @param phy_addr  MDIO address of the PHY to be accessed
 * @param reg_addr  Index of the PHY register to be written to
 * @param value     16-bit data word to be written to the target register
 */
static void phy_xlnx_gem_mdio_write(
	uint32_t base_addr, uint8_t phy_addr,
	uint8_t reg_addr, uint16_t value)
{
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
			k_busy_wait(100);
		}
		reg_val = sys_read32(base_addr + ETH_XLNX_GEM_NWSR_OFFSET);
	} while ((reg_val & ETH_XLNX_GEM_MDIO_IDLE_BIT) == 0 && poll_cnt < 10);
	if (poll_cnt == 10) {
		LOG_ERR("GEM@0x%08X write to PHY address %hhu, "
			"register address %hhu timed out",
			base_addr, phy_addr, reg_addr);
		return;
	}

	/* Assemble & write the read command to the gem.phy_maint register */

	/* Set the bits constant for any operation */
	reg_val  = ETH_XLNX_GEM_PHY_MAINT_CONST_BITS;
	/* Indicate a read operation */
	reg_val |= ETH_XLNX_GEM_PHY_MAINT_WRITE_OP_BIT;
	/* PHY address */
	reg_val |= (((uint32_t)phy_addr & ETH_XLNX_GEM_PHY_MAINT_PHY_ADDRESS_MASK) <<
		   ETH_XLNX_GEM_PHY_MAINT_PHY_ADDRESS_SHIFT);
	/* Register address */
	reg_val |= (((uint32_t)reg_addr & ETH_XLNX_GEM_PHY_MAINT_REGISTER_ID_MASK) <<
		   ETH_XLNX_GEM_PHY_MAINT_REGISTER_ID_SHIFT);
	/* 16 bits of data for the destination register */
	reg_val |= ((uint32_t)value & ETH_XLNX_GEM_PHY_MAINT_DATA_MASK);

	sys_write32(reg_val, base_addr + ETH_XLNX_GEM_PHY_MAINTENANCE_OFFSET);

	/*
	 * Wait until gem.net_status[phy_mgmt_idle] == 1 -> current command
	 * completed.
	 */
	poll_cnt = 0;
	do {
		if (poll_cnt++ > 0) {
			k_busy_wait(100);
		}
		reg_val = sys_read32(base_addr + ETH_XLNX_GEM_NWSR_OFFSET);
	} while ((reg_val & ETH_XLNX_GEM_MDIO_IDLE_BIT) == 0 && poll_cnt < 10);
	if (poll_cnt == 10) {
		LOG_ERR("GEM@0x%08X write to PHY address %hhu, "
			"register address %hhu timed out",
			base_addr, phy_addr, reg_addr);
	}
}

/*
 * Vendor-specific PHY management functions for:
 * Marvell Alaska 88E1111 (QEMU simulated PHY)
 * Marvell Alaska 88E1510/88E1518/88E1512/88E1514 (Zedboard)
 * Register IDs & procedures are based on the corresponding datasheets:
 * https://www.marvell.com/content/dam/marvell/en/public-collateral/transceivers/marvell-phys-transceivers-alaska-88e1111-datasheet.pdf
 * https://www.marvell.com/content/dam/marvell/en/public-collateral/transceivers/marvell-phys-transceivers-alaska-88e151x-datasheet.pdf
 *
 * NOTICE: Unless indicated otherwise, page/table source references refer to
 * the 88E151x datasheet.
 */

/**
 * @brief Marvell Alaska PHY reset function
 * Reset function for the Marvell Alaska PHY series
 *
 * @param dev Pointer to the device data
 */
static void phy_xlnx_gem_marvell_alaska_reset(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint16_t phy_data;
	uint32_t retries = 0;

	/*
	 * Page 0, register address 0 = Copper control register,
	 * bit [15] = PHY reset. Register 0/0 access is R/M/W. Comp.
	 * datasheet chapter 2.6 and table 64 "Copper Control Register".
	 */
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_MRVL_COPPER_CONTROL_REGISTER);
	phy_data |= PHY_MRVL_COPPER_CONTROL_RESET_BIT;
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_MRVL_COPPER_CONTROL_REGISTER, phy_data);

	/* Bit [15] reverts to 0 once the reset is complete. */
	while (((phy_data & PHY_MRVL_COPPER_CONTROL_RESET_BIT) != 0) && (retries++ < 10)) {
		phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
						  PHY_MRVL_COPPER_CONTROL_REGISTER);
	}
	if (retries == 10) {
		LOG_ERR("%s reset PHY address %hhu (Marvell Alaska) timed out",
			dev->name, dev_data->phy_addr);
	}
}

/**
 * @brief Marvell Alaska PHY configuration function
 * Configuration function for the Marvell Alaska PHY series
 *
 * @param dev Pointer to the device data
 */
static void phy_xlnx_gem_marvell_alaska_cfg(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint16_t phy_data;
	uint16_t phy_data_gbit;
	uint32_t retries = 0;

	/*
	 * Page 0, register address 0 = Copper control register,
	 * bit [12] = auto-negotiation enable bit is to be cleared
	 * for now, afterwards, trigger a PHY reset.
	 * Register 0/0 access is R/M/W. Comp. datasheet chapter 2.6
	 * and table 64 "Copper Control Register".
	 */
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_MRVL_COPPER_CONTROL_REGISTER);
	phy_data &= ~PHY_MRVL_COPPER_CONTROL_AUTONEG_ENABLE_BIT;
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_MRVL_COPPER_CONTROL_REGISTER, phy_data);
	phy_xlnx_gem_marvell_alaska_reset(dev);

	if ((dev_data->phy_id & PHY_MRVL_PHY_ID_MODEL_MASK) ==
			PHY_MRVL_PHY_ID_MODEL_88E151X) {
		/*
		 * 88E151x only: configure the system interface and media type
		 * (i.e. "RGMII to Copper", 0x0). On the 88E1111, this setting
		 * is configured using I/O pins on the device.
		 * TODO: Make this value configurable via KConfig or DT?
		 * Page 18, register address 20 = General Control Register 1,
		 * bits [2..0] = mode configuration
		 * Comp. datasheet table 129 "General Control Register 1"
		 * NOTICE: a change of this value requires a subsequent software
		 * reset command via the same register's bit [15].
		 */
		phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
					PHY_MRVL_COPPER_PAGE_SWITCH_REGISTER,
					PHY_MRVL_GENERAL_CONTROL_1_PAGE);

		phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
						  PHY_MRVL_GENERAL_CONTROL_1_REGISTER);
		phy_data &= ~(PHY_MRVL_MODE_CONFIG_MASK << PHY_MRVL_MODE_CONFIG_SHIFT);
		phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
					PHY_MRVL_GENERAL_CONTROL_1_REGISTER, phy_data);

		/*
		 * [15] Mode Software Reset bit, affecting pages 6 and 18
		 * Reset is performed immediately, bit [15] is self-clearing.
		 * This reset bit is not to be confused with the actual PHY
		 * reset in register 0/0!
		 */
		phy_data |= PHY_MRVL_GENERAL_CONTROL_1_RESET_BIT;
		phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
					PHY_MRVL_GENERAL_CONTROL_1_REGISTER, phy_data);

		/* Bit [15] reverts to 0 once the reset is complete. */
		while (((phy_data & PHY_MRVL_GENERAL_CONTROL_1_RESET_BIT) != 0) &&
				(retries++ < 10)) {
			phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr,
				dev_data->phy_addr,
				PHY_MRVL_GENERAL_CONTROL_1_REGISTER);
		}
		if (retries == 10) {
			LOG_ERR("%s configure PHY address %hhu (Marvell Alaska) timed out",
				dev->name, dev_data->phy_addr);
			return;
		}

		/* Revert to register page 0 */
		phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
			PHY_MRVL_COPPER_PAGE_SWITCH_REGISTER,
			PHY_MRVL_BASE_REGISTERS_PAGE);
	}

	/*
	 * Configure MDIX
	 * TODO: Make this value configurable via KConfig or DT?
	 * 88E151x: Page 0, register address 16 = Copper specific control register 1,
	 * 88E1111: Page any, register address 16 = PHY specific control register,
	 * bits [6..5] = MDIO crossover mode. Comp. datasheet table 76.
	 * NOTICE: a change of this value requires a subsequent software
	 * reset command via Copper Control Register's bit [15].
	 */

	/* [6..5] 11 = Enable auto cross over detection */
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_MRVL_COPPER_CONTROL_1_REGISTER);
	phy_data &= ~(PHY_MRVL_MDIX_CONFIG_MASK << PHY_MRVL_MDIX_CONFIG_SHIFT);
	phy_data |= (PHY_MRVL_MDIX_AUTO_CROSSOVER_ENABLE << PHY_MRVL_MDIX_CONFIG_SHIFT);
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_MRVL_COPPER_CONTROL_1_REGISTER, phy_data);

	/*
	 * Configure the Copper Specific Interrupt Enable Register
	 * (88E151x) / Interrupt Enable Register (88E1111).
	 * The interrupt status register provides a convenient way to
	 * detect relevant state changes, also, PHY management could
	 * eventually be changed from polling to interrupt-driven.
	 * There's just one big catch: at least on the Zedboard, the
	 * PHY interrupt line isn't wired up, therefore, the GEM can
	 * never trigger a PHY interrupt. Still, the PHY interrupts
	 * are configured & enabled in order to obtain all relevant
	 * status data from a single source.
	 *
	 * -> all bits contained herein will be retained during the
	 * upcoming software reset operation.
	 * Page 0, register address 18 = (Copper Specific) Interrupt
	 * Enable Register,
	 * bit [14] = Speed changed interrupt enable,
	 * bit [13] = Duplex changed interrupt enable,
	 * bit [11] = Auto-negotiation completed interrupt enable,
	 * bit [10] = Link status changed interrupt enable.
	 * Comp. datasheet table 78
	 */
	phy_data = PHY_MRVL_COPPER_SPEED_CHANGED_INT_BIT |
		PHY_MRVL_COPPER_DUPLEX_CHANGED_INT_BIT |
		PHY_MRVL_COPPER_AUTONEG_COMPLETED_INT_BIT |
		PHY_MRVL_COPPER_LINK_STATUS_CHANGED_INT_BIT;
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_MRVL_COPPER_INT_ENABLE_REGISTER, phy_data);

	/* Trigger a PHY Reset, affecting pages 0, 2, 3, 5, 7. */
	phy_xlnx_gem_marvell_alaska_reset(dev);

	/*
	 * Clear the interrupt status register before advertising the
	 * supported link speed(s).
	 */
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_MRVL_COPPER_INT_STATUS_REGISTER);

	/*
	 * Set which link speeds and duplex modes shall be advertised during
	 * auto-negotiation, then re-enable auto-negotiation. PHY link speed
	 * advertisement configuration as described in Zynq-7000 TRM, chapter
	 * 16.3.4, p. 517.
	 */

	/*
	 * Advertise the link speed from the device configuration & perform
	 * auto-negotiation. This process involves:
	 *
	 * Page 0, register address 4 =
	 *     Copper Auto-Negotiation Advertisement Register,
	 * Page 0, register address 0 =
	 *     Copper Control Register, bit [15] = Reset -> apply all changes
	 *     made regarding advertisement,
	 * Page 0, register address 9 =
	 *     1000BASE-T Control Register (if link speed = 1GBit/s),
	 * Page 0, register address 1 =
	 *     Copper Status Register, bit [5] = Copper Auto-Negotiation
	 *     Complete.
	 *
	 * Comp. datasheet tables 68 & 73.
	 */

	/*
	 * 88E151x only:
	 * Register 4, bits [4..0] = Selector field, 00001 = 802.3. Those bits
	 * are reserved in other Marvell PHYs.
	 */
	if ((dev_data->phy_id & PHY_MRVL_PHY_ID_MODEL_MASK) ==
			PHY_MRVL_PHY_ID_MODEL_88E151X) {
		phy_data = PHY_MRVL_ADV_SELECTOR_802_3;
	} else {
		phy_data = 0x0000;
	}

	/*
	 * Clear the 1 GBit/s FDX/HDX advertisement bits from reg. 9's current
	 * contents in case we're going to advertise anything below 1 GBit/s
	 * as maximum / nominal link speed.
	 */
	phy_data_gbit = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					       PHY_MRVL_1000BASET_CONTROL_REGISTER);
	phy_data_gbit &= ~PHY_MRVL_ADV_1000BASET_FDX_BIT;
	phy_data_gbit &= ~PHY_MRVL_ADV_1000BASET_HDX_BIT;

	if (dev_conf->enable_fdx) {
		if (dev_conf->max_link_speed == LINK_1GBIT) {
			/* Advertise 1 GBit/s, full duplex */
			phy_data_gbit |= PHY_MRVL_ADV_1000BASET_FDX_BIT;
			if (dev_conf->phy_advertise_lower) {
				/* + 100 MBit/s, full duplex */
				phy_data |= PHY_MRVL_ADV_100BASET_FDX_BIT;
				/* + 10 MBit/s, full duplex */
				phy_data |= PHY_MRVL_ADV_10BASET_FDX_BIT;
			}
		} else if (dev_conf->max_link_speed == LINK_100MBIT) {
			/* Advertise 100 MBit/s, full duplex */
			phy_data |= PHY_MRVL_ADV_100BASET_FDX_BIT;
			if (dev_conf->phy_advertise_lower) {
				/* + 10 MBit/s, full duplex */
				phy_data |= PHY_MRVL_ADV_10BASET_FDX_BIT;
			}
		} else if (dev_conf->max_link_speed == LINK_10MBIT) {
			/* Advertise 10 MBit/s, full duplex */
			phy_data |= PHY_MRVL_ADV_10BASET_FDX_BIT;
		}
	} else {
		if (dev_conf->max_link_speed == LINK_1GBIT) {
			/* Advertise 1 GBit/s, half duplex */
			phy_data_gbit = PHY_MRVL_ADV_1000BASET_HDX_BIT;
			if (dev_conf->phy_advertise_lower) {
				/* + 100 MBit/s, half duplex */
				phy_data |= PHY_MRVL_ADV_100BASET_HDX_BIT;
				/* + 10 MBit/s, half duplex */
				phy_data |= PHY_MRVL_ADV_10BASET_HDX_BIT;
			}
		} else if (dev_conf->max_link_speed == LINK_100MBIT) {
			/* Advertise 100 MBit/s, half duplex */
			phy_data |= PHY_MRVL_ADV_100BASET_HDX_BIT;
			if (dev_conf->phy_advertise_lower) {
				/* + 10 MBit/s, half duplex */
				phy_data |= PHY_MRVL_ADV_10BASET_HDX_BIT;
			}
		} else if (dev_conf->max_link_speed == LINK_10MBIT) {
			/* Advertise 10 MBit/s, half duplex */
			phy_data |= PHY_MRVL_ADV_10BASET_HDX_BIT;
		}
	}

	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_MRVL_1000BASET_CONTROL_REGISTER, phy_data_gbit);
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_MRVL_COPPER_AUTONEG_ADV_REGISTER, phy_data);

	/*
	 * Trigger a PHY reset, affecting pages 0, 2, 3, 5, 7.
	 * Afterwards, set the auto-negotiation enable bit [12] in the
	 * Copper Control Register.
	 */
	phy_xlnx_gem_marvell_alaska_reset(dev);
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_MRVL_COPPER_CONTROL_REGISTER);
	phy_data |= PHY_MRVL_COPPER_CONTROL_AUTONEG_ENABLE_BIT;
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_MRVL_COPPER_CONTROL_REGISTER, phy_data);

	/*
	 * Set the link speed to 'link down' for now, once auto-negotiation
	 * is complete, the result will be handled by the system work queue.
	 */
	dev_data->eff_link_speed = LINK_DOWN;
}

/**
 * @brief Marvell Alaska PHY status change polling function
 * Status change polling function for the Marvell Alaska PHY series
 *
 * @param dev Pointer to the device data
 * @return A set of bits indicating whether one or more of the following
 *         events has occurred: auto-negotiation completed, link state
 *         changed, link speed changed.
 */
static uint16_t phy_xlnx_gem_marvell_alaska_poll_sc(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint16_t phy_data;
	uint16_t phy_status = 0;

	/*
	 * PHY status change detection is implemented by reading the
	 * interrupt status register.
	 * Page 0, register address 19 = Copper Interrupt Status Register
	 * bit [14] = Speed changed interrupt,
	 * bit [13] = Duplex changed interrupt,
	 * bit [11] = Auto-negotiation completed interrupt,
	 * bit [10] = Link status changed interrupt.
	 * Comp. datasheet table 79
	 */
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_MRVL_COPPER_INT_STATUS_REGISTER);

	if ((phy_data & PHY_MRVL_COPPER_AUTONEG_COMPLETED_INT_BIT) != 0) {
		phy_status |= PHY_XLNX_GEM_EVENT_AUTONEG_COMPLETE;
	}
	if (((phy_data & PHY_MRVL_COPPER_DUPLEX_CHANGED_INT_BIT) != 0) ||
		((phy_data & PHY_MRVL_COPPER_LINK_STATUS_CHANGED_INT_BIT) != 0)) {
		phy_status |= PHY_XLNX_GEM_EVENT_LINK_STATE_CHANGED;
	}
	if ((phy_data & PHY_MRVL_COPPER_SPEED_CHANGED_INT_BIT) != 0) {
		phy_status |= PHY_XLNX_GEM_EVENT_LINK_SPEED_CHANGED;
	}

	/*
	 * Clear the status register, preserve reserved bit [3] as indicated
	 * by the datasheet
	 */
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_MRVL_COPPER_INT_STATUS_REGISTER, (phy_data & 0x8));

	return phy_status;
}

/**
 * @brief Marvell Alaska PHY link status polling function
 * Link status polling function for the Marvell Alaska PHY series
 *
 * @param dev Pointer to the device data
 * @return 1 if the PHY indicates link up, 0 if the link is down
 */
static uint8_t phy_xlnx_gem_marvell_alaska_poll_lsts(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint16_t phy_data;

	/*
	 * Current link status is obtained from:
	 * Page 0, register address 1 = Copper Status Register
	 * bit [2] = Copper Link Status
	 */
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_MRVL_COPPER_STATUS_REGISTER);

	return ((phy_data >> PHY_MRVL_COPPER_LINK_STATUS_BIT_SHIFT) & 0x0001);
}

/**
 * @brief Marvell Alaska PHY link speed polling function
 * Link speed polling function for the Marvell Alaska PHY series
 *
 * @param dev Pointer to the device data
 * @return    Enum containing the current link speed reported by the PHY
 */
static enum eth_xlnx_link_speed phy_xlnx_gem_marvell_alaska_poll_lspd(
	const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data  = dev->data;
	enum eth_xlnx_link_speed link_speed;
	uint16_t phy_data;

	/*
	 * Current link speed is obtained from:
	 * Page 0, register address 17 = Copper Specific Status Register 1
	 * bits [15 .. 14] = Speed.
	 */
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_MRVL_COPPER_STATUS_1_REGISTER);
	phy_data >>= PHY_MRVL_LINK_SPEED_SHIFT;
	phy_data  &= PHY_MRVL_LINK_SPEED_MASK;

	/*
	 * Link speed bit masks: comp. datasheet, table 77 @ description
	 * of the 'Speed' bits.
	 */
	switch (phy_data) {
	case PHY_MRVL_LINK_SPEED_10MBIT:
		link_speed = LINK_10MBIT;
		break;
	case PHY_MRVL_LINK_SPEED_100MBIT:
		link_speed = LINK_100MBIT;
		break;
	case PHY_MRVL_LINK_SPEED_1GBIT:
		link_speed = LINK_1GBIT;
		break;
	default:
		link_speed = LINK_DOWN;
		break;
	};

	return link_speed;
}

/*
 * Vendor-specific PHY management functions for:
 * Texas Instruments TLK105
 * Texas Instruments DP83822
 * with the DP83822 being the successor to the deprecated TLK105.
 * Register IDs & procedures are based on the corresponding datasheets:
 * https://www.ti.com/lit/gpn/tlk105
 * https://www.ti.com/lit/gpn/dp83822i
 */

/**
 * @brief TI TLK105 & DP83822 PHY reset function
 * Reset function for the TI TLK105 & DP83822 PHYs
 *
 * @param dev Pointer to the device data
 */
static void phy_xlnx_gem_ti_dp83822_reset(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint16_t phy_data;
	uint32_t retries = 0;

	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_TI_BASIC_MODE_CONTROL_REGISTER);
	phy_data |= PHY_TI_BASIC_MODE_CONTROL_RESET_BIT;
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_TI_BASIC_MODE_CONTROL_REGISTER, phy_data);

	while (((phy_data & PHY_TI_BASIC_MODE_CONTROL_RESET_BIT) != 0) && (retries++ < 10)) {
		phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
						  PHY_TI_BASIC_MODE_CONTROL_REGISTER);
	}
	if (retries == 10) {
		LOG_ERR("%s reset PHY address %hhu (TI TLK105/DP83822) timed out",
			dev->name, dev_data->phy_addr);
	}
}

/**
 * @brief TI TLK105 & DP83822 PHY configuration function
 * Configuration function for the TI TLK105 & DP83822 PHYs
 *
 * @param dev Pointer to the device data
 */
static void phy_xlnx_gem_ti_dp83822_cfg(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint16_t phy_data = PHY_TI_ADV_SELECTOR_802_3;

	/* Configure link advertisement */
	if (dev_conf->enable_fdx) {
		if (dev_conf->max_link_speed == LINK_100MBIT) {
			/* Advertise 100BASE-TX, full duplex */
			phy_data |= PHY_TI_ADV_100BASET_FDX_BIT;
			if (dev_conf->phy_advertise_lower) {
				/* + 10BASE-TX, full duplex */
				phy_data |= PHY_TI_ADV_10BASET_FDX_BIT;
			}
		} else if (dev_conf->max_link_speed == LINK_10MBIT) {
			/* Advertise 10BASE-TX, full duplex */
			phy_data |= PHY_TI_ADV_10BASET_FDX_BIT;
		}
	} else {
		if (dev_conf->max_link_speed == LINK_100MBIT) {
			/* Advertise 100BASE-TX, half duplex */
			phy_data |= PHY_TI_ADV_100BASET_HDX_BIT;
			if (dev_conf->phy_advertise_lower) {
				/* + 10BASE-TX, half duplex */
				phy_data |= PHY_TI_ADV_10BASET_HDX_BIT;
			}
		} else if (dev_conf->max_link_speed == LINK_10MBIT) {
			/* Advertise 10BASE-TX, half duplex */
			phy_data |= PHY_TI_ADV_10BASET_HDX_BIT;
		}
	}
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_TI_AUTONEG_ADV_REGISTER, phy_data);

	/* Enable auto-negotiation */
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_TI_BASIC_MODE_CONTROL_REGISTER);
	phy_data |= PHY_TI_BASIC_MODE_CONTROL_AUTONEG_ENABLE_BIT;
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_TI_BASIC_MODE_CONTROL_REGISTER, phy_data);

	/* Robust Auto MDIX */
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_TI_CONTROL_REGISTER_1);
	phy_data |= PHY_TI_CR1_ROBUST_AUTO_MDIX_BIT;
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_TI_CONTROL_REGISTER_1, phy_data);

	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_TI_PHY_CONTROL_REGISTER);
	/* Auto MDIX enable */
	phy_data |= PHY_TI_PHY_CONTROL_AUTO_MDIX_ENABLE_BIT;
	/* Link LED shall only indicate link up or down, no RX/TX activity */
	phy_data |= PHY_TI_PHY_CONTROL_LED_CONFIG_LINK_ONLY_BIT;
	/* Force MDIX disable */
	phy_data &= ~PHY_TI_PHY_CONTROL_FORCE_MDIX_BIT;
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_TI_PHY_CONTROL_REGISTER, phy_data);

	/* Set blink rate to 5 Hz */
	phy_data = (PHY_TI_LED_CONTROL_BLINK_RATE_5HZ <<
		    PHY_TI_LED_CONTROL_BLINK_RATE_SHIFT);
	phy_xlnx_gem_mdio_write(dev_conf->base_addr, dev_data->phy_addr,
				PHY_TI_LED_CONTROL_REGISTER, phy_data);

	/*
	 * Set the link speed to 'link down' for now, once auto-negotiation
	 * is complete, the result will be handled by the system work queue.
	 */
	dev_data->eff_link_speed = LINK_DOWN;
}

/**
 * @brief TI TLK105 & DP83822 PHY status change polling function
 * Status change polling function for the TI TLK105 & DP83822 PHYs
 *
 * @param dev Pointer to the device data
 * @return A set of bits indicating whether one or more of the following
 *         events has occurred: auto-negotiation completed, link state
 *         changed, link speed changed.
 */
static uint16_t phy_xlnx_gem_ti_dp83822_poll_sc(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint16_t phy_data;
	uint16_t phy_status = 0;

	/*
	 * The relevant status bits are obtained from the MII Interrupt
	 * Status Register 1. The upper byte of the register's data word
	 * contains the status bits which are set regardless of whether
	 * the corresponding interrupt enable bits are set in the lower
	 * byte or not (comp. TLK105 documentation, chapter 8.1.16).
	 */
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_TI_MII_INTERRUPT_STATUS_REGISTER_1);

	if ((phy_data & PHY_TI_AUTONEG_COMPLETED_INT_BIT) != 0) {
		phy_status |= PHY_XLNX_GEM_EVENT_AUTONEG_COMPLETE;
	}
	if ((phy_data & PHY_TI_DUPLEX_CHANGED_INT_BIT) != 0) {
		phy_status |= PHY_XLNX_GEM_EVENT_LINK_STATE_CHANGED;
	}
	if ((phy_data & PHY_TI_LINK_STATUS_CHANGED_INT_BIT) != 0) {
		phy_status |= PHY_XLNX_GEM_EVENT_LINK_STATE_CHANGED;
	}
	if ((phy_data & PHY_TI_SPEED_CHANGED_INT_BIT) != 0) {
		phy_status |= PHY_XLNX_GEM_EVENT_LINK_SPEED_CHANGED;
	}

	return phy_status;
}

/**
 * @brief TI TLK105 & DP83822 PHY link status polling function
 * Link status polling function for the TI TLK105 & DP83822 PHYs
 *
 * @param dev Pointer to the device data
 * @return 1 if the PHY indicates link up, 0 if the link is down
 */
static uint8_t phy_xlnx_gem_ti_dp83822_poll_lsts(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint16_t phy_data;

	/*
	 * Double read of the BMSR is intentional - the relevant bit is latched
	 * low so that after a link down -> link up transition, the first read
	 * of the BMSR will still return the latched link down status rather
	 * than the current status.
	 */
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_TI_BASIC_MODE_STATUS_REGISTER);
	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_TI_BASIC_MODE_STATUS_REGISTER);

	return ((phy_data & PHY_TI_BASIC_MODE_STATUS_LINK_STATUS_BIT) != 0);
}

/**
 * @brief TI TLK105 & DP83822 PHY link speed polling function
 * Link speed polling function for the TI TLK105 & DP83822 PHYs
 *
 * @param dev Pointer to the device data
 * @return    Enum containing the current link speed reported by the PHY
 */
static enum eth_xlnx_link_speed phy_xlnx_gem_ti_dp83822_poll_lspd(
	const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	enum eth_xlnx_link_speed link_speed;
	uint16_t phy_data;

	phy_data = phy_xlnx_gem_mdio_read(dev_conf->base_addr, dev_data->phy_addr,
					  PHY_TI_PHY_STATUS_REGISTER);

	/* PHYSCR[0] is the link established indication bit */
	if ((phy_data & PHY_TI_PHY_STATUS_LINK_BIT) != 0) {
		/* PHYSCR[1] is the speed status bit: 0 = 100 Mbps, 1 = 10 Mbps. */
		if ((phy_data & PHY_TI_PHY_STATUS_SPEED_BIT) != 0) {
			link_speed = LINK_10MBIT;
		} else {
			link_speed = LINK_100MBIT;
		}
	} else {
		link_speed = LINK_DOWN;
	}

	return link_speed;
}

/**
 * @brief Marvell Alaska PHY function pointer table
 * Function pointer table for the Marvell Alaska PHY series
 * specific management functions
 */
static struct phy_xlnx_gem_api phy_xlnx_gem_marvell_alaska_api = {
	.phy_reset_func              = phy_xlnx_gem_marvell_alaska_reset,
	.phy_configure_func          = phy_xlnx_gem_marvell_alaska_cfg,
	.phy_poll_status_change_func = phy_xlnx_gem_marvell_alaska_poll_sc,
	.phy_poll_link_status_func   = phy_xlnx_gem_marvell_alaska_poll_lsts,
	.phy_poll_link_speed_func    = phy_xlnx_gem_marvell_alaska_poll_lspd
};

/**
 * @brief Texas Instruments TLK105 & DP83822 PHY function pointer table
 * Function pointer table for the Texas Instruments TLK105 / DP83822 PHY
 * series specific management functions
 */
static struct phy_xlnx_gem_api phy_xlnx_gem_ti_dp83822_api = {
	.phy_reset_func              = phy_xlnx_gem_ti_dp83822_reset,
	.phy_configure_func          = phy_xlnx_gem_ti_dp83822_cfg,
	.phy_poll_status_change_func = phy_xlnx_gem_ti_dp83822_poll_sc,
	.phy_poll_link_status_func   = phy_xlnx_gem_ti_dp83822_poll_lsts,
	.phy_poll_link_speed_func    = phy_xlnx_gem_ti_dp83822_poll_lspd
};

/*
 * All vendor-specific API structs & code are located above
 * -> assemble the top-level list of supported devices the
 * upcoming function phy_xlnx_gem_detect will work with.
 */

/**
 * @brief Top-level table of supported PHYs
 * Top-level table of PHYs supported by the GEM driver. Contains 1..n
 * supported PHY specifications, consisting of the PHY ID plus a mask
 * for masking out variable parts of the PHY ID such as hardware revisions,
 * as well as a textual description of the PHY model and a pointer to
 * the corresponding PHY management function pointer table.
 */
static struct phy_xlnx_gem_supported_dev phy_xlnx_gem_supported_devs[] = {
	{
		.phy_id      = PHY_MRVL_PHY_ID_MODEL_88E1111,
		.phy_id_mask = PHY_MRVL_PHY_ID_MODEL_MASK,
		.api         = &phy_xlnx_gem_marvell_alaska_api,
		.identifier  = "Marvell Alaska 88E1111"
	},
	{
		.phy_id      = PHY_MRVL_PHY_ID_MODEL_88E151X,
		.phy_id_mask = PHY_MRVL_PHY_ID_MODEL_MASK,
		.api         = &phy_xlnx_gem_marvell_alaska_api,
		.identifier  = "Marvell Alaska 88E151x"
	},
	{
		.phy_id      = PHY_TI_PHY_ID_MODEL_DP83822,
		.phy_id_mask = PHY_TI_PHY_ID_MODEL_MASK,
		.api         = &phy_xlnx_gem_ti_dp83822_api,
		.identifier  = "Texas Instruments DP83822"
	},
	{
		.phy_id      = PHY_TI_PHY_ID_MODEL_TLK105,
		.phy_id_mask = PHY_TI_PHY_ID_MODEL_MASK,
		.api         = &phy_xlnx_gem_ti_dp83822_api,
		.identifier  = "Texas Instruments TLK105"
	}
};

/**
 * @brief Top-level PHY detection function
 * Top-level PHY detection function called by the GEM driver if PHY management
 * is enabled for the current GEM device instance. This function is generic
 * and does not require any knowledge regarding PHY vendors, models etc.
 *
 * @param dev Pointer to the device data
 * @retval    -ENOTSUP if PHY management is disabled for the current GEM
 *            device instance
 * @retval    -EIO if no (supported) PHY was detected
 * @retval    0 if a supported PHY has been detected
 */
int phy_xlnx_gem_detect(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;

	uint8_t phy_curr_addr;
	uint8_t phy_first_addr = dev_conf->phy_mdio_addr_fix;
	uint8_t phy_last_addr = (dev_conf->phy_mdio_addr_fix != 0) ?
		dev_conf->phy_mdio_addr_fix : 31;
	uint32_t phy_id;
	uint16_t phy_data;
	uint32_t list_iter;

	/*
	 * Clear the PHY address & ID in the device data struct -> may be
	 * pre-initialized with a non-zero address meaning auto detection
	 * is disabled. If eventually a supported PHY is found, a non-
	 * zero address will be written back to the data struct.
	 */
	dev_data->phy_addr = 0;
	dev_data->phy_id = 0;
	dev_data->phy_access_api = NULL;

	if (!dev_conf->init_phy) {
		return -ENOTSUP;
	}

	/*
	 * PHY detection as described in Zynq-7000 TRM, chapter 16.3.4,
	 * p. 517
	 */
	for (phy_curr_addr = phy_first_addr;
		phy_curr_addr <= phy_last_addr;
		phy_curr_addr++) {
		/* Read the upper & lower PHY ID 16-bit words */
		phy_data = phy_xlnx_gem_mdio_read(
			dev_conf->base_addr, phy_curr_addr,
			PHY_IDENTIFIER_1_REGISTER);
		phy_id = (((uint32_t)phy_data << 16) & 0xFFFF0000);
		phy_data = phy_xlnx_gem_mdio_read(
			dev_conf->base_addr, phy_curr_addr,
			PHY_IDENTIFIER_2_REGISTER);
		phy_id |= ((uint32_t)phy_data & 0x0000FFFF);

		if (phy_id != 0x00000000 && phy_id != 0xFFFFFFFF) {
			LOG_DBG("%s detected PHY at address %hhu: "
				"ID 0x%08X",
				dev->name,
				phy_curr_addr, phy_id);

			/*
			 * Iterate the list of all supported PHYs -> if the
			 * current PHY is supported, store all related data
			 * in the device's run-time data struct.
			 */
			for (list_iter = 0; list_iter < ARRAY_SIZE(phy_xlnx_gem_supported_devs);
					list_iter++) {
				if (phy_xlnx_gem_supported_devs[list_iter].phy_id ==
					(phy_xlnx_gem_supported_devs[list_iter].phy_id_mask
					& phy_id)) {
					LOG_DBG("%s identified supported PHY: %s",
						dev->name,
						phy_xlnx_gem_supported_devs[list_iter].identifier);

					/*
					 * Store the numeric values of the PHY ID and address
					 * as well as the corresponding set of function pointers
					 * in the device's run-time data struct.
					 */
					dev_data->phy_addr = phy_curr_addr;
					dev_data->phy_id = phy_id;
					dev_data->phy_access_api =
						phy_xlnx_gem_supported_devs[list_iter].api;

					return 0;
				}
			}
		}
	}

	LOG_ERR("%s PHY detection failed", dev->name);
	return -EIO;
}
