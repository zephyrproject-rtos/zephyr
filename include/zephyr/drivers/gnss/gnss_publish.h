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

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_H_ */
