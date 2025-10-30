/*
 * Copyright (c) 2025, CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <zephyr/sys/util_macro.h>

#include "async_channels.h"

/* Primitive paste (no expansion) */
#define PP_CAT(a, b) a##b
/* Expand-then-paste (two-stage) */
#define CAT2(a, b) PP_CAT(a, b)
/* Expands to SENSOR_STREAM_DATA_INCLUDE / _DROP / _NOP */
#define OPT_FROM_TOKEN(_opt_token) CAT2(SENSOR_STREAM_DATA_, _opt_token)

#define TRIGGER_GEN(_trig, _opt, ...) \
    TRIG_X(_trig, _opt, __VA_ARGS__)

#define TRIGGER_CHAN_GEN(_trig, _opt, _chan, _idx) \
    IF_ENABLED(CONCAT(CONFIG_SAMPLE_TRIG_, _trig, _, _opt, _CHAN_, _chan), ( \
        TRIGGER_GEN(_trig, _opt, SENSOR_CHAN_##_chan, _idx) \
    ))

#define TRIGGER_OPT_GEN(_trig, _opt) \
    COND_CODE_1(CONCAT(CONFIG_SAMPLE_TRIGGER_, _trig, _, _opt, _CHAN), ( \
        SENSOR_AVAILABLE_CHANNEL(TRIGGER_CHAN_GEN, _trig, _opt) \
    ), ( \
        TRIGGER_GEN(_trig, _opt) \
    ))

#define TRIGGER_OPT(_trig) \
    IF_ENABLED(CONCAT(CONFIG_SAMPLE_TRIG_, _trig, _NOP), (TRIGGER_OPT_GEN(_trig, NOP))) \
    IF_ENABLED(CONCAT(CONFIG_SAMPLE_TRIG_, _trig, _INCLUDE), (TRIGGER_OPT_GEN(_trig, INCLUDE))) \
    IF_ENABLED(CONCAT(CONFIG_SAMPLE_TRIG_, _trig, _DROP), (TRIGGER_OPT_GEN(_trig, DROP)))

#define TRIGGER_LIST(APPLY) \
    IF_ENABLED(CONFIG_SAMPLE_TRIGGER_DATA_READY, (APPLY(DATA_READY))) \
    IF_ENABLED(CONFIG_SAMPLE_TRIGGER_FIFO_WATERMARK, (APPLY(FIFO_WATERMARK))) \
    IF_ENABLED(CONFIG_SAMPLE_TRIGGER_OVERFLOW, (APPLY(OVERFLOW)))

#define TRIG_X(trig) (enum sensor_trigger_type)(SENSOR_TRIG_##trig),
enum sensor_trigger_type trigger_list[] = {TRIGGER_LIST(TRIG_X)};
#undef TRIG_X

#define TRIG_X(trig) #trig,
const char *trigger_names[] = {TRIGGER_LIST(TRIG_X)};
#undef TRIG_X

#define TRIG_X(trig) sizeof(#trig),
const size_t trigger_widths[] = {TRIGGER_LIST(TRIG_X)};
#undef TRIG_X
