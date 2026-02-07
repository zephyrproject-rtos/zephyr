/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MISC_SYNA_AON_EVENT_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MISC_SYNA_AON_EVENT_GPIO_H_

#define AON_GPO0 0
#define AON_GPO1 1
#define AON_GPO2 2

/* GPO0/1 common events */
#define AON_GPO_EVENT_CDM_EVENT               0
#define AON_GPO_EVENT_STATS_EVENT             1
#define AON_GPO_EVENT_IMG_FRM_END             2
#define AON_GPO_EVENT_AUDIO_EVENT             3
#define AON_GPO_EVENT_TIMER1                  4
#define AON_GPO_EVENT_TIMER2                  5
#define AON_GPO_EVENT_TIMER3                  6
#define AON_GPO_EVENT_RESERVED                7
/* Additional GPO0 events */
#define AON_GPO0_EVENT_SDA_SDI_AFTER_DELAY    8
#define AON_GPO0_EVENT_ANA_BUCK_PWORK_0P8     9
#define AON_GPO0_EVENT_CLK_32K_MUXED          10
#define AON_GPO0_EVENT_ANA_BUCK_SW_CLK        11
#define AON_GPO0_EVENT_AON_G1_RSTN            12
#define AON_GPO0_EVENT_AON_G2_RSTN            13
#define AON_GPO0_EVENT_AON_G3_RSTN            14
#define AON_GPO0_EVENT_AON_G1_SM_IDLE         15
/* Additional GPO1 events */
#define AON_GPO1_EVENT_SCL_SCLK_AFTER_DELAY   8
#define AON_GPO1_EVENT_ANA_BUCK_PWORK         9
#define AON_GPO1_EVENT_ANA_BUCK_SW_CLK        10
#define AON_GPO1_EVENT_PMU_EN                 11
#define AON_GPO1_EVENT_ANA_ANALOG_OK          12
#define AON_GPO1_EVENT_G1_ACTIVE              13
#define AON_GPO1_EVENT_G2_ACTIVE              14
#define AON_GPO1_EVENT_G3_ACTIVE              15
/*  GPO2 events */
#define AON_GPO2_EVENT_I2C_SM_WAKE            0
#define AON_GPO2_EVENT_IMGCAP_SM_WAKE         1
#define AON_GPO2_EVENT_LPPROC_SM_WAKE         2
#define AON_GPO2_EVENT_MAIN_SM_WAKE           3
#define AON_GPO2_EVENT_I2C_SM_IDLE_N          4
#define AON_GPO2_EVENT_IMGCAP_SM_IDLE_N       5
#define AON_GPO2_EVENT_LPPROC_SM_IDLE_N       6
#define AON_GPO2_EVENT_MAIN_SM_IDLE_N         7
#define AON_GPO2_EVENT_BASECLK_SM_IDLE_N      8
#define AON_GPO2_EVENT_AON_LPPROGRAM_SM_START 9
#define AON_GPO2_EVENT_AON_G1_PS_C            10
#define AON_GPO2_EVENT_AON_G2_G1_PS_B         11
#define AON_GPO2_EVENT_AON_G2_XTAL_PS_D       12
#define AON_GPO2_EVENT_G2_BUCK_VDD_PD         13
#define AON_GPO2_EVENT_XTAL_VALID             14
#define AON_GPO2_EVENT_G1_TIMEOUT_ERROR       15

#endif
