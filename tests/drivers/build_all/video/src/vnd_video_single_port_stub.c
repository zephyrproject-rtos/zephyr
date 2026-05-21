/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

DT_FOREACH_STATUS_OKAY_VARGS(vnd_video_single_port,
			     DEVICE_DT_DEFINE,
			     NULL, NULL, NULL, NULL,
			     POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,
			     NULL);
