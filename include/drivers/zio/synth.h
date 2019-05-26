/*
 * Copyright (c) 2019  Thomas Burdick <thomas.burdick@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ZIO_SYNTH_H_
#define ZEPHYR_INCLUDE_DRIVERS_ZIO_SYNTH_H_

#include <zio.h>


#define SYNTH_LEFT_CHANNEL (ZIO_CHAN_TYPES + 1)
#define SYNTH_RIGHT_CHANNEL (ZIO_CHAN_TYPES + 2)

/* channel attribute type(s) */
#define SYNTH_FREQUENCY_ATTR (ZIO_ATTR_TYPES + 1)
#define SYNTH_PHASE_ATTR (ZIO_ATTR_TYPES + 2)

#define ZIO_SAMPLE_DEVICE_ATTR (ZIO_ATTR_TYPES + 3)

/* datum type */
struct synth_datum {
	s16_t samples[2];
};

#endif
