/*******************************************************************************
* \file cy_lpcomp.c
* \version 1.20
*
* \brief
*  This file provides the driver code to the API for the Low Power Comparator
*  component.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/
#include "cy_lpcomp.h"

#ifdef CY_IP_MXLPCOMP

#if defined(__cplusplus)
extern "C" {
#endif

static cy_stc_lpcomp_context_t cy_lpcomp_context;

/*******************************************************************************
* Function Name: Cy_LPComp_Init
****************************************************************************//**
*
*  Initializes LPCOMP and returns the LPCOMP register address.
*
* \param *base
*     LPCOMP registers structure pointer.
*
* \param *config
*     The pointer to the configuration structure for PDL.
*
* \param channel
*     The LPCOMP channel index.
*
* \return cy_en_lpcomp_status_t
*     *base checking result. If the pointer is NULL, returns error.
*
*******************************************************************************/
cy_en_lpcomp_status_t Cy_LPComp_Init(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel, const cy_stc_lpcomp_config_t* config)
{
    cy_en_lpcomp_status_t ret = CY_LPCOMP_BAD_PARAM;
    
    CY_ASSERT_L3(CY_LPCOMP_IS_CHANNEL_VALID(channel));
    CY_ASSERT_L3(CY_LPCOMP_IS_OUT_MODE_VALID(config->outputMode));
    CY_ASSERT_L3(CY_LPCOMP_IS_HYSTERESIS_VALID(config->hysteresis));
    CY_ASSERT_L3(CY_LPCOMP_IS_POWER_VALID(config->power));
    CY_ASSERT_L3(CY_LPCOMP_IS_INTR_MODE_VALID(config->intType));

    if ((base != NULL) && (config != NULL))
    {
        Cy_LPComp_GlobalEnable(base);

        if (CY_LPCOMP_CHANNEL_0 == channel)
        {  
            LPCOMP_CMP0_CTRL(base) = _VAL2FLD(LPCOMP_CMP0_CTRL_HYST0, (uint32_t)config->hysteresis) |
                              _VAL2FLD(LPCOMP_CMP0_CTRL_DSI_BYPASS0, (uint32_t)config->outputMode) |
                              _VAL2FLD(LPCOMP_CMP0_CTRL_DSI_LEVEL0, (uint32_t)config->outputMode >> 1u);
        }
        else
        {
            LPCOMP_CMP1_CTRL(base) = _VAL2FLD(LPCOMP_CMP1_CTRL_HYST1, (uint32_t)config->hysteresis) |
                              _VAL2FLD(LPCOMP_CMP1_CTRL_DSI_BYPASS1, (uint32_t)config->outputMode) |
                              _VAL2FLD(LPCOMP_CMP1_CTRL_DSI_LEVEL1, (uint32_t)config->outputMode >> 1u);
        }
        
        /* Save intType to use it in the Cy_LPComp_Enable() function */
        cy_lpcomp_context.intType[(uint8_t)channel - 1u] = config->intType;
          
        /* Save power to use it in the Cy_LPComp_Enable() function */
        cy_lpcomp_context.power[(uint8_t)channel - 1u] = config->power;
          
        ret = CY_LPCOMP_SUCCESS;
    }
    
    return (ret);
}


/*******************************************************************************
* Function Name: Cy_LPComp_Enable
****************************************************************************//**
*
* Enables the LPCOMP and sets the LPCOMP interrupt mode.
*
* \param *base
*     The LPCOMP register structure pointer.
*
* \param channel
*     The LPCOMP channel index.
*
* \return None
*
*******************************************************************************/
void Cy_LPComp_Enable(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel)
{
    cy_en_lpcomp_pwr_t powerSpeed;
    
    CY_ASSERT_L3(CY_LPCOMP_IS_CHANNEL_VALID(channel));
    
    powerSpeed = cy_lpcomp_context.power[(uint8_t)channel - 1u];
    
    /* Set power */
    Cy_LPComp_SetPower(base, channel, powerSpeed);
    
    /* Make delay before enabling the comparator interrupt to prevent false triggering */
    if (CY_LPCOMP_MODE_ULP == powerSpeed)
    {
        Cy_SysLib_DelayUs(CY_LPCOMP_ULP_POWER_DELAY);
    }
    else if (CY_LPCOMP_MODE_LP == powerSpeed)
    {
        Cy_SysLib_DelayUs(CY_LPCOMP_LP_POWER_DELAY);
    }
    else
    {
        Cy_SysLib_DelayUs(CY_LPCOMP_NORMAL_POWER_DELAY); 
    }
    
    /* Enable the comparator interrupt */
    Cy_LPComp_SetInterruptTriggerMode(base, channel, cy_lpcomp_context.intType[(uint8_t)channel - 1u]);
}


/*******************************************************************************
* Function Name: Cy_LPComp_Disable
****************************************************************************//**
*
* Disables the LPCOMP power and sets the interrupt mode to disabled.
*
* \param *base
*     The LPCOMP register structure pointer.
*
* \param channel
*     The LPCOMP channel index.
*
* \return None
*
*******************************************************************************/
void Cy_LPComp_Disable(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel)
{   
    CY_ASSERT_L3(CY_LPCOMP_IS_CHANNEL_VALID(channel));

    /* Disable the comparator interrupt */
    Cy_LPComp_SetInterruptTriggerMode(base, channel, CY_LPCOMP_INTR_DISABLE);
    
    /* Set power off */
    Cy_LPComp_SetPower(base, channel, CY_LPCOMP_MODE_OFF);
}


/*******************************************************************************
* Function Name: Cy_LPComp_SetInterruptTriggerMode
****************************************************************************//**
*
* Sets the interrupt edge-detect mode. 
* This also controls the value provided on the output.
* \note  Interrupts can be enabled after the block is enabled and the appropriate 
* start-up time has elapsed:
* 3 us for the normal power mode;
* 6 us for the LP mode;
* 50 us for the ULP mode.
*
* \param *base
*     The LPCOMP register structure pointer.
*
* \param channel
*     The LPCOMP channel index.
*
* \param intType
*   Interrupt edge trigger selection 
*   CY_LPCOMP_INTR_DISABLE (=0) - Disabled, no interrupt will be detected 
*   CY_LPCOMP_INTR_RISING (=1) - Rising edge 
*   CY_LPCOMP_INTR_FALLING (=2) - Falling edge 
*   CY_LPCOMP_INTR_BOTH (=3) - Both rising and falling edges.
*
* \return None
*
*******************************************************************************/
void Cy_LPComp_SetInterruptTriggerMode(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel, cy_en_lpcomp_int_t intType)
{   
    CY_ASSERT_L3(CY_LPCOMP_IS_CHANNEL_VALID(channel));
    CY_ASSERT_L3(CY_LPCOMP_IS_INTR_MODE_VALID(intType));

    if (CY_LPCOMP_CHANNEL_0 == channel)
    {
        LPCOMP_CMP0_CTRL(base) = _CLR_SET_FLD32U(LPCOMP_CMP0_CTRL(base), LPCOMP_CMP0_CTRL_INTTYPE0, (uint32_t)intType);
    }
    else
    {
        LPCOMP_CMP1_CTRL(base) = _CLR_SET_FLD32U(LPCOMP_CMP1_CTRL(base), LPCOMP_CMP1_CTRL_INTTYPE1, (uint32_t)intType);
    }

    /* Save interrupt type to use it in the Cy_LPComp_Enable() function */
    cy_lpcomp_context.intType[(uint8_t)channel - 1u] = intType;
}


/*******************************************************************************
* Function Name: Cy_LPComp_SetPower
****************************************************************************//**
*
* Sets the drive power and speeds to one of the four settings.
* \note Interrupts can be enabled after the block is enabled and the appropriate 
* start-up time has elapsed:
* 3 us for the normal power mode;
* 6 us for the LP mode;
* 50 us for the ULP mode. 
* Otherwise, unexpected interrupts events can occur.
*
* \param *base
*     The LPCOMP register structure pointer.
*
* \param channel
*     The LPCOMP channel index.
*
* \param power
*     The power setting sets an operation mode of the component:
*     CY_LPCOMP_OFF_POWER (=0) - Off power 
*     CY_LPCOMP_MODE_ULP (=1) - Slow/ultra low power 
*     CY_LPCOMP_MODE_LP (=2) - Medium/low power 
*     CY_LPCOMP_MODE_NORMAL(=3) - Fast/normal power
*
* \return None
*
*******************************************************************************/
void Cy_LPComp_SetPower(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel, cy_en_lpcomp_pwr_t power)
{
    CY_ASSERT_L3(CY_LPCOMP_IS_CHANNEL_VALID(channel));
    CY_ASSERT_L3(CY_LPCOMP_IS_POWER_VALID(power));

    if (CY_LPCOMP_CHANNEL_0 == channel)
    {
        LPCOMP_CMP0_CTRL(base) = _CLR_SET_FLD32U(LPCOMP_CMP0_CTRL(base), LPCOMP_CMP0_CTRL_MODE0, (uint32_t)power);
    }
    else
    {
        LPCOMP_CMP1_CTRL(base) = _CLR_SET_FLD32U(LPCOMP_CMP1_CTRL(base), LPCOMP_CMP1_CTRL_MODE1, (uint32_t)power);
    }
}


/*******************************************************************************
* Function Name: Cy_LPComp_SetHysteresis
****************************************************************************//**
*
* Adds the 30mV hysteresis to the comparator.
*
* \param *base
*     The LPCOMP register structure pointer.
*
* \param channel
*     The LPCOMP channel index.
*
* \param hysteresis
*   Sets an operation mode of the component 
*   CY_LPCOMP_HYST_ENABLE (=1) - Enables HYST 
*   CY_LPCOMP_HYST_DISABLE(=0) - Disable HYST.
*
* \return None
*
*******************************************************************************/
void Cy_LPComp_SetHysteresis(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel, cy_en_lpcomp_hyst_t hysteresis)
{
    CY_ASSERT_L3(CY_LPCOMP_IS_CHANNEL_VALID(channel));
    CY_ASSERT_L3(CY_LPCOMP_IS_HYSTERESIS_VALID(hysteresis));

    if (CY_LPCOMP_CHANNEL_0 == channel)
    {
        LPCOMP_CMP0_CTRL(base) = _CLR_SET_FLD32U(LPCOMP_CMP0_CTRL(base), LPCOMP_CMP0_CTRL_HYST0, (uint32_t)hysteresis);
    }
    else
    {
        LPCOMP_CMP1_CTRL(base) = _CLR_SET_FLD32U(LPCOMP_CMP1_CTRL(base) , LPCOMP_CMP1_CTRL_HYST1, (uint32_t)hysteresis);
    }
}


/*******************************************************************************
* Function Name: Cy_LPComp_SetInputs
****************************************************************************//**
*
* Sets the comparator input sources. The comparator inputs can be connected 
* to the dedicated GPIO pins or AMUXBUSA/AMUXBUSB. Additionally, the negative 
* comparator input can be connected to the local VREF.  
* At least one unconnected input causes a comparator undefined output.
*
* \note Connection to AMUXBUSA/AMUXBUSB requires closing the additional
* switches which are a part of the IO system. These switches can be configured 
* using the HSIOM->AMUX_SPLIT_CTL[3] register. 
* Refer to the appropriate Technical Reference Manual (TRM) of a device 
* for a detailed description.
*
* \param *base
*     The LPCOMP register structure pointer.
*
* \param channel
*     The LPCOMP channel index.
*
* \param inputP
*   Positive input selection 
*   CY_LPCOMP_SW_GPIO (0x01u) 
*   CY_LPCOMP_SW_AMUXBUSA (0x02u) - Hi-Z in hibernate mode 
*   CY_LPCOMP_SW_AMUXBUSB (0x04u) - Hi-Z in the hibernate mode.
*
* \param inputN
*   Negative input selection 
*   CY_LPCOMP_SW_GPIO (0x01u) 
*   CY_LPCOMP_SW_AMUXBUSA (0x02u) - Hi-Z in hibernate mode 
*   CY_LPCOMP_SW_AMUXBUSB (0x04u) - Hi-Z in hibernate mode 
*   CY_LPCOMP_SW_LOCAL_VREF (0x08u) - the negative input only for a crude REF.
*
* \return None
*
*******************************************************************************/
void Cy_LPComp_SetInputs(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel, cy_en_lpcomp_inputs_t inputP, cy_en_lpcomp_inputs_t inputN)
{
    uint32_t input;

    CY_ASSERT_L3(CY_LPCOMP_IS_CHANNEL_VALID(channel));
    CY_ASSERT_L3(CY_LPCOMP_IS_INPUT_P_VALID(inputP));
    CY_ASSERT_L3(CY_LPCOMP_IS_INPUT_N_VALID(inputN));

    switch(inputP)
    {
        case CY_LPCOMP_SW_AMUXBUSA:
        {
            input = (channel == CY_LPCOMP_CHANNEL_0) ? LPCOMP_CMP0_SW_CMP0_AP0_Msk : LPCOMP_CMP1_SW_CMP1_AP1_Msk;
            HSIOM_AMUX_SPLIT_CTL_3 = _CLR_SET_FLD32U(HSIOM_AMUX_SPLIT_CTL_3, CY_HSIOM_AMUX_SPLIT_CTL_SWITCH_AA_SL_SR, 3u);
            break;
        }
        case CY_LPCOMP_SW_AMUXBUSB:
        {
            input = (channel == CY_LPCOMP_CHANNEL_0) ? LPCOMP_CMP0_SW_CMP0_BP0_Msk : LPCOMP_CMP1_SW_CMP1_BP1_Msk;
            HSIOM_AMUX_SPLIT_CTL_3 = _CLR_SET_FLD32U(HSIOM_AMUX_SPLIT_CTL_3, CY_HSIOM_AMUX_SPLIT_CTL_SWITCH_BB_SL_SR, 3u);
            break;
        }
        default:
        {
            input = (channel == CY_LPCOMP_CHANNEL_0) ? LPCOMP_CMP0_SW_CMP0_IP0_Msk : LPCOMP_CMP1_SW_CMP1_IP1_Msk;
            break;
        }
    }
    
    switch(inputN)
    {
        case CY_LPCOMP_SW_AMUXBUSA:
        {
            input |= (channel == CY_LPCOMP_CHANNEL_0) ? LPCOMP_CMP0_SW_CMP0_AN0_Msk : LPCOMP_CMP1_SW_CMP1_AN1_Msk;
            HSIOM_AMUX_SPLIT_CTL_3 = _CLR_SET_FLD32U(HSIOM_AMUX_SPLIT_CTL_3, CY_HSIOM_AMUX_SPLIT_CTL_SWITCH_AA_SL_SR, 3u);
            break;
        }
        case CY_LPCOMP_SW_AMUXBUSB:
        {
            input |= (channel == CY_LPCOMP_CHANNEL_0) ? LPCOMP_CMP0_SW_CMP0_BN0_Msk : LPCOMP_CMP1_SW_CMP1_BN1_Msk;
            HSIOM_AMUX_SPLIT_CTL_3 = _CLR_SET_FLD32U(HSIOM_AMUX_SPLIT_CTL_3, CY_HSIOM_AMUX_SPLIT_CTL_SWITCH_BB_SL_SR, 3u);
            break;
        }
        case CY_LPCOMP_SW_LOCAL_VREF:
        {
            input |= (channel == CY_LPCOMP_CHANNEL_0) ? LPCOMP_CMP0_SW_CMP0_VN0_Msk : LPCOMP_CMP1_SW_CMP1_VN1_Msk;
            break;
        }
        default:
        {
            input |= (channel == CY_LPCOMP_CHANNEL_0) ? LPCOMP_CMP0_SW_CMP0_IN0_Msk : LPCOMP_CMP1_SW_CMP1_IN1_Msk;
            break;
        }
    }

    if (CY_LPCOMP_CHANNEL_0 == channel)
    {
        LPCOMP_CMP0_SW_CLEAR(base) = CY_LPCOMP_CMP0_SW_POS_Msk | CY_LPCOMP_CMP0_SW_NEG_Msk;
        LPCOMP_CMP0_SW(base) = input;
    }
    else
    {
        LPCOMP_CMP1_SW_CLEAR(base) = CY_LPCOMP_CMP1_SW_POS_Msk | CY_LPCOMP_CMP1_SW_NEG_Msk;
        LPCOMP_CMP1_SW(base) = input;
    }
}


/*******************************************************************************
* Function Name: Cy_LPComp_SetOutputMode
****************************************************************************//**
*
* Sets the type of the comparator DSI output.
*
* \param *base
*     The LPCOMP register structure pointer.
*
* \param channel
*     The LPCOMP channel index.
*
* \param outType
*   Interrupt edge trigger selection 
*   CY_LPCOMP_OUT_PULSE (=0) - the DSI output with the pulse option, no bypass 
*   CY_LPCOMP_OUT_DIRECT (=1) - the bypass mode, the direct output of the comparator 
*   CY_LPCOMP_OUT_SYNC (=2) - DSI output with the level option, it is similar to the
*   bypass mode but it is 1 cycle slow than the bypass. 
*   [DSI_LEVELx : DSI_BYPASSx] = [Bit11 : Bit10] 
*   0 : 0 = 0x00 -> Pulse (PULSE) 
*   1 : 0 = 0x02 -> Level (SYNC) 
*   x : 1 = 0x01 -> Bypass (Direct).
*
* \return None
*
*******************************************************************************/
void Cy_LPComp_SetOutputMode(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel, cy_en_lpcomp_out_t outType)
{
    CY_ASSERT_L3(CY_LPCOMP_IS_CHANNEL_VALID(channel));
    CY_ASSERT_L3(CY_LPCOMP_IS_OUT_MODE_VALID(outType));

    if (CY_LPCOMP_CHANNEL_0 == channel)
    {
        LPCOMP_CMP0_CTRL(base) = _CLR_SET_FLD32U(LPCOMP_CMP0_CTRL(base), CY_LPCOMP_CMP0_OUTPUT_CONFIG, (uint32_t)outType);
    }
    else
    {
        LPCOMP_CMP1_CTRL(base) = _CLR_SET_FLD32U(LPCOMP_CMP1_CTRL(base), CY_LPCOMP_CMP1_OUTPUT_CONFIG, (uint32_t)outType);
    }
}


/*******************************************************************************
* Function Name: Cy_LPComp_DeepSleepCallback
****************************************************************************//**
*
*  This function checks the current power mode of LPComp and then disables the 
*  LPComp block if there is no wake-up source from LPComp in the deep-sleep mode. 
*  It stores the state of the LPComp enable and then disables the LPComp block 
*  before going to the low power modes, and recovers the LPComp power state after
*  wake-up using the stored value.
*
* \param *callbackParams
*     The \ref cy_stc_syspm_callback_params_t structure with the callback 
*     parameters which consists of mode, base and context fields:
*    *base - LPComp register structure pointer;
*    *context - Context for the call-back function;
*     mode
*     CY_SYSPM_CHECK_READY - No action for this state.
*     CY_SYSPM_CHECK_FAIL - No action for this state.
*     CY_SYSPM_BEFORE_TRANSITION - Checks the LPComp interrupt mask and the power 
*                            mode, and then disables or enables the LPComp block  
*                            according to the condition.
*                            Stores the LPComp state to recover the state after
*                            wake up.
*     CY_SYSPM_AFTER_TRANSITION - Enables the LPComp block, if it was disabled   
*                            before the sleep mode.
*
* \return
* \ref cy_en_syspm_status_t
*
*******************************************************************************/
cy_en_syspm_status_t Cy_LPComp_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams)
{
    cy_en_syspm_status_t ret = CY_SYSPM_FAIL;
    LPCOMP_Type *locBase = (LPCOMP_Type *) (callbackParams->base);
    static uint32_t enabled_status;

    switch(callbackParams->mode)
    {
        case CY_SYSPM_CHECK_READY:
        {
            ret = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_CHECK_FAIL:
        {                
            ret = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_BEFORE_TRANSITION:
        {
            /* Save the LPComp the enabled/disabled status. */
            enabled_status = _FLD2VAL(LPCOMP_CONFIG_ENABLED, locBase->CONFIG);

            if (0u != enabled_status)
            {
                /* Disable the LPComp block when there is no wake-up source from any channel. */
                if( !(((_FLD2VAL(LPCOMP_CMP0_CTRL_MODE0, locBase->CMP0_CTRL) == (uint32_t)CY_LPCOMP_MODE_ULP) && 
                        _FLD2BOOL(LPCOMP_INTR_MASK_COMP0_MASK, locBase->INTR_MASK)) ||
                      ((_FLD2VAL(LPCOMP_CMP1_CTRL_MODE1, locBase->CMP1_CTRL) == (uint32_t)CY_LPCOMP_MODE_ULP) && 
                        _FLD2BOOL(LPCOMP_INTR_MASK_COMP1_MASK, locBase->INTR_MASK))) )
                    
                {
                    /* Disable the LPComp block to avoid leakage. */
                    Cy_LPComp_GlobalDisable(locBase);
                }
                else
                {
                    /* Set LPComp the status to the not changed state. */
                    enabled_status = 0u;
                }
            }
            else
            {
                /* The LPComp block was already disabled and 
                *  the system is allowed to go to the low power mode.
                */
            }

            ret = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_AFTER_TRANSITION:
        {
            /* Enable LPComp to operate if it was enabled 
            * before entering to the low power mode. 
            */
            if (0u != enabled_status)
            {
                Cy_LPComp_GlobalEnable(locBase);
            }
            else
            {
                /* The LPComp block was disabled before calling this API 
                * with mode = CY_SYSPM_CHECK_READY.
                */
            }

            ret = CY_SYSPM_SUCCESS;
        }
        break;

        default:
            break;
    }

    return (ret);
}


/*******************************************************************************
* Function Name: Cy_LPComp_HibernateCallback
****************************************************************************//**
*
*  This function checks the current power mode of LPComp and then disable the 
*  LPComp block, if there is no wake-up source from LPComp in the hibernate mode. 
*
* \param *callbackParams
*     The \ref cy_stc_syspm_callback_params_t structure with the callback 
*     parameters which consists of mode, base and context fields:
*    *base - LPComp register structure pointer;
*    *context - Context for the call-back function;
*     mode
*     CY_SYSPM_CHECK_READY - No action for this state.
*     CY_SYSPM_CHECK_FAIL - No action for this state.
*     CY_SYSPM_BEFORE_TRANSITION - Checks the wake-up source from the hibernate mode 
*                            of the LPComp block, and then disables or enables 
*                            the LPComp block according to the condition.
*
* \return
* \ref cy_en_syspm_status_t
*
*******************************************************************************/
cy_en_syspm_status_t Cy_LPComp_HibernateCallback(cy_stc_syspm_callback_params_t *callbackParams)
{
    cy_en_syspm_status_t ret = CY_SYSPM_FAIL;
    LPCOMP_Type *locBase = (LPCOMP_Type *) (callbackParams->base);
    static uint32_t enabled_status;

    switch(callbackParams->mode)
    {
        case CY_SYSPM_CHECK_READY:
        {
            ret = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_CHECK_FAIL:
        {                
            ret = CY_SYSPM_SUCCESS;
        }
        break;

        case CY_SYSPM_BEFORE_TRANSITION:
        {
            /* Save the LPComp the enabled/disabled status. */
            enabled_status = _FLD2VAL(LPCOMP_CONFIG_ENABLED, locBase->CONFIG);

            if (0u != enabled_status)
            {
                /* Disable the LPComp block when there is no wake-up source from any channel. */
                if( !(((_FLD2VAL(LPCOMP_CMP0_CTRL_MODE0, locBase->CMP0_CTRL)) == (uint32_t)CY_LPCOMP_MODE_ULP) && 
                        _FLD2BOOL(CY_LPCOMP_WAKEUP_PIN0, SRSS_PWR_HIBERNATE)) ||
                      ((_FLD2VAL(LPCOMP_CMP1_CTRL_MODE1, locBase->CMP1_CTRL) == (uint32_t)CY_LPCOMP_MODE_ULP) && 
                        _FLD2BOOL(CY_LPCOMP_WAKEUP_PIN1, SRSS_PWR_HIBERNATE)))
                    
                {
                    /* Disable the LPComp block to avoid leakage. */
                    Cy_LPComp_GlobalDisable(locBase);
                }
                else
                {
                    /* Set LPComp the status to the not changed state. */
                    enabled_status = 0u;
                }
            }
            else
            {
                /* The LPComp block was already disabled and 
                *  the system is allowed to go to the low power mode.
                */
            }

            ret = CY_SYSPM_SUCCESS;
        }
        break;

        default:
            break;
    }
    
    return (ret);
}

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXLPCOMP */

/* [] END OF FILE */
