/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file tsm_ll_timing.c
* TSM and LL timing register initialization code for MKW40Z4.
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
* Include
*************************************************************************************
********************************************************************************** */
#include <stdint.h>
#include "tsm_ll_timing.h"
#include "fsl_device_registers.h"

/*! *********************************************************************************
*************************************************************************************
* Constants
*************************************************************************************
********************************************************************************** */
#define USE_RAM_TESTING 0
#define END_OF_SEQ_END_OF_TX_WU_MASK   0xFF
#define END_OF_SEQ_END_OF_TX_WU_SHIFT  0

#define END_OF_SEQ_END_OF_TX_WD_MASK   0xFF00
#define END_OF_SEQ_END_OF_TX_WD_SHIFT  8

#define END_OF_SEQ_END_OF_RX_WU_MASK   0xFF0000
#define END_OF_SEQ_END_OF_RX_WU_SHIFT  16

#define END_OF_SEQ_END_OF_RX_WD_MASK   0xFF000000
#define END_OF_SEQ_END_OF_RX_WD_SHIFT  24

#define BLE_REG_BASE                    0x4005B000
#define TX_RX_ON_DELAY                 ( 0x190 ) /* 32-bit:0x190 address */
#define TX_RX_SYNTH_DELAY              ( 0x198 ) /* 32-bit:0x198 address */

#define PLL_REG_EN_INDEX                (0)
#define PLL_VCO_REG_EN_INDEX            (1)
#define PLL_QGEN_REG_EN_INDEX           (2)
#define PLL_TCA_TX_REG_EN_INDEX         (3)
#define ADC_REG_EN_INDEX                (4)
#define PLL_REF_CLK_EN_INDEX            (5)
#define ADC_CLK_EN_INDEX                (6)
#define PLL_VCO_AUTOTUNE_INDEX          (7)
#define PLL_CYCLE_SLIP_LD_EN_INDEX      (8)
#define PLL_VCO_EN_INDEX                (9)
#define PLL_VCO_BUF_RX_EN_INDEX         (10)
#define PLL_VCO_BUF_TX_EN_INDEX         (11)
#define PLL_PA_BUF_EN_INDEX             (12)
#define PLL_LDV_EN_INDEX                (13)
#define PLL_RX_LDV_RIPPLE_MUX_EN_INDEX  (14)
#define PLL_TX_LDV_RIPPLE_MUX_EN_INDEX  (15)
#define PLL_FILTER_CHARGE_EN_INDEX      (16)
#define PLL_PHDET_EN_INDEX              (17)
#define QGEN25_EN_INDEX                 (18)
#define TX_EN_INDEX                     (19)
#define ADC_EN_INDEX                    (20)
#define ADC_I_Q_EN_INDEX                (21)
#define ADC_DAC_EN_INDEX                (22)
#define ADC_RST_EN_INDEX                (23)
#define BBF_EN_INDEX                    (24)
#define TCA_EN_INDEX                    (25)
#define PLL_DIG_EN_INDEX                (26)
#define TX_DIG_EN_INDEX                 (27)
#define RX_DIG_EN_INDEX                 (28)
#define RX_INIT_INDEX                   (29)
#define SIGMA_DELTA_EN_INDEX            (30)
#define ZBDEM_RX_EN_INDEX               (31)
#define DCOC_EN_INDEX                   (32)
#define DCOC_INIT_EN_INDEX              (33)
#define FREQ_TARG_LD_EN_INDEX           (34)
#define SAR_ADC_TRIG_EN_INDEX           (35)
#define TSM_SPARE0_EN_INDEX             (36)
#define TSM_SPARE1_EN_INDEX             (37)
#define TSM_SPARE2_EN_INDEX             (38)
#define TSM_SPARE03_EN_INDEX            (39)
#define GPIO0_TRIG_EN_INDEX             (40)
#define GPIO1_TRIG_EN_INDEX             (41)
#define GPIO2_TRIG_EN_INDEX             (42)
#define GPIO3_TRIG_EN_INDEX             (43)

/*! *********************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
********************************************************************************** */

/* Array of initialization values for TSM Timing registers.
 * Stored in the same order as the registers so that it can be copied easily.
 */
static const uint32_t ble_tsm_init_blt [NUM_TSM_TIMING_REGS] =
{
    TSM_REG_VALUE(PLL_REG_EN_RX_WD, PLL_REG_EN_RX_WU, PLL_REG_EN_TX_WD, PLL_REG_EN_TX_WU),
    TSM_REG_VALUE(PLL_VCO_REG_EN_RX_WD, PLL_VCO_REG_EN_RX_WU, PLL_VCO_REG_EN_TX_WD, PLL_VCO_REG_EN_TX_WU),
    TSM_REG_VALUE(PLL_QGEN_REG_EN_RX_WD, PLL_QGEN_REG_EN_RX_WU, PLL_QGEN_REG_EN_TX_WD, PLL_QGEN_REG_EN_TX_WU),
    TSM_REG_VALUE(PLL_TCA_TX_REG_EN_RX_WD, PLL_TCA_TX_REG_EN_RX_WU, PLL_TCA_TX_REG_EN_TX_WD, PLL_TCA_TX_REG_EN_TX_WU),
    TSM_REG_VALUE(ADC_REG_EN_RX_WD, ADC_REG_EN_RX_WU, ADC_REG_EN_TX_WD, ADC_REG_EN_TX_WU),
    TSM_REG_VALUE(PLL_REF_CLK_EN_RX_WD, PLL_REF_CLK_EN_RX_WU, PLL_REF_CLK_EN_TX_WD, PLL_REF_CLK_EN_TX_WU),
    TSM_REG_VALUE(ADC_CLK_EN_RX_WD, ADC_CLK_EN_RX_WU, ADC_CLK_EN_TX_WD, ADC_CLK_EN_TX_WU),
    TSM_REG_VALUE(PLL_VCO_AUTOTUNE_RX_WD, PLL_VCO_AUTOTUNE_RX_WU, PLL_VCO_AUTOTUNE_TX_WD, PLL_VCO_AUTOTUNE_TX_WU),
    TSM_REG_VALUE(PLL_CYCLE_SLIP_LD_EN_RX_WD, PLL_CYCLE_SLIP_LD_EN_RX_WU, PLL_CYCLE_SLIP_LD_EN_TX_WD, PLL_CYCLE_SLIP_LD_EN_TX_WU),
    TSM_REG_VALUE(PLL_VCO_EN_RX_WD, PLL_VCO_EN_RX_WU, PLL_VCO_EN_TX_WD, PLL_VCO_EN_TX_WU),
    TSM_REG_VALUE(PLL_VCO_BUF_RX_EN_RX_WD, PLL_VCO_BUF_RX_EN_RX_WU, PLL_VCO_BUF_RX_EN_TX_WD, PLL_VCO_BUF_RX_EN_TX_WU),
    TSM_REG_VALUE(PLL_VCO_BUF_TX_EN_RX_WD, PLL_VCO_BUF_TX_EN_RX_WU, PLL_VCO_BUF_TX_EN_TX_WD, PLL_VCO_BUF_TX_EN_TX_WU),
    TSM_REG_VALUE(PLL_PA_BUF_EN_RX_WD, PLL_PA_BUF_EN_RX_WU, PLL_PA_BUF_EN_TX_WD, PLL_PA_BUF_EN_TX_WU),
    TSM_REG_VALUE(PLL_LDV_EN_RX_WD, PLL_LDV_EN_RX_WU, PLL_LDV_EN_TX_WD, PLL_LDV_EN_TX_WU),
    TSM_REG_VALUE(PLL_RX_LDV_RIPPLE_MUX_EN_RX_WD, PLL_RX_LDV_RIPPLE_MUX_EN_RX_WU, PLL_RX_LDV_RIPPLE_MUX_EN_TX_WD, PLL_RX_LDV_RIPPLE_MUX_EN_TX_WU),
    TSM_REG_VALUE(PLL_TX_LDV_RIPPLE_MUX_EN_RX_WD, PLL_TX_LDV_RIPPLE_MUX_EN_RX_WU, PLL_TX_LDV_RIPPLE_MUX_EN_TX_WD, PLL_TX_LDV_RIPPLE_MUX_EN_TX_WU),
    TSM_REG_VALUE(PLL_FILTER_CHARGE_EN_RX_WD, PLL_FILTER_CHARGE_EN_RX_WU, PLL_FILTER_CHARGE_EN_TX_WD, PLL_FILTER_CHARGE_EN_TX_WU),
    TSM_REG_VALUE(PLL_PHDET_EN_RX_WD, PLL_PHDET_EN_RX_WU, PLL_PHDET_EN_TX_WD, PLL_PHDET_EN_TX_WU),
    TSM_REG_VALUE(QGEN25_EN_RX_WD, QGEN25_EN_RX_WU, QGEN25_EN_TX_WD, QGEN25_EN_TX_WU),
    TSM_REG_VALUE(TX_EN_RX_WD, TX_EN_RX_WU, TX_EN_TX_WD, TX_EN_TX_WU),
    TSM_REG_VALUE(ADC_EN_RX_WD, ADC_EN_RX_WU, ADC_EN_TX_WD, ADC_EN_TX_WU),
    TSM_REG_VALUE(ADC_I_Q_EN_RX_WD, ADC_I_Q_EN_RX_WU, ADC_I_Q_EN_TX_WD, ADC_I_Q_EN_TX_WU),
    TSM_REG_VALUE(ADC_DAC_EN_RX_WD, ADC_DAC_EN_RX_WU, ADC_DAC_EN_TX_WD, ADC_DAC_EN_TX_WU),
    TSM_REG_VALUE(ADC_RST_EN_RX_WD, ADC_RST_EN_RX_WU, ADC_RST_EN_TX_WD, ADC_RST_EN_TX_WU),
    TSM_REG_VALUE(BBF_EN_RX_WD, BBF_EN_RX_WU, BBF_EN_TX_WD, BBF_EN_TX_WU),
    TSM_REG_VALUE(TCA_EN_RX_WD, TCA_EN_RX_WU, TCA_EN_TX_WD, TCA_EN_TX_WU),
    TSM_REG_VALUE(PLL_DIG_EN_RX_WD, PLL_DIG_EN_RX_WU, PLL_DIG_EN_TX_WD, PLL_DIG_EN_TX_WU),
    TSM_REG_VALUE(TX_DIG_EN_RX_WD, TX_DIG_EN_RX_WU, TX_DIG_EN_TX_WD, TX_DIG_EN_TX_WU),
    TSM_REG_VALUE(RX_DIG_EN_RX_WD, RX_DIG_EN_RX_WU, RX_DIG_EN_TX_WD, RX_DIG_EN_TX_WU),
    TSM_REG_VALUE(RX_INIT_RX_WD, RX_INIT_RX_WU, RX_INIT_TX_WD, RX_INIT_TX_WU),
    TSM_REG_VALUE(SIGMA_DELTA_EN_RX_WD, SIGMA_DELTA_EN_RX_WU, SIGMA_DELTA_EN_TX_WD, SIGMA_DELTA_EN_TX_WU),
    TSM_REG_VALUE(ZBDEM_RX_EN_RX_WD, ZBDEM_RX_EN_RX_WU, ZBDEM_RX_EN_TX_WD, ZBDEM_RX_EN_TX_WU),
    TSM_REG_VALUE(DCOC_EN_RX_WD, DCOC_EN_RX_WU, DCOC_EN_TX_WD, DCOC_EN_TX_WU),
    TSM_REG_VALUE(DCOC_INIT_EN_RX_WD, DCOC_INIT_EN_RX_WU, DCOC_INIT_EN_TX_WD, DCOC_INIT_EN_TX_WU),
    TSM_REG_VALUE(FREQ_TARG_LD_EN_RX_WD, FREQ_TARG_LD_EN_RX_WU, FREQ_TARG_LD_EN_TX_WD, FREQ_TARG_LD_EN_TX_WU),
    TSM_REG_VALUE(SAR_ADC_TRIG_EN_RX_WD, SAR_ADC_TRIG_EN_RX_WU, SAR_ADC_TRIG_EN_TX_WD, SAR_ADC_TRIG_EN_TX_WU),
    TSM_REG_VALUE(TSM_SPARE0_EN_RX_WD, TSM_SPARE0_EN_RX_WU, TSM_SPARE0_EN_TX_WD, TSM_SPARE0_EN_TX_WU),
    TSM_REG_VALUE(TSM_SPARE1_EN_RX_WD, TSM_SPARE1_EN_RX_WU, TSM_SPARE1_EN_TX_WD, TSM_SPARE1_EN_TX_WU),
    TSM_REG_VALUE(TSM_SPARE2_EN_RX_WD, TSM_SPARE2_EN_RX_WU, TSM_SPARE2_EN_TX_WD, TSM_SPARE2_EN_TX_WU),
    TSM_REG_VALUE(TSM_SPARE03_EN_RX_WD, TSM_SPARE03_EN_RX_WU, TSM_SPARE03_EN_TX_WD, TSM_SPARE03_EN_TX_WU),
    TSM_REG_VALUE(GPIO0_TRIG_EN_RX_WD, GPIO0_TRIG_EN_RX_WU, GPIO0_TRIG_EN_TX_WD, GPIO0_TRIG_EN_TX_WU),
    TSM_REG_VALUE(GPIO1_TRIG_EN_RX_WD, GPIO1_TRIG_EN_RX_WU, GPIO1_TRIG_EN_TX_WD, GPIO1_TRIG_EN_TX_WU),
    TSM_REG_VALUE(GPIO2_TRIG_EN_RX_WD, GPIO2_TRIG_EN_RX_WU, GPIO2_TRIG_EN_TX_WD, GPIO2_TRIG_EN_TX_WU),
    TSM_REG_VALUE(GPIO3_TRIG_EN_RX_WD, GPIO3_TRIG_EN_RX_WU, GPIO3_TRIG_EN_TX_WD, GPIO3_TRIG_EN_TX_WU),
};

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */

/* Frequency remap initialization performs any required init for both 
 * the LL and TSM registers to implement proper time sequences.
 */
void tsm_ll_timing_init(PHY_RADIO_T mode)
{
    uint8_t i;
#if USE_RAM_TESTING
    /* Test only code which places the timing values in a RAM array */
    static uint32_t tsm_values_test[NUM_TSM_TIMING_REGS];
    uint32_t * tsm_reg_ptr = &tsm_values_test[0];
#else
    /* Normal operational code */
    uint32_t * tsm_reg_ptr = (uint32_t *)&XCVR_TSM_TIMING00;
#endif
    
    /* Setup TSM timing registers with time sequences for BLE unconditionally */
    /* NOTE: This code depends on the array of init values being in the proper order! */
    for (i=0;i<NUM_TSM_TIMING_REGS;i++)
    {
        *tsm_reg_ptr++ = ble_tsm_init_blt[i];
    }
    
    /* Now (if needed) adjust from BLE timings to Zigbee timings since they are VERY similar */
    if (mode == ZIGBEE_RADIO)
    {
        /* TX_DIG_EN must be earlier for Zigbee */
        XCVR_TSM_TIMING27 = TSM_REG_VALUE(TX_DIG_EN_RX_WD, TX_DIG_EN_RX_WU, TX_DIG_EN_TX_WD, TX_DIG_EN_TX_WU-ZB_TX_DIG_ADVANCE);
        
        /* ZBDEM_RX_EN must be setup properly (it is disabled in BLE).
        * It's timing is the same as RX_DIG_EN! 
        */
        XCVR_TSM_TIMING31 = TSM_REG_VALUE(RX_DIG_EN_RX_WD, RX_DIG_EN_RX_WU, RX_DIG_EN_TX_WD, RX_DIG_EN_TX_WU);
    }
    
    /* Setup XCVR registers for END_OF_TX_WU, END_OF_TX_WD, END_OF_RX_WU, END_OF_RX_WD
    * based on the sequence components defined for the TSM timing registers. 
    */
    XCVR_END_OF_SEQ = END_OF_SEQ_VALUE;
    
    if (mode == BLE_RADIO) /* Only do this when in BLE mode! */
    {
        /* Setup LL registers with adjustment for how early to start the TX or RX sequence */
        *(volatile uint16_t *)(BLE_REG_BASE + TX_RX_ON_DELAY) = TX_RX_ON_DELAY_VALUE;
        *(volatile uint16_t *)(BLE_REG_BASE + TX_RX_SYNTH_DELAY) = TX_RX_SYNTH_DELAY_VALUE;
    }
}