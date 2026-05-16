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

#define CNN_FAULT_WINDOW_SIZE 64U
#define CNN_FAULT_NUM_CLASSES 3U

enum cnn_fault_label {
CNN_FAULT_NORMAL = 0,
CNN_FAULT_HIGH_FREQUENCY = 1,
CNN_FAULT_IMPULSE = 2,
};

struct cnn_fault_window {
const char *name;
enum cnn_fault_label label;
const int16_t *samples;
size_t len;
};

const struct cnn_fault_window *cnn_fault_get_window(size_t index);
const char *cnn_fault_label_name(enum cnn_fault_label label);

#ifdef __cplusplus
}
#endif
