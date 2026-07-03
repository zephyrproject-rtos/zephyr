/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stream_features.h"
#include "input_stream.h"

#include <stddef.h>
#include <stdint.h>

static const int16_t normal_stream[STREAM_SAMPLES_PER_CLASS] = {
	0, 195, 383, 556, 707, 831, 924, 981,
	1000, 981, 924, 831, 707, 556, 383, 195,
	0, -195, -383, -556, -707, -831, -924, -981,
	-1000, -981, -924, -831, -707, -556, -383, -195,
	0, 195, 383, 556, 707, 831, 924, 981,
	1000, 981, 924, 831, 707, 556, 383, 195,
	0, -195, -383, -556, -707, -831, -924, -981,
	-1000, -981, -924, -831, -707, -556, -383, -195,
	0, 195, 383, 556, 707, 831, 924, 981,
	1000, 981, 924, 831, 707, 556, 383, 195,
	0, -195, -383, -556, -707, -831, -924, -981,
	-1000, -981, -924, -831, -707, -556, -383, -195
};

static const int16_t drift_stream[STREAM_SAMPLES_PER_CLASS] = {
	0, 76, 152, 227, 299, 370, 437, 501,
	561, 616, 667, 712, 752, 786, 814, 836,
	852, 862, 865, 862, 853, 839, 818, 793,
	763, 728, 690, 647, 602, 555, 506, 455,
	404, 353, 303, 253, 206, 161, 119, 80,
	46, 15, -10, -30, -45, -54, -56, -53,
	-44, -28, -6, 22, 56, 96, 142, 192,
	248, 308, 372, 439, 509, 582, 656, 732,
	808, 885, 960, 1035, 1108, 1178, 1245, 1309,
	1369, 1425, 1475, 1521, 1561, 1595, 1623, 1645,
	1661, 1670, 1673, 1670, 1662, 1647, 1627, 1601,
	1571, 1537, 1498, 1456, 1411, 1363, 1314, 1264
};

static const int16_t transient_stream[STREAM_SAMPLES_PER_CLASS] = {
	0, 156, 306, 444, 566, 665, 739, 785,
	800, 785, 739, 665, 566, 444, 306, 156,
	0, -156, -306, -444, 1934, -665, -739, -785,
	-800, -785, -739, -665, -566, -444, -306, -156,
	0, 156, 306, 444, 566, 665, 739, 785,
	800, 785, 739, 665, 566, 444, 306, 156,
	0, -156, -306, -444, -566, -665, -739, -785,
	-800, -785, -739, -665, -566, -444, -306, -156,
	0, 156, 306, 444, 566, 665, 739, 785,
	-1500, 785, 739, 665, 566, 444, 306, 156,
	0, -156, -306, -444, -566, -665, -739, -785,
	-800, -785, -739, -665, -566, -444, -306, -156
};

static const int16_t *const streams[STREAM_NUM_CLASSES] = {
	[0] = normal_stream,
	[1] = drift_stream,
	[2] = transient_stream
};

const int16_t *input_stream_get_samples(enum stream_label label)
{
return streams[(size_t)label];
}

enum stream_label input_stream_expected_label(size_t stream_window)
{
return (enum stream_label)(stream_window / STREAM_WINDOWS_PER_CLASS);
}

const char *input_stream_label_name(enum stream_label label)
{
switch (label) {
case STREAM_LABEL_NORMAL:
return "normal";
case STREAM_LABEL_DRIFT:
return "drift";
case STREAM_LABEL_TRANSIENT:
return "transient";
default:
return "unknown";
}
}

size_t input_stream_window_start(size_t stream_window)
{
return (stream_window % STREAM_WINDOWS_PER_CLASS) * STREAM_HOP_SIZE;
}
