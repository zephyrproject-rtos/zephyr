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

#define STREAM_WINDOWS_PER_CLASS 2U
#define STREAM_NUM_CLASSES 3U
#define STREAM_TOTAL_WINDOWS (STREAM_WINDOWS_PER_CLASS * STREAM_NUM_CLASSES)
#define STREAM_SAMPLES_PER_CLASS (STREAM_WINDOW_SIZE + STREAM_HOP_SIZE)

enum stream_label {
STREAM_LABEL_NORMAL = 0,
STREAM_LABEL_DRIFT = 1,
STREAM_LABEL_TRANSIENT = 2,
};

const int16_t *input_stream_get_samples(enum stream_label label);
enum stream_label input_stream_expected_label(size_t stream_window);
const char *input_stream_label_name(enum stream_label label);
size_t input_stream_window_start(size_t stream_window);

#ifdef __cplusplus
}
#endif
