/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**************************************************************************************************
 * File Name    : qe_touch_define.h
 * Description  : This file includes definitions.
 **************************************************************************************************/
/**************************************************************************************************
 * History      : MM/DD/YYYY Version Description
 *              : 09/02/2019 1.00    First Release
 *              : 12/26/2019 1.10    Corresponding for FSP V0.10.0
 *              : 01/16/2020 1.11    Adding "Button State Mask" macros
 *              : 02/20/2020 1.20    Corresponding for FSP V0.12.0
 *              : 03/04/2020 1.21    Corresponding for FSP V1.0.0 RC0
 *              : 06/26/2020 1.22    Corresponding for FSP V1.1.0
 *              : 09/10/2020 1.30    Corresponding for FSP V2.0.0 Beta2
 *              : 05/26/2021 1.40    Adding Diagnosis Supporting
 *              : 06/01/2021 1.41    Fixing a Little
 *              : 11/17/2021 1.50    Adding information for Initial Offset Tuning
 *              : 12/06/2021 1.51    Fixing a Little
 *              : 09/05/2022 1.52    Fixing a Little
 *              : 03/23/2023 1.60    Adding 3 Frequency Judgement Supporting
 *              : 03/31/2023 1.61    Improving Traceability
 *              : 07/25/2024 1.70    Adding Auto Correction / Auto Multi Clock Correction / MEC
 *Supporting : 09/04/2024 1.71    Adding version info macro of QE : 12/06/2024 1.72    Adding macro
 *for auto judgment
 **************************************************************************************************/
/**************************************************************************************************
 * Touch I/F Configuration File  : quickstart_rssk_ra2l1_ep.tifcfg
 * Tuning Log File               : quickstart_rssk_ra2l1_ep_log_tuning20230904134024.log
 **************************************************************************************************/

#ifndef QE_TOUCH_DEFINE_H
#define QE_TOUCH_DEFINE_H

/**************************************************************************************************
 * Macro definitions
 **************************************************************************************************/
#define QE_TOUCH_VERSION (0x0410)

#define CTSU_CFG_NUM_SELF_ELEMENTS (12)

#define CTSU_CFG_NUM_MUTUAL_ELEMENTS (0)
#define CTSU_CFG_NUM_CFC             (0)
#define CTSU_CFG_NUM_CFC_TX          (0)

#define TOUCH_CFG_MONITOR_ENABLE (1)
#define TOUCH_CFG_NUM_BUTTONS    (3)
#define TOUCH_CFG_NUM_SLIDERS    (1)
#define TOUCH_CFG_NUM_WHEELS     (1)
#define TOUCH_CFG_PAD_ENABLE     (0)

#define QE_TOUCH_MACRO_CTSU_IP_KIND (2)

#define CTSU_CFG_VCC_MV           (5000)
#define CTSU_CFG_LOW_VOLTAGE_MODE (0)

#define CTSU_CFG_PCLK_DIVISION (0)

#define CTSU_CFG_TSCAP_PORT (0x010C)

#define CTSU_CFG_NUM_SUMULTI (3)
#define CTSU_CFG_SUMULTI0    (0x3F)
#define CTSU_CFG_SUMULTI1    (0x36)
#define CTSU_CFG_SUMULTI2    (0x48)

#define CTSU_CFG_CALIB_RTRIM_SUPPORT     (0)
#define CTSU_CFG_TEMP_CORRECTION_SUPPORT (0)
#define CTSU_CFG_TEMP_CORRECTION_TS      (0)
#define CTSU_CFG_TEMP_CORRECTION_TIME    (0)

#define CTSU_CFG_TARGET_VALUE_QE_SUPPORT (1)

#define CTSU_CFG_MAJORITY_MODE                 (1)
#define CTSU_CFG_NUM_AUTOJUDGE_SELF_ELEMENTS   (0)
#define CTSU_CFG_NUM_AUTOJUDGE_MUTUAL_ELEMENTS (0)

/**************************************************************************************************
 * Button State Mask for each configuration.
 **************************************************************************************************/
#define CONFIG01_INDEX_BUTTON00 (2)
#define CONFIG01_MASK_BUTTON00  (1ULL << CONFIG01_INDEX_BUTTON00)
#define CONFIG01_INDEX_BUTTON01 (1)
#define CONFIG01_MASK_BUTTON01  (1ULL << CONFIG01_INDEX_BUTTON01)
#define CONFIG01_INDEX_BUTTON02 (0)
#define CONFIG01_MASK_BUTTON02  (1ULL << CONFIG01_INDEX_BUTTON02)

#endif /* QE_TOUCH_DEFINE_H */
