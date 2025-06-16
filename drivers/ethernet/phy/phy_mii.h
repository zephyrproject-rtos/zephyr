/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_PHY_MII_H_
#define ZEPHYR_PHY_MII_H_

#include <zephyr/device.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>

static inline int phy_mii_set_anar_reg(const struct device *dev, enum phy_link_speed adv_speeds)
{
	uint32_t anar_reg = 0U;
	uint32_t anar_reg_old;

	if (phy_read(dev, MII_ANAR, &anar_reg) < 0) {
		return -EIO;
	}
	anar_reg_old = anar_reg;

	WRITE_BIT(anar_reg, MII_ADVERTISE_10_FULL_BIT, (adv_speeds & LINK_FULL_10BASE) != 0U);
	WRITE_BIT(anar_reg, MII_ADVERTISE_10_HALF_BIT, (adv_speeds & LINK_HALF_10BASE) != 0U);
	WRITE_BIT(anar_reg, MII_ADVERTISE_100_FULL_BIT, (adv_speeds & LINK_FULL_100BASE) != 0U);
	WRITE_BIT(anar_reg, MII_ADVERTISE_100_HALF_BIT, (adv_speeds & LINK_HALF_100BASE) != 0U);

	if (anar_reg == anar_reg_old) {
		return -EALREADY;
	}

	if (phy_write(dev, MII_ANAR, anar_reg) < 0) {
		return -EIO;
	}

	return 0;
}

static inline int phy_mii_set_c1kt_reg(const struct device *dev, enum phy_link_speed adv_speeds)
{
	uint32_t c1kt_reg = 0U;
	uint32_t c1kt_reg_old;

	if (phy_read(dev, MII_1KTCR, &c1kt_reg) < 0) {
		return -EIO;
	}
	c1kt_reg_old = c1kt_reg;

	WRITE_BIT(c1kt_reg, MII_ADVERTISE_1000_FULL_BIT, (adv_speeds & LINK_FULL_1000BASE) != 0U);
	WRITE_BIT(c1kt_reg, MII_ADVERTISE_1000_HALF_BIT, (adv_speeds & LINK_HALF_1000BASE) != 0U);

	if (c1kt_reg == c1kt_reg_old) {
		return -EALREADY;
	}

	if (phy_write(dev, MII_1KTCR, c1kt_reg) < 0) {
		return -EIO;
	}

	return 0;
}
#endif /* ZEPHYR_PHY_MII_H_ */
