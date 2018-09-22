/***************************************************************************//**
* \file cy_tcpwm.h
* \version 1.10
*
* The header file of the TCPWM driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_tcpwm Timer Counter PWM (TCPWM)
* \{
* \defgroup group_tcpwm_common  Common
* \defgroup group_tcpwm_counter Timer/Counter (TCPWM)
* \defgroup group_tcpwm_pwm     PWM (TCPWM)
* \defgroup group_tcpwm_quaddec Quadrature Decoder (TCPWM)
* \} */

/**
* \addtogroup group_tcpwm
* \{
* 
* The TCPWM driver is a multifunction driver that implements Timer Counter, 
* PWM, and Quadrature Decoder functionality using the TCPWM block.
*
* Each TCPWM block is a collection of counters that can all be triggered 
* simultaneously. For each function call, the base register address of 
* the TCPWM being used must be passed first, followed by the index of 
* the counter you want to touch next. 
* For some functions, you can manage multiple counters simultaneously. You 
* provide a bit field representing each counter, rather than the single counter 
* index). 
*
* The TCPWM supports three operating modes:
* * Timer/Counter
* * PWM
* * Quadrature Decoder
*
* \n
* \b Timer/Counter
*
* Use this mode whenever a specific timing interval or measurement is 
* needed. Examples include:
* * Creating a periodic interrupt for running other system tasks
* * Measuring frequency of an input signal
* * Measuring pulse width of an input signal
* * Measuring time between two external events
* * Counting events
* * Triggering other system resources after x number events
* * Capturing time stamps when events occur
*
* The Timer/Counter has the following features:
* * 16- or 32-bit Timer/Counter
* * Programmable Period Register
* * Programmable Compare Register. Compare value can be swapped with a 
* buffered compare value on comparison event
* * Capture with buffer register
* * Count Up, Count Down, or Count Up and Down Counting modes
* * Continuous or One Shot Run modes
* * Interrupt and Output on Overflow, Underflow, Capture, or Compare 
* * Start, Reload, Stop, Capture, and Count Inputs
*
* \n
* \b PWM
*
* Use this mode when an output square wave is needed with a specific 
* period and duty cycle, such as:
* * Creating arbitrary square wave outputs
* * Driving an LED (changing the brightness)
* * Driving Motors (dead time assertion available)
*
* The PWM has the following features:
* * 16- or 32-bit Counter
* * Two Programmable Period registers that can be swapped
* * Two Output Compare registers that can be swapped on overflow and/or 
* underflow
* * Left Aligned, Right Aligned, Center Aligned, and Asymmetric Aligned modes
* * Continuous or One Shot run modes
* * Pseudo Random mode
* * Two PWM outputs with Dead Time insertion, and programmable polarity
* * Interrupt and Output on Overflow, Underflow, or Compare 
* * Start, Reload, Stop, Swap (Capture), and Count Inputs
* * Multiple Components can be synchronized together for applications 
* such as three phase motor control
*
* \n
* \b Quadrature \b Decoder
*
* A quadrature decoder is used to decode the output of a quadrature encoder. 
* A quadrature encoder senses the position, velocity, and direction of 
* an object (for example a rotating axle, or a spinning mouse ball). 
* A quadrature decoder can also be used for precision measurement of speed, 
* acceleration, and position of a motor's rotor, or with a rotary switch to 
* determine user input. \n
* 
* The Quadrature Decoder has the following features:
* * 16- or 32-bit Counter
* * Counter Resolution of x1, x2, and x4 the frequency of the phiA (Count) and 
* phiB (Start) inputs
* * Index Input to determine absolute position
* * A positive edge on phiA increments the counter when phiB is 0 and decrements
* the counter when phiB is 1
*
* \section group_tcpwm_configuration Configuration Considerations
*
* For each mode, the TCPWM driver has a configuration structure, an Init 
* function, and an Enable function. 
*
* Provide the configuration parameters in the appropriate structure (see
* Counter \ref group_tcpwm_data_structures_counter, PWM 
* \ref group_tcpwm_data_structures_pwm, or QuadDec 
* \ref group_tcpwm_data_structures_quaddec). 
* Then call the appropriate Init function: 
* \ref Cy_TCPWM_Counter_Init, \ref Cy_TCPWM_PWM_Init, or 
* \ref Cy_TCPWM_QuadDec_Init. Provide the address of the filled structure as a 
* parameter. To enable the counter, call the appropriate Enable function: 
* \ref Cy_TCPWM_Counter_Enable, \ref Cy_TCPWM_PWM_Enable, or 
* \ref Cy_TCPWM_QuadDec_Enable).
*
* Many functions work with an individual counter. You can also manage multiple 
* counters simultaneously for certain functions. These are listed in the 
* \ref group_tcpwm_functions_common 
* section of the TCPWM. You can enable, disable, or trigger (in various ways) 
* multiple counters simultaneously. For these functions you provide a bit field 
* representing each counter in the TCPWM you want to control. You can 
* represent the bit field as an ORed mask of each counter, like 
* ((1U << cntNumX) | (1U << cntNumX) | (1U << cntNumX)), where X is the counter 
* number from 0 to 31.
*
* \note
* * If none of the input terminals (start, reload(index)) are used, the 
* software event \ref Cy_TCPWM_TriggerStart or 
* \ref Cy_TCPWM_TriggerReloadOrIndex must be called to start the counting.
* * If count input terminal is not used, the \ref CY_TCPWM_INPUT_LEVEL macro 
* should be set for the countInputMode parameter and the \ref CY_TCPWM_INPUT_1 
* macro should be set for the countInput parameter in the configuration 
* structure of the appropriate mode(Counter 
* \ref group_tcpwm_data_structures_counter, PWM 
* \ref group_tcpwm_data_structures_pwm, or QuadDec 
* \ref group_tcpwm_data_structures_quaddec).
*
* \section group_tcpwm_more_information More Information
*
* For more information on the TCPWM peripheral, refer to the technical 
* reference manual (TRM).
*
* \section group_tcpwm_MISRA MISRA-C Compliance 
* <table class="doxtable">
*   <tr>
*     <th>MISRA Rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>14.2</td>
*     <td>R</td>
*     <td>All non-null statements shall either: a) have at least one side-effect
*         however executed, or b) cause control flow to change.</td>
*     <td>The unused function parameters are cast to void. This statement
*         has no side-effect and is used to suppress a compiler warning.</td>
*   </tr>
* </table>
*
* \section group_tcpwm_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.10</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td>1.0.1</td>
*     <td>Added a deviation to the MISRA Compliance section.
*         Added function-level code snippets.</td>
*     <td>Documentation update and clarification</td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*/

/** \} group_tcpwm */

/**
* \addtogroup group_tcpwm_common
* Common API for the Timer Counter PWM Block.
* \{
* \defgroup group_tcpwm_macros_common           Macros
* \defgroup group_tcpwm_functions_common        Functions
* \defgroup group_tcpwm_data_structures_common  Data Structures
* \defgroup group_tcpwm_enums Enumerated Types
*/


#if !defined(CY_TCPWM_H)
#define CY_TCPWM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cy_syslib.h"
#include "cy_device_headers.h"
#include "cy_device.h"

#ifdef CY_IP_MXTCPWM

#if defined(__cplusplus)
extern "C" {
#endif

/**
* \addtogroup group_tcpwm_macros_common
* \{
*/

/** Driver major version */
#define CY_TCPWM_DRV_VERSION_MAJOR       1

/** Driver minor version */
#define CY_TCPWM_DRV_VERSION_MINOR       10


/******************************************************************************
* API Constants
******************************************************************************/

/** TCPWM driver identifier */
#define CY_TCPWM_ID                     (CY_PDL_DRV_ID(0x2DU))

/** \defgroup group_tcpwm_input_selection  TCPWM Input Selection
* \{
* Selects which input to use
*/
#define CY_TCPWM_INPUT_0                (0U)  /**< Input is tied to logic 0 */
#define CY_TCPWM_INPUT_1                (1U)  /**< Input is tied to logic 1 */
#define CY_TCPWM_INPUT_TRIG_0           (2U)  /**< Input is connected to the trigger input 0 */
#define CY_TCPWM_INPUT_TRIG_1           (3U)  /**< Input is connected to the trigger input 1 */
#define CY_TCPWM_INPUT_TRIG_2           (4U)  /**< Input is connected to the trigger input 2 */
#define CY_TCPWM_INPUT_TRIG_3           (5U)  /**< Input is connected to the trigger input 3 */
#define CY_TCPWM_INPUT_TRIG_4           (6U)  /**< Input is connected to the trigger input 4 */
#define CY_TCPWM_INPUT_TRIG_5           (7U)  /**< Input is connected to the trigger input 5 */
#define CY_TCPWM_INPUT_TRIG_6           (8U)  /**< Input is connected to the trigger input 6 */
#define CY_TCPWM_INPUT_TRIG_7           (9U)  /**< Input is connected to the trigger input 7 */
#define CY_TCPWM_INPUT_TRIG_8           (10U) /**< Input is connected to the trigger input 8 */
#define CY_TCPWM_INPUT_TRIG_9           (11U) /**< Input is connected to the trigger input 9 */
#define CY_TCPWM_INPUT_TRIG_10          (12U) /**< Input is connected to the trigger input 10 */
#define CY_TCPWM_INPUT_TRIG_11          (13U) /**< Input is connected to the trigger input 11 */
#define CY_TCPWM_INPUT_TRIG_12          (14U) /**< Input is connected to the trigger input 12 */
#define CY_TCPWM_INPUT_TRIG_13          (15U) /**< Input is connected to the trigger input 13 */

/** Input is defined by Creator, and Init() function does not need to configure input */
#define CY_TCPWM_INPUT_CREATOR          (0xFFFFFFFFU)
/** \} group_tcpwm_input_selection */

/**
* \defgroup group_tcpwm_input_modes  Input Modes
* \{
* Configures how TCPWM inputs behave
*/
/** A rising edge triggers the event (Capture, Start, Reload, etc..) */
#define CY_TCPWM_INPUT_RISINGEDGE       (0U)
/** A falling edge triggers the event (Capture, Start, Reload, etc..) */
#define CY_TCPWM_INPUT_FALLINGEDGE      (1U)
/** A rising edge or falling edge triggers the event (Capture, Start, Reload, etc..) */
#define CY_TCPWM_INPUT_EITHEREDGE       (2U)
/** The event is triggered on each edge of the TCPWM clock if the input is high */
#define CY_TCPWM_INPUT_LEVEL            (3U)
/** \} group_tcpwm_input_modes */

/**
* \defgroup group_tcpwm_interrupt_sources  Interrupt Sources
* \{
* Interrupt Sources 
*/
#define CY_TCPWM_INT_ON_TC              (1U) /**< Interrupt on Terminal count(TC) */
#define CY_TCPWM_INT_ON_CC              (2U) /**< Interrupt on Compare/Capture(CC) */
#define CY_TCPWM_INT_NONE               (0U) /**< No Interrupt */
#define CY_TCPWM_INT_ON_CC_OR_TC        (3U) /**< Interrupt on TC or CC */
/** \} group_tcpwm_interrupt_sources */


/***************************************
*        Registers Constants
***************************************/

/**
* \defgroup group_tcpwm_reg_const Default registers constants
* \{
* Default constants for CNT Registers
*/
#define CY_TCPWM_CNT_CTRL_DEFAULT           (0x0U) /**< Default value for CTRL register */
#define CY_TCPWM_CNT_COUNTER_DEFAULT        (0x0U) /**< Default value for COUNTER register */
#define CY_TCPWM_CNT_CC_DEFAULT             (0xFFFFFFFFU) /**< Default value for CC register */
#define CY_TCPWM_CNT_CC_BUFF_DEFAULT        (0xFFFFFFFFU) /**< Default value for CC_BUFF register */
#define CY_TCPWM_CNT_PERIOD_DEFAULT         (0xFFFFFFFFU) /**< Default value for PERIOD register */
#define CY_TCPWM_CNT_PERIOD_BUFF_DEFAULT    (0xFFFFFFFFU) /**< Default value for PERIOD_BUFF register */
#define CY_TCPWM_CNT_TR_CTRL0_DEFAULT       (0x10U)  /**< Default value for TR_CTRL0 register */
#define CY_TCPWM_CNT_TR_CTRL1_DEFAULT       (0x3FFU) /**< Default value for TR_CTRL1 register */
#define CY_TCPWM_CNT_TR_CTRL2_DEFAULT       (0x3FU)  /**< Default value for TR_CTRL2 register */
#define CY_TCPWM_CNT_INTR_DEFAULT           (0x3U)   /**< Default value for INTR register */
#define CY_TCPWM_CNT_INTR_SET_DEFAULT       (0x0U)   /**< Default value for INTR_SET register */
#define CY_TCPWM_CNT_INTR_MASK_DEFAULT      (0x0U)   /**< Default value for INTR_MASK register */
/** \} group_tcpwm_reg_const */

/** Position of Up counting counter status */
#define CY_TCPWM_CNT_STATUS_UP_POS          (0x1U)
/** Initial value for the counter in the Up counting mode */
#define CY_TCPWM_CNT_UP_INIT_VAL            (0x0U)
/** Initial value for the counter in the Up/Down counting modes */
#define CY_TCPWM_CNT_UP_DOWN_INIT_VAL       (0x1U)
/** \} group_tcpwm_macros_common */


/*******************************************************************************
 * Enumerations
 ******************************************************************************/

 /**
* \addtogroup group_tcpwm_enums
* \{
*/

/** TCPWM status definitions */
typedef enum 
{
    CY_TCPWM_SUCCESS = 0x00U,                                           /**< Successful */
    CY_TCPWM_BAD_PARAM = CY_TCPWM_ID | CY_PDL_STATUS_ERROR | 0x01U,     /**< One or more invalid parameters */
} cy_en_tcpwm_status_t;
/** \} group_tcpwm_enums */
 
/*******************************************************************************
*        Function Prototypes
*******************************************************************************/

/**
* \addtogroup group_tcpwm_functions_common
* \{
*/

__STATIC_INLINE void Cy_TCPWM_Enable_Multiple(TCPWM_Type *base, uint32_t counters);
__STATIC_INLINE void Cy_TCPWM_Disable_Multiple(TCPWM_Type *base, uint32_t counters);
__STATIC_INLINE void Cy_TCPWM_TriggerStart(TCPWM_Type *base, uint32_t counters);
__STATIC_INLINE void Cy_TCPWM_TriggerReloadOrIndex(TCPWM_Type *base, uint32_t counters);
__STATIC_INLINE void Cy_TCPWM_TriggerStopOrKill(TCPWM_Type *base, uint32_t counters);
__STATIC_INLINE void Cy_TCPWM_TriggerCaptureOrSwap(TCPWM_Type *base, uint32_t counters);
__STATIC_INLINE uint32_t Cy_TCPWM_GetInterruptStatus(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE void Cy_TCPWM_ClearInterrupt(TCPWM_Type *base, uint32_t cntNum, uint32_t source);
__STATIC_INLINE void Cy_TCPWM_SetInterrupt(TCPWM_Type *base, uint32_t cntNum, uint32_t source);
__STATIC_INLINE void Cy_TCPWM_SetInterruptMask(TCPWM_Type *base, uint32_t cntNum, uint32_t mask);
__STATIC_INLINE uint32_t Cy_TCPWM_GetInterruptMask(TCPWM_Type const *base, uint32_t cntNum);
__STATIC_INLINE uint32_t Cy_TCPWM_GetInterruptStatusMasked(TCPWM_Type const *base, uint32_t cntNum);


/*******************************************************************************
* Function Name: Cy_TCPWM_Enable_Multiple
****************************************************************************//**
*
* Enables the counter(s) in the TCPWM block. Multiple blocks can be started 
* simultaneously.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param counters
* A bit field representing each counter in the TCPWM block.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Enable_Multiple
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_Enable_Multiple(TCPWM_Type *base, uint32_t counters)
{
    TCPWM_CTRL_SET(base) = counters;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_Disable_Multiple
****************************************************************************//**
*
* Disables the counter(s) in the TCPWM block. Multiple TCPWM can be disabled 
* simultaneously.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param counters
* A bit field representing each counter in the TCPWM block.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Disable_Multiple
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_Disable_Multiple(TCPWM_Type *base, uint32_t counters)
{
    TCPWM_CTRL_CLR(base) = counters;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_TriggerStart
****************************************************************************//**
*
* Triggers a software start on the selected TCPWMs.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param counters
* A bit field representing each counter in the TCPWM block.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Enable_Multiple
*
*******************************************************************************/
__STATIC_INLINE  void Cy_TCPWM_TriggerStart(TCPWM_Type *base, uint32_t counters)
{
    TCPWM_CMD_START(base) = counters;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_TriggerReloadOrIndex
****************************************************************************//**
*
* Triggers a software reload event (or index in QuadDec mode).
*
* \param base
* The pointer to a TCPWM instance
*
* \param counters
* A bit field representing each counter in the TCPWM block.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_TriggerReloadOrIndex
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_TriggerReloadOrIndex(TCPWM_Type *base, uint32_t counters)
{
    TCPWM_CMD_RELOAD(base) = counters;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_TriggerStopOrKill
****************************************************************************//**
*
* Triggers a stop in the Timer Counter mode, or a kill in the PWM mode.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param counters
* A bit field representing each counter in the TCPWM block.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_TriggerStopOrKill
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_TriggerStopOrKill(TCPWM_Type *base, uint32_t counters)
{
    TCPWM_CMD_STOP(base) = counters;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_TriggerCaptureOrSwap
****************************************************************************//**
*
* Triggers a Capture in the Timer Counter mode, and a Swap in the PWM mode.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param counters
* A bit field representing each counter in the TCPWM block.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_Counter_Capture
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_TriggerCaptureOrSwap(TCPWM_Type *base, uint32_t counters)
{
    TCPWM_CMD_CAPTURE(base) = counters;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_GetInterruptStatus
****************************************************************************//**
*
* Returns which event triggered the interrupt.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return 
*. See \ref group_tcpwm_interrupt_sources
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_GetInterruptStatus
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_GetInterruptStatus(TCPWM_Type const *base, uint32_t cntNum)
{
    return(TCPWM_CNT_INTR(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_ClearInterrupt
****************************************************************************//**
*
* Clears Active Interrupt Source
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \param source
* source to clear. See \ref group_tcpwm_interrupt_sources
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_GetInterruptStatusMasked
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_ClearInterrupt(TCPWM_Type *base, uint32_t cntNum,  uint32_t source)
{
    TCPWM_CNT_INTR(base, cntNum) = source;
    (void)TCPWM_CNT_INTR(base, cntNum);
}


/*******************************************************************************
* Function Name: Cy_TCPWM_SetInterrupt
****************************************************************************//**
*
* Triggers an interrupt via a software write.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \param source
* The source to set an interrupt. See \ref group_tcpwm_interrupt_sources.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_SetInterrupt
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_SetInterrupt(TCPWM_Type *base, uint32_t cntNum,  uint32_t source)
{
    TCPWM_CNT_INTR_SET(base, cntNum) = source;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_SetInterruptMask
****************************************************************************//**
*
* Sets an interrupt mask. A 1 means that when the event occurs, it will cause an 
* interrupt; a 0 means no interrupt will be triggered.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \param mask
*. See \ref group_tcpwm_interrupt_sources
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_SetInterruptMask
*
*******************************************************************************/
__STATIC_INLINE void Cy_TCPWM_SetInterruptMask(TCPWM_Type *base, uint32_t cntNum, uint32_t mask)
{
    TCPWM_CNT_INTR_MASK(base, cntNum) = mask;
}


/*******************************************************************************
* Function Name: Cy_TCPWM_GetInterruptMask
****************************************************************************//**
*
* Returns the interrupt mask.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return 
* Interrupt Mask. See \ref group_tcpwm_interrupt_sources
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_SetInterruptMask
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_GetInterruptMask(TCPWM_Type const *base, uint32_t cntNum)
{
    return(TCPWM_CNT_INTR_MASK(base, cntNum));
}


/*******************************************************************************
* Function Name: Cy_TCPWM_GetInterruptStatusMasked
****************************************************************************//**
*
* Returns which masked interrupt triggered the interrupt.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum 
* The Counter instance number in the selected TCPWM.
*
* \return 
* Interrupt Mask. See \ref group_tcpwm_interrupt_sources
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_counter_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_GetInterruptStatusMasked
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_TCPWM_GetInterruptStatusMasked(TCPWM_Type const *base, uint32_t cntNum)
{
    return(TCPWM_CNT_INTR_MASKED(base, cntNum));
}

/** \} group_tcpwm_functions_common */

/** \} group_tcpwm_common */

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXTCPWM */

#endif /* CY_TCPWM_H */

/* [] END OF FILE */
