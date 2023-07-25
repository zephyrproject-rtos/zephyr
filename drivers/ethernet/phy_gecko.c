/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SiLabs Giant Gecko GG11 Ethernet PHY driver. */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/mii.h>
#include "phy_gecko.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_gecko_phy, CONFIG_ETHERNET_LOG_LEVEL);

/* Maximum time to establish a link through auto-negotiation for
 * 10BASE-T, 100BASE-TX is 3.7s, to add an extra margin the timeout
 * is set at 4s.
 */
#define PHY_AUTONEG_TIMEOUT_MS   4000

/* Enable MDIO serial bus between MAC and PHY. */
static void mdio_bus_enable(ETH_TypeDef *eth)
{
	eth->NETWORKCTRL |= ETH_NETWORKCTRL_MANPORTEN;
}

/* Enable MDIO serial bus between MAC and PHY. */
static void mdio_bus_disable(ETH_TypeDef *eth)
{
	eth->NETWORKCTRL &= ~ETH_NETWORKCTRL_MANPORTEN;
}

/* Wait PHY operation complete. */
static int mdio_bus_wait(ETH_TypeDef *eth)
{
	uint32_t retries = 100U;  /* will wait up to 1 s */

	while (!(eth->NETWORKSTATUS & ETH_NETWORKSTATUS_MANDONE)) {
		if (retries-- == 0U) {
			LOG_ERR("timeout");
			return -ETIMEDOUT;
		}

		k_sleep(K_MSEC(10));
	}

	return 0;
}

/* Send command to PHY over MDIO serial bus */
static int mdio_bus_send(ETH_TypeDef *eth, uint8_t phy_addr, uint8_t reg_addr,
			 uint8_t rw, uint16_t data)
{
	int retval;

	/* Write PHY management register */
	eth->PHYMNGMNT = ETH_PHYMNGMNT_WRITE0_DEFAULT
		| ETH_PHYMNGMNT_WRITE1
		| ((rw ? 0x02 : 0x01) << _ETH_PHYMNGMNT_OPERATION_SHIFT)
		| ((phy_addr << _ETH_PHYMNGMNT_PHYADDR_SHIFT)
			& _ETH_PHYMNGMNT_PHYADDR_MASK)
		| ((reg_addr << _ETH_PHYMNGMNT_REGADDR_SHIFT)
			& _ETH_PHYMNGMNT_REGADDR_MASK)
		| (0x2 << _ETH_PHYMNGMNT_WRITE10_SHIFT)
		| (data & _ETH_PHYMNGMNT_PHYRWDATA_MASK);

	/* Wait until PHY is ready */
	retval = mdio_bus_wait(eth);
	if (retval < 0) {
		return retval;
	}

	return 0;
}

/* Read PHY register. */
static int phy_read(const struct phy_gecko_dev *phy, uint8_t reg_addr,
		    uint32_t *value)
{
	ETH_TypeDef *const eth = phy->regs;
	uint8_t phy_addr = phy->address;
	int retval;

	retval = mdio_bus_send(eth, phy_addr, reg_addr, 1, 0);
	if (retval < 0) {
		return retval;
	}

	/* Read data */
	*value = eth->PHYMNGMNT & _ETH_PHYMNGMNT_PHYRWDATA_MASK;

	return 0;
}

/* Write PHY register. */
static int phy_write(const struct phy_gecko_dev *phy, uint8_t reg_addr,
		     uint32_t value)
{
	ETH_TypeDef *const eth = phy->regs;
	uint8_t phy_addr = phy->address;

	return mdio_bus_send(eth, phy_addr, reg_addr, 0, value);
}

/* Issue a PHY soft reset. */
static int phy_soft_reset(const struct phy_gecko_dev *phy)
{
	uint32_t phy_reg;
	uint32_t retries = 12U;
	int retval;

	/* Issue a soft reset */
	retval = phy_write(phy, MII_BMCR, MII_BMCR_RESET);
	if (retval < 0) {
		return retval;
	}

	/* Wait up to 0.6s for the reset sequence to finish. According to
	 * IEEE 802.3, Section 2, Subsection 22.2.4.1.1 a PHY reset may take
	 * up to 0.5 s.
	 */
	do {
		if (retries-- == 0U) {
			return -ETIMEDOUT;
		}

		k_sleep(K_MSEC(50));

		retval = phy_read(phy, MII_BMCR, &phy_reg);
		if (retval < 0) {
			return retval;
		}
	} while (phy_reg & MII_BMCR_RESET);

	return 0;
}

int phy_gecko_init(const struct phy_gecko_dev *phy)
{
	ETH_TypeDef *const eth = phy->regs;
	int phy_id;

	mdio_bus_enable(eth);

	LOG_INF("Soft Reset of ETH PHY");
	phy_soft_reset(phy);

	/* Verify that the PHY device is responding */
	phy_id = phy_gecko_id_get(phy);
	if (phy_id == 0xFFFFFFFF) {
		LOG_ERR("Unable to detect a valid PHY");
		return -1;
	}

	LOG_INF("PHYID: 0x%X at addr: %d", phy_id, phy->address);

	mdio_bus_disable(eth);

	return 0;
}

uint32_t phy_gecko_id_get(const struct phy_gecko_dev *phy)
{
	ETH_TypeDef *const eth = phy->regs;
	uint32_t phy_reg;
	uint32_t phy_id;

	mdio_bus_enable(eth);

	if (phy_read(phy, MII_PHYID1R, &phy_reg) < 0) {
		return 0xFFFFFFFF;
	}

	phy_id = (phy_reg & 0xFFFF) << 16;

	if (phy_read(phy, MII_PHYID2R, &phy_reg) < 0) {
		return 0xFFFFFFFF;
	}

	phy_id |= (phy_reg & 0xFFFF);

	mdio_bus_disable(eth);

	return phy_id;
}

int phy_gecko_auto_negotiate(const struct phy_gecko_dev *phy,
				uint32_t *status)
{
	ETH_TypeDef *const eth = phy->regs;
	uint32_t val;
	uint32_t ability_adv;
	uint32_t ability_rcvd;
	uint32_t retries = PHY_AUTONEG_TIMEOUT_MS / 100;
	int retval;

	mdio_bus_enable(eth);

	LOG_DBG("Starting ETH PHY auto-negotiate sequence");

	/* Read PHY default advertising parameters */
	retval = phy_read(phy, MII_ANAR, &ability_adv);
	if (retval < 0) {
		goto auto_negotiate_exit;
	}

	/* Configure and start auto-negotiation process */
	retval = phy_read(phy, MII_BMCR, &val);
	if (retval < 0) {
		goto auto_negotiate_exit;
	}

	val |= MII_BMCR_AUTONEG_ENABLE | MII_BMCR_AUTONEG_RESTART;
	val &= ~MII_BMCR_ISOLATE;  /* Don't isolate the PHY */

	retval = phy_write(phy, MII_BMCR, val);
	if (retval < 0) {
		goto auto_negotiate_exit;
	}

	/* Wait for the auto-negotiation process to complete */
	do {
		if (retries-- == 0U) {
			retval = -ETIMEDOUT;
			goto auto_negotiate_exit;
		}

		k_sleep(K_MSEC(100));

		retval = phy_read(phy, MII_BMSR, &val);
		if (retval < 0) {
			goto auto_negotiate_exit;
		}
	} while (!(val & MII_BMSR_AUTONEG_COMPLETE));

	LOG_DBG("PHY auto-negotiate sequence completed");

	/* Read abilities of the remote device */
	retval = phy_read(phy, MII_ANLPAR, &ability_rcvd);
	if (retval < 0) {
		goto auto_negotiate_exit;
	}

	/* Determine the best possible mode of operation */
	if ((ability_adv & ability_rcvd) & MII_ADVERTISE_100_FULL) {
		*status = ETH_NETWORKCFG_FULLDUPLEX | ETH_NETWORKCFG_SPEED;
	} else if ((ability_adv & ability_rcvd) & MII_ADVERTISE_100_HALF) {
		*status = ETH_NETWORKCFG_SPEED;
	} else if ((ability_adv & ability_rcvd) & MII_ADVERTISE_10_FULL) {
		*status = ETH_NETWORKCFG_FULLDUPLEX;
	} else {
		*status = 0;
	}

	LOG_DBG("common abilities: speed %s Mb, %s duplex",
		*status & ETH_NETWORKCFG_SPEED ? "100" : "10",
		*status & ETH_NETWORKCFG_FULLDUPLEX ? "full" : "half");

auto_negotiate_exit:
	mdio_bus_disable(eth);
	return retval;
}

bool phy_gecko_is_linked(const struct phy_gecko_dev *phy)
{
	ETH_TypeDef *const eth = phy->regs;
	uint32_t phy_reg;
	bool phy_linked = false;

	mdio_bus_enable(eth);

	if (phy_read(phy, MII_BMSR, &phy_reg) < 0) {
		return phy_linked;
	}

	phy_linked = (phy_reg & MII_BMSR_LINK_STATUS);

	mdio_bus_disable(eth);

	return phy_linked;
}
