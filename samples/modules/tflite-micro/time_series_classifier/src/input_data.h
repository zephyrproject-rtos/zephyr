/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_SERIES_WINDOW_SIZE 64U
#define TIME_SERIES_NUM_WINDOWS 3U

enum time_series_label {
TIME_SERIES_NORMAL = 0,
TIME_SERIES_LOW_FREQUENCY = 1,
TIME_SERIES_TRANSIENT = 2,
};

struct time_series_window {
const char *name;
enum time_series_label label;
const int16_t *samples;
size_t len;
};

const struct time_series_window *time_series_get_window(size_t index);
const char *time_series_label_name(enum time_series_label label);

#ifdef __cplusplus
}
#endif
