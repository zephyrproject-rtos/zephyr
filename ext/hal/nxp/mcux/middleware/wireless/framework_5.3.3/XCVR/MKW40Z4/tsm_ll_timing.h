/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file tsm_ll_timing.h
* Header file for the TSM and LL timing initialization module.
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
#ifndef TSM_LL_TIMING_H
#define TSM_LL_TIMING_H

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "tsm_timing_ble.h"
#include "tsm_timing_zigbee.h"

/*! *********************************************************************************
*************************************************************************************
* Macros
*************************************************************************************
********************************************************************************** */
#define mem16(x) (*(volatile uint16_t *)(x))

/* Static assertion checks */
#define STATIC_ASSERT(COND,MSG) typedef char static_assertion_##MSG[(!!(COND))*2-1]

#define COMPILE_TIME_ASSERT3(X,L) STATIC_ASSERT(X,static_assertion_at_line_##L)
#define COMPILE_TIME_ASSERT2(X,L) COMPILE_TIME_ASSERT3(X,L)
#define COMPILE_TIME_ASSERT(X)    COMPILE_TIME_ASSERT2(X,__LINE__)

/*! *********************************************************************************
*************************************************************************************
* Constants
*************************************************************************************
********************************************************************************** */
/* LL register setting defines for BLE */
#define TX_ON_DELAY         (146)
#ifndef BLE_SNIFFER_CORE
#define RX_ON_DELAY         (128)
#else
#define RX_ON_DELAY         (119)
#endif /* BLE_SNIFFER_CORE */
#define TX_ON_DELAY_SHIFT   (8) 
#define RX_ON_DELAY_SHIFT   (0)
STATIC_ASSERT((TX_ON_DELAY>END_OF_TX_WU_BLE),TX_Warmup_is_too_long_for_specified_TX_ON_Delay);
STATIC_ASSERT((RX_ON_DELAY>(END_OF_RX_WU_BLE+1)),RX_Warmup_is_too_long_for_specified_RX_ON_Delay);
#define TX_SYNTH_DELAY          (TX_ON_DELAY-(END_OF_TX_WU_BLE-0)) /* END_OF_TX_WU_BLE from tsm_timing_ble.h */
#define RX_SYNTH_DELAY          (RX_ON_DELAY-(END_OF_RX_WU_BLE-1)) /* END_OF_RX_WU_BLE from tsm_timing_ble.h */
#define TX_RX_ON_DELAY_VALUE    ((TX_ON_DELAY<<TX_ON_DELAY_SHIFT) | (RX_ON_DELAY<<RX_ON_DELAY_SHIFT))
#define TX_RX_SYNTH_DELAY_VALUE ((TX_SYNTH_DELAY<<TX_ON_DELAY_SHIFT) | (RX_SYNTH_DELAY<<RX_ON_DELAY_SHIFT))

/* LL timing defines for Zigbee */
#define END_OF_TX_WU_ZB      END_OF_TX_WU_BLE+ZB_TSM_EXTRA_DELAY  /* Zigbee == BLE timing plus an adjustment */
#define END_OF_RX_WU_ZB      END_OF_RX_WU_BLE+ZB_TSM_EXTRA_DELAY  /* Zigbee == BLE timing plus an adjustment */

#define NUM_TSM_TIMING_REGS     (45)

/*! *********************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
********************************************************************************** */
typedef enum
{
    BLE_RADIO = 0,
    ZIGBEE_RADIO = 1
} PHY_RADIO_T;

/*! *********************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
********************************************************************************** */
#ifdef __cplusplus
extern "C"
{
#endif

void tsm_ll_timing_init(PHY_RADIO_T mode);

#ifdef __cplusplus
}
#endif


#endif /* TSM_LL_TIMING_H */
