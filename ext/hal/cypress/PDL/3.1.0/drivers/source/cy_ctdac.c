/***************************************************************************//**
* \file cy_ctdac.c
* \version 1.10
*
* Provides the public functions for the API for the CTDAC driver.
*
********************************************************************************
* \copyright
* Copyright 2017-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_ctdac.h"

#ifdef CY_IP_MXS40PASS_CTDAC

#if defined(__cplusplus)
extern "C" {
#endif

/** Static function to configure the clock */
static void Cy_CTDAC_ConfigureClock(cy_en_ctdac_update_t updateMode, cy_en_divider_types_t dividerType,
                                           uint32_t dividerNum, uint32_t dividerIntValue, uint32_t dividerFracValue);

const cy_stc_ctdac_fast_config_t Cy_CTDAC_Fast_VddaRef_UnbufferedOut =
{
    /*.refSource      */ CY_CTDAC_REFSOURCE_VDDA,
    /*.outputBuffer   */ CY_CTDAC_OUTPUT_UNBUFFERED,
};

const cy_stc_ctdac_fast_config_t Cy_CTDAC_Fast_VddaRef_BufferedOut =
{
    /*.refSource      */ CY_CTDAC_REFSOURCE_VDDA,
    /*.outputBuffer   */ CY_CTDAC_OUTPUT_BUFFERED,
};

const cy_stc_ctdac_fast_config_t Cy_CTDAC_Fast_OA1Ref_UnbufferedOut =
{
    /*.refSource      */ CY_CTDAC_REFSOURCE_EXTERNAL,
    /*.outputBuffer   */ CY_CTDAC_OUTPUT_UNBUFFERED,
};

const cy_stc_ctdac_fast_config_t Cy_CTDAC_Fast_OA1Ref_BufferedOut =
{
    /*.refSource      */ CY_CTDAC_REFSOURCE_EXTERNAL,
    /*.outputBuffer   */ CY_CTDAC_OUTPUT_BUFFERED,
};

/*******************************************************************************
* Function Name: Cy_CTDAC_Init
****************************************************************************//**
*
* Initialize all CTDAC configuration registers
*
* \param base
* Pointer to structure describing registers
*
* \param config
* Pointer to structure containing configuration data
*
* \return
* Status of initialization, \ref CY_CTDAC_SUCCESS or \ref CY_CTDAC_BAD_PARAM
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_INIT_CUSTOM
*
*******************************************************************************/
cy_en_ctdac_status_t Cy_CTDAC_Init(CTDAC_Type *base, const cy_stc_ctdac_config_t *config)
{
    CY_ASSERT_L1(NULL != base);
    CY_ASSERT_L1(NULL != config);

    cy_en_ctdac_status_t result;
    uint32_t ctdacCtrl = CY_CTDAC_DEINIT;
    uint32_t setSwitch = CY_CTDAC_DEINIT;
    uint32_t clearSwitch = CY_CTDAC_DEINIT;

    if ((NULL == base) || (NULL == config))
    {
       result = CY_CTDAC_BAD_PARAM;
    }
    else
    {

        CY_ASSERT_L3(CY_CTDAC_REFSOURCE(config->refSource));
        CY_ASSERT_L3(CY_CTDAC_FORMAT(config->formatMode));
        CY_ASSERT_L3(CY_CTDAC_UPDATE(config->updateMode));
        CY_ASSERT_L3(CY_CTDAC_DEGLITCH(config->deglitchMode));
        CY_ASSERT_L3(CY_CTDAC_OUTPUTMODE(config->outputMode));
        CY_ASSERT_L3(CY_CTDAC_OUTPUTBUFFER(config->outputBuffer));
        CY_ASSERT_L3(CY_CTDAC_DEEPSLEEP(config->deepSleep));
        CY_ASSERT_L2(CY_CTDAC_DEGLITCHCYCLES(config->deglitchCycles));

        /* Handle the deglitch counts */
        ctdacCtrl |= (config->deglitchCycles << CTDAC_CTDAC_CTRL_DEGLITCH_CNT_Pos) & CTDAC_CTDAC_CTRL_DEGLITCH_CNT_Msk;

        /* Handle the deglitch mode */
        ctdacCtrl |= (uint32_t)config->deglitchMode;

        /* Handle the update mode */
        if ((config->updateMode == CY_CTDAC_UPDATE_STROBE_EDGE_IMMEDIATE) \
        || (config->updateMode == CY_CTDAC_UPDATE_STROBE_EDGE_SYNC) \
        || (config->updateMode == CY_CTDAC_UPDATE_STROBE_LEVEL))
        {
            ctdacCtrl |= CTDAC_CTDAC_CTRL_DSI_STROBE_EN_Msk;
        }

        if (config->updateMode == CY_CTDAC_UPDATE_STROBE_LEVEL)
        {
            ctdacCtrl |= CTDAC_CTDAC_CTRL_DSI_STROBE_LEVEL_Msk;
        }

        /* Handle the sign format */
        ctdacCtrl |= (uint32_t)config->formatMode;

        /* Handle the Deep Sleep mode */
        ctdacCtrl |= (uint32_t)config->deepSleep;

        /* Handle the output mode */
        ctdacCtrl |= (uint32_t)config->outputMode;

        /* Handle the reference source */
        switch(config->refSource)
        {
        case CY_CTDAC_REFSOURCE_VDDA:

            /* Close the CVD switch to use Vdda as the reference source */
            setSwitch |= CTDAC_CTDAC_SW_CTDD_CVD_Msk;
            break;
        case CY_CTDAC_REFSOURCE_EXTERNAL:
        default:
            clearSwitch |= CTDAC_CTDAC_SW_CLEAR_CTDD_CVD_Msk;
            break;
        }

        /* Handle the output buffer switch CO6 */
        switch(config->outputBuffer)
        {
        case CY_CTDAC_OUTPUT_UNBUFFERED:

            /* Close the CO6 switch to send output to a direct pin unbuffered */
            setSwitch |= CTDAC_CTDAC_SW_CTDO_CO6_Msk;
            break;
        case CY_CTDAC_OUTPUT_BUFFERED:
        default:
            clearSwitch |= CTDAC_CTDAC_SW_CTDO_CO6_Msk;
            break;
        }

        CTDAC_INTR_MASK(base)         = (uint32_t)config->enableInterrupt << CTDAC_INTR_VDAC_EMPTY_Pos;
        CTDAC_CTDAC_SW(base)          = setSwitch;
        CTDAC_CTDAC_SW_CLEAR(base)    = clearSwitch;
        CTDAC_CTDAC_VAL(base)         = (((uint32_t)config->value) << CTDAC_CTDAC_VAL_VALUE_Pos) & CTDAC_CTDAC_VAL_VALUE_Msk;
        CTDAC_CTDAC_VAL_NXT(base)     = (((uint32_t)config->nextValue) << CTDAC_CTDAC_VAL_NXT_VALUE_Pos) & CTDAC_CTDAC_VAL_NXT_VALUE_Msk;

        if (config->configClock)
        {
            Cy_CTDAC_ConfigureClock(config->updateMode, config->dividerType, config->dividerNum, config->dividerIntValue, config->dividerFracValue);
        }

        CTDAC_CTDAC_CTRL(base)        = ctdacCtrl;
        result                  = CY_CTDAC_SUCCESS;
    }

    return result;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_DeInit
****************************************************************************//**
*
* Reset CTDAC registers back to power on reset defaults.
*
* \note
* Does not disable the clock.
*
* \param base
* Pointer to structure describing registers
*
* \param deInitRouting
* If true, all switches are reset to their default state.
* If false, switch registers are untouched.
*
* \return
* Status of initialization, \ref CY_CTDAC_SUCCESS, or \ref CY_CTDAC_BAD_PARAM
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_DEINIT
*
*******************************************************************************/
cy_en_ctdac_status_t Cy_CTDAC_DeInit(CTDAC_Type *base, bool deInitRouting)
{
    CY_ASSERT_L1(NULL != base);

    cy_en_ctdac_status_t result;

    if (NULL == base)
    {
       result = CY_CTDAC_BAD_PARAM;
    }
    else
    {
        CTDAC_CTDAC_CTRL(base)        = CY_CTDAC_DEINIT;
        CTDAC_INTR_MASK(base)         = CY_CTDAC_DEINIT;
        CTDAC_CTDAC_VAL(base)         = CY_CTDAC_DEINIT;
        CTDAC_CTDAC_VAL_NXT(base)     = CY_CTDAC_DEINIT;

        if (deInitRouting)
        {
            CTDAC_CTDAC_SW_CLEAR(base) = CY_CTDAC_DEINIT;
        }

        result                  = CY_CTDAC_SUCCESS;
    }

    return result;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_FastInit
****************************************************************************//**
*
* Initialize the CTDAC to one of the common use modes.
* This function provides a quick and easy method of configuring the CTDAC when using
* the PDL driver for device configuration.
*
* The other configuration options are set to:
*   - .formatMode       = \ref CY_CTDAC_FORMAT_UNSIGNED
*   - .updateMode       = \ref CY_CTDAC_UPDATE_BUFFERED_WRITE
*   - .deglitchMode     = \ref CY_CTDAC_DEGLITCHMODE_NONE
*   - .outputMode       = \ref CY_CTDAC_OUTPUT_VALUE
*   - .deepSleep        = \ref CY_CTDAC_DEEPSLEEP_DISABLE
*   - .deglitchCycles   = \ref CY_CTDAC_DEINIT
*   - .value            = \ref CY_CTDAC_UNSIGNED_MID_CODE_VALUE
*   - .nextValue        = \ref CY_CTDAC_UNSIGNED_MID_CODE_VALUE
*   - .enableInterrupt  = true
*   - .configClock      = true
*   - .dividerType      = \ref CY_CTDAC_FAST_CLKCFG_TYPE
*   - .dividerNum       = \ref CY_CTDAC_FAST_CLKCFG_NUM
*   - .dividerInitValue = \ref CY_CTDAC_FAST_CLKCFG_DIV
*   - .dividerFracValue = \ref CY_CTDAC_DEINIT
*
* A separate call to \ref Cy_CTDAC_Enable is needed to turn on the hardware.
*
* \param base
* Pointer to structure describing registers
*
* \param config
* Pointer to structure containing configuration data for quick initialization.
* Define your own or use one of the provided structures:
* - \ref Cy_CTDAC_Fast_VddaRef_UnbufferedOut
* - \ref Cy_CTDAC_Fast_VddaRef_BufferedOut
* - \ref Cy_CTDAC_Fast_OA1Ref_UnbufferedOut
* - \ref Cy_CTDAC_Fast_OA1Ref_BufferedOut
*
* \return
* Status of initialization, \ref CY_CTDAC_SUCCESS or \ref CY_CTDAC_BAD_PARAM
*
* \funcusage
*
* The following code snippets configures VDDA as the reference source and
* routes the output directly to Pin 6 (unbuffered).
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_FAST_INIT
*
* \funcusage
*
* The following code snippet shows how the CTDAC and CTB blocks can
* quickly be configured to work together. The code
* configures the CTDAC to use a buffered output,
* a buffered reference source from the internal bandgap voltage, and closes
* all required analog routing switches.
*
* \image html ctdac_fast_init_funcusage.png
* \image latex ctdac_fast_init_funcusage.png
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_FAST_INIT_CTB
*
*******************************************************************************/
cy_en_ctdac_status_t Cy_CTDAC_FastInit(CTDAC_Type *base, const cy_stc_ctdac_fast_config_t *config)
{
    CY_ASSERT_L1(NULL != base);
    CY_ASSERT_L1(NULL != config);

    cy_en_ctdac_status_t result;
    uint32_t ctdacCtrl;
    uint32_t setSwitch = CY_CTDAC_DEINIT;
    uint32_t clearSwitch = CY_CTDAC_DEINIT;

    if ((NULL == base) || (NULL == config))
    {
       result = CY_CTDAC_BAD_PARAM;
    }
    else
    {
        CY_ASSERT_L3(CY_CTDAC_REFSOURCE(config->refSource));
        CY_ASSERT_L3(CY_CTDAC_OUTPUTBUFFER(config->outputBuffer));

        ctdacCtrl = (uint32_t) CY_CTDAC_DEGLITCHMODE_NONE \
                    | (uint32_t) CY_CTDAC_UPDATE_BUFFERED_WRITE \
                    | (uint32_t) CY_CTDAC_FORMAT_UNSIGNED \
                    | (uint32_t) CY_CTDAC_DEEPSLEEP_DISABLE \
                    | (uint32_t) CY_CTDAC_OUTPUT_VALUE;

        /* Handle the reference source */
        switch(config->refSource)
        {
        case CY_CTDAC_REFSOURCE_VDDA:

            /* Close the CVD switch to use Vdda as the reference source */
            setSwitch |= CTDAC_CTDAC_SW_CTDD_CVD_Msk;
            break;
        case CY_CTDAC_REFSOURCE_EXTERNAL:
        default:
            clearSwitch |= CTDAC_CTDAC_SW_CLEAR_CTDD_CVD_Msk;
            break;
        }

        /* Handle the output buffer switch CO6 */
        switch(config->outputBuffer)
        {
        case CY_CTDAC_OUTPUT_UNBUFFERED:

            /* Close the CO6 switch to send output to a direct pin unbuffered */
            setSwitch |= CTDAC_CTDAC_SW_CTDO_CO6_Msk;
            break;
        case CY_CTDAC_OUTPUT_BUFFERED:
        default:
            clearSwitch |= CTDAC_CTDAC_SW_CTDO_CO6_Msk;
            break;
        }

        CTDAC_INTR_MASK(base)         = CTDAC_INTR_VDAC_EMPTY_Msk;
        CTDAC_CTDAC_SW(base)          = setSwitch;
        CTDAC_CTDAC_SW_CLEAR(base)    = clearSwitch;
        CTDAC_CTDAC_VAL(base)         = CY_CTDAC_UNSIGNED_MID_CODE_VALUE;
        CTDAC_CTDAC_VAL_NXT(base)     = CY_CTDAC_UNSIGNED_MID_CODE_VALUE;

        /* For fast configuration, the DAC clock is the Peri clock divided by 100. */
        Cy_CTDAC_ConfigureClock(CY_CTDAC_UPDATE_BUFFERED_WRITE, CY_CTDAC_FAST_CLKCFG_TYPE, CY_CTDAC_FAST_CLKCFG_NUM, CY_CTDAC_FAST_CLKCFG_DIV, CY_CTDAC_DEINIT);

        CTDAC_CTDAC_CTRL(base)        = ctdacCtrl;
        result                  = CY_CTDAC_SUCCESS;
    }

    return result;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_ConfigureClock
****************************************************************************//**
*
* Private function for configuring the CTDAC clock based on the desired
* update mode. This function is called by \ref Cy_CTDAC_Init.
*
* \param updateMode
* Update mode value. See \ref cy_en_ctdac_update_t for values.
*
* \return None
*
*******************************************************************************/
static void Cy_CTDAC_ConfigureClock(cy_en_ctdac_update_t updateMode, cy_en_divider_types_t dividerType,
                                           uint32_t dividerNum, uint32_t dividerIntValue, uint32_t dividerFracValue)
{
    if (updateMode == CY_CTDAC_UPDATE_DIRECT_WRITE)
    { /* In direct mode, there is not a clock */
    }
    else if(updateMode == CY_CTDAC_UPDATE_STROBE_EDGE_IMMEDIATE)
    {

        /* In this mode, the Peri Clock is divided by 1 to give a constant logic high on the CTDAC clock. */
        (void)Cy_SysClk_PeriphDisableDivider(dividerType, dividerNum);

        (void)Cy_SysClk_PeriphAssignDivider(PCLK_PASS_CLOCK_CTDAC, dividerType, dividerNum);

        if ((dividerType == CY_SYSCLK_DIV_8_BIT) || (dividerType == CY_SYSCLK_DIV_16_BIT))
        {
            (void)Cy_SysClk_PeriphSetDivider(dividerType, dividerNum, CY_CTDAC_STROBE_EDGE_IMMEDIATE_DIV);
        }
        else
        {
            (void)Cy_SysClk_PeriphSetFracDivider(dividerType, dividerNum, CY_CTDAC_STROBE_EDGE_IMMEDIATE_DIV, CY_CTDAC_STROBE_EDGE_IMMEDIATE_DIV_FRAC);
        }

        (void)Cy_SysClk_PeriphEnableDivider(dividerType, dividerNum);
    }
    else
    {

        /* All other modes, require a CTDAC clock configured to the desired user frequency */
        (void)Cy_SysClk_PeriphDisableDivider(dividerType, dividerNum);

        (void)Cy_SysClk_PeriphAssignDivider(PCLK_PASS_CLOCK_CTDAC, dividerType, dividerNum);

        if ((dividerType == CY_SYSCLK_DIV_8_BIT) || (dividerType == CY_SYSCLK_DIV_16_BIT))
        {
            (void)Cy_SysClk_PeriphSetDivider(dividerType, dividerNum, dividerIntValue);
        }
        else
        {
            (void)Cy_SysClk_PeriphSetFracDivider(dividerType, dividerNum, dividerIntValue, dividerFracValue);
        }
        (void)Cy_SysClk_PeriphEnableDivider(dividerType, dividerNum);
    }

}

/*******************************************************************************
* Function Name: Cy_CTDAC_SetSignMode
****************************************************************************//**
*
* Set whether to interpret the DAC value as signed or unsigned.
* In unsigned mode, the DAC value register is used without any decoding.
* In signed  mode, the MSB is inverted by adding 0x800 to the DAC value.
* This converts the lowest signed number, 0x800, to the lowest unsigned
* number, 0x000.
*
* \param base
* Pointer to structure describing registers
*
* \param formatMode
* Mode can be signed or unsigned. See \ref cy_en_ctdac_format_t for values.
*
* \return None
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_SET_SIGN_MODE
*
*******************************************************************************/
void Cy_CTDAC_SetSignMode(CTDAC_Type *base, cy_en_ctdac_format_t formatMode)
{
    CY_ASSERT_L3(CY_CTDAC_FORMAT(formatMode));

    uint32_t ctdacCtrl;

    /* Clear the CTDAC_MODE bits */
    ctdacCtrl = CTDAC_CTDAC_CTRL(base) & ~CTDAC_CTDAC_CTRL_CTDAC_MODE_Msk;

    CTDAC_CTDAC_CTRL(base) = ctdacCtrl | (uint32_t)formatMode;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_SetDeepSleepMode
****************************************************************************//**
*
* Enable or disable the DAC hardware operation in Deep Sleep mode.
*
* \param base
* Pointer to structure describing registers
*
* \param deepSleep
* Enable or disable Deep Sleep operation. Select value from \ref cy_en_ctdac_deep_sleep_t.
*
* \return None
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_SET_DEEPSLEEP_MODE
*
*******************************************************************************/
void Cy_CTDAC_SetDeepSleepMode(CTDAC_Type *base, cy_en_ctdac_deep_sleep_t deepSleep)
{
    CY_ASSERT_L3(CY_CTDAC_DEEPSLEEP(deepSleep));

    uint32_t ctdacCtrl;

    ctdacCtrl = CTDAC_CTDAC_CTRL(base) & ~CTDAC_CTDAC_CTRL_DEEPSLEEP_ON_Msk;

    CTDAC_CTDAC_CTRL(base) = ctdacCtrl | (uint32_t)deepSleep;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_SetOutputMode
****************************************************************************//**
*
* Set the output mode of the CTDAC:
*   - \ref CY_CTDAC_OUTPUT_HIGHZ : Disable the output
*   - \ref CY_CTDAC_OUTPUT_VALUE : Enable the output and drive the value
*       stored in the CTDAC_VAL register.
*   - \ref CY_CTDAC_OUTPUT_VALUE_PLUS1 : Enable the output and drive the
*       value stored in the CTDAC_VAL register plus 1.
*   - \ref CY_CTDAC_OUTPUT_VSSA : Output pulled to VSSA through 1.1 MOhm (typ) resistor.
*   - \ref CY_CTDAC_OUTPUT_VREF : Output pulled to VREF through 1.1 MOhm (typ) resistor.
*
* \param base
* Pointer to structure describing registers
*
* \param outputMode
* Select a value from \ref cy_en_ctdac_output_mode_t.
*
* \return None
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_SET_OUTPUT_MODE
*
*******************************************************************************/
void Cy_CTDAC_SetOutputMode(CTDAC_Type *base, cy_en_ctdac_output_mode_t outputMode)
{
    CY_ASSERT_L3(CY_CTDAC_OUTPUTMODE(outputMode));

    uint32_t ctdacCtrl;

    /* Clear out the three affected bits */
    ctdacCtrl = CTDAC_CTDAC_CTRL(base) & ~(CTDAC_CTDAC_CTRL_OUT_EN_Msk | CTDAC_CTDAC_CTRL_DISABLED_MODE_Msk | CTDAC_CTDAC_CTRL_CTDAC_RANGE_Msk);

    CTDAC_CTDAC_CTRL(base) = ctdacCtrl | (uint32_t)outputMode;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_SetDeglitchMode
****************************************************************************//**
*
* Enable deglitching on the unbuffered path, buffered path, both, or
* disable deglitching. The deglitch mode should match the configured output path.
*
* \param base
* Pointer to structure describing registers
*
* \param deglitchMode
* Deglitching mode selection. See \ref cy_en_ctdac_deglitch_t for values.
*
* \return None
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_SET_DEGLITCH_MODE
*
*******************************************************************************/
void Cy_CTDAC_SetDeglitchMode(CTDAC_Type *base, cy_en_ctdac_deglitch_t deglitchMode)
{
    CY_ASSERT_L3(CY_CTDAC_DEGLITCH(deglitchMode));

    uint32_t ctdacCtrl;

    /* Clear out DEGLITCH_CO6 and DEGLITCH_C0S bits */
    ctdacCtrl = CTDAC_CTDAC_CTRL(base) & ~(CTDAC_CTDAC_CTRL_DEGLITCH_COS_Msk | CTDAC_CTDAC_CTRL_DEGLITCH_CO6_Msk);

    CTDAC_CTDAC_CTRL(base) = ctdacCtrl | (uint32_t)deglitchMode;
}

/*******************************************************************************
* Function Name: Cy_CTDAC_SetDeglitchCycles
****************************************************************************//**
*
* Set the number of deglitch cycles (0 to 63) that will be used.
* To calculate the deglitch time:
*
*       (DEGLITCH_CNT + 1) / PERI_CLOCK_FREQ
*
* The optimal deglitch time is 700 ns.
*
* \param base
* Pointer to structure describing registers
*
* \param deglitchCycles
* Number of cycles to deglitch
*
* \return None
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_SET_DEGLITCH_CYCLES
*
*******************************************************************************/
void Cy_CTDAC_SetDeglitchCycles(CTDAC_Type *base, uint32_t deglitchCycles)
{
    CY_ASSERT_L2(CY_CTDAC_DEGLITCHCYCLES(deglitchCycles));

    uint32_t ctdacCtrl;

    ctdacCtrl = (CTDAC_CTDAC_CTRL(base)) & ~CTDAC_CTDAC_CTRL_DEGLITCH_CNT_Msk;

    CTDAC_CTDAC_CTRL(base) = ctdacCtrl | ((deglitchCycles << CTDAC_CTDAC_CTRL_DEGLITCH_CNT_Pos) & CTDAC_CTDAC_CTRL_DEGLITCH_CNT_Msk);
}

/*******************************************************************************
* Function Name: Cy_CTDAC_SetRef
****************************************************************************//**
*
* Set the CTDAC reference source to Vdda or an external reference.
* The external reference must come from Opamp1 of the CTB.
*
* \param base
* Pointer to structure describing registers
*
* \param refSource
* The reference source. Select a value from \ref cy_en_ctdac_ref_source_t.
*
* \return None
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_SET_REF
*
*******************************************************************************/
void Cy_CTDAC_SetRef(CTDAC_Type *base, cy_en_ctdac_ref_source_t refSource)
{
    CY_ASSERT_L3(CY_CTDAC_REFSOURCE(refSource));

    switch(refSource)
    {
    case CY_CTDAC_REFSOURCE_VDDA:

        /* Close the CVD switch to use Vdda as the reference source */
        CTDAC_CTDAC_SW(base) |= CTDAC_CTDAC_SW_CTDD_CVD_Msk;
        break;
    case CY_CTDAC_REFSOURCE_EXTERNAL:
    default:
        CTDAC_CTDAC_SW_CLEAR(base) = CTDAC_CTDAC_SW_CLEAR_CTDD_CVD_Msk;
        break;
    }
}

/*******************************************************************************
* Function Name: Cy_CTDAC_SetAnalogSwitch
****************************************************************************//**
*
* Provide firmware control of the CTDAC switches. Each call to this function
* can open a set of switches or close a set of switches.
*
* \note
* The switches are configured by the reference
* source and output mode selections during initialization.
*
* \param base
* Pointer to structure describing registers
*
* \param switchMask
* The mask of the switches to either open or close.
* Select one or more values from \ref cy_en_ctdac_switches_t and "OR" them together.
*
* \param state
* Open or close the switche(s). Select a value from \ref cy_en_ctdac_switch_state_t.
*
* \return None
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_SET_ANALOG_SWITCH
*
*******************************************************************************/
void Cy_CTDAC_SetAnalogSwitch(CTDAC_Type *base, uint32_t switchMask, cy_en_ctdac_switch_state_t state)
{
    CY_ASSERT_L2(CY_CTDAC_SWITCHMASK(switchMask));
    CY_ASSERT_L3(CY_CTDAC_SWITCHSTATE(state));

    switch(state)
    {
    case CY_CTDAC_SWITCH_CLOSE:
        CTDAC_CTDAC_SW(base) |= switchMask;
        break;
    case CY_CTDAC_SWITCH_OPEN:
    default:

        /* Unlike the close case, do not OR the register. Set 1 to clear.*/
        CTDAC_CTDAC_SW_CLEAR(base) = switchMask;
        break;
    }
}

/*******************************************************************************
* Function Name: Cy_CTDAC_DeepSleepCallback
****************************************************************************//**
*
* Callback to prepare the CTDAC before entering and after exiting Deep Sleep
* mode. If deglitching is used, it is disabled before entering Deep Sleep
* to ensure the deglitch switches are closed. This is needed only
* if the CTDAC will be enabled in DeepSleep. Upon wakeup, deglitching will
* be re-enabled if it was previously used.
*
* \param callbackParams
* Pointer to structure of type \ref cy_stc_syspm_callback_params_t
*
* \return
* See \ref cy_en_syspm_status_t
*
* \funcusage
*
* \snippet ctdac_sut_01.cydsn/main_cm4.c CTDAC_SNIPPET_DEEP_SLEEP_CALLBACK
*
*******************************************************************************/
cy_en_syspm_status_t Cy_CTDAC_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams)
{
    /* Static variable preserved between function calls.
    * Tracks the state of the deglitch mode before sleep so that it can be re-enabled after wakeup */
    static uint32_t deglitchModeBeforeSleep;

    cy_en_syspm_status_t returnValue = CY_SYSPM_SUCCESS;

    CTDAC_V1_Type *ctdacBase = (CTDAC_V1_Type *)callbackParams->base;

    if (CY_SYSPM_BEFORE_TRANSITION == callbackParams->mode)
    { /* Actions that should be done before entering the Deep Sleep mode */

        /* Store the state of the deglitch switches before turning deglitch off */
        deglitchModeBeforeSleep = ctdacBase->CTDAC_CTRL & (CTDAC_CTDAC_CTRL_DEGLITCH_CO6_Msk | CTDAC_CTDAC_CTRL_DEGLITCH_COS_Msk);

        /* Turn deglitch off before entering Deep Sleep */
        ctdacBase->CTDAC_CTRL &= ~(CTDAC_CTDAC_CTRL_DEGLITCH_CO6_Msk | CTDAC_CTDAC_CTRL_DEGLITCH_COS_Msk);
    }
    else if (CY_SYSPM_AFTER_TRANSITION == callbackParams->mode)
    { /* Actions that should be done after exiting the Deep Sleep mode */

        /* Re-enable the deglitch mode that was configured before Deep Sleep entry */
        ctdacBase->CTDAC_CTRL |= deglitchModeBeforeSleep;
    }
    else
    { /* Does nothing in other modes */
    }

    return returnValue;
}

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXS40PASS_CTDAC */

/* [] END OF FILE */

