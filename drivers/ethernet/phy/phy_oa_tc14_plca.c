/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/mdio.h>
#include <zephyr/net/phy.h>
#include <errno.h>

/* Open Alliance TC14 (10BASE-T1S) PLCA registers */
#define MDIO_OATC14_PLCA_IDVER  0xca00 /* PLCA ID and version */
#define MDIO_OATC14_PLCA_CTRL0  0xca01 /* PLCA Control register 0 */
#define MDIO_OATC14_PLCA_CTRL1  0xca02 /* PLCA Control register 1 */
#define MDIO_OATC14_PLCA_STATUS 0xca03 /* PLCA Status register */
#define MDIO_OATC14_PLCA_TOTMR  0xca04 /* PLCA TO Timer register */
#define MDIO_OATC14_PLCA_BURST  0xca05 /* PLCA BURST mode register */

/* Open Alliance TC14 PLCA IDVER register */
#define MDIO_OATC14_PLCA_IDM GENMASK(15, 8) /* PLCA MAP ID */
#define MDIO_OATC14_PLCA_VER GENMASK(7, 0)  /* PLCA MAP version */

/* Open Alliance TC14 PLCA CTRL0 register */
#define MDIO_OATC14_PLCA_EN  BIT(15) /* PLCA enable */
#define MDIO_OATC14_PLCA_RST BIT(14) /* PLCA reset */

/* Open Alliance TC14 PLCA CTRL1 register */
#define MDIO_OATC14_PLCA_NCNT GENMASK(15, 8) /* PLCA node count */
#define MDIO_OATC14_PLCA_ID   GENMASK(7, 0)  /* PLCA local node ID */

/* Open Alliance TC14 PLCA STATUS register */
#define MDIO_OATC14_PLCA_PST BIT(15) /* PLCA status indication */

/* Open Alliance TC14 PLCA TOTMR register */
#define MDIO_OATC14_PLCA_TOT GENMASK(7, 0)

/* Open Alliance TC14 PLCA BURST register */
#define MDIO_OATC14_PLCA_MAXBC GENMASK(15, 8)
#define MDIO_OATC14_PLCA_BTMR  GENMASK(7, 0)

/* Version Identifiers */
#define OATC14_IDM 0x0a00

int genphy_set_plca_cfg(const struct device *dev, struct phy_plca_cfg *plca_cfg)
{
	uint16_t val;
	int ret;

	/* Disable plca before doing the configuration */
	ret = phy_write_c45(dev, MDIO_MMD_VENDOR_SPECIFIC2, MDIO_OATC14_PLCA_CTRL0, 0x0);
	if (ret) {
		return ret;
	}

	if (!plca_cfg->enable) {
		/* As the PLCA is disabled above, just return */
		return 0;
	}

	val = (plca_cfg->node_count << 8) | plca_cfg->node_id;
	ret = phy_write_c45(dev, MDIO_MMD_VENDOR_SPECIFIC2, MDIO_OATC14_PLCA_CTRL1, val);
	if (ret) {
		return ret;
	}

	val = (plca_cfg->burst_count << 8) | plca_cfg->burst_timer;
	ret = phy_write_c45(dev, MDIO_MMD_VENDOR_SPECIFIC2, MDIO_OATC14_PLCA_BURST, val);
	if (ret) {
		return ret;
	}

	ret = phy_write_c45(dev, MDIO_MMD_VENDOR_SPECIFIC2, MDIO_OATC14_PLCA_TOTMR,
			    plca_cfg->to_timer);
	if (ret) {
		return ret;
	}

	/* Enable plca after doing all the configuration */
	return phy_write_c45(dev, MDIO_MMD_VENDOR_SPECIFIC2, MDIO_OATC14_PLCA_CTRL0,
			     MDIO_OATC14_PLCA_EN);
}

int genphy_get_plca_cfg(const struct device *dev, struct phy_plca_cfg *plca_cfg)
{
	uint16_t val;
	int ret;

	ret = phy_read_c45(dev, MDIO_MMD_VENDOR_SPECIFIC2, MDIO_OATC14_PLCA_IDVER, &val);
	if (ret) {
		return ret;
	}

	if ((val & MDIO_OATC14_PLCA_IDM) != OATC14_IDM) {
		return -ENODEV;
	}

	plca_cfg->version = ret & ~MDIO_OATC14_PLCA_IDM;

	ret = phy_read_c45(dev, MDIO_MMD_VENDOR_SPECIFIC2, MDIO_OATC14_PLCA_CTRL0, &val);
	if (ret) {
		return ret;
	}

	plca_cfg->enable = !!(val & MDIO_OATC14_PLCA_EN);

	ret = phy_read_c45(dev, MDIO_MMD_VENDOR_SPECIFIC2, MDIO_OATC14_PLCA_CTRL1, &val);
	if (ret) {
		return ret;
	}

	plca_cfg->node_id = val & MDIO_OATC14_PLCA_ID;
	plca_cfg->node_count = (val & MDIO_OATC14_PLCA_NCNT) >> 8;

	ret = phy_read_c45(dev, MDIO_MMD_VENDOR_SPECIFIC2, MDIO_OATC14_PLCA_BURST, &val);
	if (ret) {
		return ret;
	}

	plca_cfg->burst_timer = val & MDIO_OATC14_PLCA_BTMR;
	plca_cfg->burst_count = (val & MDIO_OATC14_PLCA_MAXBC) >> 8;

	ret = phy_read_c45(dev, MDIO_MMD_VENDOR_SPECIFIC2, MDIO_OATC14_PLCA_TOTMR, &val);
	if (ret) {
		return ret;
	}

	plca_cfg->to_timer = val & MDIO_OATC14_PLCA_TOT;

	return 0;
}

int genphy_get_plca_sts(const struct device *dev, bool *plca_sts)
{
	uint16_t val;
	int ret;

	ret = phy_read_c45(dev, MDIO_MMD_VENDOR_SPECIFIC2, MDIO_OATC14_PLCA_STATUS, &val);
	if (ret) {
		return ret;
	}

	*plca_sts = !!(val & MDIO_OATC14_PLCA_PST);

	return 0;
}
