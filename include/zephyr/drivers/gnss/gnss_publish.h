/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GNSS driver backend helpers for publishing data and satellite information.
 * @in_driverbackendgroup{gnss_interface}
 */

#ifndef ZEPHYR_DRIVERS_GNSS_GNSS_H_
#define ZEPHYR_DRIVERS_GNSS_GNSS_H_

#include <zephyr/drivers/gnss.h>

/**
 * @in_driverbackendgroup{gnss_interface}
 * @{
 */

/**
 * @brief Publish GNSS data to registered callbacks.
 *
 * Invokes all callbacks registered with GNSS_DATA_CALLBACK_DEFINE() and
 * GNSS_DT_DATA_CALLBACK_DEFINE() that match @p dev.
 *
 * GNSS drivers should call this function whenever new navigation data is available from the
 * receiver.
 *
 * @param dev GNSS device instance publishing the data.
 * @param data Navigation data, fix information, and UTC time to publish.
 *
 * @see GNSS_DATA_CALLBACK_DEFINE()
 * @see GNSS_DT_DATA_CALLBACK_DEFINE()
 */
void gnss_publish_data(const struct device *dev, const struct gnss_data *data);

/**
 * @brief Publish GNSS satellite information to registered callbacks.
 *
 * Invokes all callbacks registered with GNSS_SATELLITES_CALLBACK_DEFINE() and
 * GNSS_DT_SATELLITES_CALLBACK_DEFINE() that match @p dev.
 *
 * GNSS drivers should call this function whenever updated satellite tracking information is
 * available from the receiver.
 *
 * @kconfig_dep{CONFIG_GNSS_SATELLITES}
 *
 * @param dev GNSS device instance publishing the satellite data.
 * @param satellites Array of satellite tracking data.
 * @param size Number of elements in @p satellites.
 *
 * @see GNSS_SATELLITES_CALLBACK_DEFINE()
 * @see GNSS_DT_SATELLITES_CALLBACK_DEFINE()
 */
void gnss_publish_satellites(const struct device *dev, const struct gnss_satellite *satellites,
			     uint16_t size);

/** @} */

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_H_ */
