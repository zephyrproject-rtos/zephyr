/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**************************************************************************************************
 * File Name    : qe_touch_config.h
 * Description  : This file includes definitions.
 **************************************************************************************************/
/**************************************************************************************************
 * History      : MM/DD/YYYY Version Description
 *              : 09/02/2019 1.00    First Release
 *              : 01/16/2020 1.01    Visible CTSU control structure for API using
 *              : 02/20/2020 1.10    Corresponding for FSP V0.12.0
 *              : 02/26/2020 1.11    Adding information for Temperature Correction
 *              : 03/02/2020 1.20    Corresponding for FSP V1.0.0 RC0
 *              : 05/26/2021 1.30    Adding Diagnosis Supporting
 *              : 07/15/2021 1.31    Fixing a Little
 *              : 03/31/2023 1.32    Improving Traceability
 *              : 07/30/2024 1.40    Adding Auto Judgement Supporting
 **************************************************************************************************/
/**************************************************************************************************
 * Touch I/F Configuration File  : quickstart_rssk_ra2l1_ep.tifcfg
 * Tuning Log File               : quickstart_rssk_ra2l1_ep_log_tuning20230904134024.log
 **************************************************************************************************/

#ifndef QE_TOUCH_CONFIG_H
#define QE_TOUCH_CONFIG_H

#include "hal_data.h"
#include "qe_touch_define.h"

/**************************************************************************************************
 * Exported global variables
 **************************************************************************************************/
extern const ctsu_instance_t g_qe_ctsu_instance_config01;
extern const ctsu_instance_t g_qe_ctsu_instance_config02;
extern const ctsu_instance_t g_qe_ctsu_instance_config03;
extern const touch_instance_t g_qe_touch_instance_config01;
extern const touch_instance_t g_qe_touch_instance_config02;
extern const touch_instance_t g_qe_touch_instance_config03;

extern volatile uint8_t g_qe_touch_flag;
extern volatile ctsu_event_t g_qe_ctsu_event;

/**************************************************************************************************
 * Exported global functions (to be accessed by other files)
 **************************************************************************************************/
extern void qe_touch_callback(touch_callback_args_t *p_args);

#endif /* QE_TOUCH_CONFIG_H */
