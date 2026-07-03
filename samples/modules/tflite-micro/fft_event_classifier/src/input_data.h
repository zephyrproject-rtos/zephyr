/*
 * Copyright (c) 2026 Mahad Faisal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum fft_event_label {
FFT_EVENT_NORMAL = 0,
FFT_EVENT_HIGH_FREQUENCY = 1,
FFT_EVENT_TRANSIENT = 2,
};

struct fft_event_window {
const char *name;
enum fft_event_label label;
const int16_t *samples;
size_t len;
};

const struct fft_event_window *fft_event_get_window(size_t index);
const char *fft_event_label_name(enum fft_event_label label);

#ifdef __cplusplus
}
#endif
