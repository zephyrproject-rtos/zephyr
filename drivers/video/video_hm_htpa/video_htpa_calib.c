/*
 * Copyright (c) 2026 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#include "video_htpa_calib.h"

void htpa_apply_calib(const struct htpa_calib *calib, struct video_buffer *frame)
{
	ARG_UNUSED(calib);
	ARG_UNUSED(frame);
}
