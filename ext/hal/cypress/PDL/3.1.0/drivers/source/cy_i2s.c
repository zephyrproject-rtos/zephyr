/***************************************************************************//**
* \file cy_i2s.c
* \version 2.10
*
* The source code file for the I2S driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_i2s.h"

#ifdef CY_IP_MXAUDIOSS

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************
* Function Name: Cy_I2S_Init
****************************************************************************//**
*
* Initializes the I2S module in accordance with a configuration structure.
*
* \pre If the I2S module is initialized previously, the \ref Cy_I2S_DeInit()
* must be called before calling this function.
*
* \param base      The pointer to the I2S instance address.
*
* \param config    The pointer to a configuration structure.
*
* \return error / status code. See \ref cy_en_i2s_status_t.
*
* \funcusage
* \snippet i2s/i2s_v2_0_sut_00.cydsn/main_cm4.c snippet_Cy_I2S_Init
*
*******************************************************************************/
cy_en_i2s_status_t Cy_I2S_Init(I2S_Type * base, cy_stc_i2s_config_t const * config)
{
    cy_en_i2s_status_t ret = CY_I2S_BAD_PARAM;
    
    if((NULL != base) && (NULL != config))
    {
        cy_en_i2s_ws_pw_t wsPulseWidth;
        cy_en_i2s_len_t channelLength;
        uint32_t channels;
        uint32_t clockDiv = (uint32_t)config->clkDiv - 1U;
        
        CY_ASSERT_L2(CY_I2S_IS_CLK_DIV_VALID(clockDiv));
        
        /* The clock setting */
        I2S_CLOCK_CTL(base) = _VAL2FLD(I2S_CLOCK_CTL_CLOCK_DIV, clockDiv) |
                          _BOOL2FLD(I2S_CLOCK_CTL_CLOCK_SEL, config->extClk);

        /* The Tx setting */
        if (config->txEnabled)
        {
            CY_ASSERT_L3(CY_I2S_IS_ALIGNMENT_VALID(config->txAlignment));
            CY_ASSERT_L3(CY_I2S_IS_OVHDATA_VALID(config->txOverheadValue));
            
            if ((CY_I2S_TDM_MODE_A == config->txAlignment) || (CY_I2S_TDM_MODE_B == config->txAlignment))
            {
                channels = (uint32_t)config->txChannels - 1UL;
                wsPulseWidth = config->txWsPulseWidth;
                channelLength = CY_I2S_LEN32;
                
                CY_ASSERT_L2(CY_I2S_IS_CHANNELS_VALID(channels));
                CY_ASSERT_L3(CY_I2S_IS_WSPULSE_VALID(wsPulseWidth));
                CY_ASSERT_L3(CY_I2S_IS_LEN_VALID(config->txWordLength));
            }
            else
            {
                channels = 1UL;
                wsPulseWidth = CY_I2S_WS_ONE_CHANNEL_LENGTH;
                channelLength = config->txChannelLength;
                
                CY_ASSERT_L3(CY_I2S_IS_CHAN_WORD_VALID(channelLength, config->txWordLength));
            }
            
            CY_ASSERT_L2(CY_I2S_IS_TRIG_LEVEL_VALID(config->txFifoTriggerLevel, channels));
            
            I2S_TX_WATCHDOG(base) = config->txWatchdogValue;

            I2S_TX_CTL(base) = _VAL2FLD(I2S_TX_CTL_I2S_MODE, config->txAlignment) |
                           _BOOL2FLD(I2S_TX_CTL_B_CLOCK_INV, config->txSdoLatchingTime) |
                           _VAL2FLD(I2S_TX_CTL_CH_NR, channels) |
                           _BOOL2FLD(I2S_TX_CTL_MS, config->txMasterMode) |
                           _VAL2FLD(I2S_TX_CTL_WS_PULSE, wsPulseWidth) |
                           _BOOL2FLD(I2S_TX_CTL_WD_EN, config->txWatchdogEnable) |
                           _BOOL2FLD(I2S_TX_CTL_SCKO_POL, config->txSckoInversion) |
                           _BOOL2FLD(I2S_TX_CTL_SCKI_POL, config->txSckiInversion) |
                           _VAL2FLD(I2S_TX_CTL_CH_LEN, channelLength) |
                           _VAL2FLD(I2S_TX_CTL_WORD_LEN, config->txWordLength) |
                           _VAL2FLD(I2S_TX_CTL_OVHDATA, config->txOverheadValue);
        }

        /* The Rx setting */
        if (config->rxEnabled)
        {
            CY_ASSERT_L3(CY_I2S_IS_ALIGNMENT_VALID(config->rxAlignment));
            
            if ((CY_I2S_TDM_MODE_A == config->rxAlignment) || (CY_I2S_TDM_MODE_B == config->rxAlignment))
            {
                channels = (uint32_t)config->rxChannels - 1UL;
                wsPulseWidth = config->rxWsPulseWidth;
                channelLength = CY_I2S_LEN32;
                
                CY_ASSERT_L2(CY_I2S_IS_CHANNELS_VALID(channels));
                CY_ASSERT_L3(CY_I2S_IS_WSPULSE_VALID(wsPulseWidth));
                CY_ASSERT_L3(CY_I2S_IS_LEN_VALID(config->rxWordLength));
            }
            else
            {
                channels = 1UL;
                wsPulseWidth = CY_I2S_WS_ONE_CHANNEL_LENGTH;
                channelLength = config->rxChannelLength;
                
                CY_ASSERT_L3(CY_I2S_IS_CHAN_WORD_VALID(channelLength, config->rxWordLength));
            }
            
            CY_ASSERT_L2(CY_I2S_IS_TRIG_LEVEL_VALID(config->rxFifoTriggerLevel, channels));

            I2S_RX_WATCHDOG(base) = config->rxWatchdogValue;

            I2S_RX_CTL(base) = _VAL2FLD(I2S_RX_CTL_I2S_MODE, config->rxAlignment) |
                           _BOOL2FLD(I2S_RX_CTL_B_CLOCK_INV, config->rxSdiLatchingTime) |
                           _VAL2FLD(I2S_RX_CTL_CH_NR, channels) |
                           _BOOL2FLD(I2S_RX_CTL_MS, config->rxMasterMode) |
                           _VAL2FLD(I2S_RX_CTL_WS_PULSE, wsPulseWidth) |
                           _BOOL2FLD(I2S_RX_CTL_WD_EN, config->rxWatchdogEnable) |
                           _BOOL2FLD(I2S_RX_CTL_SCKO_POL, config->rxSckoInversion) |
                           _BOOL2FLD(I2S_RX_CTL_SCKI_POL, config->rxSckiInversion) |
                           _VAL2FLD(I2S_RX_CTL_CH_LEN, channelLength) |
                           _VAL2FLD(I2S_RX_CTL_WORD_LEN, config->rxWordLength) |
                           _BOOL2FLD(I2S_RX_CTL_BIT_EXTENSION, config->rxSignExtension);
        }

        /* The I2S enable setting */
        if (config->txEnabled)
        {
            I2S_CTL(base) |= I2S_CTL_TX_ENABLED_Msk;
        }

        if (config->rxEnabled)
        {
            I2S_CTL(base) |= I2S_CTL_RX_ENABLED_Msk;
        }

        /* The FIFO setting */
        if (config->txEnabled)
        {
            I2S_TX_FIFO_CTL(base) = _VAL2FLD(I2S_TX_FIFO_CTL_TRIGGER_LEVEL, config->txFifoTriggerLevel);

            I2S_TR_CTL(base) |= _BOOL2FLD(I2S_TR_CTL_TX_REQ_EN, config->txDmaTrigger);
        }

        if (config->rxEnabled)
        {
            I2S_RX_FIFO_CTL(base) = _VAL2FLD(I2S_RX_FIFO_CTL_TRIGGER_LEVEL, config->rxFifoTriggerLevel);

            I2S_TR_CTL(base) |= _BOOL2FLD(I2S_TR_CTL_RX_REQ_EN, config->rxDmaTrigger);
        }
        
        ret = CY_I2S_SUCCESS;
    }

    return (ret);
}


/*******************************************************************************
* Function Name: Cy_I2S_DeInit
****************************************************************************//**
*
* Uninitializes the I2S module (reverts default register values).
*
* \param base The pointer to the I2S instance address.
*
* \funcusage
* \snippet i2s/i2s_v2_0_sut_00.cydsn/main_cm4.c snippet_Cy_I2S_DeInit
*
*******************************************************************************/
void Cy_I2S_DeInit(I2S_Type * base)
{
    I2S_INTR_MASK(base); /* Disable interrupts prior to stopping the operation */
    I2S_CMD(base) = 0UL;
    I2S_TR_CTL(base) = 0UL;
    I2S_TX_FIFO_CTL(base) = 0UL;
    I2S_RX_FIFO_CTL(base) = 0UL;
    I2S_CTL(base) = 0UL;
    I2S_TX_CTL(base) = CY_I2S_TX_CTL_DEFAULT;
    I2S_RX_CTL(base) = CY_I2S_RX_CTL_DEFAULT;
    I2S_TX_WATCHDOG(base) = 0UL;
    I2S_RX_WATCHDOG(base) = 0UL;
    I2S_CLOCK_CTL(base) = 0UL;
}


/*******************************************************************************
* Function Name: Cy_I2S_DeepSleepCallback
****************************************************************************//**
*
* This is a callback function to be used at the application layer to
* manage an I2S operation during the Deep-Sleep cycle. It stores the I2S state 
* (Tx/Rx enabled/disabled/paused) into the context structure and stops the 
* communication before entering into Deep-Sleep power mode and restores the I2S
* state after waking up.
*
* \param 
* callbackParams - The pointer to the callback parameters structure, 
* see \ref cy_stc_syspm_callback_params_t.
*
* \return the SysPm callback status \ref cy_en_syspm_status_t.
*
* \note Use the \ref cy_stc_i2s_context_t data type for definition of the 
* *context element of the \ref cy_stc_syspm_callback_params_t strusture.
*
* \funcusage
* \snippet i2s/i2s_v2_0_sut_00.cydsn/main_cm4.c snippet_Cy_I2S_DeepSleepCallback
*
*******************************************************************************/
cy_en_syspm_status_t Cy_I2S_DeepSleepCallback(cy_stc_syspm_callback_params_t * callbackParams)
{
    cy_en_syspm_status_t ret = CY_SYSPM_SUCCESS;
    CY_ASSERT_L1(NULL != callbackParams->context);
    I2S_Type * locBase = (I2S_Type*) callbackParams->base;
    uint32_t * locInterruptMask = (uint32_t*) &(((cy_stc_i2s_context_t*)(callbackParams->context))->interruptMask);
    uint32_t * locState = (uint32_t*) &(((cy_stc_i2s_context_t*)(callbackParams->context))->enableState);
    
    switch(callbackParams->mode)
    {
        case CY_SYSPM_CHECK_READY:            
        case CY_SYSPM_CHECK_FAIL:
            break;
        
        case CY_SYSPM_BEFORE_TRANSITION:
            *locInterruptMask = Cy_I2S_GetInterruptMask(locBase); /* Store I2S interrupts */
            *locState = Cy_I2S_GetCurrentState(locBase); /* Store I2S state */
            if (0UL != (*locState & I2S_CMD_TX_START_Msk))
            {
                Cy_I2S_DisableTx(locBase); /* Stop TX operation */
            }
            if (0UL != (*locState & I2S_CMD_RX_START_Msk))
            {
                Cy_I2S_DisableRx(locBase); /* Stop RX operation */
            }
            Cy_I2S_SetInterruptMask(locBase, 0UL); /* Disable I2S interrupts */
            /* Unload FIFOs in order not to lose data (if needed) */
            break;
            
        case CY_SYSPM_AFTER_TRANSITION:
            if (0UL != (*locState & I2S_CMD_RX_START_Msk))
            {
                Cy_I2S_ClearRxFifo(locBase); /* Clear the RX FIFO */
                Cy_I2S_EnableRx(locBase); /* Start RX operation */
            }
            if (0UL != (*locState & I2S_CMD_TX_START_Msk))
            {
                Cy_I2S_ClearTxFifo(locBase); /* Clear the TX FIFO */
                Cy_I2S_WriteTxData(locBase, 0UL); /* Fill at least one TX frame */
                Cy_I2S_WriteTxData(locBase, 0UL);
                if (0UL != (*locState & I2S_CMD_TX_PAUSE_Msk))
                {
                    Cy_I2S_PauseTx(locBase); /* Restore the TX paused state */
                }
                Cy_I2S_EnableTx(locBase); /* Start TX operation */
            }
            Cy_I2S_ClearInterrupt(locBase, *locInterruptMask); /* Clear possible pending I2S interrupts */
            Cy_I2S_SetInterruptMask(locBase, *locInterruptMask); /* Restore I2S interrupts */
            break;
        
        default:
            ret = CY_SYSPM_FAIL;
            break;
    }
        
    return(ret);
}


#ifdef __cplusplus
}
#endif

#endif /* CY_IP_MXAUDIOSS */

/* [] END OF FILE */
