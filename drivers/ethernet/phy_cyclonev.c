#ifndef PHY_CYCLONEV_SRC
#define PHY_CYCLONEV_SRC
/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2022, Intel Corporation
 * Description:
 * Driver for the PHY KSZ9021RL/RN Datasheet:(https://ww1.microchip.com/
 * downloads/en/DeviceDoc/KSZ9021RL-RN-Data-Sheet-DS00003050A.pdf)
 * specifically designed for Cyclone V SoC DevKit use only.
 */

/* PHY */
/* According to default Cyclone V DevKit Bootstrap Encoding Scheme */
#include "eth_cyclonev_priv.h"

#include <stdio.h>

#include <zephyr/kernel.h>

#include <sys/types.h>

#define PHY_ADDR (4)

/*  PHY_Read_write_Timeouts   */
#define PHY_READ_TO  ((uint32_t)0x0004FFFF)
#define PHY_WRITE_TO ((uint32_t)0x0004FFFF)

/* Speed and Duplex mask values  */
#define PHY_SPEED_100  (0x0020)
#define PHY_SPEED_1000 (0x0040)

#define PHY_CLK_AND_CONTROL_PAD_SKEW_VALUE 0xa0d0
#define PHY_RX_DATA_PAD_SKEW_VALUE	   0x0000

/* Write/read to/from extended registers */
#define MII_KSZPHY_EXTREG	0x0b
#define KSZPHY_EXTREG_WRITE	0x8000
#define MII_KSZPHY_EXTREG_WRITE 0x0c
#define MII_KSZPHY_EXTREG_READ	0x0d

/* PHY Regs */

/* Basic Control Register */
#define PHY_BCR			    (0)
#define PHY_RESET		    BIT(15) /* Do a PHY reset */
#define PHY_AUTONEGOTIATION	    BIT(12)
#define PHY_RESTART_AUTONEGOTIATION BIT(9)

/* Basic Status Register */
#define PHY_BSR		      BIT(0)
#define PHY_AUTOCAP	      BIT(30) /* Auto-negotiation capability */
#define PHY_LINKED_STATUS     BIT(2)
#define PHY_AUTONEGO_COMPLETE BIT(5)

/* Auto-Negotiation Advertisement */
#define PHY_AUTON	   (4)
#define PHYANA_10BASET	   BIT(5)
#define PHYANA_10BASETFD   BIT(6)
#define PHYANA_100BASETX   BIT(7)
#define PHYANA_100BASETXFD BIT(8)
#define PHYASYMETRIC_PAUSE BIT(11)

/* 1000Base-T Control */
#define PHY_1GCTL (9)

/* PHY Control Register */
#define PHY_CR		      (31)
#define PHY_DUPLEX_STATUS     (0x0008)
#define PHYADVERTISE_1000FULL BIT(9)
#define PHYADVERTISE_1000HALF BIT(8)

/* Extended registers */
#define MII_KSZPHY_CLK_CONTROL_PAD_SKEW 0x104
#define MII_KSZPHY_RX_DATA_PAD_SKEW	0x105
#define MII_KSZPHY_TX_DATA_PAD_SKEW	0x106


int alt_eth_phy_write_register(uint16_t emac_instance, uint16_t phy_reg,
		uint16_t phy_value, struct eth_cyclonev_priv *p);
int alt_eth_phy_read_register(uint16_t emac_instance, uint16_t phy_reg,
		uint16_t *rdval, struct eth_cyclonev_priv *p);
int alt_eth_phy_write_register_extended(uint16_t emac_instance, uint16_t phy_reg,
		uint16_t phy_value, struct eth_cyclonev_priv *p);
int alt_eth_phy_read_register_extended(uint16_t emac_instance, uint16_t phy_reg,
		uint16_t *rdval, struct eth_cyclonev_priv *p);
int alt_eth_phy_config(uint16_t instance, struct eth_cyclonev_priv *p);
int alt_eth_phy_reset(uint16_t instance, struct eth_cyclonev_priv *p);
int alt_eth_phy_get_duplex_and_speed(uint16_t *phy_duplex_status, uint16_t *phy_speed,
				     uint16_t instance, struct eth_cyclonev_priv *p);

int alt_eth_phy_write_register(uint16_t emac_instance, uint16_t phy_reg,
		uint16_t phy_value, struct eth_cyclonev_priv *p)
{
	uint16_t tmpreg = 0;
	volatile uint16_t timeout = 0;
	uint16_t phy_addr;

	if (emac_instance > 1) {
		return -1;
	}

	phy_addr = PHY_ADDR;

	/* Prepare the MII address register value */
	tmpreg = 0;
	/* Set the PHY device address */
	tmpreg |= EMAC_GMAC_GMII_ADDR_PA_SET(phy_addr);
	/* Set the PHY register address */
	tmpreg |= EMAC_GMAC_GMII_ADDR_GR_SET(phy_reg);
	/* Set the write mode */
	tmpreg |= EMAC_GMAC_GMII_ADDR_GW_SET_MSK;
	/* Set the clock divider */
	tmpreg |= EMAC_GMAC_GMII_ADDR_CR_SET(EMAC_GMAC_GMII_ADDR_CR_E_DIV102);
	/* Set the MII Busy bit */
	tmpreg |= EMAC_GMAC_GMII_ADDR_GB_SET(EMAC_GMAC_GMII_ADDR_GB_SET_MSK);

	/* Give the value to the MII data register */
	sys_write32(phy_value & 0xffff, EMAC_GMAC_GMII_DATA_ADDR(p->base_addr));
	/* Write the result value into the MII Address register */
	sys_write32(tmpreg & 0xffff, EMAC_GMAC_GMII_ADDR_ADDR(p->base_addr));


	/* Check the Busy flag */
	do {
		timeout++;
		tmpreg = sys_read32(EMAC_GMAC_GMII_ADDR_ADDR(p->base_addr));
	} while ((tmpreg & EMAC_GMAC_GMII_ADDR_GB_SET_MSK) && (timeout < PHY_WRITE_TO));

	/* Return ERROR in case of timeout */
	if (timeout == PHY_WRITE_TO) {
		return -1;
	}

	/* Return SUCCESS */
	return 0;
}

int alt_eth_phy_read_register(uint16_t emac_instance, uint16_t phy_reg, uint16_t *rdval,
		struct eth_cyclonev_priv *p)
{
	uint16_t tmpreg = 0;
	volatile uint16_t timeout = 0;
	uint16_t phy_addr;

	if (emac_instance > 1) {
		return -1;
	}

	phy_addr = PHY_ADDR;

	/* Prepare the MII address register value */
	tmpreg = 0;
	/* Set the PHY device address */
	tmpreg |= EMAC_GMAC_GMII_ADDR_PA_SET(phy_addr);
	/* Set the PHY register address */
	tmpreg |= EMAC_GMAC_GMII_ADDR_GR_SET(phy_reg);
	/* Set the read mode */
	tmpreg &= EMAC_GMAC_GMII_ADDR_GW_CLR_MSK;
	/* Set the clock divider */
	tmpreg |= EMAC_GMAC_GMII_ADDR_CR_SET(EMAC_GMAC_GMII_ADDR_CR_E_DIV102);
	/* Set the MII Busy bit */
	tmpreg |= EMAC_GMAC_GMII_ADDR_GB_SET(EMAC_GMAC_GMII_ADDR_GB_SET_MSK);

	/* Write the result value into the MII Address register */
	sys_write32(tmpreg & 0xffff, EMAC_GMAC_GMII_ADDR_ADDR(p->base_addr));

	/* Check the Busy flag */
	do {
		timeout++;
		tmpreg = sys_read32(EMAC_GMAC_GMII_ADDR_ADDR(p->base_addr));
	} while ((tmpreg & EMAC_GMAC_GMII_ADDR_GB_SET_MSK) && (timeout < PHY_READ_TO));

	/* Return ERROR in case of timeout */
	if (timeout == PHY_READ_TO) {
		return -1;
	}

	/* Return data register value */
	*rdval = sys_read32(EMAC_GMAC_GMII_DATA_ADDR(p->base_addr));

	return 0;
}

int alt_eth_phy_write_register_extended(uint16_t emac_instance, uint16_t phy_reg,
					uint16_t phy_value, struct eth_cyclonev_priv *p)
{
	int rc;

	rc = alt_eth_phy_write_register(emac_instance, MII_KSZPHY_EXTREG,
					KSZPHY_EXTREG_WRITE | phy_reg, p);

	if (rc == -1) {
		return rc;
	}

	rc = alt_eth_phy_write_register(emac_instance, MII_KSZPHY_EXTREG_WRITE, phy_value, p);
	return rc;
}

int alt_eth_phy_read_register_extended(uint16_t emac_instance, uint16_t phy_reg, uint16_t *rdval,
		struct eth_cyclonev_priv *p)
{
	int rc;

	rc = alt_eth_phy_write_register(emac_instance, MII_KSZPHY_EXTREG, phy_reg, p);

	if (rc == -1) {
		return rc;
	}
	k_sleep(K_MSEC(1));

	rc = alt_eth_phy_read_register(emac_instance, MII_KSZPHY_EXTREG_READ, rdval, p);

	return rc;
}

int alt_eth_phy_config(uint16_t instance, struct eth_cyclonev_priv *p)
{

	int rc;
	uint16_t rdval, timeout;
	/*--------------------   Configure the PHY skew values  ----------------*/

	rc = alt_eth_phy_write_register_extended(instance, MII_KSZPHY_CLK_CONTROL_PAD_SKEW,
						 PHY_CLK_AND_CONTROL_PAD_SKEW_VALUE, p);
	if (rc == -1) {
		return rc;
	}

	rc = alt_eth_phy_write_register_extended(instance, MII_KSZPHY_RX_DATA_PAD_SKEW,
						 PHY_RX_DATA_PAD_SKEW_VALUE, p);
	if (rc == -1) {
		return rc;
	}

	/* Implement Auto-negotiation Process */

	/* Check PHY Status if auto-negotiation is supported */
	rc = alt_eth_phy_read_register(instance, PHY_BSR, &rdval, p);
	if (((rdval & PHY_AUTOCAP) == 0) || (rc == -1)) {
		return -1;
	}

	/* Set Advertise capabilities for 10Base-T/
	 *10Base-T full-duplex/100Base-T/100Base-T full-duplex
	 */
	rc = alt_eth_phy_read_register(instance, PHY_AUTON, &rdval, p);
	if (rc == -1) {
		return rc;
	}

	rdval |= (PHYANA_10BASET | PHYANA_10BASETFD | PHYANA_100BASETX | PHYANA_100BASETXFD |
		  PHYASYMETRIC_PAUSE);
	rc = alt_eth_phy_write_register(instance, PHY_AUTON, rdval, p);
	if (rc == -1) {
		return rc;
	}

	/* Set Advertise capabilities for 1000 Base-T/1000 Base-T full-duplex */

	rc = alt_eth_phy_write_register(instance, PHY_1GCTL,
					PHYADVERTISE_1000FULL | PHYADVERTISE_1000HALF, p);
	if (rc == -1) {
		return rc;
	}

	/* Wait for linked status... */
	timeout = 0;
	do {
		timeout++;
		rc = alt_eth_phy_read_register(instance, PHY_BSR, &rdval, p);
	} while (!(rdval & PHY_LINKED_STATUS) && (timeout < PHY_READ_TO) && (rc == 0));

	/* Return ERROR in case of timeout */
	if ((timeout == PHY_READ_TO) || (rc == -1)) {
		LOG_ERR("Error Link Down\n");
		return -1;
	}
	LOG_INF("Link is up!");

	/* Configure the PHY for AutoNegotiate */
	rc = alt_eth_phy_read_register(instance, PHY_BCR, &rdval, p);
	if (rc == -1) {
		return rc;
	}

	rdval |= PHY_AUTONEGOTIATION;
	rdval |= PHY_RESTART_AUTONEGOTIATION;
	rc = alt_eth_phy_write_register(instance, PHY_BCR, rdval, p);
	if (rc == -1) {
		return rc;
	}

	/* Wait until the auto-negotiation is completed */
	timeout = 0;
	do {
		timeout++;
		rc = alt_eth_phy_read_register(instance, PHY_BSR, &rdval, p);
	} while (!(rdval & PHY_AUTONEGO_COMPLETE) && (timeout < PHY_READ_TO) && (rc == 0));

	/* Return ERROR in case of timeout */
	if ((timeout == PHY_READ_TO) || (rc == -1)) {
		alt_eth_phy_read_register(instance, PHY_BSR, &rdval, p);
		LOG_ERR("Auto Negotiation: Status reg = 0x%x\n", rdval);
		return -1;
	}
	LOG_INF("Auto Negotiation Complete!");

	return rc;
};

int alt_eth_phy_reset(uint16_t instance, struct eth_cyclonev_priv *p)
{
	int i;
	int rc;
	uint16_t rdval;

	/* Put the PHY in reset mode */
	if ((alt_eth_phy_write_register(instance, PHY_BCR, PHY_RESET, p)) != 0) {
		/* Return ERROR in case of write timeout */
		return -1;
	}

	/* Wait for the reset to clear */
	for (i = 0; i < 10; i++) {
		k_sleep(K_MSEC(10));
		rc = alt_eth_phy_read_register(instance, PHY_BCR, &rdval, p);
		if (((rdval & PHY_RESET) == 0) || (rc == -1)) {
			break;
		}
	}

	if (i == 10) {
		return -1;
	}
	/* Delay to assure PHY reset */
	k_sleep(K_MSEC(10));

	return rc;
};

int alt_eth_phy_get_duplex_and_speed(uint16_t *phy_duplex_status, uint16_t *phy_speed,
				     uint16_t instance, struct eth_cyclonev_priv *p)
{

	LOG_DBG("PHY: func_alt_eth_phy_get_duplex_and_speed\n");
	uint16_t regval = 0;
	int rc;

	rc = alt_eth_phy_read_register(instance, PHY_CR, &regval, p);

	if (regval & PHY_DUPLEX_STATUS) {
		*phy_duplex_status = 1;
	} else {
		*phy_duplex_status = 0;
	}

	if (regval & PHY_SPEED_100) {
		*phy_speed = 100;
	} else {
		if (regval & PHY_SPEED_1000) {
			*phy_speed = 1000;
		} else {
			*phy_speed = 10;
		}
	}

	return rc;
}

#endif
