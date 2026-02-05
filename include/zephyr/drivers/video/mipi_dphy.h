/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIDEO_MIPI_DPHY_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIDEO_MIPI_DPHY_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MIPI D-PHY configuration options
 */
struct phy_configure_opts_mipi_dphy {
	/** High-speed clock rate in Hz */
	uint64_t hs_clk_rate;
	/** Number of data lanes */
	uint8_t lanes;
};

/**
 * @brief PHY driver API structure
 */
struct phy_driver_api {
	/**
	 * @brief Configure PHY parameters
	 */
	int (*configure)(const struct device *dev, void *opts);

	/**
	 * @brief Initialize PHY
	 */
	int (*init)(const struct device *dev);

	/**
	 * @brief Power on and start PHY
	 */
	int (*power_on)(const struct device *dev);

	/**
	 * @brief Power off PHY
	 */
	int (*power_off)(const struct device *dev);

	/**
	 * @brief Exit PHY
	 */
	int (*exit)(const struct device *dev);
};

/**
 * @brief Configure PHY parameters
 *
 * @param dev PHY device pointer
 * @param opts Configuration options pointer (struct phy_configure_opts_mipi_dphy *)
 * @return 0 on success, negative error code otherwise
 */
static inline int phy_configure(const struct device *dev, void *opts)
{
	const struct phy_driver_api *api =
		(const struct phy_driver_api *)dev->api;

	if (!api || !api->configure) {
		return -ENOTSUP;
	}

	return api->configure(dev, opts);
}

/**
 * @brief Initialize PHY
 *
 * Enable clocks and reset hardware to prepare PHY for operation
 *
 * @param dev PHY device pointer
 * @return 0 on success, negative error code otherwise
 */
static inline int phy_init(const struct device *dev)
{
	const struct phy_driver_api *api =
		(const struct phy_driver_api *)dev->api;

	if (!api || !api->init) {
		return -ENOTSUP;
	}

	return api->init(dev);
}

/**
 * @brief Power on and start PHY
 *
 * Must be called after phy_init() and phy_configure()
 *
 * @param dev PHY device pointer
 * @return 0 on success, negative error code otherwise
 */
static inline int phy_power_on(const struct device *dev)
{
	const struct phy_driver_api *api =
		(const struct phy_driver_api *)dev->api;

	if (!api || !api->power_on) {
		return -ENOTSUP;
	}

	return api->power_on(dev);
}

/**
 * @brief Power off PHY
 *
 * @param dev PHY device pointer
 * @return 0 on success, negative error code otherwise
 */
static inline int phy_power_off(const struct device *dev)
{
	const struct phy_driver_api *api =
		(const struct phy_driver_api *)dev->api;

	if (!api || !api->power_off) {
		return -ENOTSUP;
	}

	return api->power_off(dev);
}

/**
 * @brief Exit PHY
 *
 * Disable clocks and cleanup resources
 *
 * @param dev PHY device pointer
 * @return 0 on success, negative error code otherwise
 */
static inline int phy_exit(const struct device *dev)
{
	const struct phy_driver_api *api =
		(const struct phy_driver_api *)dev->api;

	if (!api || !api->exit) {
		return -ENOTSUP;
	}

	return api->exit(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIDEO_MIPI_DPHY_H_ */
