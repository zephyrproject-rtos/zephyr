/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "input_data.h"

static const int16_t normal_window[TIME_SERIES_WINDOW_SIZE] = {
0, 195, 383, 556, 707, 831, 924, 981,
1000, 981, 924, 831, 707, 556, 383, 195,
0, -195, -383, -556, -707, -831, -924, -981,
-1000, -981, -924, -831, -707, -556, -383, -195,
0, 195, 383, 556, 707, 831, 924, 981,
1000, 981, 924, 831, 707, 556, 383, 195,
0, -195, -383, -556, -707, -831, -924, -981,
-1000, -981, -924, -831, -707, -556, -383, -195,
};

static const int16_t low_frequency_window[TIME_SERIES_WINDOW_SIZE] = {
0, 98, 195, 290, 383, 471, 556, 634,
707, 773, 831, 882, 924, 957, 981, 995,
1000, 995, 981, 957, 924, 882, 831, 773,
707, 634, 556, 471, 383, 290, 195, 98,
0, -98, -195, -290, -383, -471, -556, -634,
-707, -773, -831, -882, -924, -957, -981, -995,
-1000, -995, -981, -957, -924, -882, -831, -773,
-707, -634, -556, -471, -383, -290, -195, -98,
};

static const int16_t transient_window[TIME_SERIES_WINDOW_SIZE] = {
0, 98, 195, 290, 383, 471, 556, 634,
707, 773, 831, 882, 924, 957, 981, 995,
3200, 995, 981, 957, 924, 882, 831, 773,
707, 634, 556, 471, 383, 290, 195, 98,
0, -98, -195, -290, -383, -471, -556, -634,
-707, -773, -831, -882, -924, -957, -981, -995,
-2800, -995, -981, -957, -924, -882, -831, -773,
-707, -634, -556, -471, -383, -290, -195, -98,
};

static const struct time_series_window windows[TIME_SERIES_NUM_WINDOWS] = {
{
.name = "normal",
.label = TIME_SERIES_NORMAL,
.samples = normal_window,
.len = TIME_SERIES_WINDOW_SIZE,
},
{
.name = "low_frequency",
.label = TIME_SERIES_LOW_FREQUENCY,
.samples = low_frequency_window,
.len = TIME_SERIES_WINDOW_SIZE,
},
{
.name = "transient",
.label = TIME_SERIES_TRANSIENT,
.samples = transient_window,
.len = TIME_SERIES_WINDOW_SIZE,
},
};

const struct time_series_window *time_series_get_window(size_t index)
{
return &windows[index % TIME_SERIES_NUM_WINDOWS];
}

const char *time_series_label_name(enum time_series_label label)
{
switch (label) {
case TIME_SERIES_NORMAL:
return "normal";
case TIME_SERIES_LOW_FREQUENCY:
return "low_frequency";
case TIME_SERIES_TRANSIENT:
return "transient";
default:
return "unknown";
}
}
