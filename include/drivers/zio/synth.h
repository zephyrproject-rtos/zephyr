/*
 * Copyright (c) 2019  Thomas Burdick <thomas.burdick@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ZIO_SYNTH_H_
#define ZEPHYR_INCLUDE_DRIVERS_ZIO_SYNTH_H_

#include <zio.h>


#define SYNTH_LEFT_CHANNEL 0
#define SYNTH_RIGHT_CHANNEL 1

/* channel attribute type(s) */
#define SYNTH_FREQUENCY_ATTR 0
#define SYNTH_PHASE_ATTR 1
#define ZIO_SAMPLE_DEVICE_ATTR 2

/* datum type */
struct synth_datum {
	s16_t samples[2];
};

#endif
