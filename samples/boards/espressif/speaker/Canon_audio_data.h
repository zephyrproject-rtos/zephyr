/*
 * Copyright (c) 2026 NotioNext LTD.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AUDIO_DATA_H
#define AUDIO_DATA_H

#include <stdint.h>

#define AUDIO_DATA_SIZE 1920000
#define AUDIO_SAMPLE_RATE 8000
#define AUDIO_CHANNELS 2
#define AUDIO_BITS_PER_SAMPLE 16

extern const int16_t audio_data[];

#endif /* AUDIO_DATA_H */
