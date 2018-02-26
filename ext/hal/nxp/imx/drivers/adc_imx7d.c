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

#include "adc_imx7d.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*******************************************************************************
 * ADC Module Initialization and Configuration functions.
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

    /* Set ADC Module Sample Rate */
    ADC_SetSampleRate(base, initConfig->sampleRate);

    /* Enable ADC Build-in voltage level shifter */
    if (initConfig->levelShifterEnable)
        ADC_LevelShifterEnable(base);
    else
        ADC_LevelShifterDisable(base);

    /* Wait until ADC module power-up completely. */
    while((ADC_ADC_CFG_REG(base) & ADC_ADC_CFG_ADC_PD_OK_MASK));
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
    /* Stop all continues conversions */
    ADC_SetConvertCmd(base, adcLogicChA, false);
    ADC_SetConvertCmd(base, adcLogicChB, false);
    ADC_SetConvertCmd(base, adcLogicChC, false);
    ADC_SetConvertCmd(base, adcLogicChD, false);

    /* Reset ADC Module Register content to default value */
    ADC_CH_A_CFG1_REG(base)     = 0x0;
    ADC_CH_A_CFG2_REG(base)     = ADC_CH_A_CFG2_CHA_AUTO_DIS_MASK;
    ADC_CH_B_CFG1_REG(base)     = 0x0;
    ADC_CH_B_CFG2_REG(base)     = ADC_CH_B_CFG2_CHB_AUTO_DIS_MASK;
    ADC_CH_C_CFG1_REG(base)     = 0x0;
    ADC_CH_C_CFG2_REG(base)     = ADC_CH_C_CFG2_CHC_AUTO_DIS_MASK;
    ADC_CH_D_CFG1_REG(base)     = 0x0;
    ADC_CH_D_CFG2_REG(base)     = ADC_CH_D_CFG2_CHD_AUTO_DIS_MASK;
    ADC_CH_SW_CFG_REG(base)     = 0x0;
    ADC_TIMER_UNIT_REG(base)    = 0x0;
    ADC_DMA_FIFO_REG(base)      = ADC_DMA_FIFO_DMA_WM_LVL(0xF);
    ADC_INT_SIG_EN_REG(base)    = 0x0;
    ADC_INT_EN_REG(base)        = 0x0;
    ADC_INT_STATUS_REG(base)    = 0x0;
    ADC_ADC_CFG_REG(base)       = ADC_ADC_CFG_ADC_EN_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetSampleRate
 * Description   : This function is used to set ADC module sample rate.
 *
 *END**************************************************************************/
void ADC_SetSampleRate(ADC_Type* base, uint32_t sampleRate)
{
    uint8_t preDiv;
    uint8_t coreTimerUnit;

    assert((sampleRate <= 1000000) && (sampleRate >= 1563));

    for (preDiv = 0 ; preDiv < 6; preDiv++)
    {
        uint32_t divider = 24000000 >> (2 + preDiv);
        divider /= sampleRate * 6;
        if(divider <= 32)
        {
            coreTimerUnit = divider - 1;
            break;
        }
    }

    if (0x6 == preDiv)
    {
        preDiv = 0x5;
        coreTimerUnit = 0x1F;
    }

    ADC_TIMER_UNIT_REG(base) = 0x0;
    ADC_TIMER_UNIT_REG(base) = ADC_TIMER_UNIT_PRE_DIV(preDiv) | ADC_TIMER_UNIT_CORE_TIMER_UNIT(coreTimerUnit);
}

/*******************************************************************************
 * ADC Low power control functions.
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetClockDownCmd
 * Description   : This function is used to stop all digital part power.
 *
 *END**************************************************************************/
void ADC_SetClockDownCmd(ADC_Type* base, bool clockDown)
{
    if (clockDown)
        ADC_ADC_CFG_REG(base) |= ADC_ADC_CFG_ADC_CLK_DOWN_MASK;
    else
        ADC_ADC_CFG_REG(base) &= ~ADC_ADC_CFG_ADC_CLK_DOWN_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetPowerDownCmd
 * Description   : This function is used to power down ADC analogue core.
 *                 Before entering into stop-mode, power down ADC analogue
 *                 core first.
 *
 *END**************************************************************************/
void ADC_SetPowerDownCmd(ADC_Type* base, bool powerDown)
{
    if (powerDown)
    {
        ADC_ADC_CFG_REG(base) |= ADC_ADC_CFG_ADC_PD_MASK;
        /* Wait until power down action finish. */
        while((ADC_ADC_CFG_REG(base) & ADC_ADC_CFG_ADC_PD_OK_MASK));
    }
    else
    {
        ADC_ADC_CFG_REG(base) &= ~ADC_ADC_CFG_ADC_PD_MASK;
    }
}

/*******************************************************************************
 * ADC Convert Channel Initialization and Configuration functions.
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_LogicChInit
 * Description   : Initialize ADC Logic channel with initialization structure.
 *
 *END**************************************************************************/
void ADC_LogicChInit(ADC_Type* base, uint8_t logicCh, const adc_logic_ch_init_config_t* chInitConfig)
{
    assert(chInitConfig);

    /* Select input channel */
    ADC_SelectInputCh(base, logicCh, chInitConfig->inputChannel);

    /* Set Continuous Convert Rate. */
    if (chInitConfig->coutinuousEnable)
        ADC_SetConvertRate(base, logicCh, chInitConfig->convertRate);

    /* Set Hardware average Number. */
    if (chInitConfig->averageEnable)
    {
        ADC_SetAverageNum(base, logicCh, chInitConfig->averageNumber);
        ADC_SetAverageCmd(base, logicCh, true);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_LogicChDeinit
 * Description   : Reset target ADC logic channel registers to default value.
 *
 *END**************************************************************************/
void ADC_LogicChDeinit(ADC_Type* base, uint8_t logicCh)
{
    assert(logicCh <= adcLogicChSW);

    switch (logicCh)
    {
        case adcLogicChA:
            ADC_CH_A_CFG1_REG(base) = 0x0;
            ADC_CH_A_CFG2_REG(base) = 0x8000;
            break;
        case adcLogicChB:
            ADC_CH_B_CFG1_REG(base) = 0x0;
            ADC_CH_B_CFG2_REG(base) = 0x8000;
            break;
        case adcLogicChC:
            ADC_CH_C_CFG1_REG(base) = 0x0;
            ADC_CH_C_CFG2_REG(base) = 0x8000;
            break;
        case adcLogicChD:
            ADC_CH_D_CFG1_REG(base) = 0x0;
            ADC_CH_D_CFG2_REG(base) = 0x8000;
            break;
        case adcLogicChSW:
            ADC_CH_SW_CFG_REG(base) = 0x0;
            break;
        default:
            break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SelectInputCh
 * Description   : Select input channel for target logic channel.
 *
 *END**************************************************************************/
void ADC_SelectInputCh(ADC_Type* base, uint8_t logicCh, uint8_t inputCh)
{
    assert(logicCh <= adcLogicChSW);

    switch (logicCh)
    {
        case adcLogicChA:
            ADC_CH_A_CFG1_REG(base) = (ADC_CH_A_CFG1_REG(base) & ~ADC_CH_A_CFG1_CHA_SEL_MASK) | \
                                      ADC_CH_A_CFG1_CHA_SEL(inputCh);
            break;
        case adcLogicChB:
            ADC_CH_B_CFG1_REG(base) = (ADC_CH_B_CFG1_REG(base) & ~ADC_CH_B_CFG1_CHB_SEL_MASK) | \
                                      ADC_CH_B_CFG1_CHB_SEL(inputCh);
            break;
        case adcLogicChC:
            ADC_CH_C_CFG1_REG(base) = (ADC_CH_C_CFG1_REG(base) & ~ADC_CH_C_CFG1_CHC_SEL_MASK) | \
                                      ADC_CH_C_CFG1_CHC_SEL(inputCh);
            break;
        case adcLogicChD:
            ADC_CH_D_CFG1_REG(base) = (ADC_CH_D_CFG1_REG(base) & ~ADC_CH_D_CFG1_CHD_SEL_MASK) | \
                                      ADC_CH_D_CFG1_CHD_SEL(inputCh);
            break;
        case adcLogicChSW:
            ADC_CH_SW_CFG_REG(base) = (ADC_CH_SW_CFG_REG(base) & ~ADC_CH_SW_CFG_CH_SW_SEL_MASK) | \
                                      ADC_CH_SW_CFG_CH_SW_SEL(inputCh);
            break;
        default:
            break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetConvertRate
 * Description   : Set ADC conversion rate of target logic channel.
 *
 *END**************************************************************************/
void ADC_SetConvertRate(ADC_Type* base, uint8_t logicCh, uint32_t convertRate)
{
    assert(logicCh <= adcLogicChD);

    /* Calculate ADC module's current sample rate */
    uint32_t sampleRate = (4000000 >> (2 + (ADC_TIMER_UNIT_REG(base) >> ADC_TIMER_UNIT_PRE_DIV_SHIFT))) / \
                           ((ADC_TIMER_UNIT_REG(base) & ADC_TIMER_UNIT_CORE_TIMER_UNIT_MASK) + 1);

    uint32_t convertDiv = sampleRate / convertRate;
    assert((sampleRate / convertRate) <= ADC_CH_A_CFG1_CHA_TIMER_MASK);

    switch (logicCh)
    {
        case adcLogicChA:
            ADC_CH_A_CFG1_REG(base) = (ADC_CH_A_CFG1_REG(base) & ~ADC_CH_A_CFG1_CHA_TIMER_MASK) | \
                                      ADC_CH_A_CFG1_CHA_TIMER(convertDiv);
            break;
        case adcLogicChB:
            ADC_CH_B_CFG1_REG(base) = (ADC_CH_B_CFG1_REG(base) & ~ADC_CH_B_CFG1_CHB_TIMER_MASK) | \
                                      ADC_CH_B_CFG1_CHB_TIMER(convertDiv);
            break;
        case adcLogicChC:
            ADC_CH_C_CFG1_REG(base) = (ADC_CH_C_CFG1_REG(base) & ~ADC_CH_C_CFG1_CHC_TIMER_MASK) | \
                                      ADC_CH_C_CFG1_CHC_TIMER(convertDiv);
            break;
        case adcLogicChD:
            ADC_CH_D_CFG1_REG(base) = (ADC_CH_D_CFG1_REG(base) & ~ADC_CH_D_CFG1_CHD_TIMER_MASK) | \
                                      ADC_CH_D_CFG1_CHD_TIMER(convertDiv);
            break;
        default:
            break;
    } 
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetAverageCmd
 * Description   : Set work state of hardware average feature of target
 *                 logic channel.
 *
 *END**************************************************************************/
void ADC_SetAverageCmd(ADC_Type* base, uint8_t logicCh, bool enable)
{
    assert(logicCh <= adcLogicChSW);

    if (enable)
    {
        switch (logicCh)
        {
            case adcLogicChA:
                ADC_CH_A_CFG1_REG(base) |= ADC_CH_A_CFG1_CHA_AVG_EN_MASK;
                break;
            case adcLogicChB:
                ADC_CH_B_CFG1_REG(base) |= ADC_CH_B_CFG1_CHB_AVG_EN_MASK;
                break;
            case adcLogicChC:
                ADC_CH_C_CFG1_REG(base) |= ADC_CH_C_CFG1_CHC_AVG_EN_MASK;
                break;
            case adcLogicChD:
                ADC_CH_D_CFG1_REG(base) |= ADC_CH_D_CFG1_CHD_AVG_EN_MASK;
                break;
            case adcLogicChSW:
                ADC_CH_SW_CFG_REG(base) |= ADC_CH_SW_CFG_CH_SW_AVG_EN_MASK;
                break;
            default:
                break;
        }
    }
    else
    {
        switch (logicCh)
        {
            case adcLogicChA:
                ADC_CH_A_CFG1_REG(base) &= ~ADC_CH_A_CFG1_CHA_AVG_EN_MASK;
                break;
            case adcLogicChB:
                ADC_CH_B_CFG1_REG(base) &= ~ADC_CH_B_CFG1_CHB_AVG_EN_MASK;
                break;
            case adcLogicChC:
                ADC_CH_C_CFG1_REG(base) &= ~ADC_CH_C_CFG1_CHC_AVG_EN_MASK;
                break;
            case adcLogicChD:
                ADC_CH_D_CFG1_REG(base) &= ~ADC_CH_D_CFG1_CHD_AVG_EN_MASK;
                break;
            case adcLogicChSW:
                ADC_CH_SW_CFG_REG(base) &= ~ADC_CH_SW_CFG_CH_SW_AVG_EN_MASK;
                break;
            default:
                break;
        }
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetAverageNum
 * Description   : Set hardware average number of target logic channel.
 *
 *END**************************************************************************/
void ADC_SetAverageNum(ADC_Type* base, uint8_t logicCh, uint8_t avgNum)
{
    assert(logicCh <= adcLogicChSW);
    assert(avgNum <= adcAvgNum32);

    switch (logicCh)
    {
        case adcLogicChA:
            ADC_CH_A_CFG2_REG(base) = (ADC_CH_A_CFG2_REG(base) & ~ADC_CH_A_CFG2_CHA_AVG_NUMBER_MASK) | \
                                      ADC_CH_A_CFG2_CHA_AVG_NUMBER(avgNum);
            break;
        case adcLogicChB:
            ADC_CH_B_CFG2_REG(base) = (ADC_CH_B_CFG2_REG(base) & ~ADC_CH_B_CFG2_CHB_AVG_NUMBER_MASK) | \
                                      ADC_CH_B_CFG2_CHB_AVG_NUMBER(avgNum);
            break;
        case adcLogicChC:
            ADC_CH_C_CFG2_REG(base) = (ADC_CH_C_CFG2_REG(base) & ~ADC_CH_C_CFG2_CHC_AVG_NUMBER_MASK) | \
                                      ADC_CH_C_CFG2_CHC_AVG_NUMBER(avgNum);
            break;
        case adcLogicChD:
            ADC_CH_D_CFG2_REG(base) = (ADC_CH_D_CFG2_REG(base) & ~ADC_CH_D_CFG2_CHD_AVG_NUMBER_MASK) | \
                                      ADC_CH_D_CFG2_CHD_AVG_NUMBER(avgNum);
            break;
        case adcLogicChSW:
            ADC_CH_SW_CFG_REG(base) = (ADC_CH_SW_CFG_REG(base) & ~ADC_CH_SW_CFG_CH_SW_AVG_NUMBER_MASK) | \
                                      ADC_CH_SW_CFG_CH_SW_AVG_NUMBER(avgNum);
            break;
        default:
            break;
    }
}

/*******************************************************************************
 * ADC Conversion Control functions.
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetConvertCmd
 * Description   : Set continuous convert work mode of target logic channel.
 *
 *END**************************************************************************/
void ADC_SetConvertCmd(ADC_Type* base, uint8_t logicCh, bool enable)
{
    assert(logicCh <= adcLogicChD);

    if (enable)
    {
        switch (logicCh)
        {
            case adcLogicChA:
                ADC_CH_A_CFG1_REG(base) = (ADC_CH_A_CFG1_REG(base) & ~ADC_CH_A_CFG1_CHA_SINGLE_MASK) |
                                          ADC_CH_A_CFG1_CHA_EN_MASK;
                break;
            case adcLogicChB:
                ADC_CH_B_CFG1_REG(base) = (ADC_CH_B_CFG1_REG(base) & ~ADC_CH_B_CFG1_CHB_SINGLE_MASK) |
                                          ADC_CH_B_CFG1_CHB_EN_MASK;
                break;
            case adcLogicChC:
                ADC_CH_C_CFG1_REG(base) = (ADC_CH_C_CFG1_REG(base) & ~ADC_CH_C_CFG1_CHC_SINGLE_MASK) |
                                          ADC_CH_C_CFG1_CHC_EN_MASK;
                break;
            case adcLogicChD:
                ADC_CH_D_CFG1_REG(base) = (ADC_CH_D_CFG1_REG(base) & ~ADC_CH_D_CFG1_CHD_SINGLE_MASK) |
                                          ADC_CH_D_CFG1_CHD_EN_MASK;
                break;
            default:
                break;
        }
    }
    else
    {
        switch (logicCh)
        {
            case adcLogicChA:
                ADC_CH_A_CFG1_REG(base) &= ~ADC_CH_A_CFG1_CHA_EN_MASK;
                break;
            case adcLogicChB:
                ADC_CH_B_CFG1_REG(base) &= ~ADC_CH_B_CFG1_CHB_EN_MASK;
                break;
            case adcLogicChC:
                ADC_CH_C_CFG1_REG(base) &= ~ADC_CH_C_CFG1_CHC_EN_MASK;
                break;
            case adcLogicChD:
                ADC_CH_D_CFG1_REG(base) &= ~ADC_CH_D_CFG1_CHD_EN_MASK;
                break;
            default:
                break;
        }
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_TriggerSingleConvert
 * Description   : Trigger single time convert on the target logic channel.
 *
 *END**************************************************************************/
void ADC_TriggerSingleConvert(ADC_Type* base, uint8_t logicCh)
{
    assert(logicCh <= adcLogicChSW);

    switch (logicCh)
    {
        case adcLogicChA:
            ADC_CH_A_CFG1_REG(base) |= ADC_CH_A_CFG1_CHA_SINGLE_MASK | ADC_CH_A_CFG1_CHA_EN_MASK;
            break;
        case adcLogicChB:
            ADC_CH_B_CFG1_REG(base) |= ADC_CH_B_CFG1_CHB_SINGLE_MASK | ADC_CH_B_CFG1_CHB_EN_MASK;
            break;
        case adcLogicChC:
            ADC_CH_C_CFG1_REG(base) |= ADC_CH_C_CFG1_CHC_SINGLE_MASK | ADC_CH_C_CFG1_CHC_EN_MASK;
            break;
        case adcLogicChD:
            ADC_CH_D_CFG1_REG(base) |= ADC_CH_D_CFG1_CHD_SINGLE_MASK | ADC_CH_D_CFG1_CHD_EN_MASK;
            break;
        case adcLogicChSW:
            ADC_CH_SW_CFG_REG(base) |= ADC_CH_SW_CFG_START_CONV_MASK;
            break;
        default:
            break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_StopConvert
 * Description   : Stop current convert on the target logic channel.
 *
 *END**************************************************************************/
void ADC_StopConvert(ADC_Type* base, uint8_t logicCh)
{
    assert(logicCh <= adcLogicChSW);

    switch (logicCh)
    {
        case adcLogicChA:
            ADC_CH_A_CFG1_REG(base) &= ~ADC_CH_A_CFG1_CHA_EN_MASK;
            break;
        case adcLogicChB:
            ADC_CH_B_CFG1_REG(base) &= ~ADC_CH_B_CFG1_CHB_EN_MASK;
            break;
        case adcLogicChC:
            ADC_CH_C_CFG1_REG(base) &= ~ADC_CH_C_CFG1_CHC_EN_MASK;
            break;
        case adcLogicChD:
            ADC_CH_D_CFG1_REG(base) &= ~ADC_CH_D_CFG1_CHD_EN_MASK;
            break;
        case adcLogicChSW:
            /* Wait until ADC conversion finish. */
            while (ADC_CH_SW_CFG_REG(base) & ADC_CH_SW_CFG_START_CONV_MASK);
            break;
        default:
            break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_GetConvertResult
 * Description   : Get 12-bit length right aligned convert result.
 *
 *END**************************************************************************/
uint16_t ADC_GetConvertResult(ADC_Type* base, uint8_t logicCh)
{
    assert(logicCh <= adcLogicChSW);

    switch (logicCh)
    {
        case adcLogicChA:
            return ADC_CHA_B_CNV_RSLT_REG(base) & ADC_CHA_B_CNV_RSLT_CHA_CNV_RSLT_MASK;
        case adcLogicChB:
            return ADC_CHA_B_CNV_RSLT_REG(base) >> ADC_CHA_B_CNV_RSLT_CHB_CNV_RSLT_SHIFT;
        case adcLogicChC:
            return ADC_CHC_D_CNV_RSLT_REG(base) & ADC_CHC_D_CNV_RSLT_CHC_CNV_RSLT_MASK;
        case adcLogicChD:
            return ADC_CHC_D_CNV_RSLT_REG(base) >> ADC_CHC_D_CNV_RSLT_CHD_CNV_RSLT_SHIFT;
        case adcLogicChSW:
            return ADC_CH_SW_CNV_RSLT_REG(base) & ADC_CH_SW_CNV_RSLT_CH_SW_CNV_RSLT_MASK;
        default:
            return 0;
    }
}

/*******************************************************************************
 * ADC Comparer Control functions.
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetCmpMode
 * Description   : Set the work mode of ADC module build-in comparer on target
 *                 logic channel.
 *
 *END**************************************************************************/
void ADC_SetCmpMode(ADC_Type* base, uint8_t logicCh, uint8_t cmpMode)
{
    assert(logicCh <= adcLogicChD);
    assert(cmpMode <= adcCmpModeOutOffInterval);

    switch (logicCh)
    {
        case adcLogicChA:
            ADC_CH_A_CFG2_REG(base) = (ADC_CH_A_CFG2_REG(base) & ~ADC_CH_A_CFG2_CHA_CMP_MODE_MASK) | \
                                      ADC_CH_A_CFG2_CHA_CMP_MODE(cmpMode);
            break;
        case adcLogicChB:
            ADC_CH_B_CFG2_REG(base) = (ADC_CH_B_CFG2_REG(base) & ~ADC_CH_B_CFG2_CHB_CMP_MODE_MASK) | \
                                      ADC_CH_B_CFG2_CHB_CMP_MODE(cmpMode);
            break;
        case adcLogicChC:
            ADC_CH_C_CFG2_REG(base) = (ADC_CH_C_CFG2_REG(base) & ~ADC_CH_C_CFG2_CHC_CMP_MODE_MASK) | \
                                      ADC_CH_C_CFG2_CHC_CMP_MODE(cmpMode);
            break;
        case adcLogicChD:
            ADC_CH_D_CFG2_REG(base) = (ADC_CH_D_CFG2_REG(base) & ~ADC_CH_D_CFG2_CHD_CMP_MODE_MASK) | \
                                      ADC_CH_D_CFG2_CHD_CMP_MODE(cmpMode);
            break;
        default:
            break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetCmpHighThres
 * Description   : Set ADC module build-in comparer high threshold on target
 *                 logic channel.
 *
 *END**************************************************************************/
void ADC_SetCmpHighThres(ADC_Type* base, uint8_t logicCh, uint16_t threshold)
{
    assert(logicCh <= adcLogicChD);
    assert(threshold <= 0xFFF);

    switch (logicCh)
    {
        case adcLogicChA:
            ADC_CH_A_CFG2_REG(base) = (ADC_CH_A_CFG2_REG(base) & ~ADC_CH_A_CFG2_CHA_HIGH_THRES_MASK) | \
                                      ADC_CH_A_CFG2_CHA_HIGH_THRES(threshold);
            break;
        case adcLogicChB:
            ADC_CH_B_CFG2_REG(base) = (ADC_CH_B_CFG2_REG(base) & ~ADC_CH_B_CFG2_CHB_HIGH_THRES_MASK) | \
                                      ADC_CH_B_CFG2_CHB_HIGH_THRES(threshold);
            break;
        case adcLogicChC:
            ADC_CH_C_CFG2_REG(base) = (ADC_CH_C_CFG2_REG(base) & ~ADC_CH_C_CFG2_CHC_HIGH_THRES_MASK) | \
                                      ADC_CH_C_CFG2_CHC_HIGH_THRES(threshold);
            break;
        case adcLogicChD:
            ADC_CH_D_CFG2_REG(base) = (ADC_CH_D_CFG2_REG(base) & ~ADC_CH_D_CFG2_CHD_HIGH_THRES_MASK) | \
                                      ADC_CH_D_CFG2_CHD_HIGH_THRES(threshold);
            break;
        default:
            break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetCmpLowThres
 * Description   : Set ADC module build-in comparer low threshold on target
 *                 logic channel.
 *
 *END**************************************************************************/
void ADC_SetCmpLowThres(ADC_Type* base, uint8_t logicCh, uint16_t threshold)
{
    assert(logicCh <= adcLogicChD);
    assert(threshold <= 0xFFF);

    switch (logicCh)
    {
        case adcLogicChA:
            ADC_CH_A_CFG2_REG(base) = (ADC_CH_A_CFG2_REG(base) & ~ADC_CH_A_CFG2_CHA_LOW_THRES_MASK) | \
                                      ADC_CH_A_CFG2_CHA_LOW_THRES(threshold);
            break;
        case adcLogicChB:
            ADC_CH_B_CFG2_REG(base) = (ADC_CH_B_CFG2_REG(base) & ~ADC_CH_B_CFG2_CHB_LOW_THRES_MASK) | \
                                      ADC_CH_B_CFG2_CHB_LOW_THRES(threshold);
            break;
        case adcLogicChC:
            ADC_CH_C_CFG2_REG(base) = (ADC_CH_C_CFG2_REG(base) & ~ADC_CH_C_CFG2_CHC_LOW_THRES_MASK) | \
                                      ADC_CH_B_CFG2_CHB_LOW_THRES(threshold);
            break;
        case adcLogicChD:
            ADC_CH_D_CFG2_REG(base) = (ADC_CH_D_CFG2_REG(base) & ~ADC_CH_D_CFG2_CHD_LOW_THRES_MASK) | \
                                      ADC_CH_D_CFG2_CHD_LOW_THRES(threshold);
            break;
        default:
            break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetAutoDisableCmd
 * Description   : Set the working mode of ADC module auto disable feature on
 *                 target logic channel.
 *
 *END**************************************************************************/
void ADC_SetAutoDisableCmd(ADC_Type* base, uint8_t logicCh, bool enable)
{
    assert(logicCh <= adcLogicChD);

    if (enable)
    {
        switch (logicCh)
        {
            case adcLogicChA:
                ADC_CH_A_CFG2_REG(base) |= ADC_CH_A_CFG2_CHA_AUTO_DIS_MASK;
                break;
            case adcLogicChB:
                ADC_CH_B_CFG2_REG(base) |= ADC_CH_B_CFG2_CHB_AUTO_DIS_MASK;
                break;
            case adcLogicChC:
                ADC_CH_C_CFG2_REG(base) |= ADC_CH_C_CFG2_CHC_AUTO_DIS_MASK;
                break;
            case adcLogicChD:
                ADC_CH_D_CFG2_REG(base) |= ADC_CH_D_CFG2_CHD_AUTO_DIS_MASK;
                break;
            default:
                break;
        }
    }
    else
    {
        switch (logicCh)
        {
            case adcLogicChA:
                ADC_CH_A_CFG2_REG(base) &= ~ADC_CH_A_CFG2_CHA_AUTO_DIS_MASK;
                break;
            case adcLogicChB:
                ADC_CH_B_CFG2_REG(base) &= ~ADC_CH_B_CFG2_CHB_AUTO_DIS_MASK;
                break;
            case adcLogicChC:
                ADC_CH_C_CFG2_REG(base) &= ~ADC_CH_C_CFG2_CHC_AUTO_DIS_MASK;
                break;
            case adcLogicChD:
                ADC_CH_D_CFG2_REG(base) &= ~ADC_CH_D_CFG2_CHD_AUTO_DIS_MASK;
                break;
            default:
                break;
        }
    }
}

/*******************************************************************************
 * Interrupt and Flag control functions.
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetIntCmd
 * Description   : Enables or disables ADC interrupt requests.
 *
 *END**************************************************************************/
void ADC_SetIntCmd(ADC_Type* base, uint32_t intSource, bool enable)
{
    if (enable)
        ADC_INT_EN_REG(base) |= intSource;
    else
        ADC_INT_EN_REG(base) &= ~intSource;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetIntSigCmd
 * Description   : Enables or disables ADC interrupt flag when interrupt
 *                 condition met.
 *
 *END**************************************************************************/
void ADC_SetIntSigCmd(ADC_Type* base, uint32_t intSignal, bool enable)
{
    if (enable)
        ADC_INT_SIG_EN_REG(base) |= intSignal;
    else
        ADC_INT_SIG_EN_REG(base) &= ~intSignal;
}

/*******************************************************************************
 * DMA & FIFO control functions.
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetDmaReset
 * Description   : Set the reset state of ADC internal DMA part.
 *
 *END**************************************************************************/
void ADC_SetDmaReset(ADC_Type* base, bool active)
{
    if (active)
        ADC_DMA_FIFO_REG(base) |= ADC_DMA_FIFO_DMA_RST_MASK;
    else
        ADC_DMA_FIFO_REG(base) &= ~ADC_DMA_FIFO_DMA_RST_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetDmaCmd
 * Description   : Set the work mode of ADC DMA part.
 *
 *END**************************************************************************/
void ADC_SetDmaCmd(ADC_Type* base, bool enable)
{
    if (enable)
        ADC_DMA_FIFO_REG(base) |= ADC_DMA_FIFO_DMA_EN_MASK;
    else
        ADC_DMA_FIFO_REG(base) &= ~ADC_DMA_FIFO_DMA_EN_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ADC_SetDmaFifoCmd
 * Description   : Set the work mode of ADC DMA FIFO part.
 *
 *END**************************************************************************/
void ADC_SetDmaFifoCmd(ADC_Type* base, bool enable)
{
    if (enable)
        ADC_DMA_FIFO_REG(base) |= ADC_DMA_FIFO_DMA_FIFO_EN_MASK;
    else
        ADC_DMA_FIFO_REG(base) &= ~ADC_DMA_FIFO_DMA_FIFO_EN_MASK;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
