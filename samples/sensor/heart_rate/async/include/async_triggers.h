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

#define TRIGGER_OPT(APPLY, _trig) \
    IF_ENABLED(CONCAT(CONFIG_SAMPLE_TRIG_, _trig, _NOP), (APPLY(_trig, NOP))) \
    IF_ENABLED(CONCAT(CONFIG_SAMPLE_TRIG_, _trig, _INCLUDE), (APPLY(_trig, INCLUDE))) \
    IF_ENABLED(CONCAT(CONFIG_SAMPLE_TRIG_, _trig, _DROP), (APPLY(_trig, DROP)))

#define TRIGGER_GEN(_trig, _opt, ...) \
    TRIG_X(_trig, _opt, __VA_ARGS__)

#define TRIGGER_CHAN_GEN(_trig, _opt, _chan, _name, _idx) \
    IF_ENABLED(CONCAT(CONFIG_SAMPLE_TRIG_, _trig, _, _opt, _CHAN_, _chan), ( \
        TRIGGER_GEN(_trig, _opt, SENSOR_CHAN_##_chan, _idx) \
    ))

#define TRIGGER_OPT_GEN(_trig, _opt) \
    COND_CODE_1(CONCAT(CONFIG_SAMPLE_TRIGGER_, _trig, _, _opt, _CHAN), ( \
        SENSOR_AVAILABLE_CHANNEL(TRIGGER_CHAN_GEN, _trig, _opt) \
    ), ( \
        TRIGGER_GEN(_trig, _opt) \
    ))

/*
 * Trigger types definition
 * X(trig, name, opt, chan)
 */
#define TRIGGER_GENERATOR(_trig) \
    IF_ENABLED(CONCAT(CONFIG_SAMPLE_TRIGGER_, _trig), ( \
        TRIGGER_OPT(TRIGGER_OPT_GEN, _trig) \
    ))

#define TRIGGER_LIST \
	TRIGGER_GENERATOR(DATA_READY) \
    TRIGGER_GENERATOR(FIFO_WATERMARK) \
    TRIGGER_GENERATOR(OVERFLOW)
