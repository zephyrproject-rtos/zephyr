/***************************************************************************//**
* \file cy_mcwdt.c
* \version 1.20
*
*  Description:
*   Provides a system API for the MCWDT driver.
*
********************************************************************************
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_mcwdt.h"

#ifdef CY_IP_MXS40SRSS_MCWDT

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
* Function Name: Cy_MCWDT_Init
****************************************************************************//**
*
* Initializes the MCWDT block.
*
*  \param base
*  The base pointer to a structure that describes the registers.
*
*  \param config
*  The pointer to a structure that contains component configuration data.
*
* \return cy_en_mcwdt_status_t
*     *base checking result. If the pointer is NULL, returns error.
*
*  \note
*  This API should not be called when the counters are running. Prior to calling
*  this API the counter should be disabled.
*
*******************************************************************************/
cy_en_mcwdt_status_t Cy_MCWDT_Init(MCWDT_STRUCT_Type *base, cy_stc_mcwdt_config_t const *config)
{
    cy_en_mcwdt_status_t ret = CY_MCWDT_BAD_PARAM;

    if ((base != NULL) && (config != NULL))
    {
        CY_ASSERT_L2(CY_MCWDT_IS_MATCH_VALID(config->c0ClearOnMatch, config->c0Match));
        CY_ASSERT_L2(CY_MCWDT_IS_MATCH_VALID(config->c1ClearOnMatch, config->c1Match));
        CY_ASSERT_L2(CY_MCWDT_IS_BIT_VALID(config->c2ToggleBit));
        CY_ASSERT_L3(CY_MCWDT_IS_MODE_VALID((cy_en_mcwdtmode_t)config->c0Mode));
        CY_ASSERT_L3(CY_MCWDT_IS_MODE_VALID((cy_en_mcwdtmode_t)config->c1Mode));
        CY_ASSERT_L3(CY_MCWDT_IS_MODE_VALID((cy_en_mcwdtmode_t)config->c2Mode));

        MCWDT_STRUCT_MCWDT_MATCH(base) = _VAL2FLD(MCWDT_STRUCT_MCWDT_MATCH_WDT_MATCH1, config->c1Match) |
                                         _VAL2FLD(MCWDT_STRUCT_MCWDT_MATCH_WDT_MATCH0, config->c0Match);

        MCWDT_STRUCT_MCWDT_CONFIG(base) = _VAL2FLD(MCWDT_STRUCT_MCWDT_CONFIG_WDT_BITS2, config->c2ToggleBit)      |
                                       _VAL2FLD(MCWDT_STRUCT_MCWDT_CONFIG_WDT_MODE2, config->c2Mode)              |
                                       _VAL2FLD(MCWDT_STRUCT_MCWDT_CONFIG_WDT_CLEAR0, config->c0ClearOnMatch)     |
                                       _VAL2FLD(MCWDT_STRUCT_MCWDT_CONFIG_WDT_CLEAR1, config->c1ClearOnMatch)     |
                                       (config->c1c2Cascade ? MCWDT_STRUCT_MCWDT_CONFIG_WDT_CASCADE1_2_Msk : 0UL) |
                                       _VAL2FLD(MCWDT_STRUCT_MCWDT_CONFIG_WDT_MODE1, config->c1Mode)              |
                                       (config->c0c1Cascade ? MCWDT_STRUCT_MCWDT_CONFIG_WDT_CASCADE0_1_Msk : 0UL) |
                                       _VAL2FLD(MCWDT_STRUCT_MCWDT_CONFIG_WDT_MODE0, config->c0Mode);
                           
        ret = CY_MCWDT_SUCCESS;
    }
                           
    return (ret);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_DeInit
****************************************************************************//**
*
*  De-initializes the MCWDT block, returns register values to their default state.
*
*  \param base
*  The base pointer to a structure that describes the registers.
*
*  \note
*  This API should not be called when the counters are running. Prior to calling
*  this API the counter should be disabled.
*
*******************************************************************************/
void Cy_MCWDT_DeInit(MCWDT_STRUCT_Type *base)
{
    Cy_MCWDT_Unlock(base);

    MCWDT_STRUCT_MCWDT_CNTLOW(base)    = 0UL;
    MCWDT_STRUCT_MCWDT_CNTHIGH(base)   = 0UL;
    MCWDT_STRUCT_MCWDT_MATCH(base)     = 0UL;
    MCWDT_STRUCT_MCWDT_CONFIG(base)    = 0UL;
    MCWDT_STRUCT_MCWDT_CTL(base)       = 0UL;
    MCWDT_STRUCT_MCWDT_INTR(base)      = 0UL;
    MCWDT_STRUCT_MCWDT_INTR_SET(base)  = 0UL;
    MCWDT_STRUCT_MCWDT_INTR_MASK(base) = 0UL;
}


/*******************************************************************************
* Function Name: Cy_MCWDT_GetCountCascaded
****************************************************************************//**
*
*  Reports the current value of combined C1-C0 cascaded counters.
*
*  \param base
*  The base pointer to a structure that describes the registers.
*
*  \note
*  The user must enable both counters, and cascade C0 to C1, 
*  before calling this function. C2 is not reported. 
*  Instead, to get a 64-bit C2-C1-C0 cascaded value, the
*  user must call this function followed by
*  Cy_MCWDT_GetCount(base, CY_MCWDT_COUNTER2), and then combine the results.
*  \note This function does not return the correct result when it is called 
*  after the Cy_MCWDT_Enable() or Cy_MCWDT_ResetCounters() function with 
*  a delay less than two lf_clk cycles. The recommended waitUs parameter 
*  value is 100 us.
*
*******************************************************************************/
uint32_t Cy_MCWDT_GetCountCascaded(MCWDT_STRUCT_Type const *base)
{
    uint32_t countVal = MCWDT_STRUCT_MCWDT_CNTLOW(base);
    uint32_t counter1 = countVal >> MCWDT_STRUCT_MCWDT_CNTLOW_WDT_CTR1_Pos;
    uint32_t counter0 = countVal & MCWDT_STRUCT_MCWDT_CNTLOW_WDT_CTR0_Msk;
    uint32_t match0 = _FLD2VAL(MCWDT_STRUCT_MCWDT_MATCH_WDT_MATCH0, MCWDT_STRUCT_MCWDT_MATCH(base));
    uint32_t match1 = _FLD2VAL(MCWDT_STRUCT_MCWDT_MATCH_WDT_MATCH1, MCWDT_STRUCT_MCWDT_MATCH(base));
    
    /* 
     * The counter  counter0 goes to zero when it reaches the match0 
     * value (c0ClearOnMatch = 1) or reaches the maximum 
     * value (c0ClearOnMatch = 0). The counter counter1 increments on 
     * the next rising edge of the MCWDT clock after 
     * the Clear On Match event takes place. 
     * The software increments counter1 to eliminate the case 
     * when the both counter0 and counter1 counters have zeros.
    */
    if (0u == counter0)
    {
        counter1++;
    }

    /* Check if the counter0 is Free running */
    if (0u == _FLD2VAL(MCWDT_STRUCT_MCWDT_CONFIG_WDT_CLEAR0, MCWDT_STRUCT_MCWDT_CONFIG(base)))
    {
        /* Save match0 value with the correction when counter0
         *  goes to zero when it reaches the match0 value. 
        */
        countVal = match0 + 1u;
        
        if (0u < counter1) 
        {
            /* Set match to the maximum value */
            match0 = MCWDT_STRUCT_MCWDT_CNTLOW_WDT_CTR0_Msk; 
        }
        
        if (countVal < counter0) 
        {
            /* Decrement counter1 when the counter0 is great than match0 value */
            counter1--;
        }
    }

    /* Add the correction to counter0 */
    counter0 += counter1;
    
    /* Set counter1 match value to 65535 when the counter1 is free running */
    if (0u == _FLD2VAL(MCWDT_STRUCT_MCWDT_CONFIG_WDT_CLEAR1, MCWDT_STRUCT_MCWDT_CONFIG(base)))
    {
        match1 = MCWDT_STRUCT_MCWDT_CNTLOW_WDT_CTR1_Msk >> MCWDT_STRUCT_MCWDT_CNTLOW_WDT_CTR1_Pos;
    }

    /* Check for overflow */
    if (match1 < counter1)
    {
        counter1 = 0u;
    }

    /* Calculate the combined value for C1-C0 cascaded counters */
    countVal = counter0 + (counter1 * match0);

    return (countVal);
}


#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXS40SRSS_MCWDT */

/* [] END OF FILE */
