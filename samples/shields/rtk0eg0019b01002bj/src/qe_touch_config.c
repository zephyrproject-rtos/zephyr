/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Generated content from Renesas QE Capacitive Touch */

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
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x000, .snum = 0x07, .sdpa = 0x1F},
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x00F, .snum = 0x07, .sdpa = 0x1F},
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x007, .snum = 0x07, .sdpa = 0x1F},
};

const ctsu_cfg_t g_qe_ctsu_cfg_config01 = {
	.cap = CTSU_CAP_SOFTWARE,

	.txvsel = CTSU_TXVSEL_INTERNAL_POWER,
	.txvsel2 = CTSU_TXVSEL_MODE,

	.atune12 = CTSU_ATUNE12_40UA,
	.md = CTSU_MODE_SELF_MULTI_SCAN,
	.posel = CTSU_POSEL_SAME_PULSE,

	.ctsuchac0 = 0x01,  /* ch0-ch7 enable mask */
	.ctsuchac1 = 0x0E,  /* ch8-ch15 enable mask */
	.ctsuchac2 = 0x00,  /* ch16-ch23 enable mask */
	.ctsuchac3 = 0x00,  /* ch24-ch31 enable mask */
	.ctsuchac4 = 0x00,  /* ch32-ch39 enable mask */
	.ctsuchtrc0 = 0x01, /* ch0-ch7 mutual tx mask */
	.ctsuchtrc1 = 0x00, /* ch8-ch15 mutual tx mask */
	.ctsuchtrc2 = 0x00, /* ch16-ch23 mutual tx mask */
	.ctsuchtrc3 = 0x00, /* ch24-ch31 mutual tx mask */
	.ctsuchtrc4 = 0x00, /* ch32-ch39 mutual tx mask */
	.num_rx = 3,
	.num_tx = 0,
	.p_elements = g_qe_ctsu_element_cfg_config01,

#if (CTSU_TARGET_VALUE_CONFIG_SUPPORT == 1)
	.tuning_self_target_value = 4608,
	.tuning_mutual_target_value = 10240,
#endif

	.num_moving_average = 4,
	.p_callback = &qe_touch_callback,
#if (CTSU_CFG_DTC_SUPPORT_ENABLE == 1)
	.p_transfer_tx = &g_transfer0,
	.p_transfer_rx = &g_transfer1,
#else
	.p_transfer_tx = NULL,
	.p_transfer_rx = NULL,
#endif

	.write_irq = CTSU_WRITE_IRQn,
	.read_irq = CTSU_READ_IRQn,
	.end_irq = CTSU_END_IRQn,

};

ctsu_instance_ctrl_t g_qe_ctsu_ctrl_config01;

const ctsu_instance_t g_qe_ctsu_instance_config01 = {
	.p_ctrl = &g_qe_ctsu_ctrl_config01,
	.p_cfg = &g_qe_ctsu_cfg_config01,
	.p_api = &g_ctsu_on_ctsu,
};

/* Touch Related Information for [CONFIG01] configuration. */

#define QE_TOUCH_CONFIG01_NUM_BUTTONS    (3)
#define QE_TOUCH_CONFIG01_NUM_SLIDERS    (0)
#define QE_TOUCH_CONFIG01_NUM_WHEELS     (0)
#define QE_TOUCH_CONFIG01_NUM_TOUCH_PADS (0)

#if (QE_TOUCH_CONFIG01_NUM_BUTTONS != 0)
const touch_button_cfg_t g_qe_touch_button_cfg_config01[] = {
	{
		.elem_index = 2,
		.threshold = 261,
		.hysteresis = 13,
	},
	{
		.elem_index = 1,
		.threshold = 226,
		.hysteresis = 11,
	},
	{
		.elem_index = 0,
		.threshold = 306,
		.hysteresis = 15,
	},
};
#endif

#if (QE_TOUCH_CONFIG01_NUM_SLIDERS != 0)
const touch_slider_cfg_t g_qe_touch_slider_cfg_config01[] = {NULL};
#endif

#if (QE_TOUCH_CONFIG01_NUM_WHEELS != 0)
const touch_wheel_cfg_t g_qe_touch_wheel_cfg_config01[] = {NULL};
#endif

#if (QE_TOUCH_CONFIG01_NUM_TOUCH_PADS != 0)
const touch_pad_cfg_t g_qe_touch_touch_pad_cfg_config01 = {NULL};
#endif

const touch_cfg_t g_qe_touch_cfg_config01 = {
	.p_buttons = g_qe_touch_button_cfg_config01,
	.p_sliders = NULL,
	.p_wheels = NULL,
#if (TOUCH_CFG_PAD_ENABLE != 0)
	.p_pad = NULL,
#endif
	.num_buttons = QE_TOUCH_CONFIG01_NUM_BUTTONS,
	.num_sliders = QE_TOUCH_CONFIG01_NUM_SLIDERS,
	.num_wheels = QE_TOUCH_CONFIG01_NUM_WHEELS,

	.number = 0,
#if ((TOUCH_CFG_UART_MONITOR_SUPPORT == 1) || (TOUCH_CFG_UART_TUNING_SUPPORT == 1))
	.p_uart_instance = &g_uart_qe,
#else
	.p_uart_instance = NULL,
#endif

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

/* CTSU Related Information for [CONFIG02] configuration. */

const ctsu_element_cfg_t g_qe_ctsu_element_cfg_config02[] = {
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x021, .snum = 0x07, .sdpa = 0x0F},
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x032, .snum = 0x07, .sdpa = 0x0F},
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x02C, .snum = 0x07, .sdpa = 0x0F},
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x034, .snum = 0x07, .sdpa = 0x0F},
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x032, .snum = 0x07, .sdpa = 0x0F},
};

const ctsu_cfg_t g_qe_ctsu_cfg_config02 = {
	.cap = CTSU_CAP_SOFTWARE,

	.txvsel = CTSU_TXVSEL_INTERNAL_POWER,
	.txvsel2 = CTSU_TXVSEL_MODE,

	.atune12 = CTSU_ATUNE12_40UA,
	.md = CTSU_MODE_SELF_MULTI_SCAN,
	.posel = CTSU_POSEL_SAME_PULSE,

	.ctsuchac0 = 0xF4,  /* ch0-ch7 enable mask */
	.ctsuchac1 = 0x01,  /* ch8-ch15 enable mask */
	.ctsuchac2 = 0x00,  /* ch16-ch23 enable mask */
	.ctsuchac3 = 0x00,  /* ch24-ch31 enable mask */
	.ctsuchac4 = 0x00,  /* ch32-ch39 enable mask */
	.ctsuchtrc0 = 0x00, /* ch0-ch7 mutual tx mask */
	.ctsuchtrc1 = 0x01, /* ch8-ch15 mutual tx mask */
	.ctsuchtrc2 = 0x00, /* ch16-ch23 mutual tx mask */
	.ctsuchtrc3 = 0x00, /* ch24-ch31 mutual tx mask */
	.ctsuchtrc4 = 0x00, /* ch32-ch39 mutual tx mask */
	.num_rx = 5,
	.num_tx = 0,
	.p_elements = g_qe_ctsu_element_cfg_config02,

#if (CTSU_TARGET_VALUE_CONFIG_SUPPORT == 1)
	.tuning_self_target_value = 4608,
	.tuning_mutual_target_value = 10240,
#endif

	.num_moving_average = 4,
	.p_callback = &qe_touch_callback,
#if (CTSU_CFG_DTC_SUPPORT_ENABLE == 1)
	.p_transfer_tx = &g_transfer0,
	.p_transfer_rx = &g_transfer1,
#else
	.p_transfer_tx = NULL,
	.p_transfer_rx = NULL,
#endif

	.write_irq = CTSU_WRITE_IRQn,
	.read_irq = CTSU_READ_IRQn,
	.end_irq = CTSU_END_IRQn,

};

ctsu_instance_ctrl_t g_qe_ctsu_ctrl_config02;

const ctsu_instance_t g_qe_ctsu_instance_config02 = {
	.p_ctrl = &g_qe_ctsu_ctrl_config02,
	.p_cfg = &g_qe_ctsu_cfg_config02,
	.p_api = &g_ctsu_on_ctsu,
};

/* Touch Related Information for [CONFIG02] configuration. */

#define QE_TOUCH_CONFIG02_NUM_BUTTONS    (0)
#define QE_TOUCH_CONFIG02_NUM_SLIDERS    (1)
#define QE_TOUCH_CONFIG02_NUM_WHEELS     (0)
#define QE_TOUCH_CONFIG02_NUM_TOUCH_PADS (0)

#if (QE_TOUCH_CONFIG02_NUM_BUTTONS != 0)
const touch_button_cfg_t g_qe_touch_button_cfg_config02[] = {NULL};
#endif

const uint8_t g_qe_touch_elem_slider_config02_slider00[] = {1, 0, 2, 4, 3};

#if (QE_TOUCH_CONFIG02_NUM_SLIDERS != 0)
const touch_slider_cfg_t g_qe_touch_slider_cfg_config02[] = {
	{
		.p_elem_index = g_qe_touch_elem_slider_config02_slider00,
		.num_elements = 5,
		.threshold = 570,
	},
};
#endif

#if (QE_TOUCH_CONFIG02_NUM_WHEELS != 0)
const touch_wheel_cfg_t g_qe_touch_wheel_cfg_config02[] = {NULL};
#endif

#if (QE_TOUCH_CONFIG02_NUM_TOUCH_PADS != 0)
const touch_pad_cfg_t g_qe_touch_touch_pad_cfg_config02 = {NULL};
#endif

const touch_cfg_t g_qe_touch_cfg_config02 = {
	.p_buttons = NULL,
	.p_sliders = g_qe_touch_slider_cfg_config02,
	.p_wheels = NULL,
#if (TOUCH_CFG_PAD_ENABLE != 0)
	.p_pad = NULL,
#endif
	.num_buttons = QE_TOUCH_CONFIG02_NUM_BUTTONS,
	.num_sliders = QE_TOUCH_CONFIG02_NUM_SLIDERS,
	.num_wheels = QE_TOUCH_CONFIG02_NUM_WHEELS,

	.number = 1,
#if ((TOUCH_CFG_UART_MONITOR_SUPPORT == 1) || (TOUCH_CFG_UART_TUNING_SUPPORT == 1))
	.p_uart_instance = &g_uart_qe,
#else
	.p_uart_instance = NULL,
#endif

	.on_freq = 3,
	.off_freq = 3,
	.drift_freq = 255,
	.cancel_freq = 0,

	.p_ctsu_instance = &g_qe_ctsu_instance_config02,
};

touch_instance_ctrl_t g_qe_touch_ctrl_config02;

const touch_instance_t g_qe_touch_instance_config02 = {
	.p_ctrl = &g_qe_touch_ctrl_config02,
	.p_cfg = &g_qe_touch_cfg_config02,
	.p_api = &g_touch_on_ctsu,
};

/* CTSU Related Information for [CONFIG03] configuration. */

const ctsu_element_cfg_t g_qe_ctsu_element_cfg_config03[] = {
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x03F, .snum = 0x07, .sdpa = 0x0F},
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x03D, .snum = 0x07, .sdpa = 0x0F},
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x041, .snum = 0x07, .sdpa = 0x0F},
	{.ssdiv = CTSU_SSDIV_4000, .so = 0x037, .snum = 0x07, .sdpa = 0x0F},
};

const ctsu_cfg_t g_qe_ctsu_cfg_config03 = {
	.cap = CTSU_CAP_SOFTWARE,

	.txvsel = CTSU_TXVSEL_INTERNAL_POWER,
	.txvsel2 = CTSU_TXVSEL_MODE,

	.atune12 = CTSU_ATUNE12_40UA,
	.md = CTSU_MODE_SELF_MULTI_SCAN,
	.posel = CTSU_POSEL_SAME_PULSE,

	.ctsuchac0 = 0x00,  /* ch0-ch7 enable mask */
	.ctsuchac1 = 0x40,  /* ch8-ch15 enable mask */
	.ctsuchac2 = 0xA4,  /* ch16-ch23 enable mask */
	.ctsuchac3 = 0x00,  /* ch24-ch31 enable mask */
	.ctsuchac4 = 0x01,  /* ch32-ch39 enable mask */
	.ctsuchtrc0 = 0x00, /* ch0-ch7 mutual tx mask */
	.ctsuchtrc1 = 0x40, /* ch8-ch15 mutual tx mask */
	.ctsuchtrc2 = 0x00, /* ch16-ch23 mutual tx mask */
	.ctsuchtrc3 = 0x00, /* ch24-ch31 mutual tx mask */
	.ctsuchtrc4 = 0x00, /* ch32-ch39 mutual tx mask */
	.num_rx = 4,
	.num_tx = 0,
	.p_elements = g_qe_ctsu_element_cfg_config03,

#if (CTSU_TARGET_VALUE_CONFIG_SUPPORT == 1)
	.tuning_self_target_value = 4608,
	.tuning_mutual_target_value = 10240,
#endif

	.num_moving_average = 4,
	.p_callback = &qe_touch_callback,
#if (CTSU_CFG_DTC_SUPPORT_ENABLE == 1)
	.p_transfer_tx = &g_transfer0,
	.p_transfer_rx = &g_transfer1,
#else
	.p_transfer_tx = NULL,
	.p_transfer_rx = NULL,
#endif

	.write_irq = CTSU_WRITE_IRQn,
	.read_irq = CTSU_READ_IRQn,
	.end_irq = CTSU_END_IRQn,

};

ctsu_instance_ctrl_t g_qe_ctsu_ctrl_config03;

const ctsu_instance_t g_qe_ctsu_instance_config03 = {
	.p_ctrl = &g_qe_ctsu_ctrl_config03,
	.p_cfg = &g_qe_ctsu_cfg_config03,
	.p_api = &g_ctsu_on_ctsu,
};

/* Touch Related Information for [CONFIG03] configuration. */

#define QE_TOUCH_CONFIG03_NUM_BUTTONS    (0)
#define QE_TOUCH_CONFIG03_NUM_SLIDERS    (0)
#define QE_TOUCH_CONFIG03_NUM_WHEELS     (1)
#define QE_TOUCH_CONFIG03_NUM_TOUCH_PADS (0)

#if (QE_TOUCH_CONFIG03_NUM_BUTTONS != 0)
const touch_button_cfg_t g_qe_touch_button_cfg_config03[] = {NULL};
#endif

#if (QE_TOUCH_CONFIG03_NUM_SLIDERS != 0)
const touch_slider_cfg_t g_qe_touch_slider_cfg_config03[] = {NULL};
#endif

const uint8_t g_qe_touch_elem_wheel_config03_wheel00[] = {0, 1, 2, 3};

#if (QE_TOUCH_CONFIG03_NUM_WHEELS != 0)
const touch_wheel_cfg_t g_qe_touch_wheel_cfg_config03[] = {
	{
		.p_elem_index = g_qe_touch_elem_wheel_config03_wheel00,
		.num_elements = 4,
		.threshold = 585,
	},
};
#endif

#if (QE_TOUCH_CONFIG03_NUM_TOUCH_PADS != 0)
const touch_pad_cfg_t g_qe_touch_touch_pad_cfg_config03 = {NULL};
#endif

const touch_cfg_t g_qe_touch_cfg_config03 = {
	.p_buttons = NULL,
	.p_sliders = NULL,
	.p_wheels = g_qe_touch_wheel_cfg_config03,
#if (TOUCH_CFG_PAD_ENABLE != 0)
	.p_pad = NULL,
#endif
	.num_buttons = QE_TOUCH_CONFIG03_NUM_BUTTONS,
	.num_sliders = QE_TOUCH_CONFIG03_NUM_SLIDERS,
	.num_wheels = QE_TOUCH_CONFIG03_NUM_WHEELS,

	.number = 2,
#if ((TOUCH_CFG_UART_MONITOR_SUPPORT == 1) || (TOUCH_CFG_UART_TUNING_SUPPORT == 1))
	.p_uart_instance = &g_uart_qe,
#else
	.p_uart_instance = NULL,
#endif

	.on_freq = 3,
	.off_freq = 3,
	.drift_freq = 255,
	.cancel_freq = 0,

	.p_ctsu_instance = &g_qe_ctsu_instance_config03,
};

touch_instance_ctrl_t g_qe_touch_ctrl_config03;

const touch_instance_t g_qe_touch_instance_config03 = {
	.p_ctrl = &g_qe_touch_ctrl_config03,
	.p_cfg = &g_qe_touch_cfg_config03,
	.p_api = &g_touch_on_ctsu,
};
