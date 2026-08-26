/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dsp.h"

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/cache.h>

#include <zephyr/drivers/misc/nxp_rtxxx_dsp_ctrl/nxp_rtxxx_dsp_ctrl.h>

#include <zephyr_image_info.h>

static const struct device *dsp = DEVICE_DT_GET(DT_NODELABEL(dsp));
static const struct nxp_rtxxx_dsp_ctrl_segment dsp_segments[] =
	NXP_RTXXX_DSP_CTRL_SEGMENTS_FROM_IMAGE_INFO();

int dsp_start(void)
{
	int ret;

	if (!device_is_ready(dsp)) {
		return -ENODEV;
	}

	ret = nxp_rtxxx_dsp_ctrl_load(dsp, dsp_segments, ARRAY_SIZE(dsp_segments));
	if (ret < 0) {
		return ret;
	}

	/*
	 * Some i.MX RT devices have a D-cache local to the ARM core from which
	 * the DSP core is launched. This needs to be flushed to ensure coherency,
	 * so that the DSP sees all the memory sections that have been copied over.
	 */
	ret = sys_cache_data_flush_all();
	if (ret < 0 && ret != -ENOTSUP) {
		return ret;
	}

	nxp_rtxxx_dsp_ctrl_enable(dsp);
	return 0;
}
