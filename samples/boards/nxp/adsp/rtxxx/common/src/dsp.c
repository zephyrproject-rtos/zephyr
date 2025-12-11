/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dsp.h"

#include <string.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/misc/nxp_rtxxx_dsp_ctrl/nxp_rtxxx_dsp_ctrl.h>

static const struct device *dsp = DEVICE_DT_GET(DT_NODELABEL(dsp));

/* DSP binary image symbols - exported by dspimgs.S */
extern uint32_t dsp_img_reset_start[];
extern uint32_t dsp_img_reset_size;

extern uint32_t dsp_img_text_start[];
extern uint32_t dsp_img_text_size;

extern uint32_t dsp_img_data_start[];
extern uint32_t dsp_img_data_size;

void dsp_start(void)
{
	if (!device_is_ready(dsp)) {
		return;
	}

	nxp_rtxxx_dsp_ctrl_load_section(dsp,
		dsp_img_reset_start, dsp_img_reset_size, NXP_RTXXX_DSP_CTRL_SECTION_RESET);
	nxp_rtxxx_dsp_ctrl_load_section(dsp,
		dsp_img_text_start, dsp_img_text_size, NXP_RTXXX_DSP_CTRL_SECTION_TEXT);
	nxp_rtxxx_dsp_ctrl_load_section(dsp,
		dsp_img_data_start, dsp_img_data_size, NXP_RTXXX_DSP_CTRL_SECTION_DATA);

	nxp_rtxxx_dsp_ctrl_enable(dsp);
}
