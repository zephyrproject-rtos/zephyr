/***************************************************************************//**
* \file cy_tcpwm_quaddec.h
* \version 1.10.1
*
* \brief
* The header file of the TCPWM Quadrature Decoder driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#if !defined(CY_TCPWM_QUADDEC_H)
#define CY_TCPWM_QUADDEC_H

#include "cy_tcpwm.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
* \addtogroup group_tcpwm_quaddec
* Driver API for Quadrature Decoder.
* \{
*/

/**
* \defgroup group_tcpwm_macros_quaddec          Macros
* \defgroup group_tcpwm_functions_quaddec       Functions
* \defgroup group_tcpwm_data_structures_quaddec Data Structures
* \} */

/**
* \addtogroup group_tcpwm_data_structures_quaddec
* \{
*/

/** Quadrature Decoder configuration structure */
typedef struct cy_stc_tcpwm_quaddec_config
{
    /** Selects the quadrature encoding mode. See \ref group_tcpwm_quaddec_resolution */
    uint32_t    resolution;
    /** Enables an interrupt on the terminal count, capture or compare. See \ref group_tcpwm_interrupt_sources */
    uint32_t    interruptSources;
    /** Configures how the index input behaves. See \ref group_tcpwm_input_modes */
    uint32_t    indexInputMode;
    /** Selects which input the index uses. The inputs are device-specific. See \ref group_tcpwm_input_selection */
    uint32_t    indexInput;
    /** Configures how the stop input behaves. See \ref group_tcpwm_input_modes */
    uint32_t    stopInputMode;
    /** Selects which input the stop uses. The inputs are device-specific. See \ref group_tcpwm_input_selection */
    uint32_t    stopInput;
    /** Selects which input the phiA uses. The inputs are device specific. See \ref group_tcpwm_input_selection */
    uint32_t    phiAInput;
    /** Selects which input the phiB uses. The inputs are device specific. See \ref group_tcpwm_input_selection */
    uint32_t    phiBInput;
    
}cy_stc_tcpwm_quaddec_config_t;
/** \} group_tcpwm_data_structures_quaddec */

/**
* \addtogroup group_tcpwm_macros_quaddec
* \{
* \defgroup group_tcpwm_quaddec_resolution QuadDec Resolution 
* \{
* The quadrature decoder resolution.
*/
#define CY_TCPWM_QUADDEC_X1                         (0U)    /**< X1 mode */
#define CY_TCPWM_QUADDEC_X2                         (1U)    /**< X2 mode */
#define CY_TCPWM_QUADDEC_X4                         (2U)    /**< X4 mode */
/** \} group_tcpwm_quaddec_resolution */

/** \defgroup group_tcpwm_quaddec_status QuadDec Status
* \{
* The counter status.
*/
#define CY_TCPWM_QUADDEC_STATUS_DOWN_COUNTING       (0x1UL)        /**< QuadDec is down counting */
#define CY_TCPWM_QUADDEC_STATUS_UP_COUNTING         (0x2UL)        /**< QuadDec is up counting */
/** QuadDec the counter is running */
#define CY_TCPWM_QUADDEC_STATUS_COUNTER_RUNNING     (TCPWM_CNT_STATUS_RUNNING_Msk)
/** \} group_tcpwm_quaddec_status */


/***************************************
*        Registers Constants
***************************************/
/** \cond INTERNAL */
#define CY_TCPWM_QUADDEC_CTRL_QUADDEC_MODE          (0x3UL)     /**< Quadrature encoding mode for CTRL register */
/** \endcond */
/** \} group_tcpwm_macros_quaddec */


/*******************************************************************************
*        Function Prototypes
*******************************************************************************/

/**
* \addtogroup group_tcpwm_functions_quaddec
* \{
*/

cy_en_tcpwm_status_t Cy_TCPWM_QuadDec_Init(TCPWM_Type *base, uint32_t cntNum, 
                                           cy_stc_tcpwm_quaddec_config_t const *config);
void Cy_TCPWM_QuadDec_DeInit(TCPWM_Type *base, uint32_t cntNum, cy_stc_tcpwm_quaddec_config_t const *config);
__STATIC_INLINE void Cy_TCPWM_QuadDec_Enable(TCPWM_Type *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_QuadDec_Disable(TCPWM_Type *base, uint32_t cntNum);
__STATIC_INLINE uint32_t Cy_TCPWM_QuadDec_GetStatus(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE uint32_t Cy_TCPWM_QuadDec_GetCapture(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE uint32_t Cy_TCPWM_QuadDec_GetCaptureBuf(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_QuadDec_SetCounter(TCPWM_Type *base, uint32_t cntNum, uint32_t count);
__STATIC_INLINE uint32_t Cy_TCPWM_QuadDec_GetCounter(TCPWM_Type const *base, uint32_t cntNum);


/*******************************************************************************
* Function Name: Cy_TCPWM_QuadDec_Enable
****************************************************************************//**
*
* Enables the counter in the TCPWM block for the QuadDec operation.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_quaddec_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_QuadDec_Init
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_QuadDec_Enable(TCPWM_Type *base, uint32_t cntNum)
{
    TCPWM_CTRL_SET(base) = (1UL << cntNum);
}

/*******************************************************************************
* Function Name: Cy_TCPWM_QuadDec_Disable
****************************************************************************//**
*
* Disables the counter in the TCPWM block.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_quaddec_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_QuadDec_DeInit
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_QuadDec_Disable(TCPWM_Type *base, uint32_t cntNum)
{
    TCPWM_CTRL_CLR(base) = (1UL << cntNum);
}


/*******************************************************************************
* Function Name: Cy_TCPWM_QuadDec_GetStatus
****************************************************************************//**
*
* Returns the status of the QuadDec.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return
* The status. See \ref group_tcpwm_quaddec_status
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_quaddec_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_QuadDec_GetStatus
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_QuadDec_GetStatus(TCPWM_Type const *base, uint32_t cntNum)
{
    uint32_t status = TCPWM_CNT_STATUS(base, cntNum);
    
    /* Generates proper up counting status, does not generated by HW */
    status &= ~CY_TCPWM_QUADDEC_STATUS_UP_COUNTING;
    status |= ((~status & CY_TCPWM_QUADDEC_STATUS_DOWN_COUNTING & (status >> TCPWM_CNT_STATUS_RUNNING_Pos)) << 
               CY_TCPWM_CNT_STATUS_UP_POS);

    return(status);
}


/*******************************************************************************
* Function Name: Cy_TCPWM_QuadDec_GetCapture
****************************************************************************//**
*
* Returns the capture value.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return
* The capture value.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_quaddec_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_QuadDec_Capture
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_QuadDec_GetCapture(TCPWM_Type const *base, uint32_t cntNum)
{
    return(TCPWM_CNT_CC(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_QuadDec_GetCaptureBuf
****************************************************************************//**
*
* Returns the buffered capture value.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return
* The buffered capture value.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_quaddec_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_QuadDec_Capture
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_QuadDec_GetCaptureBuf(TCPWM_Type const *base, uint32_t cntNum)
{
    return(TCPWM_CNT_CC_BUFF(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_QuadDec_SetCounter
****************************************************************************//**
*
* Sets the value of the counter.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \param count
* The value to write into the counter.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_quaddec_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_QuadDec_SetCounter
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_QuadDec_SetCounter(TCPWM_Type *base, uint32_t cntNum, uint32_t count)
{
    TCPWM_CNT_COUNTER(base, cntNum) = count;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_QuadDec_GetCounter
****************************************************************************//**
*
* Returns the value in the counter.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return
* The current counter value.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_quaddec_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_QuadDec_GetCounter
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_QuadDec_GetCounter(TCPWM_Type const *base, uint32_t cntNum)
{
    return(TCPWM_CNT_COUNTER(base, cntNum));
}

/** \} group_tcpwm_functions_quaddec */

/** \} group_tcpwm_quaddec */

#if defined(__cplusplus)
}
#endif

#endif /* CY_TCPWM_QUADDEC_H */


/* [] END OF FILE */
