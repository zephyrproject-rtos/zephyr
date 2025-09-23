/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QE_TOUCH_CONFIG_H
#define QE_TOUCH_CONFIG_H

/* Generated content from Renesas QE Capacitive Touch */

#include "hal_data.h"
#include "qe_touch_define.h"

extern const ctsu_instance_t g_qe_ctsu_instance_config01;
extern const ctsu_instance_t g_qe_ctsu_instance_config02;
extern const ctsu_instance_t g_qe_ctsu_instance_config03;
extern const touch_instance_t g_qe_touch_instance_config01;
extern const touch_instance_t g_qe_touch_instance_config02;
extern const touch_instance_t g_qe_touch_instance_config03;

extern volatile uint8_t g_qe_touch_flag;
extern volatile ctsu_event_t g_qe_ctsu_event;

extern void qe_touch_callback(touch_callback_args_t *p_args);

#endif /* QE_TOUCH_CONFIG_H */
