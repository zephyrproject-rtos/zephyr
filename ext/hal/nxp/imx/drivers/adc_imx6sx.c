/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
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

#include "adc_imx6sx.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_Init
 * Description   : Initialize ADC to reset state and initialize with initialize
 *                 structure.
 *
 *END**************************************************************************/
void ADC_Init(ADC_Type* base, const adc_init_config_t* initConfig)
{
    assert(initConfig);

    /* Reset ADC register to its default value. */
    ADC_Deinit(base);

    /* Set hardware average function and number. */
    if (initConfig->averageNumber != adcAvgNumNone)
    {
        ADC_GC_REG(base) |= ADC_GC_AVGE_MASK;
        ADC_CFG_REG(base) |= ADC_CFG_AVGS(initConfig->averageNumber);
    }

    /* Set resolution mode. */
    ADC_CFG_REG(base) |= ADC_CFG_MODE(initConfig->resolutionMode);

    /* Set clock source. */
    ADC_SetClockSource(base, initConfig->clockSource, initConfig->divideRatio);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_Deinit
 * Description   : This function reset ADC module register content to its
 *                 default value.
 *
 *END**************************************************************************/
void ADC_Deinit(ADC_Type* base)
{
    /* Reset ADC Module Register content to default value */
    ADC_HC0_REG(base) = ADC_HC0_ADCH_MASK;
    ADC_HC1_REG(base) = ADC_HC1_ADCH_MASK;
    ADC_R0_REG(base)  = 0x0;
    ADC_R1_REG(base)  = 0x0;
    ADC_CFG_REG(base) = ADC_CFG_ADSTS(2);
    ADC_GC_REG(base)  = 0x0;
    ADC_GS_REG(base)  = ADC_GS_CALF_MASK | ADC_GS_AWKST_MASK;
    ADC_CV_REG(base)  = 0x0;
    ADC_OFS_REG(base) = 0x0;
    ADC_CAL_REG(base) = 0x0;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetConvertResultOverwrite
 * Description   : Enable or disable ADC overwrite conversion result register.
 *
 *END**************************************************************************/
void ADC_SetConvertResultOverwrite(ADC_Type* base, bool enable)
{
    if(enable)
        ADC_CFG_REG(base) |= ADC_CFG_OVWREN_MASK;
    else
        ADC_CFG_REG(base) &= ~ADC_CFG_OVWREN_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetConvertTrigMode
 * Description   : This function is used to set conversion trigger mode.
 *
 *END**************************************************************************/
void ADC_SetConvertTrigMode(ADC_Type* base, uint8_t mode)
{
    assert(mode <= adcHardwareTrigger);

    if(mode == adcHardwareTrigger)
        ADC_CFG_REG(base) |= ADC_CFG_ADTRG_MASK;
    else
        ADC_CFG_REG(base) &= ~ADC_CFG_ADTRG_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetConvertSpeed
 * Description   : This function is used to set conversion speed mode.
 *
 *END**************************************************************************/
void ADC_SetConvertSpeed(ADC_Type* base, uint8_t mode)
{
    assert(mode <= adcHighSpeed);

    if(mode == adcHighSpeed)
        ADC_CFG_REG(base) |= ADC_CFG_ADHSC_MASK;
    else
        ADC_CFG_REG(base) &= ~ADC_CFG_ADHSC_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetSampleTimeDuration
 * Description   : This function is used to set sample time duration.
 *
 *END**************************************************************************/
void ADC_SetSampleTimeDuration(ADC_Type* base, uint8_t duration)
{
    assert(duration <= adcSamplePeriodClock24);

    switch(duration)
    {
        case adcSamplePeriodClock2:
            ADC_CFG_REG(base) &= ~ADC_CFG_ADLSMP_MASK;
            ADC_CFG_REG(base) = (ADC_CFG_REG(base) & (~ADC_CFG_ADSTS_MASK)) |
                                ADC_CFG_ADSTS(0U);
            break;

        case adcSamplePeriodClock4:
            ADC_CFG_REG(base) &= ~ADC_CFG_ADLSMP_MASK;
            ADC_CFG_REG(base) = (ADC_CFG_REG(base) & (~ADC_CFG_ADSTS_MASK)) |
                                ADC_CFG_ADSTS(1U);
            break;

        case adcSamplePeriodClock6:
            ADC_CFG_REG(base) &= ~ADC_CFG_ADLSMP_MASK;
            ADC_CFG_REG(base) = (ADC_CFG_REG(base) & (~ADC_CFG_ADSTS_MASK)) |
                                ADC_CFG_ADSTS(2U);
            break;

        case adcSamplePeriodClock8:
            ADC_CFG_REG(base) &= ~ADC_CFG_ADLSMP_MASK;
            ADC_CFG_REG(base) = (ADC_CFG_REG(base) & (~ADC_CFG_ADSTS_MASK)) |
                                ADC_CFG_ADSTS(3U);
            break;

        case adcSamplePeriodClock12:
            ADC_CFG_REG(base) |= ADC_CFG_ADLSMP_MASK;
            ADC_CFG_REG(base) = (ADC_CFG_REG(base) & (~ADC_CFG_ADSTS_MASK)) |
                                ADC_CFG_ADSTS(0U);
            break;

        case adcSamplePeriodClock16:
            ADC_CFG_REG(base) |= ADC_CFG_ADLSMP_MASK;
            ADC_CFG_REG(base) = (ADC_CFG_REG(base) & (~ADC_CFG_ADSTS_MASK)) |
                                ADC_CFG_ADSTS(1U);
            break;

        case adcSamplePeriodClock20:
            ADC_CFG_REG(base) |= ADC_CFG_ADLSMP_MASK;
            ADC_CFG_REG(base) = (ADC_CFG_REG(base) & (~ADC_CFG_ADSTS_MASK)) |
                                ADC_CFG_ADSTS(2U);
            break;

        case adcSamplePeriodClock24:
            ADC_CFG_REG(base) |= ADC_CFG_ADLSMP_MASK;
            ADC_CFG_REG(base) = (ADC_CFG_REG(base) & (~ADC_CFG_ADSTS_MASK)) |
                                ADC_CFG_ADSTS(3U);
            break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetPowerMode
 * Description   : This function is used to set power mode.
 *
 *END**************************************************************************/
void ADC_SetPowerMode(ADC_Type* base, uint8_t powerMode)
{
    assert(powerMode <= adcLowPowerMode);

    if(powerMode == adcLowPowerMode)
        ADC_CFG_REG(base) |= ADC_CFG_ADLPC_MASK;
    else
        ADC_CFG_REG(base) &= ~ADC_CFG_ADLPC_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetClockSource
 * Description   : This function is used to set ADC clock source.
 *
 *END**************************************************************************/
void ADC_SetClockSource(ADC_Type* base, uint8_t source, uint8_t div)
{
    assert(source <= adcAsynClock);
    assert(div <= adcInputClockDiv8);

    ADC_CFG_REG(base) = (ADC_CFG_REG(base) & (~ADC_CFG_ADIV_MASK)) |
                        ADC_CFG_ADIV(div);
    ADC_CFG_REG(base) = (ADC_CFG_REG(base) & (~ADC_CFG_ADICLK_MASK)) |
                        ADC_CFG_ADICLK(source);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetAsynClockOutput
 * Description   : This function is used to enable asynchronous clock source output
 *                 regardless of the state of ADC.
 *
 *END**************************************************************************/
void ADC_SetAsynClockOutput(ADC_Type* base, bool enable)
{
    if(enable)
        ADC_GC_REG(base) |= ADC_GC_ADACKEN_MASK;
    else
        ADC_GC_REG(base) &= ~ADC_GC_ADACKEN_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetCalibration
 * Description   : Enable or disable calibration function.
 *
 *END**************************************************************************/
void ADC_SetCalibration(ADC_Type* base, bool enable)
{
    if(enable)
        ADC_GC_REG(base) |= ADC_GC_CAL_MASK;
    else
        ADC_GC_REG(base) &= ~ADC_GC_CAL_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetConvertCmd
 * Description   : Enable continuous conversion and start a conversion on target channel.
 *        This function is only used for software trigger mode. If configured as
 *        hardware trigger mode, this function just enable continuous conversion
 *        and not start the conversion.
 *
 *END**************************************************************************/
void ADC_SetConvertCmd(ADC_Type* base, uint8_t channel, bool enable)
{
    uint8_t triggerMode;

    /* Enable continuous conversion. */
    if(enable)
    {
        ADC_GC_REG(base) |= ADC_GC_ADCO_MASK;
        /* Start the conversion. */
        triggerMode = ADC_GetConvertTrigMode(base);
        if(triggerMode == adcSoftwareTrigger)
            ADC_HC0_REG(base) = (ADC_HC0_REG(base) & (~ADC_HC0_ADCH_MASK)) |
                                ADC_HC0_ADCH(channel);
        else /* Just set the channel. */
            ADC_HC1_REG(base) = (ADC_HC1_REG(base) & (~ADC_HC1_ADCH_MASK)) |
                                ADC_HC1_ADCH(channel);
    }
    else
        ADC_GC_REG(base) &= ~ADC_GC_ADCO_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_TriggerSingleConvert
 * Description   : Enable single conversion and trigger single time conversion
 *                 on target imput channel. If configured as hardware trigger
 *                 mode, this function just set input channel and not start a
 *                 conversion.
 *
 *END**************************************************************************/
void ADC_TriggerSingleConvert(ADC_Type* base, uint8_t channel)
{
    uint8_t triggerMode;

    /* Enable single conversion. */
    ADC_GC_REG(base) &= ~ADC_GC_ADCO_MASK;
    /* Start the conversion. */
    triggerMode = ADC_GetConvertTrigMode(base);
    if(triggerMode == adcSoftwareTrigger)
        ADC_HC0_REG(base) = (ADC_HC0_REG(base) & (~ADC_HC0_ADCH_MASK)) |
                            ADC_HC0_ADCH(channel);
    else /* Just set the channel. */
        ADC_HC1_REG(base) = (ADC_HC1_REG(base) & (~ADC_HC1_ADCH_MASK)) |
                            ADC_HC1_ADCH(channel);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetAverageNum
 * Description   : This function is used to enable hardware aaverage function
 *                 and set hardware average number. If avgNum is equal to 
 *                 adcAvgNumNone, it means disable hardware average function.
 *
 *END**************************************************************************/
void ADC_SetAverageNum(ADC_Type* base, uint8_t avgNum)
{
    assert(avgNum <= adcAvgNumNone);

    if(avgNum != adcAvgNumNone)
    {
        /* Enable hardware average function. */
        ADC_GC_REG(base) |= ADC_GC_AVGE_MASK;
        /* Set hardware average number. */
        ADC_CFG_REG(base) = (ADC_CFG_REG(base) & (~ADC_CFG_AVGS_MASK)) |
                            ADC_CFG_AVGS(avgNum);
    }
    else
    {
        /* Disable hardware average function. */
        ADC_GC_REG(base) &= ~ADC_GC_AVGE_MASK;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_StopConvert
 * Description   : This function is used to stop all conversions.
 *
 *END**************************************************************************/
void ADC_StopConvert(ADC_Type* base)
{
    uint8_t triggerMode;

    triggerMode = ADC_GetConvertTrigMode(base);
    /* According trigger mode to set specific register. */
    if(triggerMode == adcSoftwareTrigger)
        ADC_HC0_REG(base) |= ADC_HC0_ADCH_MASK;
    else
        ADC_HC1_REG(base) |= ADC_HC1_ADCH_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_GetConvertResult
 * Description   : This function is used to get conversion result.
 *
 *END**************************************************************************/
uint16_t ADC_GetConvertResult(ADC_Type* base)
{
    uint8_t triggerMode;

    triggerMode = ADC_GetConvertTrigMode(base);
    if(triggerMode == adcSoftwareTrigger)
        return (uint16_t)((ADC_R0_REG(base) & ADC_R0_D_MASK) >> ADC_R0_D_SHIFT);
    else
        return (uint16_t)((ADC_R1_REG(base) & ADC_R1_D_MASK) >> ADC_R1_D_SHIFT);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetCmpMode
 * Description   : This function is used to enable compare function 
 *                 and set comparer mode.
 *
 *END**************************************************************************/
void ADC_SetCmpMode(ADC_Type* base, uint8_t cmpMode, uint16_t cmpVal1, uint16_t cmpVal2)
{
    assert(cmpMode <= adcCmpModeDisable);

    switch(cmpMode)
    {
        case adcCmpModeLessThanCmpVal1:
            ADC_GC_REG(base) |= ADC_GC_ACFE_MASK;
            ADC_GC_REG(base) &= ~(ADC_GC_ACFGT_MASK | ADC_GC_ACREN_MASK);
            ADC_CV_REG(base) = (ADC_CV_REG(base) & (~ADC_CV_CV1_MASK)) | ADC_CV_CV1(cmpVal1);
            break;

        case adcCmpModeGreaterThanCmpVal1:
            ADC_GC_REG(base) |= ADC_GC_ACFE_MASK;
            ADC_GC_REG(base) = (ADC_GC_REG(base) | ADC_GC_ACFGT_MASK) & (~ADC_GC_ACREN_MASK);
            ADC_CV_REG(base) = (ADC_CV_REG(base) & (~ADC_CV_CV1_MASK)) | ADC_CV_CV1(cmpVal1);
            break;

        case adcCmpModeOutRangNotInclusive:
            ADC_GC_REG(base) |= ADC_GC_ACFE_MASK;
            ADC_GC_REG(base) = (ADC_GC_REG(base) | ADC_GC_ACREN_MASK) & (~ADC_GC_ACFGT_MASK);
            if(cmpVal1 <= cmpVal2)
            {
                ADC_CV_REG(base) = (ADC_CV_REG(base) & (~ADC_CV_CV1_MASK)) | ADC_CV_CV1(cmpVal1);
                ADC_CV_REG(base) = (ADC_CV_REG(base) & (~ADC_CV_CV2_MASK)) | ADC_CV_CV2(cmpVal2);
            }
            break;

        case adcCmpModeInRangNotInclusive:
            ADC_GC_REG(base) |= ADC_GC_ACFE_MASK;
            ADC_GC_REG(base) = (ADC_GC_REG(base) | ADC_GC_ACREN_MASK) & (~ADC_GC_ACFGT_MASK);
            if(cmpVal1 > cmpVal2)
            {
                ADC_CV_REG(base) = (ADC_CV_REG(base) & (~ADC_CV_CV1_MASK)) | ADC_CV_CV1(cmpVal1);
                ADC_CV_REG(base) = (ADC_CV_REG(base) & (~ADC_CV_CV2_MASK)) | ADC_CV_CV2(cmpVal2);
            }
            break;

        case adcCmpModeInRangInclusive:
            ADC_GC_REG(base) |= ADC_GC_ACFE_MASK;
            ADC_GC_REG(base) |= ADC_GC_ACREN_MASK | ADC_GC_ACFGT_MASK;
            if(cmpVal1 <= cmpVal2)
            {
                ADC_CV_REG(base) = (ADC_CV_REG(base) & (~ADC_CV_CV1_MASK)) | ADC_CV_CV1(cmpVal1);
                ADC_CV_REG(base) = (ADC_CV_REG(base) & (~ADC_CV_CV2_MASK)) | ADC_CV_CV2(cmpVal2);
            }
            break;

        case adcCmpModeOutRangInclusive:
            ADC_GC_REG(base) |= ADC_GC_ACFE_MASK;
            ADC_GC_REG(base) |= ADC_GC_ACREN_MASK | ADC_GC_ACFGT_MASK;
            if(cmpVal1 > cmpVal2)
            {
                ADC_CV_REG(base) = (ADC_CV_REG(base) & (~ADC_CV_CV1_MASK)) | ADC_CV_CV1(cmpVal1);
                ADC_CV_REG(base) = (ADC_CV_REG(base) & (~ADC_CV_CV2_MASK)) | ADC_CV_CV2(cmpVal2);
            }
            break;

        case adcCmpModeDisable:
            ADC_GC_REG(base) &= ~ADC_GC_ACFE_MASK;
            break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetCorrectionMode
 * Description   : This function is used to set offset correct mode.
 *
 *END**************************************************************************/
void ADC_SetCorrectionMode(ADC_Type* base, bool correctMode)
{
    if(correctMode)
        ADC_OFS_REG(base) |= ADC_OFS_SIGN_MASK;
    else
        ADC_OFS_REG(base) &= ~ADC_OFS_SIGN_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetIntCmd
 * Description   : Enables or disables ADC conversion complete interrupt request.
 *
 *END**************************************************************************/
void ADC_SetIntCmd(ADC_Type* base, bool enable)
{
    uint8_t triggerMode;

    triggerMode = ADC_GetConvertTrigMode(base);
    if(triggerMode == adcSoftwareTrigger)
    {
        if(enable)
            ADC_HC0_REG(base) |= ADC_HC0_AIEN_MASK;
        else
            ADC_HC0_REG(base) &= ~ADC_HC0_AIEN_MASK;
    }
    else
    {
        if(enable)
            ADC_HC1_REG(base) |= ADC_HC1_AIEN_MASK;
        else
            ADC_HC1_REG(base) &= ~ADC_HC1_AIEN_MASK;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_IsConvertComplete
 * Description   : This function is used to get ADC conversion complete status.
 *
 *END**************************************************************************/
bool ADC_IsConvertComplete(ADC_Type* base)
{
    uint8_t triggerMode;

    triggerMode = ADC_GetConvertTrigMode(base);
    if(triggerMode == adcSoftwareTrigger)
        return (bool)((ADC_HS_REG(base) & ADC_HS_COCO0_MASK) >> ADC_HS_COCO0_SHIFT);
    else
        return (bool)((ADC_HS_REG(base) & ADC_HS_COCO1_MASK) >> ADC_HS_COCO1_SHIFT);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetDmaCmd
 * Description   : Enable or disable DMA request.
 *
 *END**************************************************************************/
void ADC_SetDmaCmd(ADC_Type* base, bool enable)
{
    if (enable)
        ADC_GC_REG(base) |= ADC_GC_DMAEN_MASK;
    else
        ADC_GC_REG(base) &= ~ADC_GC_DMAEN_MASK;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
