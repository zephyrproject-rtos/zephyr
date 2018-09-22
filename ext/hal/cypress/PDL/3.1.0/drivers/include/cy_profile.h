/***************************************************************************//**
* \file cy_profile.h
* \version 1.10
*
* Provides an API declaration of the energy profiler driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_energy_profiler Energy Profiler (Profile)
* \{
* 
* The energy profiler driver is an API for configuring and using the profile 
* hardware block. The profile block enables measurement of the signal activity
* of select peripherals and monitor sources during a measurement window. Using
* these measurements, it is possible to construct a profile of the energy consumed
* in the device by scaling the individual peripheral actvities with appropriate
* scaling (weight) factors. This gives the application the ability to monitor
* the energy consumed by the internal resources with minimal CPU overhead and
* without external monitoring hardware.
*
* \section group_profile_details Details
*
* \subsection group_profile_hardware Profile Hardware
*
* The profile hardware consists of a number of profile counters that accept specific
* triggers for incrementing the count value. This allows the events of the source 
* (such as the number of SCB0 bus accesses or the duration of time the BLE RX radio
* is active) to be counted during the measurement window. The available monitor 
* sources in the device can be found in the en_ep_mon_sel_t enum in the device 
* configuration file (e.g. psoc62_config.h). These can be sourced to any of the
* profile counters as triggers. There are two methods of using the monitor sources
* in a profile counter.
*
* - Event: The count value is incremented when a pulse event signal is seen by the
*   counter. This type of monitoring is suitable when the monitoring source of 
*   interest needs to count the discrete events (such as the number of Flash read 
*   accesses) happening in the measurement window.
*
* - Duration: The count value is incremented at every clock edge while the monitor
*   signal is high. This type of monitoring is suitable when a signal is active for
*   a finite amount of time (such as the time the BLE TX radio is active) and the
*   duration needs to be expressed as number of clock cycles in the measurement window.
*
* Many of the available monitor sources are suitable for event type monitoring.
* Using a duration type on these signals may not give valuable information. Review 
* the device TRM for more information on the monitor sources and details on how they
* should be used.
*
* \subsection group_profile_measurement_types Measurement Types
*
* Depending on the item of interest, energy measurement can be performed by using
* the following methods.
*
* - Continuous measurement: A profile counter can be assigned a monitor signal of
*   constant 1 (PROFILE_ONE), which sets the counter to increment at every (assigned)
*   clock cycle. This can be used to give a reference time for the measurement window
*   and also allows the construction of time stamps. For example, a software controlled
*   GPIO can be "timestamped" by reading the counter value (on the fly) before it is
*   toggled. When the measurement window ends, the energy contribution caused by the
*   GPIO toggle can be incorporated into the final calculation.
*
* - Event measurement: Monitored events happening in a measurement window can be 
*   used to increment a profile counter. This gives the activity numbers, which can
*   then be multiplied by the instantaneous power numbers associated with the source
*   to give the average energy consumption (Energy = Power x time). For example, the 
*   energy consumped by an Operating System (OS) task can be estimated by monitoring
*   the processor's active cycle count (E.g. CPUSS_MONITOR_CM4) and the Flash read
*   accesses (CPUSS_MONITOR_FLASH). Note that these activity numbers can also be 
*   timestamped using the continuous measurement method to differentiate between the
*   different task switches. The activity numbers are then multiplied by the associated
*   processor and flash access power numbers to give the average energy consumed by
*   that task.
*
* - Duration measurement: A peripheral event such as the SMIF select signal can be 
*   used by a profile counter to measure the time spent on XIP communication through the 
*   SPI interface. This activity number can then be multiplied by the power associated
*   with that activity to give the average energy consumed by that block during the
*   measurement window. This type of monitoring should only be performed for signals
*   that are difficult to track in software. For example, a combination of interrupts
*   and time stamps can be used to track the activity of many peripherals in a continuous
*   monitoring model. However tracking the activity of signals such the BLE radio
*   should be done using the duration measurement method.
*
* - Low power measurement: The profile counters do not support measurement during chip 
*   deep-sleep, hibernate and off states. i.e. the profile counters are meant for active
*   run-time measurements only. In order to measure the time spent in low power modes (LPM),
*   a real-time clock (RTC) should be used. Take a timestamp before LPM entry and a 
*   timestamp upon LPM exit in a continuous measurement model. Then multiply the difference
*   by the appropriate LPM power numbers.
*
* \subsection group_profile_usage Driver Usage
*
* At the highest level, the energy profiler must perform the following steps to 
* obtain a measurement:
*
*  1. Initialize the profile hardware block.
*  2. Initialize the profile interrupt (profile_interrupt_IRQn).
*  3. Configure, initialize, and enable the profile counters.
*  4. Enable the profile interrupt and start the profiling/measurement window.
*  5. Perform run-time reads of the counters (if needed).
*  6. Disable the profile interrupt and stop the profiling/measurement window.
*  7. Read the counters and gather the results.
*  8. Calculate the energy consumption.
*
* Refer to the SysInt driver on the details of configuring the profile hardware interrupt.
*
* The profile interrupt triggers when a counter overflow event is detected on any of the
* enabled profile counters. A sample interrupt service routine Cy_Profile_ISR() is provided,
* which can be used to update the internal counter states stored in RAM. Refer to the
* Configuration Considerations for more information.
*
* \section group_profile_configuration Configuration Considerations
* 
* Each counter is a 32-bit register that counts either a number of clock cycles, 
* or a number of events. It is possible to overflow the 32-bit register. To address
* this issue, the driver implements a 32-bit overflow counter. Combined with the 32-bit
* register, this gives a 64-bit counter for each monitored source. 
*
* When an overflow occurs, the profile hardware generates an interrupt. The interrupt is 
* configured using the SysInt driver, where the sample interrupt handler Cy_Profile_ISR()
* can be used as the ISR. The ISR increments the overflow counter for each profiling counter 
* and clears the interrupt.
*
* \section group_profile_more_information More Information
*
* See the profiler chapter of the device technical reference manual (TRM).
*
* \section group_profile_MISRA MISRA-C Compliance
* <table class="doxtable">
*   <tr>
*     <th>MISRA Rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>12.4</td>
*     <td>R</td>
*     <td>Right hand operand of '&&' or '||' is an expression with possible side effects.</td>
*     <td>Function-like macros are used to achieve more efficient code.</td>
*   </tr>
*   <tr>
*     <td>16.7</td>
*     <td>A</td>
*     <td>A pointer parameter can be of type 'pointer to const'.</td>
*     <td>The pointer is cast for comparison purposes and thus can't be a const.</td>
*   </tr>
* </table>
*
* \section group_profile_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.10</td>
*     <td> <br>
*            * Added parameter check asserts
*            * Flattened the driver source code organization into the single
*              source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_profile_macros Macros
* \defgroup group_profile_functions Functions
* \{
*   \defgroup group_profile_functions_interrupt    Interrupt Functions
*   \defgroup group_profile_functions_general      General Functions
*   \defgroup group_profile_functions_counter      Counter Functions
*   \defgroup group_profile_functions_calculation  Calculation Functions
* \}
* \defgroup group_profile_data_structures Data Structures
* \defgroup group_profile_enums Enumerated Types
*/

#if !defined(CY_PROFILE_H)
#define CY_PROFILE_H

#include "cy_device_headers.h"
#include "cy_syslib.h"
#include <stddef.h>

#ifdef CY_IP_MXPROFILE

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/** \addtogroup group_profile_macros
* \{
*/

/** Driver major version */
#define CY_PROFILE_DRV_VERSION_MAJOR  1

/** Driver minor version */
#define CY_PROFILE_DRV_VERSION_MINOR  10

/** Profile driver identifier */
#define CY_PROFILE_ID   CY_PDL_DRV_ID(0x1EU)

/** Start profiling command for the CMD register */
#define CY_PROFILE_START_TR    1UL  

/** Stop profiling command for the CMD register */
#define CY_PROFILE_STOP_TR     2UL  

/** Command to clear all counter registers to 0 */
#define CY_PROFILE_CLR_ALL_CNT 0x100UL

/** \} group_profile_macros */

/***************************************
*        Constants
***************************************/

/** \cond INTERNAL */

#define CY_PROFILE_PRFL_CNT_NR                  (8u)

/* Parameter check macros */
#define CY_PROFILE_IS_MONITOR_VALID(monitor)    ((uint32_t)EP_MONITOR_COUNT > ((uint32_t)(monitor))) /* TODO Replace with max monitor count in the device config */
#define CY_PROFILE_IS_DURATION_VALID(duration)  (((duration) == CY_PROFILE_EVENT) || \
                                                 ((duration) == CY_PROFILE_DURATION))
#define CY_PROFILE_IS_REFCLK_VALID(refClk)      (((refClk) == CY_PROFILE_CLK_TIMER) || \
                                                 ((refClk) == CY_PROFILE_CLK_IMO) || \
                                                 ((refClk) == CY_PROFILE_CLK_ECO) || \
                                                 ((refClk) == CY_PROFILE_CLK_LF) || \
                                                 ((refClk) == CY_PROFILE_CLK_HF) || \
                                                 ((refClk) == CY_PROFILE_CLK_PERI))
#define CY_PROFILE_IS_CNT_VALID(numCounters)    ((uint32_t)CY_PROFILE_PRFL_CNT_NR > (numCounters))

/** \endcond */

/***************************************
*        Enumerations
***************************************/

/**
* \addtogroup group_profile_enums
* \{
*/

/**
* Profile counter reference clock source. Used when duration monitoring.
*/
typedef enum 
{
    CY_PROFILE_CLK_TIMER  = 0, /**< Timer clock (TimerClk) */
    CY_PROFILE_CLK_IMO    = 1, /**< Internal main oscillator (IMO) */
    CY_PROFILE_CLK_ECO    = 2, /**< External crystal oscillator (ECO) */
    CY_PROFILE_CLK_LF     = 3, /**< Low-frequency clock (LFCLK) */
    CY_PROFILE_CLK_HF     = 4, /**< High-Frequency clock (HFCLK0) */
    CY_PROFILE_CLK_PERI   = 5, /**< Peripheral clock (PeriClk) */
} cy_en_profile_ref_clk_t;

/**
* Monitor method type.
*/
typedef enum 
{
    CY_PROFILE_EVENT    = 0,  /**< Count (edge-detected) module events  */
    CY_PROFILE_DURATION = 1,  /**< Count (level) duration in clock cycles */
} cy_en_profile_duration_t;

/** Profiler status codes */
typedef enum 
{
    CY_PROFILE_SUCCESS = 0x00U,                                      /**< Operation completed successfully */
    CY_PROFILE_BAD_PARAM = CY_PROFILE_ID | CY_PDL_STATUS_ERROR | 1UL /**< Invalid input parameters */

 } cy_en_profile_status_t;

 /** \} group_profile_enums */

/***************************************
*       Configuration Structures
***************************************/

/**
* \addtogroup group_profile_data_structures
* \{
*/

/**
* Profile counter control register structure. For each counter, holds the CTL register fields.
*/
typedef struct
{
    cy_en_profile_duration_t  cntDuration; /**< 0 = event; 1 = duration */
    cy_en_profile_ref_clk_t   refClkSel;   /**< The reference clock used by the counter */
    en_ep_mon_sel_t           monSel;      /**< The monitor signal to be observed by the counter */
} cy_stc_profile_ctr_ctl_t; 

/**
* Software structure for holding a profile counter status and configuration information.
*/
typedef struct
{
    uint8_t                   ctrNum;      /**< Profile counter number */ 
    uint8_t                   used;        /**< 0 = available; 1 = used */
    cy_stc_profile_ctr_ctl_t  ctlRegVals;  /**< Initial counter CTL register settings */
    PROFILE_CNT_STRUCT_Type * cntAddr;     /**< Base address of the counter instance registers */
    uint32_t                  ctlReg;      /**< Current CTL register value */
    uint32_t                  cntReg;      /**< Current CNT register value */
    uint32_t                  overflow;    /**< Extension of cntReg to form a 64-bit counter value */
    uint32_t                  weight;      /**< Weighting factor for the counter */
} cy_stc_profile_ctr_t;

/**
* Pointer to a structure holding the status information for a profile counter.
*/
typedef cy_stc_profile_ctr_t * cy_stc_profile_ctr_ptr_t;
/** \} group_profile_data_structures */

/**
* \addtogroup group_profile_functions
* \{
*/

/**
* \addtogroup group_profile_functions_interrupt
* \{
*/
/* ========================================================================== */
/* ====================    INTERRUPT FUNCTION SECTION    ==================== */
/* ========================================================================== */
void Cy_Profile_ISR(void);
/** \} group_profile_functions_interrupt */

/**
* \addtogroup group_profile_functions_general
* \{
*/
__STATIC_INLINE void Cy_Profile_Init(void);
__STATIC_INLINE void Cy_Profile_DeInit(void);
void Cy_Profile_StartProfiling(void);
__STATIC_INLINE void Cy_Profile_DeInit(void);
__STATIC_INLINE void Cy_Profile_StopProfiling(void);
__STATIC_INLINE uint32_t Cy_Profile_IsProfiling(void);


/* ========================================================================== */
/* ===============    GENERAL PROFILE FUNCTIONS SECTION     ================= */
/* ========================================================================== */
/*******************************************************************************
* Function Name: Cy_Profile_Init
****************************************************************************//**
*
* Initializes and enables the profile hardware. 
*
* This function must be called once when energy profiling is desired. The 
* operation does not start a profiling session.
*
* \note The profile interrupt must also be configured. \ref Cy_Profile_ISR() 
* can be used as its handler.
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_Init
*
*******************************************************************************/
__STATIC_INLINE void Cy_Profile_Init(void)
{
    PROFILE->CTL = _VAL2FLD(PROFILE_CTL_ENABLED,  1UL/*enabled */) | 
                   _VAL2FLD(PROFILE_CTL_WIN_MODE, 0UL/*start/stop mode*/);
    PROFILE->INTR_MASK = 0UL; /* clear all counter interrupt mask bits */
}


/*******************************************************************************
* Function Name: Cy_Profile_DeInit
****************************************************************************//**
*
* Clears the interrupt mask and disables the profile hardware. 
*
* This function should be called when energy profiling is no longer desired.
*
* \note The profile interrupt is not disabled by this operation and must be
* disabled separately.
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_DeInit
*
*******************************************************************************/
__STATIC_INLINE void Cy_Profile_DeInit(void)
{
    PROFILE->CTL = _VAL2FLD(PROFILE_CTL_ENABLED, 0UL/*disabled */);
    PROFILE->INTR_MASK = 0UL; /* clear all counter interrupt mask bits */
}


/*******************************************************************************
* Function Name: Cy_Profile_StopProfiling
****************************************************************************//**
*
* Stops the profiling/measurement window.
*
* This operation prevents the enabled profile counters from counting.
*
* \note The profile interrupt should be disabled before calling this function.
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_StopProfiling
*
*******************************************************************************/
__STATIC_INLINE void Cy_Profile_StopProfiling(void)
{
    PROFILE->CMD = CY_PROFILE_STOP_TR;
}


/*******************************************************************************
* Function Name: Cy_Profile_IsProfiling
****************************************************************************//**
*
* Reports the active status of the profiling window.
*
* \return 0 = profiling is not active; 1 = profiling is active
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_IsProfiling
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_Profile_IsProfiling(void)
{
    return _FLD2VAL(PROFILE_STATUS_WIN_ACTIVE, PROFILE->STATUS);
}
/** \} group_profile_functions_general */

/**
* \addtogroup group_profile_functions_counter
* \{
*/
void Cy_Profile_ClearConfiguration(void);
__STATIC_INLINE void Cy_Profile_ClearCounters(void);
cy_stc_profile_ctr_ptr_t Cy_Profile_ConfigureCounter(en_ep_mon_sel_t monitor, cy_en_profile_duration_t duration, cy_en_profile_ref_clk_t refClk, uint32_t weight);
cy_en_profile_status_t Cy_Profile_FreeCounter(cy_stc_profile_ctr_ptr_t ctrAddr);
cy_en_profile_status_t Cy_Profile_EnableCounter(cy_stc_profile_ctr_ptr_t ctrAddr);
cy_en_profile_status_t Cy_Profile_DisableCounter(cy_stc_profile_ctr_ptr_t ctrAddr);

/* ========================================================================== */
/* ===================    COUNTER FUNCTIONS SECTION    ====================== */
/* ========================================================================== */
/*******************************************************************************
* Function Name: Cy_Profile_ClearCounters
****************************************************************************//**
*
* Clears all hardware counters to 0.
*
* \funcusage
* \snippet profile/profile_v1_0_sut_01.cydsn/main_cm4.c snippet_Cy_Profile_ClearCounters
*
*******************************************************************************/
__STATIC_INLINE void Cy_Profile_ClearCounters(void)
{
    PROFILE->CMD = CY_PROFILE_CLR_ALL_CNT;
}
/** \} group_profile_functions_counter */

/**
* \addtogroup group_profile_functions_calculation
* \{
*/
/* ========================================================================== */
/* ==================    CALCULATION FUNCTIONS SECTION    =================== */
/* ========================================================================== */
cy_en_profile_status_t Cy_Profile_GetRawCount(cy_stc_profile_ctr_ptr_t ctrAddr, uint64_t *result);
cy_en_profile_status_t Cy_Profile_GetWeightedCount(cy_stc_profile_ctr_ptr_t ctrAddr, uint64_t *result);
uint64_t Cy_Profile_GetSumWeightedCounts(cy_stc_profile_ctr_ptr_t ptrsArray[], uint32_t numCounters);
/** \} group_profile_functions_calculation */

/** \} group_profile_functions */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CY_IP_MXPROFILE */

#endif /* CY_PROFILE_H */

/** \} group_profile */


/* [] END OF FILE */
