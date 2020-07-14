/*
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family Ethernet PHY (GMAC) driver.
 */

#include <errno.h>
#include <kernel.h>
#include <net/mii.h>
#include "phy_sam_gmac.h"

#ifdef CONFIG_SOC_FAMILY_SAM0
#include "eth_sam0_gmac.h"
#endif

#define LOG_MODULE_NAME eth_sam_phy
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* Maximum time to establish a link through auto-negotiation for
 * 10BASE-T, 100BASE-TX is 3.7s, to add an extra margin the timeout
 * is set at 4s.
 */
#define PHY_AUTONEG_TIMEOUT_MS   4000

/* Enable MDIO serial bus between MAC and PHY. */
static void mdio_bus_enable(Gmac *gmac)
{
	gmac->GMAC_NCR |= GMAC_NCR_MPE;
}

/* Disable MDIO serial bus between MAC and PHY. */
static void mdio_bus_disable(Gmac *gmac)
{
	gmac->GMAC_NCR &= ~GMAC_NCR_MPE;
}

/* Wait PHY operation complete. */
static int mdio_bus_wait(Gmac *gmac)
{
	uint32_t retries = 100U;  /* will wait up to 1 s */

	while (!(gmac->GMAC_NSR & GMAC_NSR_IDLE))   {
		if (retries-- == 0U) {
			LOG_ERR("timeout");
			return -ETIMEDOUT;
		}

		k_sleep(K_MSEC(10));
	}

	return 0;
}

/* Send command to PHY over MDIO serial bus */
static int mdio_bus_send(Gmac *gmac, uint8_t phy_addr, uint8_t reg_addr,
			 uint8_t rw, uint16_t data)
{
	int retval;

	/* Write GMAC PHY maintenance register */
	gmac->GMAC_MAN =   GMAC_MAN_CLTTO
			 | (GMAC_MAN_OP(rw ? 0x2 : 0x1))
			 | GMAC_MAN_WTN(0x02)
			 | GMAC_MAN_PHYA(phy_addr)
			 | GMAC_MAN_REGA(reg_addr)
			 | GMAC_MAN_DATA(data);

	/* Wait until PHY is ready */
	retval = mdio_bus_wait(gmac);
	if (retval < 0) {
		return retval;
	}

	return 0;
}

/* Read PHY register. */
static int phy_read(const struct phy_sam_gmac_dev *phy, uint8_t reg_addr,
		    uint32_t *value)
{
	Gmac *const gmac = phy->regs;
	uint8_t phy_addr = phy->address;
	int retval;

	retval = mdio_bus_send(gmac, phy_addr, reg_addr, 1, 0);
	if (retval < 0) {
		return retval;
	}

	/* Read data */
	*value = gmac->GMAC_MAN & GMAC_MAN_DATA_Msk;

	return 0;
}

/* Write PHY register. */
static int phy_write(const struct phy_sam_gmac_dev *phy, uint8_t reg_addr,
		     uint32_t value)
{
	Gmac *const gmac = phy->regs;
	uint8_t phy_addr = phy->address;

	return mdio_bus_send(gmac, phy_addr, reg_addr, 0, value);
}

/* Issue a PHY soft reset. */
static int phy_soft_reset(const struct phy_sam_gmac_dev *phy)
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

int phy_sam_gmac_init(const struct phy_sam_gmac_dev *phy)
{
	Gmac *const gmac = phy->regs;
	int phy_id;

	mdio_bus_enable(gmac);

	LOG_INF("Soft Reset of ETH PHY");
	phy_soft_reset(phy);

	/* Verify that the PHY device is responding */
	phy_id = phy_sam_gmac_id_get(phy);
	if (phy_id == 0xFFFFFFFF) {
		LOG_ERR("Unable to detect a valid PHY");
		return -1;
	}

	LOG_INF("PHYID: 0x%X at addr: %d", phy_id, phy->address);

	mdio_bus_disable(gmac);

	return 0;
}

uint32_t phy_sam_gmac_id_get(const struct phy_sam_gmac_dev *phy)
{
	Gmac *const gmac = phy->regs;
	uint32_t phy_reg;
	uint32_t phy_id;

	mdio_bus_enable(gmac);

	if (phy_read(phy, MII_PHYID1R, &phy_reg) < 0) {
		return 0xFFFFFFFF;
	}

	phy_id = (phy_reg & 0xFFFF) << 16;

	if (phy_read(phy, MII_PHYID2R, &phy_reg) < 0) {
		return 0xFFFFFFFF;
	}

	phy_id |= (phy_reg & 0xFFFF);

	mdio_bus_disable(gmac);

	return phy_id;
}

bool phy_sam_gmac_link_status_get(const struct phy_sam_gmac_dev *phy)
{
	Gmac * const gmac = phy->regs;
	uint32_t bmsr;

	mdio_bus_enable(gmac);

	if (phy_read(phy, MII_BMSR, &bmsr) < 0) {
		return false;
	}

	mdio_bus_disable(gmac);

	return (bmsr & MII_BMSR_LINK_STATUS) != 0;
}

int phy_sam_gmac_auto_negotiate(const struct phy_sam_gmac_dev *phy,
				uint32_t *status)
{
	Gmac *const gmac = phy->regs;
	uint32_t val;
	uint32_t ability_adv;
	uint32_t ability_rcvd;
	uint32_t retries = PHY_AUTONEG_TIMEOUT_MS / 100;
	int retval;

	mdio_bus_enable(gmac);

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
		*status = PHY_DUPLEX_FULL | PHY_SPEED_100M;
	} else if ((ability_adv & ability_rcvd) & MII_ADVERTISE_100_HALF) {
		*status = PHY_DUPLEX_HALF | PHY_SPEED_100M;
	} else if ((ability_adv & ability_rcvd) & MII_ADVERTISE_10_FULL) {
		*status = PHY_DUPLEX_FULL | PHY_SPEED_10M;
	} else {
		*status = PHY_DUPLEX_HALF | PHY_SPEED_10M;
	}

	LOG_INF("common abilities: speed %s Mb, %s duplex",
		*status & PHY_SPEED_100M ? "100" : "10",
		*status & PHY_DUPLEX_FULL ? "full" : "half");

auto_negotiate_exit:
	mdio_bus_disable(gmac);
	return retval;
}
