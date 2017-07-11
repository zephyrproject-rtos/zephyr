/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file overwrites.h
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

/*! *********************************************************************************
*************************************************************************************
* Use:     This file is created exclusively for use with MKW40Z4 1.0 silicon
*          and is provided for the world to use. It contains a list of all
*          known overwrite values. Overwrite values are non-default register
*          values that configure the MKW40Z4 device to a more optimally performing
*          posture. It is expected that low level software (i.e. PHY) will
*          consume this file as a #include, and transfer the contents to the
*          the indicated addresses in the MKW40Z4 memory space. This file has
*          at least one required entry, that being its own version current version
*          number, to be stored at MKW40Z4 location 0x290 the
*          OVERWRITES_VER register. The RAM register is provided in
*          the MKW40Z4 address space to assist in future debug efforts. The
*          analyst may read this location (once device has been booted with
*          mysterious software) and have a good indication of what register
*          overwrites were performed (with all versions of the overwrites.h file
*          being archived forever at the Compass location shown above.
*
*************************************************************************************
********************************************************************************** */

#ifndef OVERWRITES_H_
#define OVERWRITES_H_

/*! *********************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
********************************************************************************** */
typedef struct overwrites_tag 
{
    unsigned int address;
    unsigned int data;
}overwrites_t;

/* The following definition of overwrites are specific to radio trim values coming from IFR. */
typedef struct 
{
    uint8_t src_address;    /*  Address of desired data in IFR */
    uint8_t src_bp;         /*  Bit position of desired trim values in IFR */
    uint8_t field_size;     /*  Number of bits of data */
    uint32_t dest_address;  /*  Address to place IFR data to. */
    uint8_t dest_shift;     /*  Number of shift to be shifted in the destination */
    uint8_t reg_access;     /*  How the destination reg to be accessed (i.e. 8, 16, 32 bit) */
} overwrites_ifr;

/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */
/* These are the recommended overwrites regardless of BLE vs. 802.15.4 posture */
overwrites_t const overwrites_common[] =
{
    {0x4005C000,0x0}, /* RX_DIG_CTRL */
    {0x4005C004,0x0}, /* AGC_CTRL_0 */
    {0x4005C008,0x0}, /* AGC_CTRL_1 */
    {0x4005C00C,0x0}, /* AGC_CTRL_2 */
    {0x4005C010,0x0}, /* AGC_CTRL_3 */
    {0x4005C018,0x0}, /* RSSI_CTRL_0 */
    {0x4005C01C,0x0}, /* RSSI_CTRL_1 */
    {0x4005C020,0x0}, /* DCOC_CTRL_0 */
    {0x4005C024,0x0}, /* DCOC_CTRL_1 */
    {0x4005C028,0x0}, /* DCOC_CTRL_2 */
    {0x4005C02C,0x0}, /* DCOC_CTRL_3 */
    {0x4005C030,0x0}, /* DCOC_CTRL_4 */
    {0x4005C034,0x0}, /* DCOC_CAL_GAIN */
    {0x4005C040,0x0}, /* DCOC_CAL_RCP */
    {0x4005C04C,0x8000}, /* IQMC_CTRL */
    {0x4005C050,0x400}, /* IQMC_CAL */
    {0x4005C054,0x3C242C14}, /* TCA_AGC_VAL_3_0 */
    {0x4005C058,0x9C846C54}, /* TCA_AGC_VAL_7_4 */
    {0x4005C05C,0xB4}, /* TCA_AGC_VAL_8 */
    {0x4005C060,0x0}, /* BBF_RES_TUNE_VAL_7_0 */
    {0x4005C064,0x0}, /* BBF_RES_TUNE_VAL_10_8 */
    {0x4005C068,0x0}, /* TCA_AGC_LIN_VAL_2_0 */
    {0x4005C06C,0x0}, /* TCA_AGC_LIN_VAL_5_3 */
    {0x4005C070,0x0}, /* TCA_AGC_LIN_VAL_8_6 */
    {0x4005C074,0x0}, /* BBF_RES_TUNE_LIN_VAL_3_0 */
    {0x4005C078,0x0}, /* BBF_RES_TUNE_LIN_VAL_7_4 */
    {0x4005C07C,0x0}, /* BBF_RES_TUNE_LIN_VAL_10_8 */
    {0x4005C080,0x0}, /* AGC_GAIN_TBL_03_00 */
    {0x4005C084,0x0}, /* AGC_GAIN_TBL_07_04 */
    {0x4005C088,0x0}, /* AGC_GAIN_TBL_11_08 */
    {0x4005C08C,0x0}, /* AGC_GAIN_TBL_15_12 */
    {0x4005C090,0x0}, /* AGC_GAIN_TBL_19_16 */
    {0x4005C094,0x0}, /* AGC_GAIN_TBL_23_20 */
    {0x4005C098,0x0}, /* AGC_GAIN_TBL_26_24 */
    {0x4005C0A0,0x0}, /* DCOC_OFFSET_0 */
    {0x4005C0A4,0x0}, /* DCOC_OFFSET_1 */
    {0x4005C0A8,0x0}, /* DCOC_OFFSET_2 */
    {0x4005C0AC,0x0}, /* DCOC_OFFSET_3 */
    {0x4005C0B0,0x0}, /* DCOC_OFFSET_4 */
    {0x4005C0B4,0x0}, /* DCOC_OFFSET_5 */
    {0x4005C0B8,0x0}, /* DCOC_OFFSET_6 */
    {0x4005C0BC,0x0}, /* DCOC_OFFSET_7 */
    {0x4005C0C0,0x0}, /* DCOC_OFFSET_8 */
    {0x4005C0C4,0x0}, /* DCOC_OFFSET_9 */
    {0x4005C0C8,0x0}, /* DCOC_OFFSET_10 */
    {0x4005C0CC,0x0}, /* DCOC_OFFSET_11 */
    {0x4005C0D0,0x0}, /* DCOC_OFFSET_12 */
    {0x4005C0D4,0x0}, /* DCOC_OFFSET_13 */
    {0x4005C0D8,0x0}, /* DCOC_OFFSET_14 */
    {0x4005C0DC,0x0}, /* DCOC_OFFSET_15 */
    {0x4005C0E0,0x0}, /* DCOC_OFFSET_16 */
    {0x4005C0E4,0x0}, /* DCOC_OFFSET_17 */
    {0x4005C0E8,0x0}, /* DCOC_OFFSET_18 */
    {0x4005C0EC,0x0}, /* DCOC_OFFSET_19 */
    {0x4005C0F0,0x0}, /* DCOC_OFFSET_20 */
    {0x4005C0F4,0x0}, /* DCOC_OFFSET_21 */
    {0x4005C0F8,0x0}, /* DCOC_OFFSET_22 */
    {0x4005C0FC,0x0}, /* DCOC_OFFSET_23 */
    {0x4005C100,0x0}, /* DCOC_OFFSET_24 */
    {0x4005C104,0x0}, /* DCOC_OFFSET_25 */
    {0x4005C108,0x0}, /* DCOC_OFFSET_26 */
    {0x4005C110,0x0}, /* DCOC_TZA_STEP_0 */
    {0x4005C114,0x0}, /* DCOC_TZA_STEP_1 */
    {0x4005C118,0x0}, /* DCOC_TZA_STEP_2 */
    {0x4005C11C,0x0}, /* DCOC_TZA_STEP_3 */
    {0x4005C120,0x0}, /* DCOC_TZA_STEP_4 */
    {0x4005C124,0x0}, /* DCOC_TZA_STEP_5 */
    {0x4005C128,0x0}, /* DCOC_TZA_STEP_6 */
    {0x4005C12C,0x0}, /* DCOC_TZA_STEP_7 */
    {0x4005C130,0x0}, /* DCOC_TZA_STEP_8 */
    {0x4005C134,0x0}, /* DCOC_TZA_STEP_9 */
    {0x4005C138,0x0}, /* DCOC_TZA_STEP_10 */
    {0x4005C178,0x0}, /* DCOC_CAL_IIR */
    {0x4005C1A0,0x0}, /* RX_CHF_COEF0 */
    {0x4005C1A4,0x0}, /* RX_CHF_COEF1 */
    {0x4005C1A8,0x0}, /* RX_CHF_COEF2 */
    {0x4005C1AC,0x0}, /* RX_CHF_COEF3 */
    {0x4005C1B0,0x0}, /* RX_CHF_COEF4 */
    {0x4005C1B4,0x0}, /* RX_CHF_COEF5 */
    {0x4005C1B8,0x0}, /* RX_CHF_COEF6 */
    {0x4005C1BC,0x0}, /* RX_CHF_COEF7 */
    {0x4005C200,0x140}, /* TX_DIG_CTRL */
    {0x4005C204,0x7FFF55AA}, /* TX_DATA_PAD_PAT */
    {0x4005C208,0x3014000}, /* TX_GFSK_MOD_CTRL */
    {0x4005C20C,0xC0630401}, /* TX_GFSK_COEFF2 */
    {0x4005C210,0xBB29960D}, /* TX_GFSK_COEFF1 */
    {0x4005C214,0x7FF1800}, /* TX_FSK_MOD_SCALE */
    {0x4005C218,0x0}, /* TX_DFT_MOD_PAT */
    {0x4005C21C,0x10000FFF}, /* TX_DFT_TONE_0_1 */
    {0x4005C220,0x1E0001FF}, /* TX_DFT_TONE_2_3 */
    {0x4005C228,0x0}, /* PLL_MOD_OVRD */
    {0x4005C22C,0x200}, /* PLL_CHAN_MAP */
    {0x4005C230,0x202600}, /* PLL_LOCK_DETECT */
    {0x4005C234,0x840000}, /* PLL_HP_MOD_CTRL */
    {0x4005C244,0x1FF0000}, /* PLL_HPM_SDM_FRACTION */
    {0x4005C248,0x8080000}, /* PLL_LP_MOD_CTRL */
    {0x4005C24C,0x260026}, /* PLL_LP_SDM_CTRL1 */
    {0x4005C250,0x2000000}, /* PLL_LP_SDM_CTRL2 */
    {0x4005C254,0x4000000}, /* PLL_LP_SDM_CTRL3 */
    {0x4005C260,0x201}, /* PLL_DELAY_MATCH */
    {0x4005C288,0x0}, /* SOFT_RESET */
    {0x4005C290,0x0}, /* OVERWRITE_VER */
    {0x4005C294,0x0}, /* DMA_CTRL */
    {0x4005C29C,0x0}, /* DTEST_CTRL */
    {0x4005C2A0,0x0}, /* PB_CTRL */
    {0x4005C2C0,0xFF000000}, /* TSM_CTRL */
    {0x4005C2C4,0x65646A67}, /* END_OF_SEQ */
    {0x4005C2C8,0x0}, /* TSM_OVRD0 */
    {0x4005C2CC,0x0}, /* TSM_OVRD1 */
    {0x4005C2D0,0x0}, /* TSM_OVRD2 */
    {0x4005C2D4,0x0}, /* TSM_OVRD3 */
    {0x4005C2D8,0x0}, /* PA_POWER */
    {0x4005C2DC,0x0}, /* PA_BIAS_TBL0 */
    {0x4005C2E0,0x0}, /* PA_BIAS_TBL1 */
    {0x4005C2E4,0x826}, /* RECYCLE_COUNT */
    {0x4005C2E8,0x65006A00}, /* TSM_TIMING00 */
    {0x4005C2EC,0x65006A00}, /* TSM_TIMING01 */
    {0x4005C2F0,0x65006A00}, /* TSM_TIMING02 */
    {0x4005C2F4,0x65006A00}, /* TSM_TIMING03 */
    {0x4005C2F8,0x6500FFFF}, /* TSM_TIMING04 */
    {0x4005C2FC,0x650B6A3F}, /* TSM_TIMING05 */
    {0x4005C300,0x651AFFFF}, /* TSM_TIMING06 */
    {0x4005C304,0x1A004E00}, /* TSM_TIMING07 */
    {0x4005C308,0x65336867}, /* TSM_TIMING08 */
    {0x4005C30C,0x65056A05}, /* TSM_TIMING09 */
    {0x4005C310,0x6509FFFF}, /* TSM_TIMING10 */
    {0x4005C314,0xFFFF6A09}, /* TSM_TIMING11 */
    {0x4005C318,0xFFFF6A64}, /* TSM_TIMING12 */
    {0x4005C31C,0x651A6A4E}, /* TSM_TIMING13 */
    {0x4005C320,0x650AFFFF}, /* TSM_TIMING14 */
    {0x4005C324,0xFFFF6A0A}, /* TSM_TIMING15 */
    {0x4005C328,0x1A104E44}, /* TSM_TIMING16 */
    {0x4005C32C,0x65106A44}, /* TSM_TIMING17 */
    {0x4005C330,0x6505FFFF}, /* TSM_TIMING18 */
    {0x4005C334,0xFFFF6864}, /* TSM_TIMING19 */
    {0x4005C338,0x651AFFFF}, /* TSM_TIMING20 */
    {0x4005C33C,0x651AFFFF}, /* TSM_TIMING21 */
    {0x4005C340,0x651AFFFF}, /* TSM_TIMING22 */
    {0x4005C344,0x651AFFFF}, /* TSM_TIMING23 */
    {0x4005C348,0x6518FFFF}, /* TSM_TIMING24 */
    {0x4005C34C,0x6518FFFF}, /* TSM_TIMING25 */
    {0x4005C350,0x65096A09}, /* TSM_TIMING26 */
    {0x4005C354,0xFFFF6A67}, /* TSM_TIMING27 */
    {0x4005C358,0x6562FFFF}, /* TSM_TIMING28 */
    {0x4005C35C,0x6362FFFF}, /* TSM_TIMING29 */
    {0x4005C360,0x65106A44}, /* TSM_TIMING30 */
    {0x4005C364,0x6562FFFF}, /* TSM_TIMING31 */
    {0x4005C368,0x6526FFFF}, /* TSM_TIMING32 */
    {0x4005C36C,0x2726FFFF}, /* TSM_TIMING33 */
    {0x4005C370,0x65336865}, /* TSM_TIMING34 */
    {0x4005C374,0xFFFFFFFF}, /* TSM_TIMING35 */
    {0x4005C378,0xFFFFFFFF}, /* TSM_TIMING36 */
    {0x4005C37C,0xFFFFFFFF}, /* TSM_TIMING37 */
    {0x4005C380,0xFFFFFFFF}, /* TSM_TIMING38 */
    {0x4005C384,0xFFFFFFFF}, /* TSM_TIMING39 */
    {0x4005C388,0xFFFFFFFF}, /* TSM_TIMING40 */
    {0x4005C38C,0xFFFFFFFF}, /* TSM_TIMING41 */
    {0x4005C390,0xFFFFFFFF}, /* TSM_TIMING42 */
    {0x4005C394,0xFFFFFFFF}, /* TSM_TIMING43 */
    {0x4005C3C0,0x482}, /* CORR_CTRL */
    {0x4005C3C4,0x1}, /* PN_TYPE */
    {0x4005C3C8,0x744AC39B}, /* PN_CODE */
    {0x4005C3CC,0x8}, /* SYNC_CTRL */
    {0x4005C3D0,0x0}, /* SNF_THR */
    {0x4005C3D4,0x82}, /* FAD_THR */
    {0x4005C3D8,0x1}, /* ZBDEM_AFC */
    {0x4005C3DC,0x0}, /* LPPS_CTRL */
    {0x4005C400,0xFFFF0001}, /* ADC_CTRL */
    {0x4005C404,0x880033}, /* ADC_TUNE */
    {0x4005C408,0x43033033}, /* ADC_ADJ */
    {0x4005C40C,0x0}, /* ADC_REGS */
    {0x4005C410,0x444}, /* ADC_TRIMS */
    {0x4005C414,0x0}, /* ADC_TEST_CTRL */
    {0x4005C420,0x173}, /* BBF_CTRL */
    {0x4005C42C,0x0}, /* RX_ANA_CTRL */
    {0x4005C434,0xACAC177}, /* XTAL_CTRL */
    {0x4005C438,0x1000}, /* XTAL_CTRL2 */
    {0x4005C43C,0x87}, /* BGAP_CTRL */
    {0x4005C444,0x24}, /* PLL_CTRL */
    {0x4005C448,0x24}, /* PLL_CTRL2 */
    {0x4005C44C,0x0}, /* PLL_TEST_CTRL */
    {0x4005C458,0x0}, /* QGEN_CTRL */
    {0x4005C464,0x0}, /* TCA_CTRL */
    {0x4005C468,0x44}, /* TZA_CTRL */
    {0x4005C474,0x0}, /* TX_ANA_CTRL */
    {0x4005C47C,0x0}, /* ANA_SPARE */
};

/* These are the recommended overwrites that are specific to the 802.15.4 radio posture */
overwrites_t const overwrites_802p15p4[] =
{
    /* Default Values */
    {0x4005C000,0x220}, /* RX_DIG_CTRL */
    {0x4005C1A0,0x0}, /* RX_CHF_COEF0 */
    {0x4005C1A4,0x0}, /* RX_CHF_COEF1 */
    {0x4005C1A8,0xFF}, /* RX_CHF_COEF2 */
    {0x4005C1AC,0x05}, /* RX_CHF_COEF3 */
    {0x4005C1B0,0xFF}, /* RX_CHF_COEF4 */
    {0x4005C1B4,0xEB}, /* RX_CHF_COEF5 */
    {0x4005C1B8,0x06}, /* RX_CHF_COEF6 */
    {0x4005C1BC,0x4C}, /* RX_CHF_COEF7 */
};

/* These are the recommended overwrites that are specific to BLE */
overwrites_t const overwrites_ble[] =
{
    /* Default Values */
    {0x4005C000,0x110}, /* RX_DIG_CTRL */
    {0x4005C1A0,0x0}, /* RX_CHF_COEF0 */
    {0x4005C1A4,0xFF}, /* RX_CHF_COEF1 */
    {0x4005C1A8,0xFD}, /* RX_CHF_COEF2 */
    {0x4005C1AC,0xFB}, /* RX_CHF_COEF3 */
    {0x4005C1B0,0xFE}, /* RX_CHF_COEF4 */
    {0x4005C1B4,0x0A}, /* RX_CHF_COEF5 */
    {0x4005C1B8,0x1B}, /* RX_CHF_COEF6 */
    {0x4005C1BC,0x27}, /* RX_CHF_COEF7 */
};

/* These are the overwrites values from IFR to be placed in Radio Address Space */
const overwrites_ifr radio_trim_ifr[] =
{
    {0x84,12,4,0x4005C468,0,2}, /* TZA_CTRL[TZA_CAP_TUNE] */
    {0x84,8,4,0x4005C420,0,2}, /* BBF_CTRL[BBF_CAP_TUNE] */
    {0x84,4,4,0x4005C420,4,2}, /* BBF_CTRL[BBF_RES_TUNE2] */
    {0x98,20,11,0x4005C050,0,2}, /* IQMC_CAL[IQMC_GAIN_ADJ] */
    {0x98,8,12,0x4005C050,16,2}, /* IQMC_CAL[IQMC_PHASE_ADJ] */
    {0x9C,28,4,0x4005C43C,4,2}, /* BGAP_CTRL[BGAP_VOLTAGE] */
    {0x9C,24,4,0x4005C43C,0,2}, /* BGAP_CTRL[BGAP_CURRENT] */
    {0x9C,20,4,0x4005C438,0,2}, /* XTAL_CTRL2[XTAL_REG_SUPPLY] */
    {0x9C,16,4,0x4005C444,8,2}, /* PLL_CTRL[PLL_REG_SUPPLY] */
    {0x9C,12,4,0x4005C458,0,2}, /* QGEN_CTRL[QGEN_REG_SUPPLY] */
    {0x9C,8,4,0x4005C464,4,2}, /* TCA_CTRL[TCA_TX_REG_SUPPLY] */
    {0x9C,4,4,0x4005C48C,4,2}, /* ADC_REGS[ADC_REG_DIG_SUPPLY] */
    {0x9C,0,4,0x4005C48C,0,2}, /* ADC_REGS[ADC_ANA_REG_SUPPLY] */
};

#endif /* OVERWRITES_H_ */