/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GNSS_GNSS_H_
#define ZEPHYR_DRIVERS_GNSS_GNSS_H_

#include <zephyr/drivers/gnss.h>

/** Internal function used by GNSS drivers to publish GNSS data */
void gnss_publish_data(const struct device *dev, const struct gnss_data *data);

/** Internal function used by GNSS drivers to publish GNSS satellites */
void gnss_publish_satellites(const struct device *dev, const struct gnss_satellite *satellites,
			     uint16_t size);

/** Internal function used by GNSS drivers to publish per-axis position accuracy */
void gnss_publish_accuracy(const struct device *dev, const struct gnss_accuracy *accuracy);

/**
 * @brief Internal function used by GNSS drivers to publish a raw NMEA sentence
 *
 * @param dev Device instance
 * @param sentence Sentence bytes — delimiter stripped, byte-faithful to the wire
 * @param len Length of @p sentence in bytes
 */
void gnss_publish_raw_nmea(const struct device *dev, const char *sentence, size_t len);

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_H_ */
