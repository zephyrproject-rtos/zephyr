/***************************************************************************//**
* \file cy_tcpwm_pwm.h
* \version 1.10.1
*
* \brief
* The header file of the TCPWM PWM driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#if !defined(CY_TCPWM_PWM_H)
#define CY_TCPWM_PWM_H

#include "cy_tcpwm.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
* \addtogroup group_tcpwm_pwm
* Driver API for PWM.
* \{
*/

/**
* \defgroup group_tcpwm_macros_pwm          Macros
* \defgroup group_tcpwm_functions_pwm       Functions
* \defgroup group_tcpwm_data_structures_pwm Data Structures
* \} */

/**
* \addtogroup group_tcpwm_data_structures_pwm
* \{
*/

/** PWM configuration structure */
typedef struct cy_stc_tcpwm_pwm_config
{
    uint32_t    pwmMode;            /**< Sets the PWM mode. See \ref group_tcpwm_pwm_modes */
    /** Sets the clock prescaler inside the TCWPM block. See \ref group_tcpwm_pwm_clk_prescalers */
    uint32_t     clockPrescaler;
    uint32_t    pwmAlignment;       /**< Sets the PWM alignment. See \ref group_tcpwm_pwm_alignment */
    uint32_t    deadTimeClocks;     /**< The number of dead time-clocks if PWM with dead time is chosen */
    uint32_t    runMode;            /**< Sets the PWM run mode. See \ref group_tcpwm_pwm_run_modes */
    uint32_t    period0;            /**< Sets the period0 of the pwm */
    uint32_t    period1;            /**< Sets the period1 of the pwm */
    bool        enablePeriodSwap;   /**< Enables swapping of period 0 and period 1 on terminal count */
    uint32_t    compare0;           /**< Sets the value for Compare0 */
    uint32_t    compare1;           /**< Sets the value for Compare1 */
    bool        enableCompareSwap;  /**< If enabled, the compare values are swapped on the terminal count */
    /** Enables an interrupt on the terminal count, capture or compare. See \ref group_tcpwm_interrupt_sources */
    uint32_t    interruptSources;
    uint32_t    invertPWMOut;       /**< Inverts the PWM output */
    uint32_t    invertPWMOutN;      /**< Inverts the PWM_n output */
    uint32_t    killMode;           /**< Configures the PWM kill modes. See \ref group_tcpwm_pwm_kill_modes */
    uint32_t    swapInputMode;      /**< Configures how the swap input behaves. See \ref group_tcpwm_input_modes */
    /** Selects which input the swap uses. Inputs are device-specific. See \ref group_tcpwm_input_selection */    
    uint32_t    swapInput;
    uint32_t    reloadInputMode;    /**< Configures how the reload input behaves. See \ref group_tcpwm_input_modes */
    /** Selects which input the reload uses. The inputs are device-specific. See \ref group_tcpwm_input_selection */
    uint32_t    reloadInput;            
    uint32_t    startInputMode;     /**< Configures how the start input behaves. See \ref group_tcpwm_input_modes */
    /** Selects which input the start uses. The inputs are device-specific. See \ref group_tcpwm_input_selection */
    uint32_t    startInput;                
    uint32_t    killInputMode;      /**< Configures how the kill input behaves. See \ref group_tcpwm_input_modes */
    /** Selects which input the kill uses. The inputs are device-specific. See \ref group_tcpwm_input_selection */
    uint32_t    killInput;
    uint32_t    countInputMode;     /**< Configures how the count input behaves. See \ref group_tcpwm_input_modes */
    /** Selects which input the count uses. The inputs are device-specific. See \ref group_tcpwm_input_selection */
    uint32_t    countInput;
}cy_stc_tcpwm_pwm_config_t;
/** \} group_tcpwm_data_structures_pwm */

/**
* \addtogroup group_tcpwm_macros_pwm
* \{
* \defgroup group_tcpwm_pwm_run_modes PWM run modes
* \{
* Run modes for the pwm timer.
*/
#define CY_TCPWM_PWM_ONESHOT            (1U)    /**< Counter runs once and then stops */
#define CY_TCPWM_PWM_CONTINUOUS         (0U)    /**< Counter runs forever */
/** \} group_tcpwm_pwm_run_modes */

/** \defgroup group_tcpwm_pwm_modes PWM modes 
* \{
* Sets the PWM modes.
*/
#define CY_TCPWM_PWM_MODE_PWM           (4U) /**< Standard PWM Mode*/
#define CY_TCPWM_PWM_MODE_DEADTIME      (5U)    /**< PWM with deadtime mode*/
#define CY_TCPWM_PWM_MODE_PSEUDORANDOM  (6U)    /**< Pseudo Random PWM */
/** \} group_tcpwm_pwm_modes */

/** \defgroup group_tcpwm_pwm_alignment PWM Alignment
* Sets the alignment of the PWM.
* \{
*/
#define CY_TCPWM_PWM_LEFT_ALIGN         (0U)     /**< PWM is left aligned, meaning it starts high */
#define CY_TCPWM_PWM_RIGHT_ALIGN        (1U)        /**< PWM is right aligned, meaning it starts low */
/** PWM is centered aligned, terminal count only occurs on underflow */
#define CY_TCPWM_PWM_CENTER_ALIGN       (2U)
/** PWM is asymmetrically aligned, terminal count occurs on overflow and underflow */
#define CY_TCPWM_PWM_ASYMMETRIC_ALIGN   (3U)
/** \} group_tcpwm_pwm_alignment */

/** \defgroup group_tcpwm_pwm_kill_modes PWM kill modes
* Sets the kill mode for the PWM.
* \{
*/
#define CY_TCPWM_PWM_STOP_ON_KILL       (2U)    /**< PWM stops counting on kill */
#define CY_TCPWM_PWM_SYNCH_KILL         (1U)    /**< PWM output is killed after next TC*/
#define CY_TCPWM_PWM_ASYNC_KILL         (0U)    /**< PWM output is killed instantly */
/** \} group_tcpwm_pwm_kill_modes */

/** \defgroup group_tcpwm_pwm_clk_prescalers PWM CLK Prescaler values
* \{
* Clock prescaler values.
*/
#define CY_TCPWM_PWM_PRESCALER_DIVBY_1      (0U) /**< Divide by 1 */
#define CY_TCPWM_PWM_PRESCALER_DIVBY_2      (1U) /**< Divide by 2 */
#define CY_TCPWM_PWM_PRESCALER_DIVBY_4      (2U) /**< Divide by 4 */
#define CY_TCPWM_PWM_PRESCALER_DIVBY_8      (3U) /**< Divide by 8 */
#define CY_TCPWM_PWM_PRESCALER_DIVBY_16     (4U) /**< Divide by 16 */
#define CY_TCPWM_PWM_PRESCALER_DIVBY_32     (5U) /**< Divide by 32 */
#define CY_TCPWM_PWM_PRESCALER_DIVBY_64     (6U) /**< Divide by 64 */
#define CY_TCPWM_PWM_PRESCALER_DIVBY_128    (7U) /**< Divide by 128 */
/** \} group_tcpwm_pwm_clk_prescalers */

/** \defgroup group_tcpwm_pwm_output_invert PWM output invert
* \{
* Output invert modes.
*/
#define CY_TCPWM_PWM_INVERT_ENABLE          (1U)  /**< Invert the output mode */
#define CY_TCPWM_PWM_INVERT_DISABLE         (0U)  /**< Do not invert the output mode */
/** \} group_tcpwm_pwm_output_invert */

/** \defgroup group_tcpwm_pwm_status PWM Status
* \{
* The counter status.
*/
#define CY_TCPWM_PWM_STATUS_DOWN_COUNTING   (0x1UL)        /**< PWM is down counting */
#define CY_TCPWM_PWM_STATUS_UP_COUNTING     (0x2UL)        /**< PWM is up counting */
#define CY_TCPWM_PWM_STATUS_COUNTER_RUNNING (TCPWM_CNT_STATUS_RUNNING_Msk)     /**< PWM counter is running */
/** \} group_tcpwm_pwm_status */


/***************************************
*        Registers Constants
***************************************/

/** \cond INTERNAL */
#define CY_TCPWM_PWM_CTRL_SYNC_KILL_OR_STOP_ON_KILL_POS  (2U)
#define CY_TCPWM_PWM_CTRL_SYNC_KILL_OR_STOP_ON_KILL_MASK (0x3UL << CY_TCPWM_PWM_CTRL_SYNC_KILL_OR_STOP_ON_KILL_POS)
/** \endcond */

/** \defgroup group_tcpwm_pwm_output_configuration PWM output signal configuration
* \{
* The configuration of PWM output signal for PWM alignment.
*/
#define CY_TCPWM_PWM_TR_CTRL2_SET           (0UL) /**< Set define for PWM output signal configuration */
#define CY_TCPWM_PWM_TR_CTRL2_CLEAR         (1UL) /**< Clear define for PWM output signal configuration */
#define CY_TCPWM_PWM_TR_CTRL2_INVERT        (2UL) /**< Invert define for PWM output signal configuration */
#define CY_TCPWM_PWM_TR_CTRL2_NO_CHANGE     (3UL) /**< No change define for PWM output signal configuration */

/** The configuration of PWM output signal in Pseudo Random Mode */
#define CY_TCPWM_PWM_MODE_PR         (_VAL2FLD(TCPWM_CNT_TR_CTRL2_CC_MATCH_MODE, CY_TCPWM_PWM_TR_CTRL2_NO_CHANGE) | \
                                      _VAL2FLD(TCPWM_CNT_TR_CTRL2_OVERFLOW_MODE, CY_TCPWM_PWM_TR_CTRL2_NO_CHANGE) | \
                                      _VAL2FLD(TCPWM_CNT_TR_CTRL2_UNDERFLOW_MODE, CY_TCPWM_PWM_TR_CTRL2_NO_CHANGE))

/** The configuration of PWM output signal for Left alignment */
#define CY_TCPWM_PWM_MODE_LEFT       (_VAL2FLD(TCPWM_CNT_TR_CTRL2_CC_MATCH_MODE, CY_TCPWM_PWM_TR_CTRL2_CLEAR) | \
                                      _VAL2FLD(TCPWM_CNT_TR_CTRL2_OVERFLOW_MODE, CY_TCPWM_PWM_TR_CTRL2_SET) | \
                                      _VAL2FLD(TCPWM_CNT_TR_CTRL2_UNDERFLOW_MODE, CY_TCPWM_PWM_TR_CTRL2_NO_CHANGE))

/** The configuration of PWM output signal for Right alignment */
#define CY_TCPWM_PWM_MODE_RIGHT      (_VAL2FLD(TCPWM_CNT_TR_CTRL2_CC_MATCH_MODE, CY_TCPWM_PWM_TR_CTRL2_SET) | \
                                      _VAL2FLD(TCPWM_CNT_TR_CTRL2_OVERFLOW_MODE, CY_TCPWM_PWM_TR_CTRL2_NO_CHANGE) | \
                                      _VAL2FLD(TCPWM_CNT_TR_CTRL2_UNDERFLOW_MODE, CY_TCPWM_PWM_TR_CTRL2_CLEAR))

/** The configuration of PWM output signal for Center and Asymmetric alignment */
#define CY_TCPWM_PWM_MODE_CNTR_OR_ASYMM (_VAL2FLD(TCPWM_CNT_TR_CTRL2_CC_MATCH_MODE, CY_TCPWM_PWM_TR_CTRL2_INVERT) | \
                                         _VAL2FLD(TCPWM_CNT_TR_CTRL2_OVERFLOW_MODE, CY_TCPWM_PWM_TR_CTRL2_SET) | \
                                         _VAL2FLD(TCPWM_CNT_TR_CTRL2_UNDERFLOW_MODE, CY_TCPWM_PWM_TR_CTRL2_CLEAR))
/** \} group_tcpwm_pwm_output_configuration */
/** \} group_tcpwm_macros_pwm */


/*******************************************************************************
*        Function Prototypes
*******************************************************************************/

/**
* \addtogroup group_tcpwm_functions_pwm
* \{
*/

cy_en_tcpwm_status_t Cy_TCPWM_PWM_Init(TCPWM_Type *base, uint32_t cntNum, cy_stc_tcpwm_pwm_config_t const *config);
void Cy_TCPWM_PWM_DeInit(TCPWM_Type *base, uint32_t cntNum, cy_stc_tcpwm_pwm_config_t const *config);
__STATIC_INLINE void Cy_TCPWM_PWM_Enable(TCPWM_Type *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_PWM_Disable(TCPWM_Type *base, uint32_t cntNum);
__STATIC_INLINE uint32_t Cy_TCPWM_PWM_GetStatus(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_PWM_SetCompare0(TCPWM_Type *base, uint32_t cntNum, uint32_t compare0);
__STATIC_INLINE uint32_t Cy_TCPWM_PWM_GetCompare0(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_PWM_SetCompare1(TCPWM_Type *base, uint32_t cntNum, uint32_t compare1);
__STATIC_INLINE uint32_t Cy_TCPWM_PWM_GetCompare1(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_PWM_EnableCompareSwap(TCPWM_Type *base, uint32_t cntNum, bool enable);
__STATIC_INLINE void Cy_TCPWM_PWM_SetCounter(TCPWM_Type *base, uint32_t cntNum, uint32_t count);
__STATIC_INLINE uint32_t Cy_TCPWM_PWM_GetCounter(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_PWM_SetPeriod0(TCPWM_Type *base, uint32_t cntNum, uint32_t period0);
__STATIC_INLINE uint32_t Cy_TCPWM_PWM_GetPeriod0(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_PWM_SetPeriod1(TCPWM_Type *base, uint32_t cntNum, uint32_t period1);
__STATIC_INLINE uint32_t Cy_TCPWM_PWM_GetPeriod1(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_PWM_EnablePeriodSwap(TCPWM_Type *base, uint32_t cntNum, bool enable);


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_Enable
****************************************************************************//**
*
* Enables the counter in the TCPWM block for the PWM operation.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_Init
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_PWM_Enable(TCPWM_Type *base, uint32_t cntNum)
{
    TCPWM_CTRL_SET(base) = (1UL << cntNum);
}

/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_Disable
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
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_DeInit
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_PWM_Disable(TCPWM_Type *base, uint32_t cntNum)
{
    TCPWM_CTRL_CLR(base) = (1UL << cntNum);
}


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_GetStatus
****************************************************************************//**
*
* Returns the status of the PWM.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return
* The status. See \ref group_tcpwm_pwm_status
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_GetStatus
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_PWM_GetStatus(TCPWM_Type const *base, uint32_t cntNum)
{
    uint32_t status = TCPWM_CNT_STATUS(base, cntNum);
    
    /* Generates proper up counting status, does not generated by HW */
    status &= ~CY_TCPWM_PWM_STATUS_UP_COUNTING;
    status |= ((~status & CY_TCPWM_PWM_STATUS_DOWN_COUNTING & (status >> TCPWM_CNT_STATUS_RUNNING_Pos)) << 
               CY_TCPWM_CNT_STATUS_UP_POS);
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_SetCompare0
****************************************************************************//**
*
* Sets the compare value for Compare0 when the compare mode enabled.
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
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_SetCompare0
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_PWM_SetCompare0(TCPWM_Type *base, uint32_t cntNum,  uint32_t compare0)
{
    TCPWM_CNT_CC(base, cntNum) = compare0;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_GetCompare0
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
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_SetCompare0
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_PWM_GetCompare0(TCPWM_Type const *base, uint32_t cntNum)
{
    return(TCPWM_CNT_CC(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_SetCompare1
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
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_SetCompare1
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_PWM_SetCompare1(TCPWM_Type *base, uint32_t cntNum,  uint32_t compare1)
{
    TCPWM_CNT_CC_BUFF(base, cntNum) = compare1;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_GetCompare1
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
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_SetCompare1
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_PWM_GetCompare1(TCPWM_Type const *base, uint32_t cntNum)
{
    return(TCPWM_CNT_CC_BUFF(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_EnableCompareSwap
****************************************************************************//**
*
* Enables the comparison swap on OV and/or UN, depending on the PWM alignment.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \param enable
* true = swap enabled; false = swap disabled
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_EnableCompareSwap
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_PWM_EnableCompareSwap(TCPWM_Type *base, uint32_t cntNum,  bool enable)
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
* Function Name: Cy_TCPWM_PWM_SetCounter
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
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_SetCounter
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_PWM_SetCounter(TCPWM_Type *base, uint32_t cntNum,  uint32_t count)
{
    TCPWM_CNT_COUNTER(base, cntNum) = count;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_GetCounter
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
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_GetCounter
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_PWM_GetCounter(TCPWM_Type const *base, uint32_t cntNum)
{
    return(TCPWM_CNT_COUNTER(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_SetPeriod0
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
* \param period0
* The value to write into a period.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_SetPeriod0
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_PWM_SetPeriod0(TCPWM_Type *base, uint32_t cntNum,  uint32_t period0)
{
    TCPWM_CNT_PERIOD(base, cntNum) = period0;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_GetPeriod0
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
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_SetPeriod0
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_PWM_GetPeriod0(TCPWM_Type const *base, uint32_t cntNum)
{
    return(TCPWM_CNT_PERIOD(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_SetPeriod1
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
* \param period1
* The value to write into a period1.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_SetPeriod1
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_PWM_SetPeriod1(TCPWM_Type *base, uint32_t cntNum,  uint32_t period1)
{
    TCPWM_CNT_PERIOD_BUFF(base, cntNum) = period1;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_GetPeriod1
****************************************************************************//**
*
* Returns the value in the period register.
*
* \param base
* The pointer to a COUNTER PWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return
* The current period value.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_SetPeriod1
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_PWM_GetPeriod1(TCPWM_Type const *base, uint32_t cntNum)
{
    return(TCPWM_CNT_PERIOD_BUFF(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_PWM_EnablePeriodSwap
****************************************************************************//**
*
* Enables a period swap on OV and/or UN, depending on the PWM alignment
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \param enable
* true = swap enabled; false = swap disabled
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_pwm_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_PWM_EnablePeriodSwap
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_PWM_EnablePeriodSwap(TCPWM_Type *base, uint32_t cntNum,  bool enable)
{
    if (enable)
    {
        TCPWM_CNT_CTRL(base, cntNum) |=  TCPWM_CNT_CTRL_AUTO_RELOAD_PERIOD_Msk;
    }
    else
    {
        TCPWM_CNT_CTRL(base, cntNum) &= ~TCPWM_CNT_CTRL_AUTO_RELOAD_PERIOD_Msk;
    }    
}

/** \} group_tcpwm_functions_pwm */

/** \} group_tcpwm_pwm */

#if defined(__cplusplus)
}
#endif

#endif /* CY_TCPWM_PWM_H */


/* [] END OF FILE */
