/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file ifr_tbl_mkw40z4_radio.h
* MKW40Z4 Radio IFR structure definition table.
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

#ifndef __IFR_TBL_MKW40Z4_RADIO_H__
#define __IFR_TBL_MKW40Z4_RADIO_H__

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "ifr_mkw40z4_radio.h"      
#include "XCVR_regs.h"

/*! *********************************************************************************
*************************************************************************************
* Constants
*************************************************************************************
********************************************************************************** */

/* ************************ */
/* IFR structure definition */
/* ************************ */
#define TEST_IFR_UNPACK                 0

/* Sizes of IFR bits */
#define TZA_CAP_TUNE_SZ                 4
#define BBF_CAP_TUNE_SZ                 4
#define BBF_RES_TUNE2_SZ                4
#define I_Q_IMB_GAIN_CORRECTION_SZ      11
#define I_Q_IMB_PHASE_CORRECTION_SZ     12
#define BGAP_VOLTAGE_TRIM_SZ            4
#define BBAP_CURRENT_TRIM_SZ            4
#define XTAL_RF_REG_TRIM_SZ             4
#define PLL_REG_TRIM_SZ                 4
#define I_Q_GEN_REG_TRIM_SZ             4
#define LNA_TX_REG_TRIM_SZ              4
#define ADC_DIG_REG_TRIM_SZ             4
#define ADC_ANLG_REG_TRIM_SZ            4

/* Source position in the 32 bits from READ_RESOURCE command */
#define TZA_CAP_TUNE_SRCPOS             12
#define BBF_CAP_TUNE_SRCPOS             8
#define BBF_RES_TUNE2_SRCPOS            4
#define I_Q_IMB_GAIN_CORRECTION_SRCPOS  20
#define I_Q_IMB_PHASE_CORRECTION_SRCPOS 8
#define BGAP_VOLTAGE_TRIM_SRCPOS        28
#define BBAP_CURRENT_TRIM_SRCPOS        24
#define XTAL_RF_REG_TRIM_SRCPOS         20
#define PLL_REG_TRIM_SRCPOS             16
#define I_Q_GEN_REG_TRIM_SRCPOS         12
#define LNA_TX_REG_TRIM_SRCPOS          8
#define ADC_DIG_REG_TRIM_SRCPOS         4
#define ADC_ANLG_REG_TRIM_SRCPOS        0

/* Destination registers for IFR bits, this set is a testing option to test in memory */
#if (TEST_IFR_UNPACK)
extern uint32_t test_register_space[];
#define TZA_CAP_TUNE_DESTREG             (uint32_t)&test_register_space[0]
#define BBF_CAP_TUNE_DESTREG             (uint32_t)&test_register_space[1]
#define BBF_RES_TUNE2_DESTREG            (uint32_t)&test_register_space[2]
#define I_Q_IMB_GAIN_CORRECTION_DESTREG  (uint32_t)&test_register_space[3]
#define I_Q_IMB_PHASE_CORRECTION_DESTREG (uint32_t)&test_register_space[4]
#define BGAP_VOLTAGE_TRIM_DESTREG        (uint32_t)&test_register_space[5]
#define BBAP_CURRENT_TRIM_DESTREG        (uint32_t)&test_register_space[6]
#define XTAL_RF_REG_TRIM_DESTREG         (uint32_t)&test_register_space[7]
#define PLL_REG_TRIM_DESTREG             (uint32_t)&test_register_space[8]
#define I_Q_GEN_REG_TRIM_DESTREG         (uint32_t)&test_register_space[9]
#define LNA_TX_REG_TRIM_DESTREG          (uint32_t)&test_register_space[10]
#define ADC_DIG_REG_TRIM_DESTREG         (uint32_t)&test_register_space[11]
#define ADC_ANLG_REG_TRIM_DESTREG        (uint32_t)&test_register_space[12]
#else
/* actual destination register addresses */
#define TZA_CAP_TUNE_DESTREG              HW_XCVR_TZA_CTRL_ADDR(XCVR_BASE)
#define BBF_CAP_TUNE_DESTREG              HW_XCVR_BBF_CTRL_ADDR(XCVR_BASE)
#define BBF_RES_TUNE2_DESTREG             HW_XCVR_BBF_CTRL_ADDR(XCVR_BASE)
#define I_Q_IMB_GAIN_CORRECTION_DESTREG   HW_XCVR_IQMC_CAL_ADDR(XCVR_BASE)
#define I_Q_IMB_PHASE_CORRECTION_DESTREG  HW_XCVR_IQMC_CAL_ADDR(XCVR_BASE)
#define BGAP_VOLTAGE_TRIM_DESTREG         HW_XCVR_BGAP_CTRL_ADDR(XCVR_BASE)
#define BBAP_CURRENT_TRIM_DESTREG         HW_XCVR_BGAP_CTRL_ADDR(XCVR_BASE)
#define XTAL_RF_REG_TRIM_DESTREG          HW_XCVR_XTAL_CTRL_ADDR(XCVR_BASE)
#define PLL_REG_TRIM_DESTREG              HW_XCVR_PLL_CTRL_ADDR(XCVR_BASE)
#define I_Q_GEN_REG_TRIM_DESTREG          HW_XCVR_QGEN_CTRL_ADDR(XCVR_BASE)
#define LNA_TX_REG_TRIM_DESTREG           HW_XCVR_TCA_CTRL_ADDR(XCVR_BASE)
#define ADC_DIG_REG_TRIM_DESTREG          HW_XCVR_ADC_REGS_ADDR(XCVR_BASE)
#define ADC_ANLG_REG_TRIM_DESTREG         HW_XCVR_ADC_REGS_ADDR(XCVR_BASE)
#endif

/* Destination register shift */
#define TZA_CAP_TUNE_DESTSHFT             BP_XCVR_TZA_CTRL_TZA_CAP_TUNE
#define BBF_CAP_TUNE_DESTSHFT             BP_XCVR_BBF_CTRL_BBF_CAP_TUNE
#define BBF_RES_TUNE2_DESTSHFT            BP_XCVR_BBF_CTRL_BBF_RES_TUNE2
#define I_Q_IMB_GAIN_CORRECTION_DESTSHFT  BP_XCVR_IQMC_CAL_IQMC_GAIN_ADJ
#define I_Q_IMB_PHASE_CORRECTION_DESTSHFT BP_XCVR_IQMC_CAL_IQMC_PHASE_ADJ
#define BGAP_VOLTAGE_TRIM_DESTSHFT        BP_XCVR_BGAP_CTRL_BGAP_VOLTAGE_TRIM
#define BBAP_CURRENT_TRIM_DESTSHFT        BP_XCVR_BGAP_CTRL_BGAP_CURRENT_TRIM
#define XTAL_RF_REG_TRIM_DESTSHFT         BP_XCVR_XTAL_CTRL2_XTAL_REG_SUPPLY
#define PLL_REG_TRIM_DESTSHFT             BP_XCVR_PLL_CTRL_PLL_REG_SUPPLY
#define I_Q_GEN_REG_TRIM_DESTSHFT         BP_XCVR_QGEN_CTRL_QGEN_REG_SUPPLY
#define LNA_TX_REG_TRIM_DESTSHFT          BP_XCVR_TCA_CTRL_TCA_TX_REG_SUPPLY
#define ADC_DIG_REG_TRIM_DESTSHFT         BP_XCVR_ADC_REGS_ADC_REG_DIG_SUPPLY
#define ADC_ANLG_REG_TRIM_DESTSHFT        BP_XCVR_ADC_REGS_ADC_ANA_REG_SUPPLY

/*! *********************************************************************************
*************************************************************************************
* Macros
*************************************************************************************
********************************************************************************** */
#define NUM_RADIO_TBL_ENTRIES           (sizeof(radio_ifr_tbl)/sizeof(RADIO_IFR_TABLE_T))

/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */
const IFR_TBL_ENTRY_T anatop_ifr_rctune[] =
{
    {TZA_CAP_TUNE_SRCPOS, TZA_CAP_TUNE_SZ, TZA_CAP_TUNE_DESTREG, TZA_CAP_TUNE_DESTSHFT, LONG_32_BIT},
    {BBF_CAP_TUNE_SRCPOS, BBF_CAP_TUNE_SZ, BBF_CAP_TUNE_DESTREG, BBF_CAP_TUNE_DESTSHFT, LONG_32_BIT},
    {BBF_RES_TUNE2_SRCPOS, BBF_RES_TUNE2_SZ, BBF_RES_TUNE2_DESTREG, BBF_RES_TUNE2_DESTSHFT, LONG_32_BIT},
};

const IFR_TBL_ENTRY_T rxdig_ifr_iqmc[] =
{
    {I_Q_IMB_GAIN_CORRECTION_SRCPOS, I_Q_IMB_GAIN_CORRECTION_SZ, I_Q_IMB_GAIN_CORRECTION_DESTREG, I_Q_IMB_GAIN_CORRECTION_DESTSHFT, LONG_32_BIT},
    {I_Q_IMB_PHASE_CORRECTION_SRCPOS, I_Q_IMB_PHASE_CORRECTION_SZ, I_Q_IMB_PHASE_CORRECTION_DESTREG, I_Q_IMB_PHASE_CORRECTION_DESTSHFT, LONG_32_BIT},
};

const IFR_TBL_ENTRY_T anatop_ifr_regs[] =
{
    {BGAP_VOLTAGE_TRIM_SRCPOS, BGAP_VOLTAGE_TRIM_SZ, BGAP_VOLTAGE_TRIM_DESTREG, BGAP_VOLTAGE_TRIM_DESTSHFT, LONG_32_BIT},
    {BBAP_CURRENT_TRIM_SRCPOS, BBAP_CURRENT_TRIM_SZ, BBAP_CURRENT_TRIM_DESTREG, BBAP_CURRENT_TRIM_DESTSHFT, LONG_32_BIT},
};

const RADIO_IFR_TABLE_T radio_ifr_tbl[] =
{
    {0x84,sizeof(anatop_ifr_rctune)/sizeof(IFR_TBL_ENTRY_T),&anatop_ifr_rctune[0]},
    {0x98,sizeof(rxdig_ifr_iqmc)/sizeof(IFR_TBL_ENTRY_T),&rxdig_ifr_iqmc[0]},
    {0x9C,sizeof(anatop_ifr_regs)/sizeof(IFR_TBL_ENTRY_T),&anatop_ifr_regs[0]}
};

#endif /* __IFR_TBL_MKW40Z4_RADIO_H__ */