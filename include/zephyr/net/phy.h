/**
 * @file
 *
 * @brief Public APIs for Ethernet PHY drivers.
 */

/*
 * Copyright (c) 2021 IP-Logix Inc.
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_PHY_H_
#define ZEPHYR_INCLUDE_DRIVERS_PHY_H_

/**
 * @brief Ethernet PHY Interface
 * @defgroup ethernet_phy Ethernet PHY Interface
 * @since 2.7
 * @version 0.8.0
 * @ingroup networking
 * @{
 */
#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Ethernet link speeds. */
enum phy_link_speed {
	/** 10Base-T Half-Duplex */
	LINK_HALF_10BASE_T		= BIT(0),
	/** 10Base-T Full-Duplex */
	LINK_FULL_10BASE_T		= BIT(1),
	/** 100Base-T Half-Duplex */
	LINK_HALF_100BASE_T		= BIT(2),
	/** 100Base-T Full-Duplex */
	LINK_FULL_100BASE_T		= BIT(3),
	/** 1000Base-T Half-Duplex */
	LINK_HALF_1000BASE_T		= BIT(4),
	/** 1000Base-T Full-Duplex */
	LINK_FULL_1000BASE_T		= BIT(5),
};

/**
 * @brief Check if phy link is full duplex.
 *
 * @param x Link capabilities
 *
 * @return True if link is full duplex, false if not.
 */
#define PHY_LINK_IS_FULL_DUPLEX(x)	(x & (BIT(1) | BIT(3) | BIT(5)))

/**
 * @brief Check if phy link speed is 1 Gbit/sec.
 *
 * @param x Link capabilities
 *
 * @return True if link is 1 Gbit/sec, false if not.
 */
#define PHY_LINK_IS_SPEED_1000M(x)	(x & (BIT(4) | BIT(5)))

/**
 * @brief Check if phy link speed is 100 Mbit/sec.
 *
 * @param x Link capabilities
 *
 * @return True if link is 1 Mbit/sec, false if not.
 */
#define PHY_LINK_IS_SPEED_100M(x)	(x & (BIT(2) | BIT(3)))

/** @brief Link state */
struct phy_link_state {
	/** Link speed */
	enum phy_link_speed speed;
	/** When true the link is active and connected */
	bool is_up;
};

/**
 * @typedef phy_callback_t
 * @brief Define the callback function signature for
 * `phy_link_callback_set()` function.
 *
 * @param dev       PHY device structure
 * @param state     Pointer to link_state structure.
 * @param user_data Pointer to data specified by user
 */
typedef void (*phy_callback_t)(const struct device *dev,
			       struct phy_link_state *state,
			       void *user_data);

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
__subsystem struct ethphy_driver_api {
	/** Get link state */
	int (*get_link)(const struct device *dev,
			struct phy_link_state *state);

	/** Configure link */
	int (*cfg_link)(const struct device *dev,
			enum phy_link_speed adv_speeds);

	/** Set callback to be invoked when link state changes. */
	int (*link_cb_set)(const struct device *dev, phy_callback_t cb,
			   void *user_data);

	/** Read PHY register */
	int (*read)(const struct device *dev, uint16_t reg_addr,
		    uint32_t *data);

	/** Write PHY register */
	int (*write)(const struct device *dev, uint16_t reg_addr,
		     uint32_t data);
};
/**
 * @endcond
 */

/**
 * @brief      Configure PHY link
 *
 * This route configures the advertised link speeds.
 *
 * @param[in]  dev     PHY device structure
 * @param      speeds  OR'd link speeds to be advertised by the PHY
 *
 * @retval 0 If successful.
 * @retval -EIO If communication with PHY failed.
 * @retval -ENOTSUP If not supported.
 */
static inline int phy_configure_link(const struct device *dev,
				     enum phy_link_speed speeds)
{
	const struct ethphy_driver_api *api =
		(const struct ethphy_driver_api *)dev->api;

	return api->cfg_link(dev, speeds);
}

/**
 * @brief      Get PHY link state
 *
 * Returns the current state of the PHY link. This can be used by
 * to determine when a link is up and the negotiated link speed.
 *
 *
 * @param[in]  dev    PHY device structure
 * @param      state  Pointer to receive PHY state
 *
 * @retval 0 If successful.
 * @retval -EIO If communication with PHY failed.
 */
static inline int phy_get_link_state(const struct device *dev,
				     struct phy_link_state *state)
{
	const struct ethphy_driver_api *api =
		(const struct ethphy_driver_api *)dev->api;

	return api->get_link(dev, state);
}

/**
 * @brief      Set link state change callback
 *
 * Sets a callback that is invoked when link state changes. This is the
 * preferred method for ethernet drivers to be notified of the PHY link
 * state change.
 *
 * @param[in]  dev        PHY device structure
 * @param      callback   Callback handler
 * @param      user_data  Pointer to data specified by user.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If not supported.
 */
static inline int phy_link_callback_set(const struct device *dev,
					phy_callback_t callback,
					void *user_data)
{
	const struct ethphy_driver_api *api =
		(const struct ethphy_driver_api *)dev->api;

	return api->link_cb_set(dev, callback, user_data);
}

/**
 * @brief      Read PHY registers
 *
 * This routine provides a generic interface to read from a PHY register.
 *
 * @param[in]  dev       PHY device structure
 * @param[in]  reg_addr  Register address
 * @param      value     Pointer to receive read value
 *
 * @retval 0 If successful.
 * @retval -EIO If communication with PHY failed.
 */
static inline int phy_read(const struct device *dev, uint16_t reg_addr,
			   uint32_t *value)
{
	const struct ethphy_driver_api *api =
		(const struct ethphy_driver_api *)dev->api;

	return api->read(dev, reg_addr, value);
}

/**
 * @brief      Write PHY register
 *
 * This routine provides a generic interface to write to a PHY register.
 *
 * @param[in]  dev       PHY device structure
 * @param[in]  reg_addr  Register address
 * @param[in]  value     Value to write
 *
 * @retval 0 If successful.
 * @retval -EIO If communication with PHY failed.
 */
static inline int phy_write(const struct device *dev, uint16_t reg_addr,
			    uint32_t value)
{
	const struct ethphy_driver_api *api =
		(const struct ethphy_driver_api *)dev->api;

	return api->write(dev, reg_addr, value);
}


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PHY_H_ */
