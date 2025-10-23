/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "qe_touch_config.h"

volatile uint8_t g_qe_touch_flag;
volatile ctsu_event_t g_qe_ctsu_event;

void qe_touch_callback(touch_callback_args_t *p_args)
{
	g_qe_touch_flag = 1;
	g_qe_ctsu_event = p_args->event;
}

/* CTSU Related Information for [CONFIG01] configuration. */

const ctsu_element_cfg_t g_qe_ctsu_element_cfg_config01[] = {
	{.ssdiv = CTSU_SSDIV_2000, .so = 0x045, .snum = 0x03, .sdpa = 0x07},
	{.ssdiv = CTSU_SSDIV_2000, .so = 0x055, .snum = 0x03, .sdpa = 0x07},
	{.ssdiv = CTSU_SSDIV_2000, .so = 0x04D, .snum = 0x03, .sdpa = 0x07},
	{.ssdiv = CTSU_SSDIV_2000, .so = 0x042, .snum = 0x03, .sdpa = 0x07},
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x0D0, .snum = 0x07, .sdpa = 0x03},
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x0C9, .snum = 0x07, .sdpa = 0x03},
};

const ctsu_cfg_t g_qe_ctsu_cfg_config01 = {
	.cap = CTSU_CAP_SOFTWARE,

	.txvsel = CTSU_TXVSEL_VCC,

	.atune1 = CTSU_ATUNE1_NORMAL,

	.md = CTSU_MODE_SELF_MULTI_SCAN,

	.ctsuchac0 = 0x80,  /* ch0-ch7 enable mask */
	.ctsuchac1 = 0x1F,  /* ch8-ch15 enable mask */
	.ctsuchac2 = 0x00,  /* ch16-ch23 enable mask */
	.ctsuchac3 = 0x00,  /* ch24-ch31 enable mask */
	.ctsuchac4 = 0x00,  /* ch32-ch39 enable mask */
	.ctsuchtrc0 = 0x00, /* ch0-ch7 mutual tx mask */
	.ctsuchtrc1 = 0x00, /* ch8-ch15 mutual tx mask */
	.ctsuchtrc2 = 0x00, /* ch16-ch23 mutual tx mask */
	.ctsuchtrc3 = 0x00, /* ch24-ch31 mutual tx mask */
	.ctsuchtrc4 = 0x00, /* ch32-ch39 mutual tx mask */
	.num_rx = 6,
	.num_tx = 0,
	.p_elements = g_qe_ctsu_element_cfg_config01,

#if (CTSU_TARGET_VALUE_CONFIG_SUPPORT == 1)
	.tuning_self_target_value = 15360,
	.tuning_mutual_target_value = 10240,
#endif

	.num_moving_average = 4,
	.tuning_enable = true,
	.p_callback = &qe_touch_callback,

};

ctsu_instance_ctrl_t g_qe_ctsu_ctrl_config01;

const ctsu_instance_t g_qe_ctsu_instance_config01 = {
	.p_ctrl = &g_qe_ctsu_ctrl_config01,
	.p_cfg = &g_qe_ctsu_cfg_config01,
	.p_api = &g_ctsu_on_ctsu,
};

/* Touch Related Information for [CONFIG01] configuration. */

#define QE_TOUCH_CONFIG01_NUM_BUTTONS (2)
#define QE_TOUCH_CONFIG01_NUM_SLIDERS (1)
#define QE_TOUCH_CONFIG01_NUM_WHEELS  (0)

/* Button configurations */
#if (QE_TOUCH_CONFIG01_NUM_BUTTONS != 0)
const touch_button_cfg_t g_qe_touch_button_cfg_config01[] = {

	/* button1 */
	{
		.elem_index = 4,
		.threshold = 2056,
		.hysteresis = 102,
	},
	/* button02 */
	{
		.elem_index = 5,
		.threshold = 2865,
		.hysteresis = 143,
	},
};
#endif

/* Slider configurations */
const uint8_t g_qe_touch_elem_slider_config01_slider01[] = {3, 2, 1, 0};

#if (QE_TOUCH_CONFIG01_NUM_SLIDERS != 0)
const touch_slider_cfg_t g_qe_touch_slider_cfg_config01[] = {
	/* slider01 */
	{
		.p_elem_index = g_qe_touch_elem_slider_config01_slider01,
		.num_elements = 4,
		.threshold = 1157,
	},
};
#endif

/* Wheel configurations */
#if (QE_TOUCH_CONFIG01_NUM_WHEELS != 0)
const touch_wheel_cfg_t g_qe_touch_wheel_cfg_config01[] = {NULL};
#endif

/* Touch configurations */
const touch_cfg_t g_qe_touch_cfg_config01 = {
	.p_buttons = g_qe_touch_button_cfg_config01,
	.p_sliders = g_qe_touch_slider_cfg_config01,
	.p_wheels = NULL,
	.num_buttons = QE_TOUCH_CONFIG01_NUM_BUTTONS,
	.num_sliders = QE_TOUCH_CONFIG01_NUM_SLIDERS,
	.num_wheels = QE_TOUCH_CONFIG01_NUM_WHEELS,

	.number = 0,

	.on_freq = 3,
	.off_freq = 3,
	.drift_freq = 255,
	.cancel_freq = 0,

	.p_ctsu_instance = &g_qe_ctsu_instance_config01,
};

touch_instance_ctrl_t g_qe_touch_ctrl_config01;

const touch_instance_t g_qe_touch_instance_config01 = {
	.p_ctrl = &g_qe_touch_ctrl_config01,
	.p_cfg = &g_qe_touch_cfg_config01,
	.p_api = &g_touch_on_ctsu,
};
