/*
 * Copyright (c) 2026 NotioNext LTD.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HI_AUDIO_DATA_H
#define HI_AUDIO_DATA_H

#include <stdint.h>

#define HI_AUDIO_DATA_SIZE 926728
#define HI_AUDIO_SAMPLE_RATE 44100
#define HI_AUDIO_CHANNELS 2
#define HI_AUDIO_BITS_PER_SAMPLE 16

extern const unsigned char hi_audio_data[];
extern unsigned int hi_audio_data_len;

#endif /* HI_AUDIO_DATA_H */
