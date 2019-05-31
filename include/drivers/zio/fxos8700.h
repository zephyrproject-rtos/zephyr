/*
 * Copyright (c) 2019  Thomas Burdick <thomas.burdick@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ZIO_FXOS8700_H_
#define ZEPHYR_INCLUDE_DRIVERS_ZIO_FXOS8700_H_

#include <zio.h>

/* channel type(s) */
#define FXOS8700_ACCEL_XYZ (ZIO_CHAN_TYPES + 1)
#define FXOS8700_MAGN_XYZ (FXOS8700_ACCEL_XYZ + 1)
#define FXOS8700_TEMP (FXOS8700_MAGN_XYZ + 1)

/**
 * FXOS8700 Datum Type
 *
 * This changes depending on the device configuration
 */
struct fxos8700_datum {
#if defined(CONFIG_ZIO_FXOS8700_MODE_HYBRID) || defined(CONFIG_ZIO_FXOS8700_MODE_ACCEL)
	s16_t acc_x;
	s16_t acc_y;
	s16_t acc_z;
#endif
#if defined(CONFIG_ZIO_FXOS8700_MODE_HYBRID) || defined(CONFIG_ZIO_FXOS8700_MODE_MAGN)
	s16_t magn_x;
	s16_t magn_y;
	s16_t magn_z;
#endif
#if defined(CONFIG_ZIO_FXOS8700_TEMP)
	s8_t temp;
#endif
};

#endif
