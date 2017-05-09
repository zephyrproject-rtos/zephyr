/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file ZigbeeDefaults.h
* This is a header file for the default register values of the transceiver used
* for Zigbee mode.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of Freescale Semiconductor, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __ZIGBEE_DEFAULTS_H__
#define __ZIGBEE_DEFAULTS_H__

/*! *********************************************************************************
*************************************************************************************
* Constants
*************************************************************************************
********************************************************************************** */
/* XCVR_CTRL Defaults */

/* XCVR_CTRL  */
#define Zigbee_TGT_PWR_SRC_def_c 0x02
#define Zigbee_PROTOCOL_def_c 0x04

/* TSM Defaults (no PA ramp)*/

/*Analog: BBW Filter  */

/* XCVR_TZA_CTRL */
#define ZGBE_TZA_CAP_TUNE_def_c          0

/* XCVR_BBF_CTRL */
#define ZGBE_BBF_CAP_TUNE_def_c          0
#define ZGBE_BBF_RES_TUNE2_def_c         0

/*RX DIG: AGC DCOC and filtering */

/*RX_DIG_CTRL*/
#define RX_DEC_FILT_OSR_Zigbee_def_c   0x02
#define RX_NORM_EN_Zigbee_def_c        0x01
#define RX_CH_FILT_BYPASS_Zigbee_def_c 0x00

/* AGC_CTRL_0 */
#define FREEZE_AGC_SRC_Zigbee_def_c    0x02

/* RSSI_CTRL 0*/
#define RSSI_HOLD_SRC_Zigbee_def_c     0x03

/* DCOC_CTRL_0 */
#define DCOC_CAL_DURATION_Zigbee_def_c 0x15  /* Max: 1F */
#define DCOC_CORR_HOLD_TIME_Zigbee_def_c 0x58 /* 0x7F makes corrections continuous. */
#define DCOC_CORR_DLY_Zigbee_def_c     0x15
#define ALPHA_RADIUS_IDX_Zigbee_def_c  0x02 /* 1/4 */
#define ALPHAC_SCALE_IDX_Zigbee_def_c  0x01 /* 1/4 */
#define SIGN_SCALE_IDX_Zigbee_def_c    0x03 /* 1/32 */
#define DCOC_CORRECT_EN_Zigbee_def_c   0x01
#define DCOC_TRACK_EN_Zigbee_def_c     0x01
#define DCOC_MAN_Zigbee_def_c          0x00

/*RX_CHF_COEFn*/
/*Dig Channel Setting 2015/05/28 - 860kHz Rx BW: Kaiser 3.0: */
#define RX_CHF_COEF0_Zigbee_def_c      0xFE
#define RX_CHF_COEF1_Zigbee_def_c      0xFD
#define RX_CHF_COEF2_Zigbee_def_c      0x05
#define RX_CHF_COEF3_Zigbee_def_c      0x08
#define RX_CHF_COEF4_Zigbee_def_c      0xF5
#define RX_CHF_COEF5_Zigbee_def_c      0xEA
#define RX_CHF_COEF6_Zigbee_def_c      0x10
#define RX_CHF_COEF7_Zigbee_def_c      0x49

/* DCOC_CAL_IIR */
#define IIR3A_IDX_Zigbee_def_c         0x001
#define IIR2A_IDX_Zigbee_def_c         0x002
#define IIR1A_IDX_Zigbee_def_c         0x002

/* CORR_CTRL */
#define CORR_VT_Zigbee_def_c           0x6B
#define CORR_NVAL_Zigbee_def_c         3


#endif /* __ZIGBEE_DEFAULTS_H__*/