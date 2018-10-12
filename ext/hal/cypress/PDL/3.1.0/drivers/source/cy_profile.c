/***************************************************************************//** 
* \file cy_profile.c
* \version 1.10
* 
* Provides an API declaration of the energy profiler (EP) driver. 
*
******************************************************************************** 
* \copyright 
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved. 
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_profile.h"
#include <string.h>

#ifdef CY_IP_MXPROFILE

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/* Number of elements in an array */
#define CY_N_ELMTS(a) (sizeof(a)/sizeof((a)[0]))

static cy_en_profile_status_t Cy_Profile_IsPtrValid(const cy_stc_profile_ctr_ptr_t ctrAddr);

/* Internal structure - Control and status information for each counter */
static cy_stc_profile_ctr_t cy_ep_ctrs[CY_PROFILE_PRFL_CNT_NR];


/* ========================================================================== */
/* =====================     LOCAL FUNCTION SECTION    ====================== */
/* ========================================================================== */
/*******************************************************************************
* Function Name: Cy_Profile_IsPtrValid
****************************************************************************//**
*
* Local utility function: reports (1) whether or not a given pointer points into
* the cy_ep_ctrs[] array, and (2) whether the counter has been assigned.
*
* \param ctrAddr The handle to (address of) the assigned counter
* 
* \return CY_PROFILE_SUCCESS, or CY_PROFILE_BAD_PARAM for invalid ctrAddr or counter not
* in use.
*
*******************************************************************************/
static cy_en_profile_status_t Cy_Profile_IsPtrValid(const cy_stc_profile_ctr_ptr_t ctrAddr)
{
    cy_en_profile_status_t retStatus = CY_PROFILE_BAD_PARAM;

    /* check for valid ctrAddr */
    uint32_t p_epCtrs = (uint32_t)cy_ep_ctrs;
    if ((p_epCtrs <= (uint32_t)ctrAddr) && ((uint32_t)ctrAddr < (p_epCtrs + (uint32_t)sizeof(cy_ep_ctrs))))
    {
        if (ctrAddr->used != 0u) /* check for counter being used */
        {
            retStatus = CY_PROFILE_SUCCESS;
        }
    }
    return (retStatus);
}


/* ========================================================================== */
/* ====================    INTERRUPT FUNCTION SECTION    ==================== */
/* ========================================================================== */
/*******************************************************************************
* Function Name: Cy_Profile_ISR
****************************************************************************//**
*
* EP interrupt handler: Increments the overflow member of the counter structure,
* for each counter that is in use and has an overflow.
*
* This handler is not configured or used automatically. You must configure the 
* interrupt handler for the EP, using Cy_SysInt_Init(). Typically you configure 
* the system to use \ref Cy_Profile_ISR() as the overflow interrupt handler. You
* can provide a custom interrupt handler to perform additional operations if
* required. Your handler can call \ref Cy_Profile_ISR() to handle counter
* overflow. 
*
*******************************************************************************/
void Cy_Profile_ISR(void)
{
    uint32_t ctr;

    /* Grab a copy of the overflow register. Each bit in the register indicates
       whether or not the respective counter has overflowed. */
    uint32_t ovflowBits = _FLD2VAL(PROFILE_INTR_MASKED_CNT_OVFLW, PROFILE->INTR_MASKED);

    PROFILE->INTR = ovflowBits; /* clear the sources of the interrupts */

    /* scan through the overflow bits, i.e., for each counter */
    for (ctr = 0UL; (ctr < (uint32_t)(PROFILE_PRFL_CNT_NR)) && (ovflowBits != 0UL); ctr++)
    {
        /* Increment the overflow bit only if the counter is being used.
           (Which should always be the case.) */
        if (((ovflowBits & 1UL) != 0UL) && (cy_ep_ctrs[ctr].used != 0u))
        {
            cy_ep_ctrs[ctr].overflow++;
        }
        ovflowBits >>= 1; /* check the next bit, by shifting it into the LS position */
    }
}


/* ========================================================================== */
/* ==================      GENERAL PROFILE FUNCTIONS     ==================== */
/* ========================================================================== */
/*******************************************************************************
* Function Name: Cy_Profile_StartProfiling
****************************************************************************//**
*
* Starts the profiling/measurement window.
*
* This operation allows the enabled profile counters to start counting.
*
* \note The profile interrupt should be enabled before calling this function
* in order for the firmware to be notified when a counter overflow occurs.
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_StartProfiling
*
*******************************************************************************/
void Cy_Profile_StartProfiling(void)
{
    uint32_t i;

    /* clear all of the counter array overflow variables */
    for (i = 0UL; i < CY_N_ELMTS(cy_ep_ctrs); cy_ep_ctrs[i++].overflow = 0UL){}
    /* send the hardware command */
    PROFILE->CMD = CY_PROFILE_START_TR;
}


/* ========================================================================== */
/* ===================    COUNTER FUNCTIONS SECTION    ====================== */
/* ========================================================================== */
/*******************************************************************************
* Function Name: Cy_Profile_ClearConfiguration
****************************************************************************//**
*
* Clears all counter configurations and sets all counters and overflow counters 
* to 0. Calls Cy_Profile_ClearCounters() to clear counter registers.
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_ClearConfiguration
*
*******************************************************************************/
void Cy_Profile_ClearConfiguration(void)
{
    (void)memset((void *)cy_ep_ctrs, 0, sizeof(cy_ep_ctrs));
    Cy_Profile_ClearCounters();
}


/*******************************************************************************
* Function Name: Cy_Profile_ConfigureCounter
****************************************************************************//**
*
* Configures and assigns a hardware profile counter to the list of used counters.
*
* This function assigns an available profile counter to a slot in the internal 
* software data structure and returns the handle for that slot location. The data
* structure is used to keep track of the counter status and to implement a 64-bit
* profile counter. If no counter slots are available, the function returns a
* NULL pointer.
* 
* \param monitor The monitor source number
*
* \param duration Events are monitored (0), or duration is monitored (1)
*
* \param refClk Counter reference clock
*
* \param weight Weighting factor for the counter value
* 
* \return A pointer to the counter data structure. NULL if no counter is
* available.
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_ConfigureCounter
*
*******************************************************************************/
cy_stc_profile_ctr_ptr_t Cy_Profile_ConfigureCounter(en_ep_mon_sel_t monitor, cy_en_profile_duration_t duration,
                                                     cy_en_profile_ref_clk_t refClk,  uint32_t weight)
{
    CY_ASSERT_L1(CY_PROFILE_IS_MONITOR_VALID(monitor));
    CY_ASSERT_L3(CY_PROFILE_IS_DURATION_VALID(duration));
    CY_ASSERT_L3(CY_PROFILE_IS_REFCLK_VALID(refClk));
    
    cy_stc_profile_ctr_ptr_t retVal = NULL; /* error value if no counter is available */
    volatile uint8_t i;
    
    /* scan through the counters for an unused one */
    for (i = 0u; (cy_ep_ctrs[i].used != 0u) && (i < CY_N_ELMTS(cy_ep_ctrs)); i++){}
    if (i < CY_N_ELMTS(cy_ep_ctrs))
    { /* found one, fill in its data structure */
        cy_ep_ctrs[i].ctrNum = i;
        cy_ep_ctrs[i].used = 1u;
        cy_ep_ctrs[i].cntAddr = (PROFILE_CNT_STRUCT_Type *)&(PROFILE->CNT_STRUCT[i]);
        cy_ep_ctrs[i].ctlRegVals.cntDuration = duration;
        cy_ep_ctrs[i].ctlRegVals.refClkSel = refClk;
        cy_ep_ctrs[i].ctlRegVals.monSel = monitor;
        cy_ep_ctrs[i].overflow = 0UL;
        cy_ep_ctrs[i].weight = weight;
        /* pass back the handle to (address of) the counter data structure */
        retVal = &cy_ep_ctrs[i];
        
        /* Load the CTL register bitfields of the assigned counter. */
        retVal->cntAddr->CTL =
            _VAL2FLD(PROFILE_CNT_STRUCT_CTL_CNT_DURATION, retVal->ctlRegVals.cntDuration) |
            _VAL2FLD(PROFILE_CNT_STRUCT_CTL_REF_CLK_SEL,  retVal->ctlRegVals.refClkSel)   |
            _VAL2FLD(PROFILE_CNT_STRUCT_CTL_MON_SEL,      retVal->ctlRegVals.monSel);

    }
    return (retVal);
}


/*******************************************************************************
* Function Name: Cy_Profile_FreeCounter
****************************************************************************//**
*
* Frees up a counter from a previously-assigned monitor source.
*
* \ref Cy_Profile_ConfigureCounter() must have been called for this counter
* before calling this function.
*
* \param ctrAddr The handle to the assigned counter (returned by calling
* \ref Cy_Profile_ConfigureCounter()).
* 
* \return 
* Status of the operation.
*
* \note The counter is not disabled by this function.
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_FreeCounter
*
*******************************************************************************/
cy_en_profile_status_t Cy_Profile_FreeCounter(cy_stc_profile_ctr_ptr_t ctrAddr)
{
   cy_en_profile_status_t retStatus = CY_PROFILE_BAD_PARAM;
   
    retStatus = Cy_Profile_IsPtrValid(ctrAddr);
    if (retStatus == CY_PROFILE_SUCCESS)
    {
        ctrAddr->used = 0u;
    }
    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_Profile_EnableCounter
****************************************************************************//**
*
* Enables an assigned counter. 
*
* \ref Cy_Profile_ConfigureCounter() must have been called for this counter
* before calling this function.
*
* \param ctrAddr The handle to the assigned counter, (returned by calling
* \ref Cy_Profile_ConfigureCounter()).
* 
* \return 
* Status of the operation.
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_EnableCounter
*
*******************************************************************************/
cy_en_profile_status_t Cy_Profile_EnableCounter(cy_stc_profile_ctr_ptr_t ctrAddr)
{
    cy_en_profile_status_t retStatus = CY_PROFILE_BAD_PARAM;
    
    retStatus = Cy_Profile_IsPtrValid(ctrAddr);
    if (retStatus == CY_PROFILE_SUCCESS)
    {
        /* set the ENABLED bit */
        ctrAddr->cntAddr->CTL |= _VAL2FLD(PROFILE_CNT_STRUCT_CTL_ENABLED, 1UL);
        /* set the INTR_MASK bit for the counter being used */
        PROFILE->INTR_MASK |= (1UL << (ctrAddr->ctrNum));
    }
    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_Profile_DisableCounter
****************************************************************************//**
*
* Disables an assigned counter.
*
* \ref Cy_Profile_ConfigureCounter() must have been called for this counter
* before calling this function.
*
* \param ctrAddr The handle to the assigned counter, (returned by calling
* \ref Cy_Profile_ConfigureCounter()).
* 
* \return 
* Status of the operation.
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_DisableCounter
*
*******************************************************************************/
cy_en_profile_status_t Cy_Profile_DisableCounter(cy_stc_profile_ctr_ptr_t ctrAddr)
{
    cy_en_profile_status_t retStatus = Cy_Profile_IsPtrValid(ctrAddr);
    if (retStatus == CY_PROFILE_SUCCESS)
    {
        /* clear the ENABLED bit */
        ctrAddr->cntAddr->CTL &= ~(_VAL2FLD(PROFILE_CNT_STRUCT_CTL_ENABLED, 1UL));
        /* clear the INTR_MASK bit for the counter being used */
        PROFILE->INTR_MASK &= ~(1UL << (ctrAddr->ctrNum));
    }
    return (retStatus);
}


/* ========================================================================== */
/* ==================    CALCULATION FUNCTIONS SECTION    =================== */
/* ========================================================================== */
/*******************************************************************************
* Function Name: Cy_Profile_GetRawCount
****************************************************************************//**
*
* Reports the raw count value for a specified counter.
*
* \param ctrAddr The handle to the assigned counter, (returned by calling
* \ref Cy_Profile_ConfigureCounter()).
*
* \param result Output parameter used to write in the result.
* 
* \return 
* Status of the operation.
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_GetRawCount
*
*******************************************************************************/
cy_en_profile_status_t Cy_Profile_GetRawCount(cy_stc_profile_ctr_ptr_t ctrAddr, uint64_t *result)
{
    cy_en_profile_status_t retStatus = CY_PROFILE_BAD_PARAM;
    
    if ((result != NULL) && (CY_PROFILE_SUCCESS == Cy_Profile_IsPtrValid(ctrAddr)))
    {
        /* read the counter control register, and the counter current value */
        ctrAddr->ctlReg = ctrAddr->cntAddr->CTL;
        ctrAddr->cntReg = ctrAddr->cntAddr->CNT;

        /* report the count with overflow */
        *result = ((uint64_t)(ctrAddr->overflow) << 32) | (uint64_t)(ctrAddr->cntReg);
    }
    
    return (retStatus);
}

/*******************************************************************************
* Function Name: Cy_Profile_GetWeightedCount
****************************************************************************//**
*
* Reports the count value for a specified counter, multiplied by the weight
* factor for that counter.
*
* \param ctrAddr The handle to the assigned counter, (returned by calling
* \ref Cy_Profile_ConfigureCounter()).
*
* \param result Output parameter used to write in the result.
* 
* \return 
* Status of the operation.
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_GetWeightedCount
*
*******************************************************************************/
cy_en_profile_status_t Cy_Profile_GetWeightedCount(cy_stc_profile_ctr_ptr_t ctrAddr, uint64_t *result)
{
    uint64_t temp;
    cy_en_profile_status_t retStatus = CY_PROFILE_BAD_PARAM;
    
    if ((result != NULL) && (CY_PROFILE_SUCCESS == Cy_Profile_GetRawCount(ctrAddr, &temp)))
    {
        /* calculate weighted count */
        *result = temp * (uint64_t)(ctrAddr->weight);
    }
    
    return (retStatus);
}

/*******************************************************************************
* Function Name: Cy_Profile_GetSumWeightedCounts
****************************************************************************//**
*
* Reports the weighted sum result of the first n number of counter count values
* starting from the specified profile counter data structure base address.
*
* Each count value is multiplied by its weighing factor before the summing
* operation is performed.
*
* \param ptrsArray Base address of the profile counter data structure
*
* \param numCounters Number of measured counters in ptrsArray[]
*
* \return
* The weighted sum of the specified counters
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_GetSumWeightedCounts
*
*******************************************************************************/
uint64_t Cy_Profile_GetSumWeightedCounts(cy_stc_profile_ctr_ptr_t ptrsArray[],
                                    uint32_t numCounters)
{
    CY_ASSERT_L2(CY_PROFILE_IS_CNT_VALID(numCounters));
    
    uint64_t daSum = (uint64_t)0UL;
    
    if(ptrsArray != NULL)
    {
        uint64_t num;
        uint32_t i;

        for (i = 0UL; i < numCounters; i++)
        {
            /* ignore error reported by Ep_GetWeightedCount() */
            if (Cy_Profile_GetWeightedCount(ptrsArray[i], &num) == CY_PROFILE_SUCCESS)
            {
                daSum += num;
            }
        }
    }
    
    return (daSum);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CY_IP_MXPROFILE */

/* [] END OF FILE */
