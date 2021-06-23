/*
 * Copyright (c) 2021 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the Texas Instruments BQ274XX
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BQ274XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BQ274XX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/sensor.h>

/** Discharge detected */
#define BQ274XX_STATUS_DSG                   BIT(0)
/** Battery insertion detected */
#define BQ274XX_STATUS_BAT_DET               BIT(3)
/** Fast charging allowed */
#define BQ274XX_STATUS_CHG                   BIT(8)
/** Full-charge detected */
#define BQ274XX_STATUS_FC                    BIT(9)
/** Under-Temperature condition detected */
#define BQ274XX_STATUS_UT                    BIT(14)
/** Over-Temperature condition detected */
#define BQ274XX_STATUS_OT                    BIT(15)

enum sensor_channel_bq274xx {
	/** Gauge status channel, in bitflags **/
	SENSOR_CHAN_BQ274XX_STATUS = SENSOR_CHAN_PRIV_START,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_BQ274XX_ */
