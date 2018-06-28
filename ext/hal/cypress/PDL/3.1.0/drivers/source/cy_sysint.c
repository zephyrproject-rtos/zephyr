/***************************************************************************//**
* \file  cy_sysint.c
* \version 1.20
*
* \brief
* Provides an API implementation of the SysInt driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_sysint.h"

#if defined(__cplusplus)
extern "C" {
#endif


/*******************************************************************************
* Function Name: Cy_SysInt_Init
****************************************************************************//**
*
* \brief Initializes the referenced interrupt by setting the priority and the
* interrupt vector.
*
* Use the CMSIS core function NVIC_EnableIRQ(config.intrSrc) to enable the interrupt.
*
* \param config
* Interrupt configuration structure
*
* \param userIsr
* Address of the ISR
*
* \return 
* Initialization status  
*
* \note The interrupt vector will only be relocated if the vector table was
* moved to __ramVectors in SRAM. Otherwise it is ignored.
*
* \funcusage
* \snippet sysint/sysint_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysInt_Init
*
*******************************************************************************/
cy_en_sysint_status_t Cy_SysInt_Init(const cy_stc_sysint_t* config, cy_israddress userIsr)
{
    cy_en_sysint_status_t status = CY_SYSINT_SUCCESS;

    if(NULL != config)
    {
        CY_ASSERT_L3(CY_SYSINT_IS_PRIORITY_VALID(config->intrPriority));

        #if (CY_CPU_CORTEX_M0P)
            if (config->intrSrc > SysTick_IRQn)
            {
                Cy_SysInt_SetInterruptSource(config->intrSrc, config->cm0pSrc);
            }
            else
            {
                status = CY_SYSINT_BAD_PARAM;
            }
        #endif
        
        NVIC_SetPriority(config->intrSrc, config->intrPriority);
        
        /* Only set the new vector if it was moved to __ramVectors */
        if (SCB->VTOR == (uint32_t)&__ramVectors)
        {
            CY_ASSERT_L1(CY_SYSINT_IS_VECTOR_VALID(userIsr));

            (void)Cy_SysInt_SetVector(config->intrSrc, userIsr);
        }
    }
    else
    {
        status = CY_SYSINT_BAD_PARAM;
    }
    
    return(status);
}


#if (CY_CPU_CORTEX_M0P) || defined (CY_DOXYGEN)

/*******************************************************************************
* Function Name: Cy_SysInt_SetInterruptSource
****************************************************************************//**
*
* \brief Configures the interrupt selection for the specified NVIC channel.
*
* Setting this value to "disconnected_IRQn" disconnects the interrupt
* source and will effectively deactivate the interrupt.
*
* \param intrSrc
* NVIC channel number connected to the CPU core
*
* \param devIntrSrc
* Device interrupt to be routed to the NVIC channel
*
* \note This function is available for CM0+ core only.
*
*
*******************************************************************************/
void Cy_SysInt_SetInterruptSource(IRQn_Type intrSrc, cy_en_intr_t devIntrSrc)
{
    if (CY_CPUSS_V1)
    {
        uint32_t regPos     = ((uint32_t)intrSrc >> CY_SYSINT_CM0P_MUX_SHIFT) & CY_SYSINT_MUX_REG_MSK;
        uint32_t bitPos     = ((uint32_t)intrSrc - (uint32_t)(regPos << CY_SYSINT_CM0P_MUX_SHIFT)) << CY_SYSINT_CM0P_MUX_SCALE;
        uint32_t bitMask    = (uint32_t)(CY_SYSINT_CM0P_MUX_MASK << bitPos);
        uint32_t bitMaskClr = (uint32_t)(~bitMask);
        uint32_t bitMaskSet = (((uint32_t)devIntrSrc << bitPos) & bitMask);
        uint32_t tempReg;

        tempReg = CPUSS_CM0_INT_CTL[regPos] & bitMaskClr;
        CPUSS_CM0_INT_CTL[regPos] = tempReg | bitMaskSet;
    }
    else /* CPUSS_V2 */
    {
        uint32_t intrEn = (devIntrSrc == disconnected_IRQn) ? CY_SYSINT_DISABLE : CY_SYSINT_ENABLE;
        ((CPUSS_V2_Type *)CPUSS)->CM0_SYSTEM_INT_CTL[devIntrSrc] = _VAL2FLD(CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_IDX, intrSrc)
                                                | _VAL2FLD(CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_VALID, intrEn);
    }
}


/*******************************************************************************
* Function Name: Cy_SysInt_GetInterruptSource
****************************************************************************//**
*
* \brief Gets the interrupt source of the NVIC channel.
*
* \param intrSrc
* NVIC channel number connected to the CPU core
*
* \return 
* Device interrupt connected to the NVIC channel. A returned value of 
* "disconnected_IRQn" indicates that the interrupt source is disconnected.  
*
* \note This function is available for CM0+ core only.
*
* \note This function is only available for devices using CPUSS v1. For all
* other devices, use the Cy_SysInt_GetNvicConnection() function.
*
* \funcusage
* \snippet sysint/sysint_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysInt_SetIntSource
*
*******************************************************************************/
cy_en_intr_t Cy_SysInt_GetInterruptSource(IRQn_Type intrSrc)
{
    uint32_t tempReg = (uint32_t)disconnected_IRQn;
    cy_en_intr_t srcVal;

    /* TODO: 
     * RFMS: Replace with device struct member check - LURE: done. 
     * RFMS: Apply proper type casting for CPUSS - LURE: to be reviewed during the code review.
     */
    if (CY_CPUSS_V1)
    {
        uint32_t regPos  = ((uint32_t)intrSrc >> CY_SYSINT_CM0P_MUX_SHIFT) & CY_SYSINT_MUX_REG_MSK;
        uint32_t bitPos  = ((uint32_t)intrSrc - (regPos <<  CY_SYSINT_CM0P_MUX_SHIFT)) <<  CY_SYSINT_CM0P_MUX_SCALE;
        uint32_t bitMask = (uint32_t)(CY_SYSINT_CM0P_MUX_MASK << bitPos);
        
        tempReg = (CPUSS_CM0_INT_CTL[regPos] & bitMask) >> bitPos;
    }
    srcVal = (cy_en_intr_t)tempReg;
    return (srcVal);
}

    
/*******************************************************************************
* Function Name: Cy_SysInt_GetNvicConnection
****************************************************************************//**
*
* \brief Gets the NVIC channel that the interrupt source is connected to.
*
* \param devIntrSrc
* Device interrupt that is potentially connected to the NVIC channel.
*
* \return
* NVIC channel number connected to the CPU core. A returned value of
* "disconnected_IRQn" indicates that the interrupt source is disconnected.
*
* \note This function is available for CM0+ core only.
*
* \note This function is only available for devices using CPUSS v2 or higher.
*
*
*******************************************************************************/
IRQn_Type Cy_SysInt_GetNvicConnection(cy_en_intr_t devIntrSrc)
{
    uint32_t tempReg = (uint32_t)disconnected_IRQn;

    /* TODO: 
     * RFMS: Replace with device struct member check - LURE: done. 
     * RFMS: Apply proper type casting for CPUSS - LURE: to be reviewed during the code review.
     */
    if ((!CY_CPUSS_V1) && (CY_SYSINT_ENABLE == _FLD2VAL(CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_VALID, CPUSS_CM0_SYSTEM_INT_CTL[devIntrSrc])))
    {
        tempReg = _FLD2VAL(CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_IDX, CPUSS_CM0_SYSTEM_INT_CTL[devIntrSrc]);
    }
    return ((IRQn_Type)tempReg);
}


/*******************************************************************************
* Function Name: Cy_SysInt_GetInterruptActive
****************************************************************************//**
*
* \brief Gets the highest priority active interrupt for the selected NVIC channel.
*
* The priority of the interrupt in a given channel is determined by the index
* value of the interrupt in the cy_en_intr_t enum. The lower the index, the 
* higher the priority. E.g. Consider a case where an interrupt source with value
* 29 and an interrupt source with value 46 both source the same NVIC channel. If
* both are active (triggered) at the same time, calling Cy_SysInt_GetInterruptActive()
* will return 29 as the active interrupt.
*
* \param intrSrc
* NVIC channel number connected to the CPU core
*
* \return
* Device interrupt connected to the NVIC channel. A returned value of 
* "disconnected_IRQn" indicates that there are no active (pending) interrupts 
* on this NVIC channel.  
*
* \note This function is available for CM0+ core only.
*
* \note This function is only available for devices using CPUSS v2 or higher.
*
*
*******************************************************************************/
cy_en_intr_t Cy_SysInt_GetInterruptActive(IRQn_Type intrSrc)
{
    uint32_t tempReg = (uint32_t)disconnected_IRQn;
    intrSrc &= CY_SYSINT_INT_STATUS_MSK;

    /* TODO: 
     * RFMS: Replace with device struct member check - LURE: done. 
     * RFMS: Apply proper type casting for CPUSS - LURE: to be reviewed during the code review.
     */
    if ((!CY_CPUSS_V1) && (CY_SYSINT_ENABLE == _FLD2VAL(CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_VALID, CPUSS_CM0_INT_STATUS[intrSrc])))
    {
        tempReg = _FLD2VAL(CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_IDX, CPUSS_CM0_INT_STATUS[intrSrc]);
    }
    return (cy_en_intr_t)tempReg;
}

#endif


/*******************************************************************************
* Function Name: Cy_SysInt_SetVector
****************************************************************************//**
*
* \brief Changes the ISR vector for the Interrupt.
*
* This function relies on the assumption that the vector table is
* relocated to __ramVectors[RAM_VECTORS_SIZE] in SRAM. Otherwise it will
* return the address of the default ISR location in the Flash vector table.
*
* \param intrSrc
* Interrrupt source
*
* \param userIsr
* Address of the ISR to set in the interrupt vector table
*
* \return
* Previous address of the ISR in the interrupt vector table
*
* \note For CM0+, this function sets the interrupt vector for the interrupt
* channel on the NVIC.
*
* \funcusage
* \snippet sysint/sysint_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysInt_SetVector
*
*******************************************************************************/
cy_israddress Cy_SysInt_SetVector(IRQn_Type intrSrc, cy_israddress userIsr)
{
    cy_israddress prevIsr;
    
    /* Only set the new vector if it was moved to __ramVectors */
    if (SCB->VTOR == (uint32_t)&__ramVectors)
    {
        CY_ASSERT_L1(CY_SYSINT_IS_VECTOR_VALID(userIsr));

        prevIsr = __ramVectors[CY_INT_IRQ_BASE + intrSrc];
        __ramVectors[CY_INT_IRQ_BASE + intrSrc] = userIsr;
    }
    else
    {
        prevIsr = __Vectors[CY_INT_IRQ_BASE + intrSrc];
    }

    return prevIsr;
}


/*******************************************************************************
* Function Name: Cy_SysInt_GetVector
****************************************************************************//**
*
* \brief Gets the address of the current ISR vector for the Interrupt.
*
* This function relies on the assumption that the vector table is
* relocated to __ramVectors[RAM_VECTORS_SIZE] in SRAM. Otherwise it will
* return the address of the default ISR location in the Flash vector table.
*
* \param intrSrc
* Interrupt source
*
* \return
* Address of the ISR in the interrupt vector table
*
* \note For CM0+, this function returns the interrupt vector for the interrupt 
* channel on the NVIC.
*
* \funcusage
* \snippet sysint/sysint_v1_10_sut_01.cydsn/main_cm4.c snippet_Cy_SysInt_SetVector
*
*******************************************************************************/
cy_israddress Cy_SysInt_GetVector(IRQn_Type intrSrc)
{
    cy_israddress currIsr;
    
    /* Only return the SRAM ISR address if it was moved to __ramVectors */
    if (SCB->VTOR == (uint32_t)&__ramVectors)
    {
        currIsr = __ramVectors[CY_INT_IRQ_BASE + intrSrc];
    }
    else
    {
        currIsr = __Vectors[CY_INT_IRQ_BASE + intrSrc];
    }
    
    return currIsr;
}


#if defined(__cplusplus)
}
#endif


/* [] END OF FILE */
