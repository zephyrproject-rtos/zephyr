/***************************************************************************//**
* \file cy_sysclk.c
* \version 1.20
*
* Provides an API implementation of the sysclk driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/


#include "cy_sysclk.h"
#include "cy_syslib.h"
#include <math.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/* # of elements in an array */
#define  CY_SYSCLK_N_ELMTS(a)  (sizeof(a) / sizeof((a)[0]))

/* ========================================================================== */
/* ===========================    ECO SECTION    ============================ */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_eco_funcs
* \{
*/
/*******************************************************************************
* Function Name: Cy_SysClk_EcoConfigure
****************************************************************************//**
*
* Configures the external crystal oscillator (ECO) trim bits based on crystal 
* characteristics. This function should be called only when the ECO is disabled.
*
* \param freq Operating frequency of the crystal in Hz.
*
* \param cLoad Crystal load capacitance in pF.
*
* \param esr Effective series resistance of the crystal in ohms.
*
* \param driveLevel Crystal drive level in uW.
*
* \return Error / status code:<br>
* CY_SYSCLK_SUCCESS - ECO configuration completed successfully<br>
* CY_SYSCLK_BAD_PARAM - One or more invalid parameters<br>
* CY_SYSCLK_INVALID_STATE - ECO already enabled
*
* \note
* The following calculations are implemented, generally in floating point:
*
* \verbatim
*   freqMHz = freq / 1000000
*   max amplitude Vpp = 1000 * sqrt(drivelevel / 2 / esr) / 3.14 / freqMHz / cLoad
*   gm_min mA/V = 5 * 4 * 3.14 * 3.14 * freqMhz^2 * cLoad^2 * 4 * esr / 1000000000
*   Number of amplifier sections = INT(gm_min / 4.5)
*
*   As a result of the above calculations, max amplitude must be >= 0.5, and the
*   number of amplifier sections must be <= 3, otherwise this function returns with
*   a parameter error.
*
*   atrim = if (max amplitude < 0.5) then error
*           else 2 * the following:
*                    max amplitude < 0.6: 0
*                    max amplitude < 0.7: 1
*                    max amplitude < 0.8: 2
*                    max amplitude < 0.9: 3
*                    max amplitude < 1.15: 5
*                    max amplitude < 1.275: 6
*                    max amplitude >= 1.275: 7
*   wdtrim = if (max amplitude < 0.5) then error
*            else 2 * the following:
*                     max amplitude < 1.2: INT(5 * max amplitude) - 2
*                     max amplitude >= 1.2: 3
*   gtrim = if (number of amplifier sections > 3) then error
*           else the following:
*                number of amplifier sections > 1: number of amplifier sections
*                number of amplifier sections = 1: 0
*                number of amplifier sections < 1: 1
*   rtrim = if (gtrim = error) then error
*           else the following:
*                freqMHz > 26.8: 0
*                freqMHz > 23.33: 1
*                freqMHz > 16.5: 2
*                freqMHz <= 16.5: 3
*   ftrim = if (atrim = error) then error
*           else INT(atrim / 2)
* \endverbatim
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_EcoConfigure
*
*******************************************************************************/
cy_en_sysclk_status_t Cy_SysClk_EcoConfigure(uint32_t freq, uint32_t cLoad, uint32_t esr, uint32_t driveLevel)
{
    /* error if ECO is not disabled - any of the 3 enable bits are set */
    cy_en_sysclk_status_t rtnval = CY_SYSCLK_INVALID_STATE;
    if ((SRSS_CLK_ECO_CONFIG & 0xE0000000UL) == 0UL)
    {
        /* calculate intemediate values */
        float32_t freqMHz = (float32_t)freq / 1000000.0f;
        float32_t maxAmplitude =
            (1000.0f * ((float32_t)sqrt((float64_t)((float32_t)driveLevel / (2.0f * (float32_t)esr))))) /
            (3.14f * freqMHz * (float32_t)cLoad);
        float32_t gm_min =
            (788.8f /*5 * 4 * 3.14 * 3.14 * 4*/ * freqMHz * freqMHz * (float32_t)cLoad * (float32_t)cLoad) /
            1000000000.0f;
        uint32_t nAmpSections = (uint32_t)(gm_min / 4.5f);

        /* Error if input parameters cause erroneous intermediate values. */
        rtnval = CY_SYSCLK_BAD_PARAM;
        if ((maxAmplitude >= 0.5f) && (nAmpSections <= 3UL))
        {
            uint32_t atrim, wdtrim, gtrim, rtrim, ftrim, reg;

            atrim = 2UL * ((maxAmplitude < 0.6f) ? 0UL :
                           ((maxAmplitude < 0.7f) ? 1UL :
                            ((maxAmplitude < 0.8f) ? 2UL :
                             ((maxAmplitude < 0.9f) ? 3UL :
                              ((maxAmplitude < 1.15f) ? 5UL :
                               ((maxAmplitude < 1.275f) ? 6UL : 7UL))))));

            wdtrim = 2UL * ((maxAmplitude < 1.2f) ? (uint32_t)(5.0f * maxAmplitude) - 2UL : 3UL);

            gtrim = ((nAmpSections > 1UL) ? nAmpSections :
                     ((nAmpSections == 1UL) ? 0UL : 1UL));

            rtrim = ((freqMHz > 26.8f) ? 0UL :
                     ((freqMHz > 23.33f) ? 1UL :
                      ((freqMHz > 16.5f) ? 2UL : 3UL)));

            ftrim = atrim / 2UL;

            /* update all fields of trim control register with one write, without
               changing the ITRIM field in bits [21:16]:
                 gtrim:  bits [13:12]
                 rtrim:  bits [11:10]
                 ftrim:  bits  [9:8]
                 atrim:  bits  [7:4]
                 wdtrim: bits  [2:0]
            */
            reg = (SRSS_CLK_TRIM_ECO_CTL & ~0x3FFFUL);
            reg |= (gtrim  & 3UL)    << 12;
            reg |= (rtrim  & 3UL)    << 10;
            reg |= (ftrim  & 3UL)    << 8;
            reg |= (atrim  & 0x0FUL) << 4;
            reg |= (wdtrim & 7UL);
            SRSS_CLK_TRIM_ECO_CTL = reg;

            rtnval = CY_SYSCLK_SUCCESS;
        } /* if valid parameters */
    } /* if ECO not enabled */

    return (rtnval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_EcoEnable
****************************************************************************//**
*
* Enables the external crystal oscillator (ECO). This function should be called
* after \ref Cy_SysClk_EcoConfigure.
*
* \param timeoutus Amount of time in microseconds to wait for the ECO to lock.
* If a lock does not occur, the ECO is stopped. To avoid waiting for a lock, set
* this parameter to 0.
*
* \return Error / status code:<br>
* CY_SYSCLK_SUCCESS - ECO locked<br>
* CY_SYSCLK_TIMEOUT - ECO timed out and did not lock
* CY_SYSCLK_INVALID_STATE - ECO already enabled
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_EcoEnable
*
*******************************************************************************/
cy_en_sysclk_status_t Cy_SysClk_EcoEnable(uint32_t timeoutus)
{
    cy_en_sysclk_status_t rtnval = CY_SYSCLK_INVALID_STATE;

    /* invalid state error if ECO is already enabled */
    if (_FLD2VAL(SRSS_CLK_ECO_CONFIG_ECO_EN, SRSS_CLK_ECO_CONFIG) == 0UL) /* 1 = enabled */
    {
        /* first set ECO enable */
        SRSS_CLK_ECO_CONFIG |= _VAL2FLD(SRSS_CLK_ECO_CONFIG_ECO_EN, 1UL); /* 1 = enable */

        /* now do the timeout wait for ECO_STATUS, bit ECO_OK */
        for (;
             ((_FLD2VAL(SRSS_CLK_ECO_STATUS_ECO_READY, SRSS_CLK_ECO_STATUS) == 0UL)) &&(timeoutus != 0UL);
             timeoutus--)
        {
            Cy_SysLib_DelayUs(1U);
        }
        rtnval = ((timeoutus == 0UL) ? CY_SYSCLK_TIMEOUT : CY_SYSCLK_SUCCESS);
    }
    return (rtnval);
}
/** \} group_sysclk_eco_funcs */


/* ========================================================================== */
/* ====================    INPUT MULTIPLEXER SECTION    ===================== */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_path_src_funcs
* \{
*/
/*******************************************************************************
* Function Name: Cy_SysClk_ClkPathSetSource
****************************************************************************//**
*
* Configures the source for the specified clock path.
*
* \param clkPath Selects which clock path to configure; 0 is the first clock
* path, which is the FLL.
*
* \param source \ref cy_en_clkpath_in_sources_t
*
* \return \ref cy_en_sysclk_status_t
*
* \note
* If calling this function changes an FLL or PLL input frequency, disable the FLL
* or PLL before calling this function. After calling this function, call the FLL
* or PLL configure function, for example \ref Cy_SysClk_FllConfigure().
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkPathSetSource
*
*******************************************************************************/
cy_en_sysclk_status_t Cy_SysClk_ClkPathSetSource(uint32_t clkPath, cy_en_clkpath_in_sources_t source)
{
    cy_en_sysclk_status_t retval = CY_SYSCLK_BAD_PARAM;
    if ((clkPath < CY_SRSS_NUM_CLKPATH) &&
        ((source <= CY_SYSCLK_CLKPATH_IN_DSIMUX) ||
         ((CY_SYSCLK_CLKPATH_IN_DSI <= source) && (source <= CY_SYSCLK_CLKPATH_IN_PILO))))
    {
        if (source >= CY_SYSCLK_CLKPATH_IN_DSI)
        {
            SRSS_CLK_DSI_SELECT[clkPath] = _VAL2FLD(SRSS_CLK_DSI_SELECT_DSI_MUX, (uint32_t)source);
            SRSS_CLK_PATH_SELECT[clkPath] = _VAL2FLD(SRSS_CLK_PATH_SELECT_PATH_MUX, (uint32_t)CY_SYSCLK_CLKPATH_IN_DSIMUX);
        }
        else
        {
            SRSS_CLK_PATH_SELECT[clkPath] = _VAL2FLD(SRSS_CLK_PATH_SELECT_PATH_MUX, (uint32_t)source);
        }
        retval = CY_SYSCLK_SUCCESS;
    }
    return (retval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkPathGetSource
****************************************************************************//**
*
* Reports which source is selected for the path mux.
*
* \param clkPath Selects which clock path to report; 0 is the first clock path,
* which is the FLL.
*
* \return \ref cy_en_clkpath_in_sources_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_ClkPathGetSource
*
*******************************************************************************/
cy_en_clkpath_in_sources_t Cy_SysClk_ClkPathGetSource(uint32_t clkPath)
{
    CY_ASSERT_L1(clkPath < CY_SRSS_NUM_CLKPATH);
    cy_en_clkpath_in_sources_t rtnval =
        (cy_en_clkpath_in_sources_t )_FLD2VAL(SRSS_CLK_PATH_SELECT_PATH_MUX, SRSS_CLK_PATH_SELECT[clkPath]);
    if (rtnval == CY_SYSCLK_CLKPATH_IN_DSIMUX)
    {
        rtnval = (cy_en_clkpath_in_sources_t)(CY_SYSCLK_CLKPATH_IN_DSI |
                    (_FLD2VAL(SRSS_CLK_DSI_SELECT_DSI_MUX, SRSS_CLK_DSI_SELECT[clkPath])));
    }
    return rtnval;
}
/** \} group_sysclk_path_src_funcs */


/* ========================================================================== */
/* ===========================    FLL SECTION    ============================ */
/* ========================================================================== */
/* min and max FLL output frequencies, in Hz */
#define  CY_SYSCLK_MIN_FLL_CCO_OUTPUT_FREQ  48000000UL
#define  CY_SYSCLK_MIN_FLL_OUTPUT_FREQ     (CY_SYSCLK_MIN_FLL_CCO_OUTPUT_FREQ / 2U)
#define  CY_SYSCLK_MAX_FLL_OUTPUT_FREQ     100000000UL

/**
* \addtogroup group_sysclk_fll_funcs
* \{
*/
/*******************************************************************************
* Function Name: Cy_SysClk_FllConfigure
****************************************************************************//**
*
* Configures the FLL, for best accuracy optimization.
*
* \param inputFreq frequency of input source, in Hz
*
* \param outputFreq Desired FLL output frequency, in Hz. Allowable range is
* 24 MHz to 100 MHz. In all cases, FLL_OUTPUT_DIV must be set; the output divide
* by 2 option is required.
*
* \param outputMode \ref cy_en_fll_pll_output_mode_t
* If output mode is bypass, then the output frequency equals the input source
* frequency regardless of the frequency parameter values.
*
* \return  Error / status code:<br>
* CY_SYSCLK_SUCCESS - FLL successfully configured<br>
* CY_SYSCLK_INVALID_STATE - FLL not configured because it is enabled<br>
* CY_SYSCLK_BAD_PARAM - desired output frequency is out of valid range
*
* \note
* Call this function after changing the FLL input frequency, for example if
* \ref Cy_SysClk_ClkPathSetSource() is called.
* \note
* Do not call this function when the FLL is enabled. If it is, then this function
* returns immediately with an error return value and no register updates.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_FllConfigure
*
*******************************************************************************/
cy_en_sysclk_status_t Cy_SysClk_FllConfigure(uint32_t inputFreq, uint32_t outputFreq, cy_en_fll_pll_output_mode_t outputMode)
{
    cy_en_sysclk_status_t returnStatus = CY_SYSCLK_SUCCESS;

    /* check for errors */
    if (_FLD2VAL(SRSS_CLK_FLL_CONFIG_FLL_ENABLE, SRSS_CLK_FLL_CONFIG) != 0U) /* 1 = enabled */
    {
        returnStatus = CY_SYSCLK_INVALID_STATE;
    }
    else if ((outputFreq < CY_SYSCLK_MIN_FLL_OUTPUT_FREQ) || (CY_SYSCLK_MAX_FLL_OUTPUT_FREQ < outputFreq)) /* invalid output frequency */
    {
        returnStatus = CY_SYSCLK_BAD_PARAM;
    }
    else if (((float32_t)outputFreq / (float32_t)inputFreq) < 2.2f) /* check output/input frequency ratio */
    {
        returnStatus = CY_SYSCLK_BAD_PARAM;
    }
    else
    { /* return status is OK */
    }

    /* no error */
    if (returnStatus == CY_SYSCLK_SUCCESS) /* no errors */
    {
        /* If output mode is bypass (input routed directly to output), then done.
           The output frequency equals the input frequency regardless of the
           frequency parameters. */
        if (outputMode != CY_SYSCLK_FLLPLL_OUTPUT_INPUT)
        {
            cy_stc_fll_manual_config_t config;
            uint32_t ccoFreq;
            bool wcoSource = ((Cy_SysClk_ClkPathGetSource(0UL/*FLL*/) == CY_SYSCLK_CLKPATH_IN_WCO) ? true : false);

            config.outputMode = outputMode;
            /* 1. Output division by 2 is always required. */
            config.enableOutputDiv = (bool)(1UL);
            /* 2. Compute the target CCO frequency from the target output frequency and output division. */
            ccoFreq = outputFreq * ((uint32_t)(config.enableOutputDiv) + 1UL);
            /* 3. Compute the CCO range value from the CCO frequency */
            config.ccoRange = ((ccoFreq >= 150339200UL) ? CY_SYSCLK_FLL_CCO_RANGE4 :
                               ((ccoFreq >= 113009380UL) ? CY_SYSCLK_FLL_CCO_RANGE3 :
                                ((ccoFreq >=  84948700UL) ? CY_SYSCLK_FLL_CCO_RANGE2 :
                                 ((ccoFreq >=  63855600UL) ? CY_SYSCLK_FLL_CCO_RANGE1 : CY_SYSCLK_FLL_CCO_RANGE0))));
            {
                /* constants indexed by ccoRange */
                const float32_t trimSteps[] = {0.0011034f, 0.001102f, 0.0011f, 0.0011f, 0.00117062f};
                const float32_t fMargin[] = {43600000.0f, 58100000.0f, 77200000.0f, 103000000.0f, 132000000.0f};

            /* 4. Compute the FLL reference divider value.
                  refDiv is a constant if the WCO is the FLL source, otherwise the formula is
                  refDiv = ROUNDUP((inputFreq / outputFreq) * 250) */
                config.refDiv = wcoSource ? 19u :
                                            ((uint16_t)ceilf(((float32_t)inputFreq / (float32_t)outputFreq) * 250.0f));
            /* 5. Compute the FLL multiplier value.
                  Formula is fllMult = ccoFreq / (inputFreq / refDiv) */
                config.fllMult = CY_SYSCLK_DIV_ROUNDUP(ccoFreq, CY_SYSCLK_DIV_ROUND(inputFreq, config.refDiv));
            /* 6. Compute the lock tolerance.
                  Formula is lock tolerance = 1.5 * fllMult * (((1 + CCO accuracy) / (1 - source clock accuracy)) - 1)
                  We assume CCO accuracy is 0.25%.
                  We assume the source clock accuracy = 1%. This is the accuracy of the IMO.
                  Therefore the formula is lock tolerance = 1.5 * fllMult * 0.012626 = 0.018939 * fllMult */
                config.lockTolerance = (uint16_t)ceilf((float32_t)(config.fllMult) * 0.018939f);
            /* 7. Compute the CCO igain and pgain. */
                {
                    /* intermediate parameters */
                    float32_t kcco = (trimSteps[config.ccoRange] * fMargin[config.ccoRange]) / 1000.0f;
                    float32_t ki_p = (0.85f / (kcco * ((float32_t)(config.refDiv) / (float32_t)inputFreq))) / 1000.0f;

                    /* igain and pgain bitfield values correspond to: 1/256, 1/128, ..., 4, 8 */
                    const float32_t gains[] = {0.00390625f, 0.0078125f, 0.015625f, 0.03125f, 0.0625f, 0.125f, 0.25f,
                                               0.5f, 1.0f, 2.0f, 4.0f, 8.0f};

                    /* find the largest IGAIN value that is less than or equal to ki_p */
                    for(config.igain = CY_SYSCLK_N_ELMTS(gains) - 1UL;
                        (gains[config.igain] > ki_p) && (config.igain != 0UL); config.igain--){}
                    /* decrement igain if the WCO is the FLL source */
                    if (wcoSource && (config.igain > 0U))
                    {
                        config.igain--;
                    }
                    /* then find the largest PGAIN value that is less than or equal to ki_p - gains[igain] */
                    for(config.pgain = CY_SYSCLK_N_ELMTS(gains) - 1UL;
                        (gains[config.pgain] > (ki_p - gains[config.igain])) && (config.pgain != 0UL);
                        config.pgain--){}
                    /* decrement pgain if the WCO is the FLL source */
                    if (wcoSource && (config.pgain > 0U))
                    {
                        config.pgain--;
                    }
                }
            /* 8. Compute the CCO_FREQ bits in CLK_FLL_CONFIG4 register. */
                config.cco_Freq = (uint16_t)
                    (floor(log((float32_t)ccoFreq / fMargin[config.ccoRange]) /
                           log(1.0f + trimSteps[config.ccoRange])));
            }
            /* 9. Compute the settling count, using a 1-usec settling time.
                  Use a constant if the WCO is the FLL source. */
            {
                float32_t ttref   = (float32_t)config.refDiv / ((float32_t)inputFreq / 1000.0f);
                float32_t testval = 6000.0f / (float32_t)outputFreq;
                float32_t divval  = ceil((float32_t)inputFreq * 0.000001f);
                float32_t altval  = ceil((divval / ttref) + 1.0f);
                config.settlingCount = (uint16)(wcoSource ? 200U : 
                                                ((ttref > testval) ? divval :
                                                 ((divval > altval) ? divval : altval)));
            }
            /* configure FLL based on calculated values */
            returnStatus = Cy_SysClk_FllManualConfigure(&config);
        } /* if not bypass output mode */

        else
        { /* bypass mode */
            /* update CLK_FLL_CONFIG3 register with divide by 2 parameter */
            CY_REG32_CLR_SET(SRSS_CLK_FLL_CONFIG3, SRSS_CLK_FLL_CONFIG3_BYPASS_SEL, (uint32_t)outputMode);
        }
    } /* if no error */

    return (returnStatus);
}

/*******************************************************************************
* Function Name: Cy_SysClk_FllManualConfigure
****************************************************************************//**
*
* Manually configures the FLL based on user inputs.
*
* \param config \ref cy_stc_fll_manual_config_t
*
* \return  Error / status code:<br>
* CY_SYSCLK_SUCCESS - FLL successfully configured<br>
* CY_SYSCLK_INVALID_STATE - FLL not configured because it is enabled
*
* \note
* Call this function after changing the FLL input frequency, for example if
* \ref Cy_SysClk_ClkPathSetSource() is called.
* \note
* Do not call this function when the FLL is enabled. If it is, then this function
* returns immediately with an error return value and no register updates.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_FllManualConfigure
*
*******************************************************************************/
cy_en_sysclk_status_t Cy_SysClk_FllManualConfigure(const cy_stc_fll_manual_config_t *config)
{
    cy_en_sysclk_status_t returnStatus = CY_SYSCLK_SUCCESS;

    CY_ASSERT_L1(config != NULL);
    /* check for errors */
    if (_FLD2VAL(SRSS_CLK_FLL_CONFIG_FLL_ENABLE, SRSS_CLK_FLL_CONFIG) != 0U) /* 1 = enabled */
    {
        returnStatus = CY_SYSCLK_INVALID_STATE;
    }
    else
    { /* return status is OK */
    }

    /* no error */
    if (returnStatus == CY_SYSCLK_SUCCESS) /* no errors */
    {
        /* update CLK_FLL_CONFIG register with 2 parameters; FLL_ENABLE is already 0 */
        /* asserts just check for bitfield overflow */
        CY_ASSERT_L1(config->fllMult <= (SRSS_CLK_FLL_CONFIG_FLL_MULT_Msk >> SRSS_CLK_FLL_CONFIG_FLL_MULT_Pos));
        uint32_t reg = _VAL2FLD(SRSS_CLK_FLL_CONFIG_FLL_MULT, config->fllMult);
        /* no assert check for enableOutputDiv, because it's a type boolean */
        SRSS_CLK_FLL_CONFIG = reg | _VAL2FLD(SRSS_CLK_FLL_CONFIG_FLL_OUTPUT_DIV, (uint32_t)(config->enableOutputDiv));

        /* update CLK_FLL_CONFIG2 register with 2 parameters */
        /* asserts just check for bitfield overflow */
        CY_ASSERT_L1(config->refDiv <= (SRSS_CLK_FLL_CONFIG2_FLL_REF_DIV_Msk >> SRSS_CLK_FLL_CONFIG2_FLL_REF_DIV_Pos));
        CY_ASSERT_L1(config->lockTolerance <= (SRSS_CLK_FLL_CONFIG2_LOCK_TOL_Msk >> SRSS_CLK_FLL_CONFIG2_LOCK_TOL_Pos));
        reg = _VAL2FLD(SRSS_CLK_FLL_CONFIG2_FLL_REF_DIV, config->refDiv);
        SRSS_CLK_FLL_CONFIG2 = reg | _VAL2FLD(SRSS_CLK_FLL_CONFIG2_LOCK_TOL, config->lockTolerance);

        /* update CLK_FLL_CONFIG3 register with 4 parameters */
        /* asserts just check for bitfield overflow */
        CY_ASSERT_L1(config->igain <= (SRSS_CLK_FLL_CONFIG3_FLL_LF_IGAIN_Msk >> SRSS_CLK_FLL_CONFIG3_FLL_LF_IGAIN_Pos));
        CY_ASSERT_L1(config->pgain <= (SRSS_CLK_FLL_CONFIG3_FLL_LF_PGAIN_Msk >> SRSS_CLK_FLL_CONFIG3_FLL_LF_PGAIN_Pos));
        CY_ASSERT_L1(config->settlingCount <= (SRSS_CLK_FLL_CONFIG3_SETTLING_COUNT_Msk >> SRSS_CLK_FLL_CONFIG3_SETTLING_COUNT_Pos));
        reg  = _VAL2FLD(SRSS_CLK_FLL_CONFIG3_FLL_LF_IGAIN, config->igain);
        reg |= _VAL2FLD(SRSS_CLK_FLL_CONFIG3_FLL_LF_PGAIN, config->pgain);
        reg |= _VAL2FLD(SRSS_CLK_FLL_CONFIG3_SETTLING_COUNT, config->settlingCount);
        SRSS_CLK_FLL_CONFIG3 = reg | _VAL2FLD(SRSS_CLK_FLL_CONFIG3_BYPASS_SEL, (uint32_t)(config->outputMode));

        /* update CLK_FLL_CONFIG4 register with 1 parameter; preserve other bits */
        /* asserts just check for bitfield overflow */
        CY_ASSERT_L1(config->ccoRange <= (SRSS_CLK_FLL_CONFIG4_CCO_RANGE_Msk >> SRSS_CLK_FLL_CONFIG4_CCO_RANGE_Pos));
        CY_ASSERT_L1(config->cco_Freq <= (SRSS_CLK_FLL_CONFIG4_CCO_FREQ_Msk >> SRSS_CLK_FLL_CONFIG4_CCO_FREQ_Pos));
        CY_REG32_CLR_SET(SRSS_CLK_FLL_CONFIG4, SRSS_CLK_FLL_CONFIG4_CCO_RANGE, (uint32_t)(config->ccoRange));
        CY_REG32_CLR_SET(SRSS_CLK_FLL_CONFIG4, SRSS_CLK_FLL_CONFIG4_CCO_FREQ, (uint32_t)(config->cco_Freq));
    } /* if no error */

    return (returnStatus);
}

/*******************************************************************************
* Function Name: Cy_SysClk_FllGetConfiguration
****************************************************************************//**
*
* Reports the FLL configuration settings.
*
* \param config \ref cy_stc_fll_manual_config_t
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_FllGetConfiguration
*
*******************************************************************************/
void Cy_SysClk_FllGetConfiguration(cy_stc_fll_manual_config_t *config)
{
    CY_ASSERT_L1(config != NULL);
    /* read 2 parameters from CLK_FLL_CONFIG register */
    uint32_t tempReg = SRSS_CLK_FLL_CONFIG;
    config->fllMult         = _FLD2VAL(SRSS_CLK_FLL_CONFIG_FLL_MULT, tempReg);
    config->enableOutputDiv = (bool)_FLD2VAL(SRSS_CLK_FLL_CONFIG_FLL_OUTPUT_DIV, tempReg);
    /* read 2 parameters from CLK_FLL_CONFIG2 register */
    tempReg = SRSS_CLK_FLL_CONFIG2;
    config->refDiv          = _FLD2VAL(SRSS_CLK_FLL_CONFIG2_FLL_REF_DIV, tempReg);
    config->lockTolerance   = _FLD2VAL(SRSS_CLK_FLL_CONFIG2_LOCK_TOL, tempReg);
    /* read 4 parameters from CLK_FLL_CONFIG3 register */
    tempReg = SRSS_CLK_FLL_CONFIG3;
    config->igain           = _FLD2VAL(SRSS_CLK_FLL_CONFIG3_FLL_LF_IGAIN, tempReg);
    config->pgain           = _FLD2VAL(SRSS_CLK_FLL_CONFIG3_FLL_LF_PGAIN, tempReg);
    config->settlingCount   = _FLD2VAL(SRSS_CLK_FLL_CONFIG3_SETTLING_COUNT, tempReg);
    config->outputMode      = (cy_en_fll_pll_output_mode_t)_FLD2VAL(SRSS_CLK_FLL_CONFIG3_BYPASS_SEL, tempReg);
    /* read 1 parameter from CLK_FLL_CONFIG4 register */
    config->ccoRange        = (cy_en_fll_cco_ranges_t)_FLD2VAL(SRSS_CLK_FLL_CONFIG4_CCO_RANGE, SRSS_CLK_FLL_CONFIG4);
}

/*******************************************************************************
* Function Name: Cy_SysClk_FllEnable
****************************************************************************//**
*
* Enables the FLL. The FLL should be configured before calling this function.
*
* \param timeoutus amount of time in micro seconds to wait for FLL to lock.
* If lock doesn't occur, FLL is stopped. To avoid waiting for lock set this to 0,
* and manually check for lock using \ref Cy_SysClk_FllLocked.
*
* \return Error / status code:<br>
* CY_SYSCLK_SUCCESS - FLL successfully enabled<br>
* CY_SYSCLK_TIMEOUT - Timeout waiting for FLL lock
*
* \note
* While waiting for the FLL to lock, the FLL bypass mode is set to \ref CY_SYSCLK_FLLPLL_OUTPUT_INPUT.
* After the FLL is locked, the FLL bypass mdoe is then set to \ref CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_FllEnable
*
*******************************************************************************/
cy_en_sysclk_status_t Cy_SysClk_FllEnable(uint32_t timeoutus)
{
    cy_en_sysclk_status_t rtnval;
    bool nonZeroTimeout = (timeoutus != 0UL);

    /* first set the CCO enable bit */
    SRSS_CLK_FLL_CONFIG4 |= _VAL2FLD(SRSS_CLK_FLL_CONFIG4_CCO_ENABLE, 1UL); /* 1 = enable */

    /* Wait until CCO is ready */
    for (; (_FLD2VAL(SRSS_CLK_FLL_STATUS_CCO_READY, SRSS_CLK_FLL_STATUS) == 0UL) &&
           (timeoutus != 0UL);
         timeoutus--)
    {
        Cy_SysLib_DelayUs(1U);
    }

    /* Set the FLL bypass mode to 2 */
    CY_REG32_CLR_SET(SRSS_CLK_FLL_CONFIG3, SRSS_CLK_FLL_CONFIG3_BYPASS_SEL, (uint32_t)CY_SYSCLK_FLLPLL_OUTPUT_INPUT);

    /* Set the FLL enable bit, if CCO is ready */
    if ((!nonZeroTimeout) || (nonZeroTimeout && (timeoutus != 0UL)))
    {
        SRSS_CLK_FLL_CONFIG |= _VAL2FLD(SRSS_CLK_FLL_CONFIG_FLL_ENABLE, 1UL); /* 1 = enable */
    }

    /* now do the timeout wait for FLL_STATUS, bit LOCKED */
    for (; (_FLD2VAL(SRSS_CLK_FLL_STATUS_LOCKED, SRSS_CLK_FLL_STATUS) == 0UL) &&
           (timeoutus != 0UL);
         timeoutus--)
    {
        Cy_SysLib_DelayUs(1U);
    }

    /* If lock doesn't occur, FLL is stopped. */
    if (nonZeroTimeout && (timeoutus == 0UL))
    {
        (void)Cy_SysClk_FllDisable();
    }
    else
    { /* Lock occurred; we need to clear the unlock occurred bit.
         Do so by writing a 1 to it. */
        SRSS_CLK_FLL_STATUS = _VAL2FLD(SRSS_CLK_FLL_STATUS_UNLOCK_OCCURRED, 1UL);
        /* Set the FLL bypass mode to 3 */
        CY_REG32_CLR_SET(SRSS_CLK_FLL_CONFIG3, SRSS_CLK_FLL_CONFIG3_BYPASS_SEL,
                          (uint32_t)CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT);
    }

    rtnval = ((timeoutus == 0UL) ? CY_SYSCLK_TIMEOUT : CY_SYSCLK_SUCCESS);
    return rtnval;
}
/** \} group_sysclk_fll_funcs */


/* ========================================================================== */
/* ===========================    PLL SECTION    ============================ */
/* ========================================================================== */
/** \cond INTERNAL */
/* PLL OUTPUT_DIV bitfield allowable range */
#define MIN_OUTPUT_DIV     2UL
#define MAX_OUTPUT_DIV    16UL

/* PLL REFERENCE_DIV bitfield allowable range */
#define MIN_REF_DIV        1UL
#define MAX_REF_DIV       18UL

/* PLL FEEDBACK_DIV bitfield allowable ranges, LF and normal modes */
#define MIN_FB_DIV_LF     19UL
#define MAX_FB_DIV_LF     56UL
#define MIN_FB_DIV_NORM   22UL
#define MAX_FB_DIV_NORM  112UL
/* PLL FEEDBACK_DIV bitfield allowable range selection */
#define MIN_FB_DIV ((config->lfMode) ? MIN_FB_DIV_LF : MIN_FB_DIV_NORM)
#define MAX_FB_DIV ((config->lfMode) ? MAX_FB_DIV_LF : MAX_FB_DIV_NORM)

/* PLL Fvco range allowable ranges, LF and normal modes */
#define MIN_FVCO_LF   170000000UL
#define MAX_FVCO_LF   200000000UL
#define MIN_FVCO_NORM 200000000UL
#define MAX_FVCO_NORM 400000000UL
/* PLL Fvco range selection */
#define MIN_FVCO ((config->lfMode) ? MIN_FVCO_LF : MIN_FVCO_NORM)
#define MAX_FVCO ((config->lfMode) ? MAX_FVCO_LF : MAX_FVCO_NORM)

/* PLL input and output frequency limits */
#define MIN_IN_FREQ    4000000UL
#define MAX_IN_FREQ   64000000UL
#define MIN_OUT_FREQ ((config->lfMode) ? (MIN_FVCO_LF / MAX_OUTPUT_DIV) : (MIN_FVCO_NORM / MAX_OUTPUT_DIV))
#define MAX_OUT_FREQ CY_HF_CLK_MAX_FREQ
/** \endcond */

/**
* \addtogroup group_sysclk_pll_funcs
* \{
*/
/*******************************************************************************
* Function Name: Cy_SysClk_PllConfigure
****************************************************************************//**
*
* Configures a given PLL.
* The configuration formula used is:
*   Fout = pll_clk * (P / Q / div_out), where:
*     Fout is the desired output frequency
*     pll_clk is the frequency of the input source
*     P is the feedback divider. Its value is in bitfield FEEDBACK_DIV.
*     Q is the reference divider. Its value is in bitfield REFERENCE_DIV.
*     div_out is the reference divider. Its value is in bitfield OUTPUT_DIV.
*
* \param clkPath Selects which PLL to configure. 1 is the first PLL; 0 is invalid.
*
* \param config \ref cy_stc_pll_config_t
*
* \return  Error / status code:<br>
* CY_SYSCLK_SUCCESS - PLL successfully configured<br>
* CY_SYSCLK_INVALID_STATE - PLL not configured because it is enabled<br>
* CY_SYSCLK_BAD_PARAM - invalid clock path number, or input or desired output frequency is out of valid range
*
* \note
* Call this function after changing the PLL input frequency, for example if
* \ref Cy_SysClk_ClkPathSetSource() is called.
* \note
* Do not call this function when the PLL is enabled. If it is, then this function
* returns immediately with an error return value and no register updates.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PllConfigure
*
*******************************************************************************/
cy_en_sysclk_status_t Cy_SysClk_PllConfigure(uint32_t clkPath, const cy_stc_pll_config_t *config)
{
    cy_en_sysclk_status_t returnStatus = CY_SYSCLK_SUCCESS;

    /* check for error */
    if ((clkPath == 0UL) || (clkPath > CY_SRSS_NUM_PLL)) /* invalid clock path number */
    {
        returnStatus = CY_SYSCLK_BAD_PARAM;
    }
    else if (_FLD2VAL(SRSS_CLK_PLL_CONFIG_ENABLE, SRSS_CLK_PLL_CONFIG[clkPath - 1UL]) != 0U) /* 1 = enabled */
    {
        returnStatus = CY_SYSCLK_INVALID_STATE;
    }
    /* invalid input frequency */
    else if (((config->inputFreq) < MIN_IN_FREQ) || (MAX_IN_FREQ < (config->inputFreq)))
    {
        returnStatus = CY_SYSCLK_BAD_PARAM;
    }
    /* invalid output frequency */
    else if (((config->outputFreq) < MIN_OUT_FREQ) || (MAX_OUT_FREQ < (config->outputFreq)))
    {
        returnStatus = CY_SYSCLK_BAD_PARAM;
    }
    else
    { /* returnStatus is OK */
    }

    /* no errors */
    if (returnStatus == CY_SYSCLK_SUCCESS)
    {
        cy_stc_pll_manual_config_t manualConfig;
        manualConfig.feedbackDiv = 0UL;
        manualConfig.referenceDiv = 0UL;
        manualConfig.outputDiv = 0UL;

        /* If output mode is bypass (input routed directly to output), then done.
           The output frequency equals the input frequency regardless of the
           frequency parameters. */
        if (config->outputMode != CY_SYSCLK_FLLPLL_OUTPUT_INPUT)
        {
            /* for each possible value of OUTPUT_DIV and REFERENCE_DIV (Q), try
               to find a value for FEEDBACK_DIV (P) that gives an output frequency
               as close as possible to the desired output frequency. */
            uint32_t p, q, out;
            uint32_t foutBest = 0UL; /* to ensure at least one pass through the for loops below */

            /* REFERENCE_DIV (Q) selection */
            for (q = MIN_REF_DIV; (q <= MAX_REF_DIV) && (foutBest != (config->outputFreq)); q++)
            {
                /* FEEDBACK_DIV (P) selection */
                for (p = MIN_FB_DIV; (p <= MAX_FB_DIV) && (foutBest != (config->outputFreq)); p++)
                {
                    /* Calculate the intermediate Fvco, and make sure that it's in range. */
                    uint32_t fvco = (uint32_t)(((uint64_t)(config->inputFreq) * (uint64_t)p) / (uint64_t)q);
                    if ((MIN_FVCO <= fvco) && (fvco <= MAX_FVCO))
                    {
                        /* OUTPUT_DIV selection */
                        for (out = MIN_OUTPUT_DIV; (out <= MAX_OUTPUT_DIV) && (foutBest != (config->outputFreq)); out++)
                        {
                            /* Calculate what output frequency will actually be produced. 
                               If it's closer to the target than what we have so far, then save it. */
                            uint32_t fout = ((p * config->inputFreq) / q) / out;
                            if ((uint32_t)abs((int32_t)fout - (int32_t)(config->outputFreq)) <
                                (uint32_t)abs((int32_t)foutBest - (int32_t)(config->outputFreq)))
                            {
                                foutBest = fout;
                                manualConfig.feedbackDiv  = p;
                                manualConfig.referenceDiv = q;
                                manualConfig.outputDiv    = out;
                            }
                        }
                    }
                }
            }
            /* exit loops if foutBest equals outputFreq */
        } /* if not bypass output mode */

        /* configure PLL based on calculated values */
        manualConfig.lfMode     = config->lfMode;
        manualConfig.outputMode = config->outputMode;
        returnStatus = Cy_SysClk_PllManualConfigure(clkPath, &manualConfig);

    } /* if no error */

    return (returnStatus);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PllManualConfigure
****************************************************************************//**
*
* Manually configures a PLL based on user inputs.
*
* \param clkPath Selects which PLL to configure. 1 is the first PLL; 0 is invalid.
*
* \param config \ref cy_stc_pll_manual_config_t
*
* \return  Error / status code:<br>
* CY_SYSCLK_SUCCESS - PLL successfully configured<br>
* CY_SYSCLK_INVALID_STATE - PLL not configured because it is enabled<br>
* CY_SYSCLK_BAD_PARAM - invalid clock path number
*
* \note
* Call this function after changing the PLL input frequency, for example if
* \ref Cy_SysClk_ClkPathSetSource() is called.
* \note
* Do not call this function when the PLL is enabled. If it is, then this function
* returns immediately with an error return value and no register updates.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PllManualConfigure
*
*******************************************************************************/
cy_en_sysclk_status_t Cy_SysClk_PllManualConfigure(uint32_t clkPath, const cy_stc_pll_manual_config_t *config)
{
    cy_en_sysclk_status_t returnStatus = CY_SYSCLK_SUCCESS;

    /* check for errors */
    if ((clkPath == 0UL) || (clkPath > CY_SRSS_NUM_PLL)) /* invalid clock path number */
    {
        returnStatus = CY_SYSCLK_BAD_PARAM;
    }
    else if (_FLD2VAL(SRSS_CLK_PLL_CONFIG_ENABLE, SRSS_CLK_PLL_CONFIG[clkPath - 1UL]) != 0U) /* 1 = enabled */
    {
        returnStatus = CY_SYSCLK_INVALID_STATE;
    }
    /* valid divider bitfield values */
    else if ((config->outputDiv    < MIN_OUTPUT_DIV) || (MAX_OUTPUT_DIV < config->outputDiv)    ||
             (config->referenceDiv <    MIN_REF_DIV) || (MAX_REF_DIV    < config->referenceDiv) ||
             (config->feedbackDiv  < (config->lfMode ? MIN_FB_DIV_LF : MIN_FB_DIV))             ||
             ((config->lfMode ? MAX_FB_DIV_LF : MAX_FB_DIV) < config->feedbackDiv))
    {
         returnStatus = CY_SYSCLK_BAD_PARAM;
    }
    else
    { /* returnStatus is OK */
    }

    /* no errors */
    if (returnStatus == CY_SYSCLK_SUCCESS)
    {
        clkPath--; /* to correctly access PLL config registers structure */
        /* If output mode is bypass (input routed directly to output), then done.
           The output frequency equals the input frequency regardless of the frequency parameters. */
        if (config->outputMode != CY_SYSCLK_FLLPLL_OUTPUT_INPUT)
        {
            SRSS_CLK_PLL_CONFIG[clkPath] =
                _VAL2FLD(SRSS_CLK_PLL_CONFIG_FEEDBACK_DIV,  (uint32_t)(config->feedbackDiv))  |
                _VAL2FLD(SRSS_CLK_PLL_CONFIG_REFERENCE_DIV, (uint32_t)(config->referenceDiv)) |
                _VAL2FLD(SRSS_CLK_PLL_CONFIG_OUTPUT_DIV,    (uint32_t)(config->outputDiv))    |
                _VAL2FLD(SRSS_CLK_PLL_CONFIG_PLL_LF_MODE,   (uint32_t)(config->lfMode));
        }

        CY_REG32_CLR_SET(SRSS_CLK_PLL_CONFIG[clkPath], SRSS_CLK_PLL_CONFIG_BYPASS_SEL, (uint32_t)config->outputMode);
    } /* if no error */

    return (returnStatus);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PllGetConfiguration
****************************************************************************//**
*
* Reports configuration settings for a PLL.
*
* \param clkPath Selects which PLL to report. 1 is the first PLL; 0 is invalid.
*
* \param config \ref cy_stc_pll_manual_config_t
*
* \return  Error / status code:<br>
* CY_SYSCLK_SUCCESS - PLL data successfully reported<br>
* CY_SYSCLK_BAD_PARAM - invalid clock path number
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PllGetConfiguration
*
*******************************************************************************/
cy_en_sysclk_status_t Cy_SysClk_PllGetConfiguration(uint32_t clkPath, cy_stc_pll_manual_config_t *config)
{
    cy_en_sysclk_status_t rtnval = CY_SYSCLK_BAD_PARAM;
    if ((clkPath != 0UL) && (clkPath <= CY_SRSS_NUM_PLL))
    {
        uint32_t tempReg = SRSS_CLK_PLL_CONFIG[clkPath - 1UL];
        config->feedbackDiv  = (uint8_t)_FLD2VAL(SRSS_CLK_PLL_CONFIG_FEEDBACK_DIV, tempReg);
        config->referenceDiv = (uint8_t)_FLD2VAL(SRSS_CLK_PLL_CONFIG_REFERENCE_DIV, tempReg);
        config->outputDiv    = (uint8_t)_FLD2VAL(SRSS_CLK_PLL_CONFIG_OUTPUT_DIV, tempReg);
        config->lfMode       = (bool)_FLD2VAL(SRSS_CLK_PLL_CONFIG_OUTPUT_DIV, tempReg);
        config->outputMode   = (cy_en_fll_pll_output_mode_t)_FLD2VAL(SRSS_CLK_PLL_CONFIG_BYPASS_SEL, tempReg);
        rtnval = CY_SYSCLK_SUCCESS;
    }
    return (rtnval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PllEnable
****************************************************************************//**
*
* Enables the PLL. The PLL should be configured before calling this function.
*
* \param clkPath Selects which PLL to enable. 1 is the first PLL; 0 is invalid.
*
* \param timeoutus amount of time in microseconds to wait for the PLL to lock.
* If lock doesn't occur, PLL is stopped. To avoid waiting for lock set this to 0,
* and manually check for lock using \ref Cy_SysClk_PllLocked.
*
* \return Error / status code:<br>
* CY_SYSCLK_SUCCESS - PLL successfully enabled<br>
* CY_SYSCLK_TIMEOUT - Timeout waiting for PLL lock<br>
* CY_SYSCLK_BAD_PARAM - invalid clock path number
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PllEnable
*
*******************************************************************************/
cy_en_sysclk_status_t Cy_SysClk_PllEnable(uint32_t clkPath, uint32_t timeoutus)
{
    cy_en_sysclk_status_t rtnval = CY_SYSCLK_BAD_PARAM;
    if ((clkPath != 0UL) && (clkPath <= CY_SRSS_NUM_PLL))
    {
        clkPath--; /* to correctly access PLL config and status registers structures */
        /* first set the PLL enable bit */
        SRSS_CLK_PLL_CONFIG[clkPath] |= _VAL2FLD(SRSS_CLK_PLL_CONFIG_ENABLE, 1UL); /* 1 = enable */

        /* now do the timeout wait for PLL_STATUS, bit LOCKED */
        for (; (_FLD2VAL(SRSS_CLK_PLL_STATUS_LOCKED, SRSS_CLK_PLL_STATUS[clkPath]) == 0UL) &&
               (timeoutus != 0UL);
             timeoutus--)
        {
            Cy_SysLib_DelayUs(1U);
        }
        rtnval = ((timeoutus == 0UL) ? CY_SYSCLK_TIMEOUT : CY_SYSCLK_SUCCESS);
    }
    return (rtnval);
}
/** \} group_sysclk_pll_funcs */


/* ========================================================================== */
/* ====================    Clock Measurement section    ===================== */
/* ========================================================================== */

/* Cy_SysClk_StartClkMeasurementCounters() input parameter saved for use later in other functions. */
static uint32_t clk1Count1;

/* These variables act as locks to prevent collisions between clock measurement and entry into
   DeepSleep mode. See Cy_SysClk_DeepSleep(). */
static bool clkCounting = false;
static bool preventCounting = false;

/**
* \addtogroup group_sysclk_calclk_funcs
* \{
*/
/*******************************************************************************
* Function Name: Cy_SysClk_StartClkMeasurementCounters
****************************************************************************//**
*
* Assigns clocks to the clock measurement counters, and starts counting. The counters
* let you measure a clock frequency using another clock as a reference. There are two
* counters.
*
* - One counter (counter1), which is clocked by clock1, is loaded with an initial
*   value and counts down to zero.
* - The second counter (counter2), which is clocked by clock2, counts up until 
*   the first counter reaches zero.
*
* Either clock1 or clock2 can be a reference clock; the other clock becomes the
* measured clock. The reference clock frequency is always known.<br>
* After calling this function, call \ref Cy_SysClk_ClkMeasurementCountersDone()
* to determine when counting is done, that is, counter1 has counted down to zero.
* Then call \ref Cy_SysClk_ClkMeasurementCountersGetFreq() to calculate the frequency
* of the measured clock.
*
* \param clock1 The clock for counter1
*
* \param count1 The initial value for counter1, from which counter1 counts down to zero.
*
* \param clock2 The clock for counter2
*
* \return Error / status code:<br>
* CY_SYSCLK_INVALID_STATE if already doing a measurement<br>
* CY_SYSCLK_BAD_PARAM if invalid clock input parameter<br>
* else CY_SYSCLK_SUCCESS
*
* \note The counters are both 24-bit, so the maximum value of count1 is 0xFFFFFF.
* If clock2 frequency is greater than clock1, make sure that count1 is low enough
* that counter2 does not overflow before counter1 reaches zero.
* \note The time to complete a measurement is count1 / clock1 frequency.
* \note The clocks for both counters must have a nonzero frequency, or
* \ref Cy_SysClk_ClkMeasurementCountersGetFreq() incorrectly reports the result of the
* previous  measurement.
* \note Do not enter a device low power mode (Sleep, Deep Sleep) while doing a measurement;
* the measured clock frequency may not be accurate.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_StartClkMeasurementCounters
*
*******************************************************************************/
cy_en_sysclk_status_t Cy_SysClk_StartClkMeasurementCounters(cy_en_meas_clks_t clock1, uint32_t count1, cy_en_meas_clks_t clock2)
{
    cy_en_sysclk_status_t rtnval = CY_SYSCLK_INVALID_STATE;

    if ((!preventCounting) /* don't start a measurement if about to enter DeepSleep mode */  ||
        (_FLD2VAL(SRSS_CLK_CAL_CNT1_CAL_COUNTER_DONE, SRSS_CLK_CAL_CNT1) != 0UL/*1 = done*/))
    {
        /* Connect the indicated clocks to the respective counters.

           if clock1 is a slow clock,
             select it in SRSS_CLK_OUTPUT_SLOW.SLOW_SEL0, and SRSS_CLK_OUTPUT_FAST.FAST_SEL0 = SLOW_SEL0
           else if clock1 is a fast clock,
             select it in SRSS_CLK_OUTPUT_FAST.FAST_SEL0,
           else error, do nothing and return.

           if clock2 is a slow clock,
             select it in SRSS_CLK_OUTPUT_SLOW.SLOW_SEL1, and SRSS_CLK_OUTPUT_FAST.FAST_SEL1 = SLOW_SEL1
           else if clock2 is a fast clock,
             select it in SRSS_CLK_OUTPUT_FAST.FAST_SEL1,
           else error, do nothing and return.
        */
        rtnval = CY_SYSCLK_BAD_PARAM;
        if ((clock1 < CY_SYSCLK_MEAS_CLK_LAST_CLK) && (clock2 < CY_SYSCLK_MEAS_CLK_LAST_CLK) &&
            (count1 <= (SRSS_CLK_CAL_CNT1_CAL_COUNTER1_Msk >> SRSS_CLK_CAL_CNT1_CAL_COUNTER1_Pos)))
        {
            /* Disallow entry into DeepSleep mode while counting. */
            clkCounting = true;

            if (clock1 < CY_SYSCLK_MEAS_CLK_FAST_CLKS)
            { /* slow clock */
                CY_REG32_CLR_SET(SRSS_CLK_OUTPUT_SLOW, SRSS_CLK_OUTPUT_SLOW_SLOW_SEL0, (uint32_t)clock1);
                CY_REG32_CLR_SET(SRSS_CLK_OUTPUT_FAST, SRSS_CLK_OUTPUT_FAST_FAST_SEL0, 7UL/*slow_sel0 output*/);
            }
            else
            { /* fast clock */
                if (clock1 < CY_SYSCLK_MEAS_CLK_PATH_CLKS)
                { /* ECO, EXT, ALTHF */
                    CY_REG32_CLR_SET(SRSS_CLK_OUTPUT_FAST, SRSS_CLK_OUTPUT_FAST_FAST_SEL0, (uint32_t)clock1);
                }
                else
                { /* PATH or CLKHF */
                    CY_REG32_CLR_SET(SRSS_CLK_OUTPUT_FAST, SRSS_CLK_OUTPUT_FAST_FAST_SEL0,
                                                            (((uint32_t)clock1 >> 8) & 0x0FUL) /*use enum bits [11:8]*/);
                    if (clock1 < CY_SYSCLK_MEAS_CLK_CLKHFS)
                    { /* PATH select */
                        CY_REG32_CLR_SET(SRSS_CLK_OUTPUT_FAST, SRSS_CLK_OUTPUT_FAST_PATH_SEL0,
                                                                ((uint32_t)clock1 & 0x0FUL) /*use enum bits [3:0]*/);
                    }
                    else
                    { /* CLKHF select */
                        CY_REG32_CLR_SET(SRSS_CLK_OUTPUT_FAST, SRSS_CLK_OUTPUT_FAST_HFCLK_SEL0,
                                                                ((uint32_t)clock1 & 0x0FUL) /*use enum bits [3:0]*/);
                    }
                }
            } /* clock1 fast clock */

            if (clock2 < CY_SYSCLK_MEAS_CLK_FAST_CLKS)
            { /* slow clock */
                CY_REG32_CLR_SET(SRSS_CLK_OUTPUT_SLOW, SRSS_CLK_OUTPUT_SLOW_SLOW_SEL1, (uint32_t)clock2);
                CY_REG32_CLR_SET(SRSS_CLK_OUTPUT_FAST, SRSS_CLK_OUTPUT_FAST_FAST_SEL1, 7UL/*slow_sel1 output*/);
            }
            else
            { /* fast clock */
                if (clock2 < CY_SYSCLK_MEAS_CLK_PATH_CLKS)
                { /* ECO, EXT, ALTHF */
                    CY_REG32_CLR_SET(SRSS_CLK_OUTPUT_FAST, SRSS_CLK_OUTPUT_FAST_FAST_SEL1, (uint32_t)clock2);
                }
                else
                { /* PATH or CLKHF */
                    CY_REG32_CLR_SET(SRSS_CLK_OUTPUT_FAST, SRSS_CLK_OUTPUT_FAST_FAST_SEL1,
                                                            (((uint32_t)clock2 >> 8) & 0x0FUL) /*use enum bits [11:8]*/);
                    if (clock2 < CY_SYSCLK_MEAS_CLK_CLKHFS)
                    { /* PATH select */
                        CY_REG32_CLR_SET(SRSS_CLK_OUTPUT_FAST, SRSS_CLK_OUTPUT_FAST_PATH_SEL1,
                                                                ((uint32_t)clock2 & 0x0FUL) /*use enum bits [3:0]*/);
                    }
                    else
                    { /* CLKHF select */
                        CY_REG32_CLR_SET(SRSS_CLK_OUTPUT_FAST, SRSS_CLK_OUTPUT_FAST_HFCLK_SEL1,
                                                                ((uint32_t)clock2 & 0x0FUL) /*use enum bits [3:0]*/);
                    }
                }
            } /* clock2 fast clock */

            rtnval = CY_SYSCLK_SUCCESS;

            /* Save this input parameter for use later, in other functions.
               No error checking is done on this parameter.*/
            clk1Count1 = count1;

            /* Counting starts when counter1 is written with a nonzero value. */
            SRSS_CLK_CAL_CNT1 = clk1Count1;
        } /* if (clock1 < CY_SYSCLK_MEAS_CLK_LAST_CLK && clock2 < CY_SYSCLK_MEAS_CLK_LAST_CLK) */
    } /* if (not done) */
    return (rtnval);
}

/*******************************************************************************
* Function Name: Cy_SysClk_ClkMeasurementCountersGetFreq
****************************************************************************//**
*
* Calculates the frequency of the indicated measured clock (clock1 or clock2).
* 
* - If clock1 is the measured clock, its frequency is:<br>
*   clock1 frequency = (count1 / count2) * clock2 frequency
* - If clock2 is the measured clock, its frequency is:<br>
*   clock2 frequency = (count2 / count1) * clock1 frequency
*
* Call this function only after counting is done; see \ref Cy_SysClk_ClkMeasurementCountersDone().
*
* \param measuredClock False (0) if the measured clock is clock1, true (1)
* if the measured clock is clock2.
*
* \param refClkFreq The reference clock frequency (clock1 or clock2).
*
* \return The frequency of the measured clock, in Hz.
*
* \funcusage
* Refer to the Cy_SysClk_StartClkMeasurementCounters() function usage.
*
*******************************************************************************/
uint32_t Cy_SysClk_ClkMeasurementCountersGetFreq(bool measuredClock, uint32_t refClkFreq)
{
    volatile uint64_t rtnval = (uint64_t)_FLD2VAL(SRSS_CLK_CAL_CNT2_CAL_COUNTER2, SRSS_CLK_CAL_CNT2);

    /* Done counting; allow entry into DeepSleep mode. */
    clkCounting = false;

    if (!measuredClock)
    { /* clock1 is the measured clock */
        if (rtnval != 0U) /* avoid divide by zero */
        {
            rtnval = CY_SYSCLK_DIV_ROUND((uint64_t)clk1Count1 * (uint64_t)refClkFreq, rtnval);
        }
    }
    else
    { /* clock2 is the measured clock */
        rtnval = CY_SYSCLK_DIV_ROUND(rtnval * (uint64_t)refClkFreq, (uint64_t)clk1Count1 );
    }
    return ((uint32_t)rtnval);
}
/** \} group_sysclk_calclk_funcs */


/* ========================================================================== */
/* ==========================    TRIM SECTION    ============================ */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_trim_funcs
* \{
*/

/*******************************************************************************
* Function Name: Cy_SysClk_IloTrim
****************************************************************************//**
*
* Trims the ILO to be as close to 32,768 Hz as possible.
*
* \param iloFreq current ILO frequency. Call \ref Cy_SysClk_StartClkMeasurementCounters
* and other measurement functions to obtain the current frequency of the ILO.
*
* \return Change in trim value; 0 if done, that is, no change in trim value.
*
* \note The watchdog timer (WDT) must be unlocked before calling this function.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_IloTrim
*
*******************************************************************************/
/** \cond INTERNAL */
/* target frequency */
#define CY_SYSCLK_ILO_TARGET_FREQ  32768u
/** \endcond */

int32_t Cy_SysClk_IloTrim(uint32_t iloFreq)
{
    /* Nominal trim step size is 1.5% of "the frequency". Using the target frequency. */
    const uint32_t trimStep = CY_SYSCLK_DIV_ROUND((uint32_t)CY_SYSCLK_ILO_TARGET_FREQ * 15UL, 1000UL);

    uint32_t newTrim = 0UL;
    uint32_t curTrim = 0UL;

    /* Do nothing if iloFreq is already within one trim step from the target */
    uint32_t diff = (uint32_t)abs((int32_t)iloFreq - (int32_t)CY_SYSCLK_ILO_TARGET_FREQ);
    if (diff >= trimStep)
    {
        curTrim = _FLD2VAL(SRSS_CLK_TRIM_ILO_CTL_ILO_FTRIM, SRSS_CLK_TRIM_ILO_CTL);
        if (iloFreq > CY_SYSCLK_ILO_TARGET_FREQ)
        { /* iloFreq is too high. Reduce the trim value */
            newTrim = curTrim - CY_SYSCLK_DIV_ROUND(iloFreq - CY_SYSCLK_ILO_TARGET_FREQ, trimStep);
        }
        else
        { /* iloFreq too low. Increase the trim value. */
            newTrim = curTrim + CY_SYSCLK_DIV_ROUND(CY_SYSCLK_ILO_TARGET_FREQ - iloFreq, trimStep);
        }

        /* Update the trim value */
        CY_REG32_CLR_SET(SRSS_CLK_TRIM_ILO_CTL, SRSS_CLK_TRIM_ILO_CTL_ILO_FTRIM, newTrim);
    }
    return (int32_t)(curTrim - newTrim);
}

/*******************************************************************************
* Function Name: Cy_SysClk_PiloTrim
****************************************************************************//**
*
* Trims the PILO to be as close to 32,768 Hz as possible.
*
* \param piloFreq current PILO frequency. Call \ref Cy_SysClk_StartClkMeasurementCounters
* and other measurement functions to obtain the current frequency of the PILO.
*
* \return Change in trim value; 0 if done, that is, no change in trim value.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PiloTrim
*
*******************************************************************************/
/** \cond INTERNAL */
/* target frequency */
#define CY_SYSCLK_PILO_TARGET_FREQ  32768UL
/* nominal trim step size */
#define CY_SYSCLK_PILO_TRIM_STEP        5UL
/** \endcond */

int32_t Cy_SysClk_PiloTrim(uint32_t piloFreq)
{
    uint32_t newTrim = 0UL;
    uint32_t curTrim = 0UL;

    /* Do nothing if piloFreq is already within one trim step from the target */
    uint32_t diff = (uint32_t)abs((int32_t)piloFreq - (int32_t)CY_SYSCLK_PILO_TARGET_FREQ);
    if (diff >= CY_SYSCLK_PILO_TRIM_STEP)
    {
        curTrim = Cy_SysClk_PiloGetTrim();
        if (piloFreq > CY_SYSCLK_PILO_TARGET_FREQ)
        { /* piloFreq too high. Decrease the trim value. */
            newTrim = curTrim - CY_SYSCLK_DIV_ROUND(piloFreq - CY_SYSCLK_PILO_TARGET_FREQ, CY_SYSCLK_PILO_TRIM_STEP);
            if ((int32_t)newTrim < 0) /* limit underflow */
            {
                newTrim = 0;
            }
        }
        else
        { /* piloFreq too low. Increase the trim value. */
            newTrim = curTrim + CY_SYSCLK_DIV_ROUND(CY_SYSCLK_PILO_TARGET_FREQ - piloFreq, CY_SYSCLK_PILO_TRIM_STEP);
            if (newTrim >= SRSS_CLK_PILO_CONFIG_PILO_FFREQ_Msk) /* limit overflow */
            {
                newTrim = SRSS_CLK_PILO_CONFIG_PILO_FFREQ_Msk;
            }
        }
        Cy_SysClk_PiloSetTrim(newTrim);
    }

    return (int32_t)(curTrim - newTrim);
}
/** \} group_sysclk_trim_funcs */


/* ========================================================================== */
/* ======================    POWER MANAGEMENT SECTION    ==================== */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_pm_funcs
* \{
*/
/** \cond INTERNAL */
/* timeout count for use in function Cy_SysClk_DeepSleepCallback() is sufficiently large for ~1 second at 100 MHz */
#define TIMEOUTK 5000000UL
/** \endcond */

/*******************************************************************************
* Function Name: Cy_SysClk_DeepSleepCallback
****************************************************************************//**
*
* Callback function to be used when entering chip deep-sleep mode. This function is
* applicable for when either the FLL or the PLL is enabled. It performs the following:
*
* 1. Before entering deep-sleep, the clock configuration is saved in SRAM. If the
*    FLL/PLL source is the ECO, then the source is updated to the IMO.
* 2. Upon wakeup from deep-sleep, the function restores the configuration and 
*    waits for the FLL/PLL to regain their frequency locks.
*
* The function prevents entry into DeepSleep mode if the measurement counters
* are currently counting; see \ref Cy_SysClk_StartClkMeasurementCounters.
*
* This function can be called during execution of \ref Cy_SysPm_DeepSleep.
* To do so, register this function as a callback before calling
* \ref Cy_SysPm_DeepSleep - specify \ref CY_SYSPM_DEEPSLEEP as the callback
* type and call \ref Cy_SysPm_RegisterCallback.
*
* \note This function must be the last callback function that is registered. 
* Doing so minimizes the time spent on low power mode entry and exit. In the case
* where the ECO sources the FLL/PLL, this also allows the ECO to stabilize before
* reconnecting it to the FLL or PLL. 
*
* \param callbackParams
* structure with the syspm callback parameters,
* see \ref cy_stc_syspm_callback_params_t.
*
* \return Error / status code; see \ref cy_en_syspm_status_t. Pass if not doing
* a clock measurement, otherwise Fail. Timeout if timeout waiting for FLL or a PLL
* to regain its frequency lock.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_DeepSleepCallback
*
*******************************************************************************/
cy_en_syspm_status_t Cy_SysClk_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams)
{
    /* bitmapped paths and roots that may be affected by FLL or PLL being sourced by ECO */
    static uint16_t changedSourcePaths;

    cy_en_syspm_status_t rtnval = CY_SYSPM_SUCCESS;

    CY_ASSERT_L1(callbackParams != NULL);

    /* Entry into DeepSleep mode tests */
    if (callbackParams->mode == CY_SYSPM_CHECK_READY)
    {
        /* Don't allow entry into DeepSleep mode if currently measuring a frequency. */
        if (clkCounting)
        {
            rtnval = CY_SYSPM_FAIL;
        }
        else 
        { /* Indicating that we can go into DeepSleep. Before doing so ... */
            uint32_t fllpll; /* 0 = FLL, all other values = a PLL */

            /* initialize record of changed paths */
            changedSourcePaths = 0U;

            /* for FLL and each PLL, */
            for (fllpll = 0UL; fllpll < (CY_SRSS_NUM_PLL + 1UL); fllpll++)
            {
                /* If FLL or PLL is enabled, */
                if (0UL != ((fllpll == 0UL) ? (_FLD2VAL(SRSS_CLK_FLL_CONFIG_FLL_ENABLE, SRSS_CLK_FLL_CONFIG)) : 
                                              (_FLD2VAL(SRSS_CLK_PLL_CONFIG_ENABLE, SRSS_CLK_PLL_CONFIG[fllpll - 1UL]))))
                {
                    /* and the FLL or PLL has ECO as a source, */
                    if (Cy_SysClk_ClkPathGetSource(fllpll) == CY_SYSCLK_CLKPATH_IN_ECO)
                    {
                        /* Change this FLL or PLL source to IMO */
                        (void)Cy_SysClk_ClkPathSetSource(fllpll, CY_SYSCLK_CLKPATH_IN_IMO);
                        /* keep a record that this FLL or PLL's source was changed from ECO */
                        changedSourcePaths |= (uint16_t)(1U << fllpll);
                    }
                    
                    /* Set the FLL/PLL bypass mode to 2 */
                    if(fllpll == 0UL)
                    {
                        CY_REG32_CLR_SET(SRSS_CLK_FLL_CONFIG3, SRSS_CLK_FLL_CONFIG3_BYPASS_SEL, (uint32_t)CY_SYSCLK_FLLPLL_OUTPUT_INPUT);
                    }
                    else
                    {
                        CY_REG32_CLR_SET(SRSS_CLK_PLL_CONFIG[fllpll - 1UL], SRSS_CLK_PLL_CONFIG_BYPASS_SEL, (uint32_t)CY_SYSCLK_FLLPLL_OUTPUT_INPUT);
                    }
                }
            }
            
            /* Prevent starting a new clock measurement until after we've come back from DeepSleep. */
            preventCounting = true;
        }
    }

    /* After return from DeepSleep, for each FLL and PLL, if needed, restore the source to ECO.
       And block until the FLL or PLL has regained its frequency locks. */
    else if (callbackParams->mode == CY_SYSPM_AFTER_TRANSITION)
    {
        /* if any FLL/PLL was sourced by the ECO, timeout wait for the ECO to become fully stabilized again. */
        if (changedSourcePaths != 0U)
        {
            uint32_t timeout;
            /* Cy_SysClk_EcoGetStatus()return value 2ul = fully stabilized */
            for (timeout = TIMEOUTK; (timeout != 0UL) && (Cy_SysClk_EcoGetStatus() != 2UL); timeout--){}
            if (timeout == 0UL)
            {
                rtnval = CY_SYSPM_TIMEOUT;
            }
        }
        
        if(rtnval == CY_SYSPM_SUCCESS)
        {
            /* for FLL and each PLL, */
            uint32_t fllpll; /* 0 = FLL, all other values = a PLL */
            for (fllpll = 0UL; fllpll < (CY_SRSS_NUM_PLL + 1UL); fllpll++)
            {
                /* If FLL or PLL is enabled, */
                if (0UL != ((fllpll == 0UL) ? (_FLD2VAL(SRSS_CLK_FLL_CONFIG_FLL_ENABLE, SRSS_CLK_FLL_CONFIG)) : 
                                              (_FLD2VAL(SRSS_CLK_PLL_CONFIG_ENABLE, SRSS_CLK_PLL_CONFIG[fllpll - 1UL]))))
                {
                    /* check the record that this FLL or PLL's source was changed from ECO */
                    if ((changedSourcePaths & (uint16_t)(1U << fllpll)) != 0U)
                    {
                        /* Change this FLL or PLL source back to ECO */
                        (void)Cy_SysClk_ClkPathSetSource(fllpll, CY_SYSCLK_CLKPATH_IN_ECO);
                    }
                    
                    /* Timeout wait for FLL or PLL to regain lock. */
                    uint32_t timout;
                    for (timout = TIMEOUTK; timout != 0UL; timout--)
                    {
                        if (true == ((fllpll == 0UL) ? Cy_SysClk_FllLocked() : Cy_SysClk_PllLocked(fllpll)))
                        {
                            break;
                        }
                    }
                    if (timout == 0UL)
                    {
                        rtnval = CY_SYSPM_TIMEOUT;
                    }
                    else
                    {
                        /* Set the FLL/PLL bypass mode to 3 */
                        if(fllpll == 0UL)
                        {
                            CY_REG32_CLR_SET(SRSS_CLK_FLL_CONFIG3, SRSS_CLK_FLL_CONFIG3_BYPASS_SEL, (uint32_t)CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT);
                        }
                        else
                        {
                            CY_REG32_CLR_SET(SRSS_CLK_PLL_CONFIG[fllpll - 1UL], SRSS_CLK_PLL_CONFIG_BYPASS_SEL, (uint32_t)CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT);
                        }
                    }
                }
            }
        }
        
        /* Allow clock measurement. */
        preventCounting = false;
    }

    /* No other modes need be checked. */
    else
    {
    }

    return rtnval;
}
/** \} group_sysclk_pm_funcs */


/* ========================================================================== */
/* ===========================    WCO SECTION    ============================ */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_wco_funcs
* \{
*/
#if (SRSS_WCOCSV_PRESENT != 0) || defined(CY_DOXYGEN)
/*******************************************************************************
* Function Name: Cy_SysClk_WcoConfigureCsv
****************************************************************************//**
*
* Configure the WCO clock supervisor (CSV).
*
* \param config \ref cy_stc_wco_csv_config_t
*
* \note
* If loss detection is enabled, writes to other register bits are ignored.
* Therefore loss detection is disabled before writing the config structure
* members to the CTL register. Note that one of the config structure members is
* an enable bit.
*******************************************************************************/
void Cy_SysClk_WcoConfigureCsv(const cy_stc_wco_csv_config_t *config)
{
    CY_ASSERT_L1(config != NULL);
    CY_ASSERT_L3(config->supervisorClock <= CY_SYSCLK_WCO_CSV_SUPERVISOR_PILO);
    CY_ASSERT_L3(config->lossWindow      <= CY_SYSCLK_CSV_LOSS_512_CYCLES);
    CY_ASSERT_L3(config->lossAction      <= CY_SYSCLK_CSV_ERROR_FAULT_RESET);

    /* First clear all bits, including the enable bit; disable loss detection. */
    SRSS_CLK_CSV_WCO_CTL = 0UL;
    /* Then write the structure elements (which include an enable bit) to the register. */
    SRSS_CLK_CSV_WCO_CTL = _VAL2FLD(SRSS_CLK_CSV_WCO_CTL_CSV_MUX, (uint32_t)config->supervisorClock)      |
                           _VAL2FLD(SRSS_CLK_CSV_WCO_CTL_CSV_LOSS_WINDOW, (uint32_t)(config->lossWindow)) |
                           _VAL2FLD(SRSS_CLK_CSV_WCO_CTL_CSV_LOSS_ACTION, (uint32_t)(config->lossAction)) |
                           _VAL2FLD(SRSS_CLK_CSV_WCO_CTL_CSV_LOSS_EN, config->enableLossDetection);
}
#endif /* (SRSS_WCOCSV_PRESENT != 0) || defined(CY_DOXYGEN) */
/** \} group_sysclk_wco_funcs */


/* ========================================================================== */
/* =========================    clkHf[n] SECTION    ========================= */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_clk_hf_funcs
* \{
*/
#if (SRSS_MASK_HFCSV != 0)  || defined(CY_DOXYGEN)
/*******************************************************************************
* Function Name: Cy_SysClk_ClkHfConfigureCsv
****************************************************************************//**
*
* Configures the clkHf clock supervisor (CSV).
*
* \param clkHf selects which clkHf CSV to configure.
*
* \param config \ref cy_stc_clkhf_csv_config_t
*
* \return Error / status code: CY_SYSCLK_INVALID_STATE if clkHf CSV is not present
* in the device, else CY_SYSCLK_SUCCESS
*
* \note
* If loss detection is enabled, writes to other register bits are ignored.
* Therefore loss detection is disabled before writing the config structure
* members to the CTL register. Note that one of the config structure members is
* an enable bit.
*******************************************************************************/
cy_en_sysclk_status_t Cy_SysClk_ClkHfConfigureCsv(uint32_t clkHf, const cy_stc_clkhf_csv_config_t *config)
{
    CY_ASSERT_L1(clkHf < CY_SRSS_NUM_HFROOT);
    CY_ASSERT_L1(config != NULL);
    CY_ASSERT_L3(config->supervisorClock <= CY_SYSCLK_CLKHF_CSV_SUPERVISOR_ALTHF);
    CY_ASSERT_L3(config->frequencyAction <= CY_SYSCLK_CSV_ERROR_FAULT_RESET);
    CY_ASSERT_L3(config->lossWindow      <= CY_SYSCLK_CSV_LOSS_512_CYCLES);
    CY_ASSERT_L3(config->lossAction      <= CY_SYSCLK_CSV_ERROR_FAULT_RESET);

    /* First update the limit bits; this can be done regardless of enable state. */
    SRSS_CLK_CSV_HF_LIMIT(clkHf) = _VAL2FLD(CLK_CSV_HF_LIMIT_UPPER_LIMIT, config->frequencyUpperLimit) |
                                   _VAL2FLD(CLK_CSV_HF_LIMIT_LOWER_LIMIT, config->frequencyLowerLimit);
    /* Then clear all CTL register bits, including the enable bit; disable loss detection. */
    SRSS_CLK_CSV_HF_CTL(clkHf) = 0UL;
    /* Finally, write the structure elements (which include an enable bit) to the CTL register. */
    SRSS_CLK_CSV_HF_CTL(clkHf) = _VAL2FLD(CLK_CSV_HF_CTL_CSV_LOSS_EN, config->enableLossDetection)             |
                                 _VAL2FLD(CLK_CSV_HF_CTL_CSV_LOSS_ACTION, (uint32_t)(config->lossAction))      |
                                 _VAL2FLD(CLK_CSV_HF_CTL_CSV_FREQ_EN, config->enableFrequencyFaultDetection)   |
                                 _VAL2FLD(CLK_CSV_HF_CTL_CSV_FREQ_ACTION, (uint32_t)(config->frequencyAction)) |
                                 _VAL2FLD(CLK_CSV_HF_CTL_CSV_LOSS_WINDOW, (uint32_t)(config->lossWindow))      |
                                 _VAL2FLD(CLK_CSV_HF_CTL_CSV_MUX, (uint32_t)(config->supervisorClock))         |
                                 _VAL2FLD(CLK_CSV_HF_CTL_CSV_FREQ_WINDOW, config->supervisingWindow);
    return CY_SYSCLK_SUCCESS; /* placeholder */
}
#endif /* (SRSS_MASK_HFCSV != 0)  || defined(CY_DOXYGEN) */
/** \} group_sysclk_clk_hf_funcs */


/* ========================================================================== */
/* =====================    clk_peripherals SECTION    ====================== */
/* ========================================================================== */
/**
* \addtogroup group_sysclk_clk_peripheral_funcs
* \{
*/
/*******************************************************************************
* Function Name: Cy_SysClk_PeriphGetFrequency
****************************************************************************//**
*
* Reports the frequency of the output of a given peripheral divider.
*
* \param dividerType specifies which type of divider to use; \ref cy_en_divider_types_t
*
* \param dividerNum specifies which divider of the selected type to configure
*
* \return The frequency, in Hz.
*
* \note
* The reported frequency may be zero, which indicates unknown. This happens if
* the source input is clk_ext, ECO, clk_althf, dsi_out, or clk_altlf.
*
* \funcusage
* \snippet sysclk/sysclk_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysClk_PeriphGetFrequency
*
*******************************************************************************/
uint32_t Cy_SysClk_PeriphGetFrequency(cy_en_divider_types_t dividerType, uint32_t dividerNum)
{
    uint32_t rtnval = 0UL; /* 0 = unknown frequency */

    CY_ASSERT_L1(((dividerType == CY_SYSCLK_DIV_8_BIT)    && (dividerNum < PERI_DIV_8_NR))    ||
                 ((dividerType == CY_SYSCLK_DIV_16_BIT)   && (dividerNum < PERI_DIV_16_NR))   ||
                 ((dividerType == CY_SYSCLK_DIV_16_5_BIT) && (dividerNum < PERI_DIV_16_5_NR)) ||
                 ((dividerType == CY_SYSCLK_DIV_24_5_BIT) && (dividerNum < PERI_DIV_24_5_NR)));

    /* FLL or PLL configuration parameters */
    union
    {
        cy_stc_fll_manual_config_t fll;
        struct
        {
            uint8_t feedbackDiv;
            uint8_t referenceDiv;
            uint8_t outputDiv;
        } pll;
    } fllpll = {0UL};

    /* variables holding intermediate clock sources and dividers */
    cy_en_fll_pll_output_mode_t mode = CY_SYSCLK_FLLPLL_OUTPUT_AUTO; /* FLL or PLL mode; n/a for direct */
    bool                     locked = 0;      /* FLL or PLL lock status; n/a for direct */
    cy_en_clkpath_in_sources_t  source;       /* source input for path (FLL, PLL, or direct) */
    uint32_t                 source_freq;     /* source clock frequency, in Hz */
    cy_en_clkhf_in_sources_t path;            /* source input for root 0 (clkHf[0]) */
    uint32_t                 path_freq = 0UL; /* path (FLL, PLL, or direct) frequency, in Hz */
    uint32_t                 root_div;        /* root prescaler (1/2/4/8) */
    uint32_t                 clkHf0_div;      /* clkHf[0] predivider to clk_peri */

    /* clk_peri divider to selected peripheral clock */
    struct
    {
        uint32_t integer;
        uint32_t frac;
    } clkdiv = {0UL, 0UL};

    /* Start by finding the source input for root 0 (clkHf[0]) */
    path = Cy_SysClk_ClkHfGetSource(0UL);

    if (path == CY_SYSCLK_CLKHF_IN_CLKPATH0) /* FLL? (always path 0) */
    {
        Cy_SysClk_FllGetConfiguration(&fllpll.fll);
        source = Cy_SysClk_ClkPathGetSource(0UL);
        mode = fllpll.fll.outputMode;
        locked = Cy_SysClk_FllLocked();
    }
    else if ((uint32_t)path <= CY_SRSS_NUM_PLL) /* PLL? (always path 1 - N)*/
    {
        cy_stc_pll_manual_config_t config = {0U,0U,0U,false,CY_SYSCLK_FLLPLL_OUTPUT_AUTO};
        (void)Cy_SysClk_PllGetConfiguration((uint32_t)path, &config);
        fllpll.pll.feedbackDiv  = config.feedbackDiv;
        fllpll.pll.referenceDiv = config.referenceDiv;
        fllpll.pll.outputDiv    = config.outputDiv;
        mode = config.outputMode;
        source = Cy_SysClk_ClkPathGetSource((uint32_t)path);
        locked = Cy_SysClk_PllLocked((uint32_t)path);
    }
    else /* assume clk_path < CY_SRSS_NUM_CLKPATH */
    { /* Direct select path. Use PLL function to get the source. */
        source = Cy_SysClk_ClkPathGetSource((uint32_t)path);
    }

    /* get the frequency of the source, i.e., the path mux input */
    switch(source)
    {
        case CY_SYSCLK_CLKPATH_IN_IMO: /* IMO frequency is fixed at 8 MHz */
            source_freq = 8000000UL; /*Hz*/
            break;
        case CY_SYSCLK_CLKPATH_IN_ILO: /* ILO and WCO frequencies are nominally 32.768 kHz */
        case CY_SYSCLK_CLKPATH_IN_WCO:
            source_freq = 32768UL; /*Hz*/
            break;
        default:
            /* don't know the frequency of clk_ext, ECO, clk_althf, dsi_out, or clk_altlf */
            source_freq = 0UL; /* unknown frequency */
            break;
    }
    if (source_freq != 0UL)
    {
        /* Calculate the path frequency */
        if (path == CY_SYSCLK_CLKHF_IN_CLKPATH0) /* FLL? (always path 0) */
        {
            path_freq = source_freq; /* for bypass mode */
            /* if not bypass mode, apply the dividers calculation */
            if ((mode == CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT) || ((mode != CY_SYSCLK_FLLPLL_OUTPUT_INPUT) && (locked != 0UL)))
            {
                /* Ffll_out = Ffll_clk * FLL_MULT / FLL_REF_DIV / (FLL_OUTPUT_DIV + 1), where:
                 *  FLL_MULT, REFERENCE_DIV, and OUTPUT_DIV are FLL configuration register bitfields
                 * Check for possible divide by 0.
                 */
                if (fllpll.fll.refDiv != 0UL)
                {
                    path_freq = (uint32_t)(((uint64_t)path_freq * (uint64_t)fllpll.fll.fllMult) /
                                           (uint64_t)fllpll.fll.refDiv) /
                                ((uint32_t)(fllpll.fll.enableOutputDiv) + 1UL);
                }
                else
                {
                    path_freq = 0UL; /* error, one of the divisors is 0 */
                }
            }
        }
        else if ((uint32_t)path <= CY_SRSS_NUM_PLL) /* PLL? (always path 1 - N)*/
        {
            path_freq = source_freq; /* for bypass mode */
            /* if not bypass mode, apply the dividers calculation */
            if ((mode == CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT) || ((mode != CY_SYSCLK_FLLPLL_OUTPUT_INPUT) && (locked != 0UL)))
            {
                /* Fpll_out = Fpll_clk * FEEDBACK_DIV / REFERENCE_DIV / OUTPUT_DIV, where:
                 *  FEEDBACK_DIV, REFERENCE_DIV, and OUTPUT_DIV are PLL configuration register bitfields
                 * Check for possible divide by 0.
                 */
                if ((fllpll.pll.referenceDiv != 0UL) && (fllpll.pll.outputDiv != 0UL))
                {
                    path_freq = (uint32_t)(((uint64_t)source_freq * (uint64_t)fllpll.pll.feedbackDiv) /
                                           (uint64_t)fllpll.pll.referenceDiv) /
                                (uint32_t)fllpll.pll.outputDiv;
                }
                else
                {
                    path_freq = 0UL; /* error, one of the divisors is 0 */
                }
            }
        }
        else /* assume clk_path < CY_SRSS_NUM_CLKPATH */
        { /* direct select path */
            path_freq = source_freq;
        }

        /* get the prescaler value for root 0, or clkHf[0]: 1/2/4/8 */
        root_div = 1UL << (uint32_t)Cy_SysClk_ClkHfGetDivider(0UL);

        /* get the predivider value for clkHf[0] to clk_peri */
        clkHf0_div = (uint32_t)Cy_SysClk_ClkPeriGetDivider() + 1UL;

        /* get the divider value for clk_peri to the selected peripheral clock */
        switch(dividerType)
        {
            case CY_SYSCLK_DIV_8_BIT:
            case CY_SYSCLK_DIV_16_BIT:
                clkdiv.integer = (uint32_t)Cy_SysClk_PeriphGetDivider(dividerType, dividerNum);
                /* frac = 0 means it's an integer divider */
                break;
            case CY_SYSCLK_DIV_16_5_BIT:
            case CY_SYSCLK_DIV_24_5_BIT:
                (void)Cy_SysClk_PeriphGetFracDivider(dividerType, dividerNum, &clkdiv.integer, &clkdiv.frac);
                break;
            default:
                break;
        }
        /* Divide the path input frequency down, and return the result.
           Stepping through the following code shows the frequency at each stage.
        */
        rtnval = path_freq / root_div; /* clkHf[0] frequency */
        rtnval /= clkHf0_div; /* clk_peri frequency */
        /* For fractional dividers, the divider is (int + 1) + frac/32.
         * Use the fractional value to round the divider to the nearest integer.
         */
        rtnval /= (clkdiv.integer + 1UL + ((clkdiv.frac >= 16UL) ? 1UL : 0UL)); /* peripheral divider output frequency */
    }

    return rtnval;
}
/** \} group_sysclk_clk_peripheral_funcs */

#if defined(__cplusplus)
}
#endif /* __cplusplus */


/* [] END OF FILE */
