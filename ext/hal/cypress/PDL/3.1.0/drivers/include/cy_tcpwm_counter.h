/***************************************************************************//**
* \file cy_tcpwm_counter.h
* \version 1.10.1
*
* \brief
* The header file of the TCPWM Timer Counter driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#if !defined(CY_TCPWM_COUNTER_H)
#define CY_TCPWM_COUNTER_H

#include "cy_tcpwm.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
* \addtogroup group_tcpwm_counter
* \{
* Driver API for Timer/Counter.
*/

/**
* \defgroup group_tcpwm_macros_counter          Macros
* \defgroup group_tcpwm_functions_counter       Functions
* \defgroup group_tcpwm_data_structures_counter Data Structures
* \} */

/**
* \addtogroup group_tcpwm_data_structures_counter
* \{
*/

/** Counter Timer configuration structure */
typedef struct cy_stc_tcpwm_counter_config
{
    uint32_t    period;             /**< Sets the period of the counter */
    /** Sets the clock prescaler inside the TCWPM block. See \ref group_tcpwm_counter_clk_prescalers */
    uint32_t    clockPrescaler;     
    uint32_t    runMode;            /**< Sets the Counter Timer Run mode. See \ref group_tcpwm_counter_run_modes */
    uint32_t    countDirection;     /**< Sets the counter direction. See \ref group_tcpwm_counter_direction */
    /** The counter can either compare or capture a value. See \ref group_tcpwm_counter_compare_capture */
    uint32_t    compareOrCapture;   
    uint32_t    compare0;           /**< Sets the value for Compare0*/
    uint32_t    compare1;           /**< Sets the value for Compare1*/
    bool        enableCompareSwap;  /**< If enabled, the compare values are swapped each time the comparison is true */
    /** Enabled an interrupt on the terminal count, capture or compare. See \ref group_tcpwm_interrupt_sources */
    uint32_t    interruptSources;
    uint32_t    captureInputMode;   /**< Configures how the capture input behaves. See \ref group_tcpwm_input_modes */
    /** Selects which input the capture uses, the inputs are device-specific. See \ref group_tcpwm_input_selection */
    uint32_t    captureInput;
    uint32_t    reloadInputMode;    /**< Configures how the reload input behaves. See \ref group_tcpwm_input_modes */
    /** Selects which input the reload uses, the inputs are device-specific. See \ref group_tcpwm_input_selection */
    uint32_t    reloadInput;
    uint32_t    startInputMode;     /**< Configures how the start input behaves. See \ref group_tcpwm_input_modes */
    /** Selects which input the start uses, the inputs are device-specific. See \ref group_tcpwm_input_selection */
    uint32_t    startInput;
    uint32_t    stopInputMode;      /**< Configures how the stop input behaves. See \ref group_tcpwm_input_modes */
    /** Selects which input the stop uses, the inputs are device-specific. See \ref group_tcpwm_input_selection */
    uint32_t    stopInput;
    uint32_t    countInputMode;     /**< Configures how the count input behaves. See \ref group_tcpwm_input_modes */
    /** Selects which input the count uses, the inputs are device-specific. See \ref group_tcpwm_input_selection */
    uint32_t    countInput;
}cy_stc_tcpwm_counter_config_t;
/** \} group_tcpwm_data_structures_counter */

/**
* \addtogroup group_tcpwm_macros_counter
* \{
* \defgroup group_tcpwm_counter_run_modes Counter Run Modes
* \{
* Run modes for the counter timer.
*/
#define CY_TCPWM_COUNTER_ONESHOT                (1U)    /**< Counter runs once and then stops */
#define CY_TCPWM_COUNTER_CONTINUOUS             (0U)    /**< Counter runs forever */
/** \} group_tcpwm_counter_run_modes */

/** \defgroup group_tcpwm_counter_direction Counter Direction
* The counter directions.
* \{
*/
#define CY_TCPWM_COUNTER_COUNT_UP               (0U)    /**< Counter counts up */
#define CY_TCPWM_COUNTER_COUNT_DOWN             (1U)    /**< Counter counts down */
/** Counter counts up and down terminal count only occurs on underflow. */
#define CY_TCPWM_COUNTER_COUNT_UP_DOWN_1        (2U)
/** Counter counts up and down terminal count occurs on both overflow and underflow. */
#define CY_TCPWM_COUNTER_COUNT_UP_DOWN_2        (3U)
/** \} group_tcpwm_counter_direction */

/** \defgroup group_tcpwm_counter_clk_prescalers Counter CLK Prescalers
* \{
* The clock prescaler values.
*/
#define CY_TCPWM_COUNTER_PRESCALER_DIVBY_1      (0U) /**< Divide by 1 */
#define CY_TCPWM_COUNTER_PRESCALER_DIVBY_2      (1U) /**< Divide by 2 */
#define CY_TCPWM_COUNTER_PRESCALER_DIVBY_4      (2U) /**< Divide by 4 */
#define CY_TCPWM_COUNTER_PRESCALER_DIVBY_8      (3U) /**< Divide by 8 */
#define CY_TCPWM_COUNTER_PRESCALER_DIVBY_16     (4U) /**< Divide by 16 */
#define CY_TCPWM_COUNTER_PRESCALER_DIVBY_32     (5U) /**< Divide by 32 */
#define CY_TCPWM_COUNTER_PRESCALER_DIVBY_64     (6U) /**< Divide by 64 */
#define CY_TCPWM_COUNTER_PRESCALER_DIVBY_128    (7U) /**< Divide by 128 */
/** \} group_tcpwm_counter_clk_prescalers */

/** \defgroup group_tcpwm_counter_compare_capture Counter Compare Capture
* \{
* A compare or capture mode.
*/
#define CY_TCPWM_COUNTER_MODE_CAPTURE           (2U)  /**< Timer/Counter is in Capture Mode */
#define CY_TCPWM_COUNTER_MODE_COMPARE           (0U)  /**< Timer/Counter is in Compare Mode */
/** \} group_tcpwm_counter_compare_capture */

/** \defgroup group_tcpwm_counter_status Counter Status
* \{
* The counter status.
*/
#define CY_TCPWM_COUNTER_STATUS_DOWN_COUNTING   (0x1UL)        /**< Timer/Counter is down counting */
#define CY_TCPWM_COUNTER_STATUS_UP_COUNTING     (0x2UL)        /**< Timer/Counter is up counting */

/** Timer/Counter is running */
#define CY_TCPWM_COUNTER_STATUS_COUNTER_RUNNING (TCPWM_CNT_STATUS_RUNNING_Msk)
/** \} group_tcpwm_counter_status */
/** \} group_tcpwm_macros_counter */


/*******************************************************************************
*        Function Prototypes
*******************************************************************************/

/**
* \addtogroup group_tcpwm_functions_counter
* \{
*/
cy_en_tcpwm_status_t Cy_TCPWM_Counter_Init(TCPWM_Type *base, uint32_t cntNum, 
                                           cy_stc_tcpwm_counter_config_t const *config);
void Cy_TCPWM_Counter_DeInit(TCPWM_Type *base, uint32_t cntNum, cy_stc_tcpwm_counter_config_t const *config);
__STATIC_INLINE void Cy_TCPWM_Counter_Enable(TCPWM_Type *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_Counter_Disable(TCPWM_Type *base, uint32_t cntNum);
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetStatus(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetCapture(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetCaptureBuf(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_Counter_SetCompare0(TCPWM_Type *base, uint32_t cntNum, uint32_t compare0);
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetCompare0(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_Counter_SetCompare1(TCPWM_Type *base, uint32_t cntNum, uint32_t compare1);
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetCompare1(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_Counter_EnableCompareSwap(TCPWM_Type *base, uint32_t cntNum, bool enable);
__STATIC_INLINE void Cy_TCPWM_Counter_SetCounter(TCPWM_Type *base, uint32_t cntNum, uint32_t count);
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetCounter(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_Counter_SetPeriod(TCPWM_Type *base, uint32_t cntNum, uint32_t period);
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetPeriod(TCPWM_Type const *base, uint32_t cntNum);


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_Enable
****************************************************************************//**
*
* Enables the counter in the TCPWM block for the Counter operation.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_Init
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_Counter_Enable(TCPWM_Type *base, uint32_t cntNum)
{
    TCPWM_CTRL_SET(base) = (1UL << cntNum);
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_Disable
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
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_DeInit
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_Counter_Disable(TCPWM_Type *base, uint32_t cntNum)
{
    TCPWM_CTRL_CLR(base) = (1UL << cntNum);
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_GetStatus
****************************************************************************//**
*
* Returns the status of the Counter Timer.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return
* The status. See \ref group_tcpwm_counter_status
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_GetStatus
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetStatus(TCPWM_Type const  *base, uint32_t cntNum)
{
    uint32_t status = TCPWM_CNT_STATUS(base, cntNum);
    
    /* Generates proper up counting status. Is not generated by HW */
    status &= ~CY_TCPWM_COUNTER_STATUS_UP_COUNTING;
    status |= ((~status & CY_TCPWM_COUNTER_STATUS_DOWN_COUNTING & (status >> TCPWM_CNT_STATUS_RUNNING_Pos)) << 
               CY_TCPWM_CNT_STATUS_UP_POS);

    return(status);
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_GetCapture
****************************************************************************//**
*
* Returns the capture value when the capture mode is enabled.
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
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_Capture
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetCapture(TCPWM_Type const  *base, uint32_t cntNum)
{
    return(TCPWM_CNT_CC(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_GetCaptureBuf
****************************************************************************//**
*
* Returns the buffered capture value when the capture mode is enabled.
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
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_Capture
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetCaptureBuf(TCPWM_Type const  *base, uint32_t cntNum)
{
    return(TCPWM_CNT_CC_BUFF(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_SetCompare0
****************************************************************************//**
*
* Sets the compare value for Compare0 when the compare mode is enabled.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \param compare0
* The Compare0 value.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_SetCompare0
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_Counter_SetCompare0(TCPWM_Type *base, uint32_t cntNum,  uint32_t compare0)
{
    TCPWM_CNT_CC(base, cntNum) = compare0;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_GetCompare0
****************************************************************************//**
*
* Returns compare value 0.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return
* Compare value 0.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_SetCompare0
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetCompare0(TCPWM_Type const  *base, uint32_t cntNum)
{
    return(TCPWM_CNT_CC(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_SetCompare1
****************************************************************************//**
*
* Sets the compare value for Compare1 when the compare mode is enabled.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \param compare1
* The Compare1 value.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_SetCompare1
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_Counter_SetCompare1(TCPWM_Type *base, uint32_t cntNum,  uint32_t compare1)
{
    TCPWM_CNT_CC_BUFF(base, cntNum) = compare1;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_GetCompare1
****************************************************************************//**
*
* Returns compare value 1.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return
* Compare value 1.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_SetCompare1
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetCompare1(TCPWM_Type const  *base, uint32_t cntNum)
{
    return(TCPWM_CNT_CC_BUFF(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_EnableCompareSwap
****************************************************************************//**
*
* Enables the comparison swap when the comparison value is true.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \param enable
* true = swap enabled, false = swap disabled
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_EnableCompareSwap
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_Counter_EnableCompareSwap(TCPWM_Type *base, uint32_t cntNum,  bool enable)
{
    if (enable)
    {
        TCPWM_CNT_CTRL(base, cntNum) |=  TCPWM_CNT_CTRL_AUTO_RELOAD_CC_Msk;
    }
    else
    {
        TCPWM_CNT_CTRL(base, cntNum) &= ~TCPWM_CNT_CTRL_AUTO_RELOAD_CC_Msk;
    }
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_SetCounter
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
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_SetCounter
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_Counter_SetCounter(TCPWM_Type *base, uint32_t cntNum, uint32_t count)
{
    TCPWM_CNT_COUNTER(base, cntNum) = count;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_GetCounter
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
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_GetCounter
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetCounter(TCPWM_Type const  *base, uint32_t cntNum)
{
    return(TCPWM_CNT_COUNTER(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_SetPeriod
****************************************************************************//**
*
* Sets the value of the period register.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \param period
* The value to write into a period.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_SetPeriod
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_Counter_SetPeriod(TCPWM_Type *base, uint32_t cntNum,  uint32_t period)
{
    TCPWM_CNT_PERIOD(base, cntNum) = period;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Counter_GetPeriod
****************************************************************************//**
*
* Returns the value in the period register.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return
* The current period value.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_SetPeriod
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_Counter_GetPeriod(TCPWM_Type const *base, uint32_t cntNum)
{
    return(TCPWM_CNT_PERIOD(base, cntNum));
}
/** \} group_tcpwm_functions_counter */

/** \} group_tcpwm_counter */

#if defined(__cplusplus)
}
#endif

#endif /* CY_TCPWM_COUNTER_H */


/* [] END OF FILE */
