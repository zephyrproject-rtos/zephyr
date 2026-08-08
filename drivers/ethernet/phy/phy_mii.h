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

/* Offset to align capabilities bits of 1000BASE-T Control and Status regs */
#define MII_1KSTSR_OFFSET 2

#define PHY_INST_GENERATE_DEFAULT_SPEEDS(n)							\
((DT_INST_ENUM_HAS_VALUE(n, default_speeds, 10base_half_duplex) ? LINK_HALF_10BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 10base_full_duplex) ? LINK_FULL_10BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 100base_half_duplex) ? LINK_HALF_100BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 100base_full_duplex) ? LINK_FULL_100BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 1000base_half_duplex) ? LINK_HALF_1000BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 1000base_full_duplex) ? LINK_FULL_1000BASE : 0))

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

static inline int phy_mii_cfg_link_autoneg(const struct device *dev, enum phy_link_speed adv_speeds,
					   bool gigabit_supported)
{
	uint32_t bmcr_reg = 0U;
	uint32_t bmcr_reg_old;
	int ret = 0;

	if (phy_read(dev, MII_BMCR, &bmcr_reg) < 0) {
		return -EIO;
	}
	bmcr_reg_old = bmcr_reg;

	/* Disable isolation */
	bmcr_reg &= ~MII_BMCR_ISOLATE;
	/* Enable auto-negotiation */
	bmcr_reg |= MII_BMCR_AUTONEG_ENABLE;

	ret = phy_mii_set_anar_reg(dev, adv_speeds);
	if (ret >= 0) {
		bmcr_reg |= MII_BMCR_AUTONEG_RESTART;
	} else if (ret != -EALREADY) {
		return ret;
	}

	if (gigabit_supported) {
		ret = phy_mii_set_c1kt_reg(dev, adv_speeds);
		if (ret >= 0) {
			bmcr_reg |= MII_BMCR_AUTONEG_RESTART;
		} else if (ret != -EALREADY) {
			return ret;
		}
	}

	if (bmcr_reg != bmcr_reg_old) {
		if (phy_write(dev, MII_BMCR, bmcr_reg) < 0) {
			return -EIO;
		}

		return 0;
	}

	return -EALREADY;
}

static inline int phy_mii_set_bmcr_reg_autoneg_disabled(const struct device *dev,
							enum phy_link_speed adv_speeds)
{
	uint32_t bmcr_reg = 0U;
	uint32_t bmcr_reg_old;

	if (phy_read(dev, MII_BMCR, &bmcr_reg) < 0) {
		return -EIO;
	}
	bmcr_reg_old = bmcr_reg;

	/* Disable auto-negotiation */
	bmcr_reg &= ~(MII_BMCR_AUTONEG_ENABLE | MII_BMCR_SPEED_LSB | MII_BMCR_SPEED_MSB);

	if (PHY_LINK_IS_SPEED_1000M(adv_speeds)) {
		bmcr_reg |= MII_BMCR_SPEED_1000;
	} else if (PHY_LINK_IS_SPEED_100M(adv_speeds)) {
		bmcr_reg |= MII_BMCR_SPEED_100;
	} else if (PHY_LINK_IS_SPEED_10M(adv_speeds)) {
		bmcr_reg |= MII_BMCR_SPEED_10;
	} else {
		LOG_ERR("Invalid speed %d", adv_speeds);
		return -EINVAL;
	}

	WRITE_BIT(bmcr_reg, MII_BMCR_DUPLEX_MODE_BIT, PHY_LINK_IS_FULL_DUPLEX(adv_speeds));

	if (bmcr_reg == bmcr_reg_old) {
		return -EALREADY;
	}

	if (phy_write(dev, MII_BMCR, bmcr_reg) < 0) {
		return -EIO;
	}

	return 0;
}

static inline enum phy_link_speed phy_mii_get_link_speed_bmcr_reg(const struct device *dev,
								  uint16_t bmcr_reg)
{
	enum phy_link_speed speed;

	switch (bmcr_reg & (MII_BMCR_DUPLEX_MODE | MII_BMCR_SPEED_MASK)) {
	case MII_BMCR_DUPLEX_MODE | MII_BMCR_SPEED_1000:
		speed = LINK_FULL_1000BASE;
		break;
	case MII_BMCR_DUPLEX_MODE | MII_BMCR_SPEED_100:
		speed = LINK_FULL_100BASE;
		break;
	case MII_BMCR_DUPLEX_MODE | MII_BMCR_SPEED_10:
		speed = LINK_FULL_10BASE;
		break;
	case MII_BMCR_SPEED_1000:
		speed = LINK_HALF_1000BASE;
		break;
	case MII_BMCR_SPEED_100:
		speed = LINK_HALF_100BASE;
		break;
	case MII_BMCR_SPEED_10:
	default:
		speed = LINK_HALF_10BASE;
		break;
	}

	return speed;
}

static inline int phy_mii_check_autoneg(const struct device *dev)
{
	uint32_t bmsr_reg = 0;

	if (phy_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	if (!IS_BIT_SET(bmsr_reg, MII_BMSR_AUTONEG_COMPLETE_BIT)) {
		return -EINPROGRESS;
	}

	/* Link status bit is latched low, so read it again to get current status */
	if (likely(IS_BIT_SET(bmsr_reg, MII_BMSR_LINK_STATUS_BIT))) {
		return 0;
	}
	/* Second read, clears the latched bits and gives the correct status */
	if (phy_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	if (!IS_BIT_SET(bmsr_reg, MII_BMSR_LINK_STATUS_BIT)) {
		return -EAGAIN;
	}

	return 0;
}

static inline int phy_mii_get_autoneg_speed(const struct device *dev, struct phy_link_state *state,
					    bool gigabit_supported)
{
	uint32_t anar_reg = 0;
	uint32_t anlpar_reg = 0;
	uint16_t mutual_capabilities;

	if (gigabit_supported) {
		uint32_t c1kt_reg = 0;
		uint32_t s1kt_reg = 0;

		if (phy_read(dev, MII_1KTCR, &c1kt_reg) < 0) {
			return -EIO;
		}
		if (phy_read(dev, MII_1KSTSR, &s1kt_reg) < 0) {
			return -EIO;
		}
		mutual_capabilities = (uint16_t)(s1kt_reg >> MII_1KSTSR_OFFSET);
		mutual_capabilities &= (uint16_t)c1kt_reg;

		if (IS_BIT_SET(mutual_capabilities, MII_ADVERTISE_1000_FULL_BIT)) {
			state->speed = LINK_FULL_1000BASE;
			state->is_up = true;
			return 0;
		}
		if (IS_BIT_SET(mutual_capabilities, MII_ADVERTISE_1000_HALF_BIT)) {
			state->speed = LINK_HALF_1000BASE;
			state->is_up = true;
			return 0;
		}
	}

	/* Read PHY default advertising parameters */
	if (phy_read(dev, MII_ANAR, &anar_reg) < 0) {
		return -EIO;
	}

	/** Read peer device capability */
	if (phy_read(dev, MII_ANLPAR, &anlpar_reg) < 0) {
		return -EIO;
	}

	mutual_capabilities = (uint16_t)(anar_reg & anlpar_reg);

	if (IS_BIT_SET(mutual_capabilities, MII_ADVERTISE_100_FULL_BIT)) {
		state->speed = LINK_FULL_100BASE;
	} else if (IS_BIT_SET(mutual_capabilities, MII_ADVERTISE_100_HALF_BIT)) {
		state->speed = LINK_HALF_100BASE;
	} else if (IS_BIT_SET(mutual_capabilities, MII_ADVERTISE_10_FULL_BIT)) {
		state->speed = LINK_FULL_10BASE;
	} else {
		state->speed = LINK_HALF_10BASE;
	}

	state->is_up = true;

	return 0;
}

#endif /* ZEPHYR_PHY_MII_H_ */
