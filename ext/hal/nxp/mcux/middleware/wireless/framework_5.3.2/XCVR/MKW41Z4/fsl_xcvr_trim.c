/*
* Copyright (c) 2016, Freescale Semiconductor, Inc.
* All rights reserved.
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

#include "fsl_device_registers.h"
#include "fsl_common.h"
#include "fsl_xcvr.h"
#include "fsl_xcvr_trim.h"
#include "dbg_ram_capture.h"
#include "math.h"

/*******************************************************************************
* Definitions
******************************************************************************/


/*******************************************************************************
* Prototypes
******************************************************************************/
void DC_Measure_short(IQ_t chan, DAC_SWEEP_STEP2_t dcoc_init_val);
float calc_dcoc_dac_step(GAIN_CALC_TBL_ENTRY2_T * meas_ptr, GAIN_CALC_TBL_ENTRY2_T * baseline_meas_ptr );


/*******************************************************************************
* Variables
******************************************************************************/
const int8_t TsettleCal = 10;
static GAIN_CALC_TBL_ENTRY2_T measurement_tbl2[NUM_I_Q_CHAN][NUM_SWEEP_STEP_ENTRIES2]; 
static const int8_t sweep_step_values2[NUM_SWEEP_STEP_ENTRIES2] = 
{ 
    0,     /* Baseline entry is first and not used in this table */
    -16,
    +16,
    -4,
    -4,
    -4,
    -4,
    -4,
    -4,
    -4,
    -4,
    -4,
    -4,
    -4,
    +4,
    +4,
    +4,
    +4,
    +4,
    +4,
    +4,
    +4,
    +4,
    +4,
    +4
};


/*******************************************************************************
* Code
******************************************************************************/
/*! *********************************************************************************
* \brief  This function performs a trim of the BBA DCOC DAC on the DUT
*
* \return status - 1 if passed, 0 if failed.
*
* \ingroup PublicAPIs
*
* \details
*   Requires the RX to be warmed up before this function is called.
*   
***********************************************************************************/
uint8_t rx_bba_dcoc_dac_trim_shortIQ(void) 
{
    uint8_t i;
    float temp_mi = 0;
    float temp_mq = 0;
    float temp_pi = 0;
    float temp_pq = 0;
    float temp_step = 0;
    uint8_t bbf_dacinit_i, bbf_dacinit_q;

    uint32_t dcoc_init_reg_value_dcgain = 0x80802020;  /* Used in 2nd & 3rd Generation DCOC Trims only. */
    uint32_t bbf_dcoc_step;
    uint32_t bbf_dcoc_step_rcp;
    TZAdcocstep_t tza_dcoc_step[11];
    uint8_t status = 0;
    
    /* Save register values. */  
    uint32_t dcoc_ctrl_0_stack;
    uint32_t dcoc_ctrl_1_stack;
    uint32_t agc_ctrl_1_stack;
    uint32_t rx_dig_ctrl_stack;
    uint32_t dcoc_cal_gain_state;
    
    XcvrCalDelay(1000);
    dcoc_ctrl_0_stack = XCVR_RX_DIG->DCOC_CTRL_0;  /* Save state of DCOC_CTRL_0 for later restore. */
    dcoc_ctrl_1_stack = XCVR_RX_DIG->DCOC_CTRL_1;  /* Save state of DCOC_CTRL_1 for later restore. */
    rx_dig_ctrl_stack =  XCVR_RX_DIG->RX_DIG_CTRL;  /* Save state of RX_DIG_CTRL for later restore. */
    agc_ctrl_1_stack = XCVR_RX_DIG->AGC_CTRL_1;  /* Save state of RX_DIG_CTRL for later restore. */
    dcoc_cal_gain_state = XCVR_RX_DIG->DCOC_CAL_GAIN;  /* Save state of DCOC_CAL_GAIN for later restore. */
    
    /* Ensure AGC, DCOC and RX_DIG_CTRL is in correct mode. */
    XCVR_RX_DIG->RX_DIG_CTRL = (XCVR_RX_DIG->RX_DIG_CTRL & ~XCVR_RX_DIG_RX_DIG_CTRL_RX_AGC_EN_MASK) | XCVR_RX_DIG_RX_DIG_CTRL_RX_AGC_EN(0);       // Turn OFF AGC
    
    XCVR_RX_DIG->AGC_CTRL_1 = (XCVR_RX_DIG->AGC_CTRL_1 & ~XCVR_RX_DIG_AGC_CTRL_1_USER_LNA_GAIN_EN_MASK) | XCVR_RX_DIG_AGC_CTRL_1_USER_LNA_GAIN_EN(1) ;  // Set LNA Manual Gain
    XCVR_RX_DIG->AGC_CTRL_1 = (XCVR_RX_DIG->AGC_CTRL_1 & ~XCVR_RX_DIG_AGC_CTRL_1_USER_BBA_GAIN_EN_MASK) | XCVR_RX_DIG_AGC_CTRL_1_USER_BBA_GAIN_EN(1) ;  // Set BBA Manual Gain
    
    XCVR_RX_DIG->RX_DIG_CTRL = (XCVR_RX_DIG->RX_DIG_CTRL & ~XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_CAL_EN_MASK) | XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_CAL_EN(0);     //Enable HW DC Calibration -- Disable for SW-DCOC
    XCVR_RX_DIG->DCOC_CTRL_0 = (XCVR_RX_DIG->DCOC_CTRL_0 & ~XCVR_RX_DIG_DCOC_CTRL_0_DCOC_MAN_MASK) | XCVR_RX_DIG_DCOC_CTRL_0_DCOC_MAN(1);         //  Enable  Manual DCOC
    /* DCOC_CTRL_0 @ 4005_C02C -- Define default DCOC DAC settings in manual mode. */
    XCVR_RX_DIG->DCOC_DAC_INIT = XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_I(0x20) | XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_Q(0x20) |  XCVR_RX_DIG_DCOC_DAC_INIT_TZA_DCOC_INIT_I(0x80) | XCVR_RX_DIG_DCOC_DAC_INIT_TZA_DCOC_INIT_Q(0x80);
    /* Set DCOC Tracking State. */
    XCVR_RX_DIG->DCOC_CTRL_0 = (XCVR_RX_DIG->DCOC_CTRL_0 & ~XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_SRC_MASK) | XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_SRC(0);   //  Disables DCOC Tracking when set to 0
    /* Apply Manual Gain. */
    XCVR_RX_DIG->AGC_CTRL_1 = XCVR_RX_DIG_AGC_CTRL_1_USER_LNA_GAIN_EN(1) | XCVR_RX_DIG_AGC_CTRL_1_USER_BBA_GAIN_EN(1) | XCVR_RX_DIG_AGC_CTRL_1_LNA_USER_GAIN(0x02) | XCVR_RX_DIG_AGC_CTRL_1_BBA_USER_GAIN(0x00) ;
    XcvrCalDelay(TsettleCal);
    
    dcoc_init_reg_value_dcgain = XCVR_RX_DIG->DCOC_DAC_INIT;  /* Capture DC null setting. */
    
    bbf_dacinit_i = (dcoc_init_reg_value_dcgain & 0x000000FFU);
    bbf_dacinit_q = (dcoc_init_reg_value_dcgain & 0x0000FF00U) >> 8;
    
    DC_Measure_short(I_CHANNEL, NOMINAL2);
    DC_Measure_short(Q_CHANNEL, NOMINAL2);
    
    /* SWEEP Q CHANNEL */
    /* BBF NEG STEP */
    XCVR_RX_DIG->DCOC_DAC_INIT = (XCVR_RX_DIG->DCOC_DAC_INIT & ~XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_Q_MASK) | XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_Q(bbf_dacinit_q - 16);
    XcvrCalDelay(TsettleCal);
    DC_Measure_short(Q_CHANNEL, BBF_NEG);
    
    /* BBF POS STEP */
    XCVR_RX_DIG->DCOC_DAC_INIT = (XCVR_RX_DIG->DCOC_DAC_INIT & ~XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_Q_MASK) | XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_Q(bbf_dacinit_q + 16);
    XcvrCalDelay(TsettleCal);
    DC_Measure_short(Q_CHANNEL, BBF_POS);
    
    XCVR_RX_DIG->DCOC_DAC_INIT = dcoc_init_reg_value_dcgain;              /* Return DAC setting to initial. */
    XcvrCalDelay(TsettleCal);  
    
    /* SWEEP I CHANNEL */
    /* BBF NEG STEP */
    XCVR_RX_DIG->DCOC_DAC_INIT = (XCVR_RX_DIG->DCOC_DAC_INIT & ~XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_I_MASK) | XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_I(bbf_dacinit_i - 16); 
    XcvrCalDelay(TsettleCal);
    DC_Measure_short(I_CHANNEL, BBF_NEG);
    /* BBF POS STEP  */
    XCVR_RX_DIG->DCOC_DAC_INIT = (XCVR_RX_DIG->DCOC_DAC_INIT & ~XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_I_MASK) | XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_I(bbf_dacinit_i + 16); 
    XcvrCalDelay(TsettleCal);
    DC_Measure_short(I_CHANNEL, BBF_POS);
    
    XCVR_RX_DIG->DCOC_DAC_INIT = dcoc_init_reg_value_dcgain;              /* Return DACs to initial. */
    XcvrCalDelay(TsettleCal);
    
    /* Calculate BBF DCOC STEPS, RECIPROCALS */
    temp_mi = calc_dcoc_dac_step(&measurement_tbl2[I_CHANNEL][BBF_NEG], &measurement_tbl2[I_CHANNEL][NOMINAL2]);
    temp_mq = calc_dcoc_dac_step(&measurement_tbl2[Q_CHANNEL][BBF_NEG], &measurement_tbl2[Q_CHANNEL][NOMINAL2]);
    temp_pi = calc_dcoc_dac_step(&measurement_tbl2[I_CHANNEL][BBF_POS], &measurement_tbl2[I_CHANNEL][NOMINAL2]);
    temp_pq = calc_dcoc_dac_step(&measurement_tbl2[Q_CHANNEL][BBF_POS], &measurement_tbl2[Q_CHANNEL][NOMINAL2]);
    
    temp_step = (temp_mi+temp_pi+temp_mq+temp_pq) / 4;
    
    bbf_dcoc_step = (uint32_t)roundf(temp_step * 8U);
    
    if ((bbf_dcoc_step > 265) & (bbf_dcoc_step < 305)) 
    {
        bbf_dcoc_step_rcp = (uint32_t)roundf((float)0x8000U / temp_step);
        
        /* Calculate TZA DCOC STEPS & RECIPROCALS and IQ_DC_GAIN_MISMATCH. */
        for (i = TZA_STEP_N0; i <= TZA_STEP_N10; i++)                               /* Relying on enumeration ordering. */
        {
            /* Calculate TZA DCOC STEPSIZE & its RECIPROCAL. */
            switch(i){
            case TZA_STEP_N0: 
                temp_step = (bbf_dcoc_step>>3U)/3.6F;
                break;
            case TZA_STEP_N1: 
                temp_step = temp_step * (xcvr_common_config.dcoc_tza_step_01_init >> 16)/(xcvr_common_config.dcoc_tza_step_00_init >> 16); 
                break;
            case TZA_STEP_N2: 
                temp_step = temp_step * (xcvr_common_config.dcoc_tza_step_02_init >> 16)/(xcvr_common_config.dcoc_tza_step_01_init >> 16); 
                break;
            case TZA_STEP_N3: 
                temp_step = temp_step * (xcvr_common_config.dcoc_tza_step_03_init >> 16)/(xcvr_common_config.dcoc_tza_step_02_init >> 16); 
                break;
            case TZA_STEP_N4: 
                temp_step = temp_step * (xcvr_common_config.dcoc_tza_step_04_init >> 16)/(xcvr_common_config.dcoc_tza_step_03_init >> 16); 
                break;
            case TZA_STEP_N5: 
                temp_step = temp_step * (xcvr_common_config.dcoc_tza_step_05_init >> 16)/(xcvr_common_config.dcoc_tza_step_04_init >> 16); 
                break;
            case TZA_STEP_N6: 
                temp_step = temp_step * (xcvr_common_config.dcoc_tza_step_06_init >> 16)/(xcvr_common_config.dcoc_tza_step_05_init >> 16);
                break;
            case TZA_STEP_N7: 
                temp_step = temp_step * (xcvr_common_config.dcoc_tza_step_07_init >> 16)/(xcvr_common_config.dcoc_tza_step_06_init >> 16); 
                break;
            case TZA_STEP_N8: 
                temp_step = temp_step * (xcvr_common_config.dcoc_tza_step_08_init >> 16)/(xcvr_common_config.dcoc_tza_step_07_init >> 16); 
                break;
            case TZA_STEP_N9: 
                temp_step = temp_step * (xcvr_common_config.dcoc_tza_step_09_init >> 16)/(xcvr_common_config.dcoc_tza_step_08_init >> 16); 
                break;
            case TZA_STEP_N10: 
                temp_step = temp_step * (xcvr_common_config.dcoc_tza_step_10_init >> 16)/(xcvr_common_config.dcoc_tza_step_09_init >> 16); 
                break;
            default:
                break;
            }
            
            tza_dcoc_step[i-TZA_STEP_N0].dcoc_step = (uint32_t)roundf(temp_step * 8);
            tza_dcoc_step[i-TZA_STEP_N0].dcoc_step_rcp = (uint32_t)roundf((float)0x8000 / temp_step);
        }
        
        /* Make the trims active. */
        XCVR_RX_DIG->DCOC_BBA_STEP = XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP(bbf_dcoc_step) | XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP_RECIP(bbf_dcoc_step_rcp) ;
        XCVR_RX_DIG->DCOC_TZA_STEP_0 = XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_GAIN_0(tza_dcoc_step[0].dcoc_step) | XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_RCP_0(tza_dcoc_step[0].dcoc_step_rcp) ;
        XCVR_RX_DIG->DCOC_TZA_STEP_1 = XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_GAIN_1(tza_dcoc_step[1].dcoc_step) | XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_RCP_1(tza_dcoc_step[1].dcoc_step_rcp) ;
        XCVR_RX_DIG->DCOC_TZA_STEP_2 = XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_GAIN_2(tza_dcoc_step[2].dcoc_step) | XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_RCP_2(tza_dcoc_step[2].dcoc_step_rcp) ;
        XCVR_RX_DIG->DCOC_TZA_STEP_3 = XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_GAIN_3(tza_dcoc_step[3].dcoc_step) | XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_RCP_3(tza_dcoc_step[3].dcoc_step_rcp) ;
        XCVR_RX_DIG->DCOC_TZA_STEP_4 = XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_GAIN_4(tza_dcoc_step[4].dcoc_step) | XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_RCP_4(tza_dcoc_step[4].dcoc_step_rcp) ;
        XCVR_RX_DIG->DCOC_TZA_STEP_5 = XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_GAIN_5(tza_dcoc_step[5].dcoc_step) | XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_RCP_5(tza_dcoc_step[5].dcoc_step_rcp) ;
        XCVR_RX_DIG->DCOC_TZA_STEP_6 = XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_GAIN_6(tza_dcoc_step[6].dcoc_step) | XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_RCP_6(tza_dcoc_step[6].dcoc_step_rcp) ;
        XCVR_RX_DIG->DCOC_TZA_STEP_7 = XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_GAIN_7(tza_dcoc_step[7].dcoc_step) | XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_RCP_7(tza_dcoc_step[7].dcoc_step_rcp) ;
        XCVR_RX_DIG->DCOC_TZA_STEP_8 = XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_GAIN_8(tza_dcoc_step[8].dcoc_step) | XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_RCP_8(tza_dcoc_step[8].dcoc_step_rcp) ;
        XCVR_RX_DIG->DCOC_TZA_STEP_9 = XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_GAIN_9(tza_dcoc_step[9].dcoc_step) | XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_RCP_9(tza_dcoc_step[9].dcoc_step_rcp) ;
        XCVR_RX_DIG->DCOC_TZA_STEP_10 = XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_GAIN_10(tza_dcoc_step[10].dcoc_step) | XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_RCP_10(tza_dcoc_step[10].dcoc_step_rcp) ;
        
        status = 1;  /* Success */
    }
    else
    {
        status = 0;  /* Failure */
    }

    /* Restore Registers. */
    XCVR_RX_DIG->DCOC_CTRL_0 = dcoc_ctrl_0_stack;  /* Restore DCOC_CTRL_0 state to prior settings. */
    XCVR_RX_DIG->DCOC_CTRL_1 = dcoc_ctrl_1_stack;  /* Restore DCOC_CTRL_1 state to prior settings. */
    XCVR_RX_DIG->RX_DIG_CTRL = rx_dig_ctrl_stack;  /* Restore RX_DIG_CTRL state to prior settings. */
    XCVR_RX_DIG->DCOC_CAL_GAIN = dcoc_cal_gain_state;  /* Restore DCOC_CAL_GAIN state to prior setting. */
    XCVR_RX_DIG->AGC_CTRL_1 = agc_ctrl_1_stack;  /* Save state of RX_DIG_CTRL for later restore. */
        
    return status;
}


/*! *********************************************************************************
* \brief  This function performs one point of the DC GAIN calibration process on the DUT
*
* \param[in] chan - whether the I or Q channel is being tested.
* \param[in] stage - whether the BBF or TZA gain stage is being tested.
* \param[in] dcoc_init_val - the value being set in the ***DCOC_INIT_* register by the parent.
* \param[in] ext_measmt - the external measurement (in milliVolts) captured by the parent after the ***DCOC_INIT_* register was setup.
*
* \ingroup PublicAPIs
*
* \details
*   Relies on a static array to store each point of data for later processing in ::DC_GainCalc().
*   
***********************************************************************************/
void DC_Measure_short(IQ_t chan, DAC_SWEEP_STEP2_t dcoc_init_val)
{
    int16_t dc_meas_i = 0;
    int16_t dc_meas_q = 0;
    int16_t sum_dc_meas_i = 0;
    int16_t sum_dc_meas_q = 0;
    
    {
        int8_t i;
        const int8_t iterations = 1;
        sum_dc_meas_i = 0;
        sum_dc_meas_q = 0;
        
        for (i = 0; i < iterations; i++)
        {
            rx_dc_sample_average(&dc_meas_i, &dc_meas_q);
            sum_dc_meas_i = sum_dc_meas_i + dc_meas_i;
            sum_dc_meas_q = sum_dc_meas_q + dc_meas_q;
        }
        sum_dc_meas_i = sum_dc_meas_i/iterations;  
        sum_dc_meas_q = sum_dc_meas_q/iterations;  
    }
    
    measurement_tbl2[chan][dcoc_init_val].step_value = sweep_step_values2[dcoc_init_val];
    
    if (chan == I_CHANNEL)
    {
        measurement_tbl2[chan][dcoc_init_val].internal_measurement = dc_meas_i;
    }
    else
    {
        measurement_tbl2[chan][dcoc_init_val].internal_measurement = dc_meas_q;
    }
}  

/*! *********************************************************************************
* \brief  This function calculates one point of DC DAC step based on digital samples of I or Q.
*
* \param[in] meas_ptr - pointer to the structure containing the measured data from internal measurement.
* \param[in] baseline_meas_ptr - pointer to the structure containing the baseline measured data from internal measurement.
*
* \return result of the calculation, the measurement DCOC DAC step value for this measurement point.
*   
***********************************************************************************/
float calc_dcoc_dac_step(GAIN_CALC_TBL_ENTRY2_T * meas_ptr, GAIN_CALC_TBL_ENTRY2_T * baseline_meas_ptr )
{
    static int16_t norm_dc_code;
    static float dc_step;
    
    /* Normalize internal measurement */
    norm_dc_code = meas_ptr->internal_measurement-baseline_meas_ptr->internal_measurement;
    dc_step = (float)(norm_dc_code)/(float)(meas_ptr->step_value);
    dc_step = (dc_step < 0)? -dc_step: dc_step;
    
    return dc_step;
}


/*! *********************************************************************************
* \brief  Temporary delay function 
*
* \param[in] none.
*
* \return none.
*
* \details
*
***********************************************************************************/
void XcvrCalDelay(uint32_t time)
{
    while(time * 32 > 0)  /* Time delay is roughly in uSec. */
    { 
        time--;
    }
}    

/*! *********************************************************************************
* \brief  This function calculates the average (DC value) based on a smaller set of digital samples of I and Q.
*
* \param[in] i_avg - pointer to the location for storing the calculated average for I channel samples.
* \param[in] q_avg - pointer to the location for storing the calculated average for Q channel samples.
*
***********************************************************************************/
void rx_dc_sample_average(int16_t * i_avg, int16_t * q_avg) 
{
    static uint32_t samples[128];  /* 544*2*2 (entire packet ram1/2 size) */
    uint16_t i;
    uint32_t rx_sample;
    uint16_t * sample_ptr;
    uint32_t temp, end_of_rx_wu;
    uint32_t num_iq_samples;
    float avg_i = 0;
    float avg_q = 0;
    
    num_iq_samples = 128;
    
    /* Clear the entire allocated sample buffer */
    for (i = 0; i < num_iq_samples; i++)
    {
        samples[i]=0;
    }
    
    /* Assume this has been called *AFTER* RxWu has completed. */
    /* XCVR_ForceRxWu(); */
    
    /* Wait for TSM to reach the end of warmup (unless you want to capture some samples during DCOC cal phase) */
    temp = XCVR_TSM->END_OF_SEQ;
    end_of_rx_wu = (temp & XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_MASK) >> XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_SHIFT;
    while ((( XCVR_MISC->XCVR_STATUS & XCVR_CTRL_XCVR_STATUS_TSM_COUNT_MASK) >> XCVR_CTRL_XCVR_STATUS_TSM_COUNT_SHIFT ) != end_of_rx_wu) {};
    
    dbg_ram_init();
    /* Argument below is # of bytes, so *2 (I+Q) and *2 (2bytes/sample) */
    (void)dbg_ram_capture(DBG_PAGE_RXDIGIQ, num_iq_samples*2*2, &samples[0]);
    
    /* Sign extend the IQ samples in place in the sample buffer. */
    
    sample_ptr = (uint16_t *)(&samples[0]);
    for (i = 0; i < num_iq_samples*2; i++)
    {
        rx_sample = *sample_ptr;
        rx_sample |= ((rx_sample & 0x800U) ? 0xF000U : 0x0U); /* Sign extend from 12 to 16 bits. */
        *sample_ptr = rx_sample;
        sample_ptr++;   
    }
    
    sample_ptr = (uint16_t *)(&samples[0]);    
    for (i = 0; i < num_iq_samples*2; i+=2)
    {
        static int16_t i_value;
        static int16_t q_value;
        
        /* Average I & Q channels separately. */
        i_value = *(sample_ptr+i);  /* Sign extend from 12 to 16 bits. */
        q_value = *(sample_ptr+i+1);  /* Sign extend from 12 to 16 bits. */
        avg_i += ((float)i_value - avg_i) / (float)(i+1);  /* Rolling average I */
        avg_q += ((float)q_value - avg_q) / (float)(i+1);  /* Rolling average Q */
    }
    XcvrCalDelay(10);
    *i_avg = (int16_t)avg_i;
    *q_avg = (int16_t)avg_q;
}

/*! *********************************************************************************
* \brief  This function calculates the average (DC value) based on a larger set of digital samples of I and Q.
*
* \param[in] i_avg - pointer to the location for storing the calculated average for I channel samples.
* \param[in] q_avg - pointer to the location for storing the calculated average for Q channel samples.
*
***********************************************************************************/
void rx_dc_sample_average_long(int16_t * i_avg, int16_t * q_avg) {
    static uint32_t samples[512];  /* 544*2*2 (entire packet ram1/2 size) */
    uint16_t i;
    uint32_t rx_sample;
    uint16_t * sample_ptr;
    uint32_t temp, end_of_rx_wu;
    uint32_t num_iq_samples;
    float avg_i = 0;
    float avg_q = 0;
    
    num_iq_samples = 512;
    
    /* Clear the entire allocated sample buffer. */
    for (i = 0; i < num_iq_samples; i++)
    {
        samples[i]=0;
    }
    
    /* Assume this has been called *AFTER* RxWu has completed. */
    /* XCVR_ForceRxWu(); */
    
    /* Wait for TSM to reach the end of warmup (unless you want to capture some samples during DCOC cal phase). */
    temp = XCVR_TSM->END_OF_SEQ;
    end_of_rx_wu = (temp & XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_MASK) >> XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_SHIFT;
    while ((( XCVR_MISC->XCVR_STATUS & XCVR_CTRL_XCVR_STATUS_TSM_COUNT_MASK) >> XCVR_CTRL_XCVR_STATUS_TSM_COUNT_SHIFT ) != end_of_rx_wu) {};
    
    dbg_ram_init();
    /* Argument below is # of bytes, so *2 (I+Q) and *2 (2bytes/sample) */
    (void)dbg_ram_capture(DBG_PAGE_RXDIGIQ, num_iq_samples*2*2, &samples[0]);
    
    /* Sign extend the IQ samples in place in the sample buffer. */
    
    sample_ptr = (uint16_t *)(&samples[0]);
    for (i = 0; i < num_iq_samples*2; i++)
    {
        rx_sample = *sample_ptr;
        rx_sample |= ((rx_sample & 0x800U) ? 0xF000U : 0x0U); /* Sign extend from 12 to 16 bits. */
        *sample_ptr = rx_sample;
        sample_ptr++;   
    }
    
    sample_ptr = (uint16_t *)(&samples[0]);    
    for (i = 0; i < num_iq_samples*2; i+=2)
    {
        static int16_t i_value;
        static int16_t q_value;
        
        /* Average I & Q channels separately. */
        i_value = *(sample_ptr+i);  /* Sign extend from 12 to 16 bits */
        q_value = *(sample_ptr+i+1);  /* Sign extend from 12 to 16 bits */
        avg_i += ((float)i_value - avg_i) / (float)(i+1);  /* Rolling average I */
        avg_q += ((float)q_value - avg_q) / (float)(i+1);  /* Rolling average Q */
    }
    XcvrCalDelay(10);
    *i_avg = (int16_t)avg_i;
    *q_avg = (int16_t)avg_q;
}



