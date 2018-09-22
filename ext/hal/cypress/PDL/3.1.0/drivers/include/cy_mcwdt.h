/***************************************************************************//**
* \file cy_mcwdt.h
* \version 1.20
*
* Provides an API declaration of the Cypress PDL 3.0 MCWDT driver
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_mcwdt Multi-Counter Watchdog (MCWDT)
* \{
* A MCWDT has two 16-bit counters and one 32-bit counter. 
* You can use this driver to create a free-running
* timer or generate periodic interrupts. The driver also
* includes support for the watchdog function to recover from CPU or
* firmware failures.
*
* There are two primary use cases for MCWDT: generating periodic CPU interrupts;
* and implementing a free-running timer. Both have many applications in
* embedded systems:
* * Measuring time between events
* * Generating periodic events
* * Synchronizing actions
* * Real-time clocking
* * Polling
*
* An additional use case is to implement a watchdog used for recovering from a CPU or
* firmware failure.
* 
* \section group_mcwdt_configuration Configuration Considerations
*
* Each MCWDT may be configured for a particular product. 
* One MCWDT block can be associated with only one CPU during runtime. 
* A single MCWDT is not intended to be used by multiple CPUs simultaneously. 
* Each block contains three sub-counters, each of which can be configured for 
* various system utility functions - free running counter, periodic interrupts, 
* watchdog reset, or three interrupts followed by a watchdog reset.
* All counters are clocked by either LFCLK (nominal 32 kHz) or by a cascaded 
* counter.
* A simplified diagram of the MCWDT hardware is shown below:
* \image html mcwdt.png
* The frequency of the periodic interrupts can be configured using the Match 
* value with combining Clear on match option, which can be set individually 
* for each counter using Cy_MCWDT_SetClearOnMatch(). When the Clear on match option 
* is not set, the periodic interrupts of the C0 and C1 16-bit sub-counters occur 
* after 65535 counts and the match value defines the shift between interrupts 
* (see the figure below). The enabled Clear on match option 
* resets the counter when the interrupt occurs.
* \image html mcwdt_counters.png
* 32-bit sub-counter C2 does not have Clear on match option. 
* The interrupt of counter C2 occurs when the counts equal 
* 2<sup>Toggle bit</sup> value.
* \image html mcwdt_subcounters.png
* To set up an MCWDT, provide the configuration parameters in the 
* cy_stc_mcwdt_config_t structure. Then call  
* Cy_MCWDT_Init() to initialize the driver. 
* Call Cy_MCWDT_Enable() to enable all specified counters.
*
* You can also set the mode of operation for any counter. If you choose
* interrupt mode, use Cy_MCWDT_SetInterruptMask() with the 
* parameter for the masks described in Macro Section. All counter interrupts 
* are OR'd together to from a single combined MCWDT interrupt.
* Additionally, enable the Global interrupts and initialize the referenced 
* interrupt by setting the priority and the interrupt vector using  
* \ref Cy_SysInt_Init() of the sysint driver.
* 
* The values of the MCWDT counters can be monitored using 
* Cy_MCWDT_GetCount().
*
* \note In addition to the MCWDTs, each device has a separate watchdog timer
* (WDT) that can also be used to generate a watchdog reset or periodic
* interrupts. For more information on the WDT, see the appropriate section
* of the PDL.
*
* \section group_mcwdt_more_information More Information
*
* For more information on the MCWDT peripheral, refer to 
* the technical reference manual (TRM).
*
* \section group_mcwdt_MISRA MISRA-C Compliance]
* The mcwdt driver does not have any specific deviations.
*
* \section group_mcwdt_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.20</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td>1.10.1</td>
*     <td>Updated description of the \ref cy_stc_mcwdt_config_t structure type</td>
*     <td>Documentation update and clarification</td>
*   </tr>
*   <tr>
*     <td>1.10</td>
*     <td>Added input parameter validation to the API functions.<br>
*     Added API function GetCountCascaded()</td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_mcwdt_macros Macros
* \defgroup group_mcwdt_functions Functions
* \defgroup group_mcwdt_data_structures Data Structures
* \defgroup group_mcwdt_enums Enumerated Types
*/

#ifndef CY_MCWDT_H
#define CY_MCWDT_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include "cy_device_headers.h"
#include "cy_device.h"
#include "cy_syslib.h"

#ifdef CY_IP_MXS40SRSS_MCWDT

/**
* \addtogroup group_mcwdt_data_structures
* \{
*/

/** The MCWDT component configuration structure. */
typedef struct
{
    uint16_t c0Match;        /**< The sub-counter#0 match comparison value, for interrupt or watchdog timeout. 
                                  Range: 0 - 65535 for c0ClearOnMatch = 0 and 1 - 65535 for 
                                  c0ClearOnMatch = 1. */
    uint16_t c1Match;        /**< The sub-counter#1 match comparison value, for interrupt or watchdog timeout.
                                  Range: 0 - 65535 for c1ClearOnMatch = 0 and 1 - 65535 for 
                                  c1ClearOnMatch = 1. */
    uint8_t  c0Mode;         /**< The sub-counter#0 mode. It can have the following values: \ref CY_MCWDT_MODE_NONE, 
                                  \ref CY_MCWDT_MODE_INT, \ref CY_MCWDT_MODE_RESET and \ref CY_MCWDT_MODE_INT_RESET. */
    uint8_t  c1Mode;         /**< The sub-counter#1 mode.  It can have the following values: \ref CY_MCWDT_MODE_NONE, 
                                  \ref CY_MCWDT_MODE_INT, \ref CY_MCWDT_MODE_RESET and \ref CY_MCWDT_MODE_INT_RESET. */
    uint8_t  c2ToggleBit;    /**< The sub-counter#2 Period / Toggle Bit value. 
                                  Range: 0 - 31. */
    uint8_t  c2Mode;         /**< The sub-counter#2 mode. It can have the following values: \ref CY_MCWDT_MODE_NONE 
                                  and \ref CY_MCWDT_MODE_INT. */
    bool     c0ClearOnMatch; /**< The sub-counter#0 Clear On Match parameter enabled/disabled. */
    bool     c1ClearOnMatch; /**< The sub-counter#1 Clear On Match parameter enabled/disabled. */
    bool     c0c1Cascade;    /**< The sub-counter#1 is clocked by LFCLK or from sub-counter#0 cascade. */
    bool     c1c2Cascade;    /**< The sub-counter#2 is clocked by LFCLK or from sub-counter#1 cascade. */
} cy_stc_mcwdt_config_t;

/** \} group_mcwdt_data_structures */

/**
* \addtogroup group_mcwdt_macros
* \{
*/

/** Driver major version */
#define CY_MCWDT_DRV_VERSION_MAJOR       1

/** Driver minor version */
#define CY_MCWDT_DRV_VERSION_MINOR       20

/** \cond INTERNAL_MACROS */

/***************************************
*        Registers Constants
***************************************/

#define CY_MCWDT_LOCK_CLR0      (1u)
#define CY_MCWDT_LOCK_CLR1      (2u)
#define CY_MCWDT_LOCK_SET01     (3u)

#define CY_MCWDT_BYTE_SHIFT     (8u)
#define CY_MCWDT_C0C1_MODE_MASK (3u)
#define CY_MCWDT_C2_MODE_MASK   (1u)


/***************************************
*            API Constants
***************************************/

#define CY_MCWDT_ALL_WDT_ENABLE_Msk (MCWDT_STRUCT_MCWDT_CTL_WDT_ENABLE0_Msk | MCWDT_STRUCT_MCWDT_CTL_WDT_ENABLE1_Msk | \
                                  MCWDT_STRUCT_MCWDT_CTL_WDT_ENABLE2_Msk)
                                  
#define CY_MCWDT_CTR0_Pos (0u)
#define CY_MCWDT_CTR1_Pos (1u)
#define CY_MCWDT_CTR2_Pos (2u)
#define CY_MCWDT_CTR_Pos  (0UL)

/** \endcond */

#define CY_MCWDT_ID       CY_PDL_DRV_ID(0x35u)                      /**< MCWDT PDL ID */

#define CY_MCWDT_CTR0     (1UL << CY_MCWDT_CTR0_Pos)                /**< The sub-counter#0 mask. This macro is used with functions 
                                                                   that handle multiple counters, including Cy_MCWDT_Enable(),
                                                                   Cy_MCWDT_Disable(), Cy_MCWDT_ClearInterrupt() and Cy_MCWDT_ResetCounters(). */
#define CY_MCWDT_CTR1     (1UL << CY_MCWDT_CTR1_Pos)                /**< The sub-counter#1 mask. This macro is used with functions 
                                                                   that handle multiple counters, including Cy_MCWDT_Enable(),
                                                                   Cy_MCWDT_Disable(), Cy_MCWDT_ClearInterrupt() and Cy_MCWDT_ResetCounters(). */
#define CY_MCWDT_CTR2     (1UL << CY_MCWDT_CTR2_Pos)                /**< The sub-counter#2 mask. This macro is used with functions 
                                                                   that handle multiple counters, including Cy_MCWDT_Enable(),
                                                                   Cy_MCWDT_Disable(), Cy_MCWDT_ClearInterrupt() and Cy_MCWDT_ResetCounters(). */
#define CY_MCWDT_CTR_Msk  (CY_MCWDT_CTR0 | CY_MCWDT_CTR1 | CY_MCWDT_CTR2) /**< The mask for all sub-counters. This macro is used with functions 
                                                                   that handle multiple counters, including Cy_MCWDT_Enable(),
                                                                   Cy_MCWDT_Disable(), Cy_MCWDT_ClearInterrupt() and Cy_MCWDT_ResetCounters(). */

/** \} group_mcwdt_macros */


/**
* \addtogroup group_mcwdt_enums
* \{
*/

/** The mcwdt sub-counter identifiers. */
typedef enum
{
    CY_MCWDT_COUNTER0,  /**< Sub-counter#0 identifier. */
    CY_MCWDT_COUNTER1,  /**< Sub-counter#1 identifier. */
    CY_MCWDT_COUNTER2   /**< Sub-counter#2 identifier. */
} cy_en_mcwdtctr_t;

/** The mcwdt modes. */
typedef enum
{
    CY_MCWDT_MODE_NONE,      /**< The No action mode. It is used for Set/GetMode functions. */
    CY_MCWDT_MODE_INT,       /**< The Interrupt mode. It is used for Set/GetMode functions. */
    CY_MCWDT_MODE_RESET,     /**< The Reset mode. It is used for Set/GetMode functions. */
    CY_MCWDT_MODE_INT_RESET  /**< The Three interrupts then watchdog reset mode. It is used for 
                               Set/GetMode functions. */
} cy_en_mcwdtmode_t;

/** The mcwdt cascading. */
typedef enum
{
    CY_MCWDT_CASCADE_NONE,  /**< The cascading is disabled. It is used for Set/GetCascade functions. */
    CY_MCWDT_CASCADE_C0C1,  /**< The sub-counter#1 is clocked by LFCLK or from sub-counter#0 cascade. 
                              It is used for Set/GetCascade functions. */
    CY_MCWDT_CASCADE_C1C2,  /**< The sub-counter#2 is clocked by LFCLK or from sub-counter#1 cascade.
                              It is used for Set/GetCascade functions. */
    CY_MCWDT_CASCADE_BOTH   /**< The sub-counter#1 is clocked by LFCLK or from sub-counter#0 cascade 
                              and the sub-counter#2 is clocked by LFCLK or from sub-counter#1 cascade. 
                              It is used for Set/GetCascade functions. */
} cy_en_mcwdtcascade_t;

/** The MCWDT error codes. */
typedef enum 
{
    CY_MCWDT_SUCCESS = 0x00u,                                            /**< Successful */
    CY_MCWDT_BAD_PARAM = CY_MCWDT_ID | CY_PDL_STATUS_ERROR | 0x01u,     /**< One or more invalid parameters */
} cy_en_mcwdt_status_t;

/** \} group_mcwdt_enums */


/** \cond PARAM_CHECK_MACROS */

/** Parameter check macros */ 
#define CY_MCWDT_IS_CNTS_MASK_VALID(counters)  (0U == ((counters) & (uint32_t)~CY_MCWDT_CTR_Msk)) 
                                               
#define CY_MCWDT_IS_CNT_NUM_VALID(counter)    ((CY_MCWDT_COUNTER0 == (counter)) || \
                                               (CY_MCWDT_COUNTER1 == (counter)) || \
                                               (CY_MCWDT_COUNTER2 == (counter)))

#define CY_MCWDT_IS_MODE_VALID(mode)          ((CY_MCWDT_MODE_NONE == (mode)) || \
                                               (CY_MCWDT_MODE_INT == (mode)) || \
                                               (CY_MCWDT_MODE_RESET == (mode)) || \
                                               (CY_MCWDT_MODE_INT_RESET == (mode)))

#define CY_MCWDT_IS_ENABLE_VALID(enable)      (1UL >= (enable))


#define CY_MCWDT_IS_CASCADE_VALID(cascade)    ((CY_MCWDT_CASCADE_NONE == (cascade)) || \
                                               (CY_MCWDT_CASCADE_C0C1 == (cascade)) || \
                                               (CY_MCWDT_CASCADE_C1C2 == (cascade)) || \
                                               (CY_MCWDT_CASCADE_BOTH == (cascade)))
                                               
#define CY_MCWDT_IS_MATCH_VALID(clearOnMatch, match)  ((clearOnMatch) ? (1UL <= (match)) : true)

#define CY_MCWDT_IS_BIT_VALID(bit)            (31UL >= (bit))                                            

                                          
/** \endcond */


/*******************************************************************************
*        Function Prototypes
*******************************************************************************/

/**
* \addtogroup group_mcwdt_functions
* \{
*/
cy_en_mcwdt_status_t     Cy_MCWDT_Init(MCWDT_STRUCT_Type *base, cy_stc_mcwdt_config_t const *config);
                void     Cy_MCWDT_DeInit(MCWDT_STRUCT_Type *base);
__STATIC_INLINE void     Cy_MCWDT_Enable(MCWDT_STRUCT_Type *base, uint32_t counters, uint16_t waitUs);
__STATIC_INLINE void     Cy_MCWDT_Disable(MCWDT_STRUCT_Type *base, uint32_t counters, uint16_t waitUs);
__STATIC_INLINE uint32_t Cy_MCWDT_GetEnabledStatus(MCWDT_STRUCT_Type const *base, cy_en_mcwdtctr_t counter);
__STATIC_INLINE void     Cy_MCWDT_Lock(MCWDT_STRUCT_Type *base);
__STATIC_INLINE void     Cy_MCWDT_Unlock(MCWDT_STRUCT_Type *base);
__STATIC_INLINE uint32_t Cy_MCWDT_GetLockedStatus(MCWDT_STRUCT_Type const *base);
__STATIC_INLINE void     Cy_MCWDT_SetMode(MCWDT_STRUCT_Type *base, cy_en_mcwdtctr_t counter, cy_en_mcwdtmode_t mode);
__STATIC_INLINE cy_en_mcwdtmode_t Cy_MCWDT_GetMode(MCWDT_STRUCT_Type const *base, cy_en_mcwdtctr_t counter);
__STATIC_INLINE void     Cy_MCWDT_SetClearOnMatch(MCWDT_STRUCT_Type *base, cy_en_mcwdtctr_t counter, uint32_t enable);
__STATIC_INLINE uint32_t Cy_MCWDT_GetClearOnMatch(MCWDT_STRUCT_Type const *base, cy_en_mcwdtctr_t counter);
__STATIC_INLINE void     Cy_MCWDT_SetCascade(MCWDT_STRUCT_Type *base, cy_en_mcwdtcascade_t cascade);
__STATIC_INLINE cy_en_mcwdtcascade_t Cy_MCWDT_GetCascade(MCWDT_STRUCT_Type const *base);
__STATIC_INLINE void     Cy_MCWDT_SetMatch(MCWDT_STRUCT_Type *base, cy_en_mcwdtctr_t counter, uint32_t match, uint16_t waitUs);
__STATIC_INLINE uint32_t Cy_MCWDT_GetMatch(MCWDT_STRUCT_Type const *base, cy_en_mcwdtctr_t counter);
__STATIC_INLINE void     Cy_MCWDT_SetToggleBit(MCWDT_STRUCT_Type *base, uint32_t bit);
__STATIC_INLINE uint32_t Cy_MCWDT_GetToggleBit(MCWDT_STRUCT_Type const *base);
__STATIC_INLINE uint32_t Cy_MCWDT_GetCount(MCWDT_STRUCT_Type const *base, cy_en_mcwdtctr_t counter);
__STATIC_INLINE void     Cy_MCWDT_ResetCounters(MCWDT_STRUCT_Type *base, uint32_t counters, uint16_t waitUs);
__STATIC_INLINE uint32_t Cy_MCWDT_GetInterruptStatus(MCWDT_STRUCT_Type const *base);
__STATIC_INLINE void     Cy_MCWDT_ClearInterrupt(MCWDT_STRUCT_Type *base, uint32_t counters);
__STATIC_INLINE void     Cy_MCWDT_SetInterrupt(MCWDT_STRUCT_Type *base, uint32_t counters);
__STATIC_INLINE uint32_t Cy_MCWDT_GetInterruptMask(MCWDT_STRUCT_Type const *base);
__STATIC_INLINE void     Cy_MCWDT_SetInterruptMask(MCWDT_STRUCT_Type *base, uint32_t counters);
__STATIC_INLINE uint32_t Cy_MCWDT_GetInterruptStatusMasked(MCWDT_STRUCT_Type const *base);
uint32_t Cy_MCWDT_GetCountCascaded(MCWDT_STRUCT_Type const *base);


/*******************************************************************************
* Function Name: Cy_MCWDT_Enable
****************************************************************************//**
*
*  Enables all specified counters.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param counters
*  OR of all counters to enable. See the \ref CY_MCWDT_CTR0, CY_MCWDT_CTR1, and
*  CY_MCWDT_CTR2  macros.
*
*  \param waitUs
*  The function waits for some delay in microseconds before returning, 
*  because the counter begins counting after two lf_clk cycles pass. 
*  The recommended value is 93 us.
*  \note
*  Setting this parameter to a zero means No wait. In this case, it is
*  the user's responsibility to check whether the selected counters were enabled 
*  immediately after the function call. This can be done by the 
*  Cy_MCWDT_GetEnabledStatus() API.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_Enable(MCWDT_STRUCT_Type *base, uint32_t counters, uint16_t waitUs)
{
    uint32_t enableCounters;

    CY_ASSERT_L2(CY_MCWDT_IS_CNTS_MASK_VALID(counters));
    
    /* Extract particular counters for enable */
    enableCounters = ((0UL != (counters & CY_MCWDT_CTR0)) ? MCWDT_STRUCT_MCWDT_CTL_WDT_ENABLE0_Msk : 0UL) |
                     ((0UL != (counters & CY_MCWDT_CTR1)) ? MCWDT_STRUCT_MCWDT_CTL_WDT_ENABLE1_Msk : 0UL) |
                     ((0UL != (counters & CY_MCWDT_CTR2)) ? MCWDT_STRUCT_MCWDT_CTL_WDT_ENABLE2_Msk : 0UL);

    MCWDT_STRUCT_MCWDT_CTL(base) |= enableCounters;

    Cy_SysLib_DelayUs(waitUs);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_Disable
****************************************************************************//**
*
*  Disables all specified counters.
*
*  \param base
*  The base pointer to a structure describing registers.
*
*  \param counters
*  OR of all counters to disable. See the \ref CY_MCWDT_CTR0, CY_MCWDT_CTR1, and
*  CY_MCWDT_CTR2 macros.
*
*  \param waitUs
*  The function waits for some delay in microseconds before returning, 
*  because the counter stops counting after two lf_clk cycles pass. 
*  The recommended value is 93 us.
*  \note
*  Setting this parameter to a zero means No wait. In this case, it is 
*  the user's responsibility to check whether the selected counters were disabled 
*  immediately after the function call. This can be done by the 
*  Cy_MCWDT_GetEnabledStatus() API.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_Disable(MCWDT_STRUCT_Type *base, uint32_t counters, uint16_t waitUs)
{
    uint32_t disableCounters;

    CY_ASSERT_L2(CY_MCWDT_IS_CNTS_MASK_VALID(counters));
    
    /* Extract particular counters for disable */
    disableCounters = ((0UL != (counters & CY_MCWDT_CTR0)) ? MCWDT_STRUCT_MCWDT_CTL_WDT_ENABLE0_Msk : 0UL) |
                      ((0UL != (counters & CY_MCWDT_CTR1)) ? MCWDT_STRUCT_MCWDT_CTL_WDT_ENABLE1_Msk : 0UL) |
                      ((0UL != (counters & CY_MCWDT_CTR2)) ? MCWDT_STRUCT_MCWDT_CTL_WDT_ENABLE2_Msk : 0UL);

    MCWDT_STRUCT_MCWDT_CTL(base) &= ~disableCounters;

    Cy_SysLib_DelayUs(waitUs);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_GetEnabledStatus
****************************************************************************//**
*
*  Reports the enabled status of the specified counter.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param counter
*  The number of the MCWDT counter. The valid range is [0-2].
*
*  \return
*  The status of the MCWDT counter: 0 = disabled, 1 = enabled.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_MCWDT_GetEnabledStatus(MCWDT_STRUCT_Type const *base, cy_en_mcwdtctr_t counter)
{
    uint32_t status = 0u;

    CY_ASSERT_L3(CY_MCWDT_IS_CNT_NUM_VALID(counter));
    
    switch (counter)
    {
        case CY_MCWDT_COUNTER0:
            status = _FLD2VAL(MCWDT_STRUCT_MCWDT_CTL_WDT_ENABLED0, MCWDT_STRUCT_MCWDT_CTL(base));
            break;
        case CY_MCWDT_COUNTER1:
            status = _FLD2VAL(MCWDT_STRUCT_MCWDT_CTL_WDT_ENABLED1, MCWDT_STRUCT_MCWDT_CTL(base));
            break;
        case CY_MCWDT_COUNTER2:
            status = _FLD2VAL(MCWDT_STRUCT_MCWDT_CTL_WDT_ENABLED2, MCWDT_STRUCT_MCWDT_CTL(base));
            break;

        default:
            CY_ASSERT(0u != 0u);
        break;
    }

    return (status);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_Lock
****************************************************************************//**
*
*  Locks out configuration changes to all MCWDT registers.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_Lock(MCWDT_STRUCT_Type *base)
{
    uint32_t interruptState;

    interruptState = Cy_SysLib_EnterCriticalSection();

    MCWDT_STRUCT_MCWDT_LOCK(base) = _CLR_SET_FLD32U(MCWDT_STRUCT_MCWDT_LOCK(base), MCWDT_STRUCT_MCWDT_LOCK_MCWDT_LOCK, (uint32_t)CY_MCWDT_LOCK_SET01);

    Cy_SysLib_ExitCriticalSection(interruptState);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_Unlock
****************************************************************************//**
*
*  Unlocks the MCWDT configuration registers.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_Unlock(MCWDT_STRUCT_Type *base)
{
    uint32_t interruptState;

    interruptState = Cy_SysLib_EnterCriticalSection();

    MCWDT_STRUCT_MCWDT_LOCK(base) = _CLR_SET_FLD32U(MCWDT_STRUCT_MCWDT_LOCK(base), MCWDT_STRUCT_MCWDT_LOCK_MCWDT_LOCK, (uint32_t)CY_MCWDT_LOCK_CLR0);
    MCWDT_STRUCT_MCWDT_LOCK(base) = _CLR_SET_FLD32U(MCWDT_STRUCT_MCWDT_LOCK(base), MCWDT_STRUCT_MCWDT_LOCK_MCWDT_LOCK, (uint32_t)CY_MCWDT_LOCK_CLR1);

    Cy_SysLib_ExitCriticalSection(interruptState);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_GetLockStatus
****************************************************************************//**
*
*  Reports the locked/unlocked state of the MCWDT.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \return
*  The state of the MCWDT counter: 0 = unlocked, 1 = locked.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_MCWDT_GetLockedStatus(MCWDT_STRUCT_Type const *base)
{
    return ((0UL != (MCWDT_STRUCT_MCWDT_LOCK(base) & MCWDT_STRUCT_MCWDT_LOCK_MCWDT_LOCK_Msk)) ? 1UL : 0UL);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_SetMode
****************************************************************************//**
*
*  Sets the mode of the specified counter.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param counter
*  The number of the WDT counter. The valid range is [0-2].
*
*  \param mode
*  The mode of operation for the counter. See enum typedef cy_en_mcwdtmode_t.
*
*  \note
*  The mode for Counter 2 can be set only to CY_MCWDT_MODE_NONE or CY_MCWDT_MODE_INT.
*
*  \note
*  This API must not be called while the counters are running. 
*  Prior to calling this API, the counter must be disabled.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_SetMode(MCWDT_STRUCT_Type *base, cy_en_mcwdtctr_t counter, cy_en_mcwdtmode_t mode)
{
    uint32_t mask, shift;

    CY_ASSERT_L3(CY_MCWDT_IS_CNT_NUM_VALID(counter));
    CY_ASSERT_L3(CY_MCWDT_IS_MODE_VALID(mode));

    shift = CY_MCWDT_BYTE_SHIFT * counter;
    mask = (counter == CY_MCWDT_COUNTER2) ? CY_MCWDT_C2_MODE_MASK : CY_MCWDT_C0C1_MODE_MASK;
    mask = mask << shift;

    MCWDT_STRUCT_MCWDT_CONFIG(base) = (MCWDT_STRUCT_MCWDT_CONFIG(base) & ~mask) | ((uint32_t) mode << shift);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_GetMode
****************************************************************************//**
*
*  Reports the mode of the  specified counter.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param counter
*  The number of the WDT counter. The valid range is [0-2].
*
*  \return
*  The current mode of the counter. See enum typedef cy_en_mcwdtmode_t.
*
*******************************************************************************/
__STATIC_INLINE cy_en_mcwdtmode_t Cy_MCWDT_GetMode(MCWDT_STRUCT_Type const *base, cy_en_mcwdtctr_t counter)
{
    uint32_t mode, mask;

    CY_ASSERT_L3(CY_MCWDT_IS_CNT_NUM_VALID(counter));

    mask = (counter == CY_MCWDT_COUNTER2) ? CY_MCWDT_C2_MODE_MASK : CY_MCWDT_C0C1_MODE_MASK;
    mode = (MCWDT_STRUCT_MCWDT_CONFIG(base) >> (CY_MCWDT_BYTE_SHIFT * counter)) & mask;

    return ((cy_en_mcwdtmode_t) mode);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_SetClearOnMatch
****************************************************************************//**
*
*  Sets the Clear on match option for the specified counter.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param counter
*   The number of the WDT counter. The valid range is [0-1].
*
*  \note
*  The match values are not supported by Counter 2.
*
*  \param enable
*  Set 0 to disable; 1 to enable.
*
*  \note
*  This API must not be called while the counters are running. 
*  Prior to calling this API, the counter must be disabled.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_SetClearOnMatch(MCWDT_STRUCT_Type *base, cy_en_mcwdtctr_t counter, uint32_t enable)
{
    CY_ASSERT_L3(CY_MCWDT_IS_CNT_NUM_VALID(counter));
    CY_ASSERT_L2(CY_MCWDT_IS_ENABLE_VALID(enable));

    if (CY_MCWDT_COUNTER0 == counter)
    {
        MCWDT_STRUCT_MCWDT_CONFIG(base) = _CLR_SET_FLD32U(MCWDT_STRUCT_MCWDT_CONFIG(base), MCWDT_STRUCT_MCWDT_CONFIG_WDT_CLEAR0, enable);
    }
    else
    {
        MCWDT_STRUCT_MCWDT_CONFIG(base) = _CLR_SET_FLD32U(MCWDT_STRUCT_MCWDT_CONFIG(base), MCWDT_STRUCT_MCWDT_CONFIG_WDT_CLEAR1, enable);
    }
}


/*******************************************************************************
* Function Name: Cy_MCWDT_GetClearOnMatch
****************************************************************************//**
*
*  Reports the Clear on match setting for the specified counter.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param counter
*  The number of the WDT counter. The valid range is [0-1].
*
*  \return
*  The Clear on match status: 1 = enabled, 0 = disabled.
*
*  \note
*  The match value is not supported by Counter 2.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_MCWDT_GetClearOnMatch(MCWDT_STRUCT_Type const *base, cy_en_mcwdtctr_t counter)
{
    uint32_t getClear;

    CY_ASSERT_L3(CY_MCWDT_IS_CNT_NUM_VALID(counter));

    if (CY_MCWDT_COUNTER0 == counter)
    {
        getClear = _FLD2VAL(MCWDT_STRUCT_MCWDT_CONFIG_WDT_CLEAR0, MCWDT_STRUCT_MCWDT_CONFIG(base));
    }
    else
    {
        getClear = _FLD2VAL(MCWDT_STRUCT_MCWDT_CONFIG_WDT_CLEAR1, MCWDT_STRUCT_MCWDT_CONFIG(base));
    }

    return (getClear);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_SetCascade
****************************************************************************//**
*
*  Sets all the counter cascade options.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param cascade
*  Sets or clears each of the cascade options.
*
*  \note
*  This API must not be called when the counters are running. 
*  Prior to calling this API, the counter must be disabled.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_SetCascade(MCWDT_STRUCT_Type *base, cy_en_mcwdtcascade_t cascade)
{
    CY_ASSERT_L3(CY_MCWDT_IS_CASCADE_VALID(cascade));
    
    MCWDT_STRUCT_MCWDT_CONFIG(base) = _CLR_SET_FLD32U(MCWDT_STRUCT_MCWDT_CONFIG(base), MCWDT_STRUCT_MCWDT_CONFIG_WDT_CASCADE0_1,
                                             (uint32_t) cascade);
    MCWDT_STRUCT_MCWDT_CONFIG(base) = _CLR_SET_FLD32U(MCWDT_STRUCT_MCWDT_CONFIG(base), MCWDT_STRUCT_MCWDT_CONFIG_WDT_CASCADE1_2, 
                                             ((uint32_t) cascade >> 1u));
}


/*******************************************************************************
* Function Name: Cy_MCWDT_GetCascade
****************************************************************************//**
*
*  Reports all the counter cascade option settings.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \return
*  The current cascade option values.
*
*******************************************************************************/
__STATIC_INLINE cy_en_mcwdtcascade_t Cy_MCWDT_GetCascade(MCWDT_STRUCT_Type const *base)
{
    uint32_t cascade;

    cascade = (_FLD2VAL(MCWDT_STRUCT_MCWDT_CONFIG_WDT_CASCADE1_2, MCWDT_STRUCT_MCWDT_CONFIG(base)) << 1u) |
               _FLD2VAL(MCWDT_STRUCT_MCWDT_CONFIG_WDT_CASCADE0_1, MCWDT_STRUCT_MCWDT_CONFIG(base));

    return ((cy_en_mcwdtcascade_t) cascade);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_SetMatch
****************************************************************************//**
*
*  Sets the match comparison value for the specified counter (0 or 1).
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param counter
*   The number of the WDT counter. The valid range is [0-1].
*
*  \param match
*  The value to match against the counter. 
*  The valid range is [0-65535] for c0ClearOnMatch (or c1ClearOnMatch) = 0 
*  and [1-65535] for c0ClearOnMatch (or c1ClearOnMatch) = 1.
*
*  \note
*  The match value is not supported by Counter 2.
*
*  \note
*  Action on match is taken on the next increment after the counter value 
*  equal to match value.
*
*  \param waitUs
*  The function waits for some delay in microseconds before returning, 
*  because the match affects after two lf_clk cycles pass. The recommended 
*  value is 93 us.
*  \note
*  Setting this parameter to a zero means No wait. This must be taken
*  into account when changing the match values on the running counters.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_SetMatch(MCWDT_STRUCT_Type *base, cy_en_mcwdtctr_t counter, uint32_t match, uint16_t waitUs)
{
    CY_ASSERT_L3(CY_MCWDT_IS_CNT_NUM_VALID(counter));
    CY_ASSERT_L2(CY_MCWDT_IS_MATCH_VALID((CY_MCWDT_COUNTER0 == counter) ?  
                                         ((MCWDT_STRUCT_MCWDT_CONFIG(base) & MCWDT_STRUCT_MCWDT_CONFIG_WDT_CLEAR0_Msk) > 0U) : 
                                         ((MCWDT_STRUCT_MCWDT_CONFIG(base) & MCWDT_STRUCT_MCWDT_CONFIG_WDT_CLEAR1_Msk) > 0U),
                                          match));

    MCWDT_STRUCT_MCWDT_MATCH(base) = (counter == CY_MCWDT_COUNTER0) ?
        _CLR_SET_FLD32U(MCWDT_STRUCT_MCWDT_MATCH(base), MCWDT_STRUCT_MCWDT_MATCH_WDT_MATCH0,
                        (match & MCWDT_STRUCT_MCWDT_MATCH_WDT_MATCH0_Msk)) :
        _CLR_SET_FLD32U(MCWDT_STRUCT_MCWDT_MATCH(base), MCWDT_STRUCT_MCWDT_MATCH_WDT_MATCH1,
                        (match & MCWDT_STRUCT_MCWDT_MATCH_WDT_MATCH0_Msk));

    Cy_SysLib_DelayUs(waitUs);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_GetMatch
****************************************************************************//**
*
*  Reports the match comparison value for the specified counter (0 or 1).
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param counter
*  The number of the WDT counter. The valid range is [0-1].
*
*  \note
*  The match values are not supported by Counter 2.
*
*  \return
*  A 16-bit match value.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_MCWDT_GetMatch(MCWDT_STRUCT_Type const *base, cy_en_mcwdtctr_t counter)
{
    uint32_t match;

    CY_ASSERT_L3(CY_MCWDT_IS_CNT_NUM_VALID(counter));

    match = (counter == CY_MCWDT_COUNTER0) ? _FLD2VAL(MCWDT_STRUCT_MCWDT_MATCH_WDT_MATCH0, MCWDT_STRUCT_MCWDT_MATCH(base)) :
                                          _FLD2VAL(MCWDT_STRUCT_MCWDT_MATCH_WDT_MATCH1, MCWDT_STRUCT_MCWDT_MATCH(base));

    return (match);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_SetToggleBit
****************************************************************************//**
*
*  Sets a bit in Counter 2 to monitor for a toggle.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param bit
*  The Counter 2 bit is set to monitor for a toggle. The valid range [0-31].
*
*  \note
*  This API must not be called when counters are running.
*  Prior to calling this API, the counter must be disabled.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_SetToggleBit(MCWDT_STRUCT_Type *base, uint32_t bit)
{
    CY_ASSERT_L2(CY_MCWDT_IS_BIT_VALID(bit));
    
    MCWDT_STRUCT_MCWDT_CONFIG(base) = _CLR_SET_FLD32U(MCWDT_STRUCT_MCWDT_CONFIG(base), MCWDT_STRUCT_MCWDT_CONFIG_WDT_BITS2, bit);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_GetToggleBit
****************************************************************************//**
*
*  Reports which bit in Counter 2 is monitored for a toggle.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \return
*  The bit that is monitored (range 0 to 31).
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_MCWDT_GetToggleBit(MCWDT_STRUCT_Type const *base)
{
    return (_FLD2VAL(MCWDT_STRUCT_MCWDT_CONFIG_WDT_BITS2, MCWDT_STRUCT_MCWDT_CONFIG(base)));
}


/*******************************************************************************
* Function Name: Cy_MCWDT_GetCount
****************************************************************************//**
*
*  Reports the current counter value of the specified counter.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param counter
*  The number of the WDT counter. The valid range is [0-2].
*
*  \return
*  A live counter value. Counters 0 and 1 are 16-bit counters and Counter 2 is
*  a 32-bit counter.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_MCWDT_GetCount(MCWDT_STRUCT_Type const *base, cy_en_mcwdtctr_t counter)
{
    uint32_t countVal = 0u;

    CY_ASSERT_L3(CY_MCWDT_IS_CNT_NUM_VALID(counter));
    
    switch (counter)
    {
        case CY_MCWDT_COUNTER0:
            countVal = _FLD2VAL(MCWDT_STRUCT_MCWDT_CNTLOW_WDT_CTR0, MCWDT_STRUCT_MCWDT_CNTLOW(base));
            break;
        case CY_MCWDT_COUNTER1:
            countVal = _FLD2VAL(MCWDT_STRUCT_MCWDT_CNTLOW_WDT_CTR1, MCWDT_STRUCT_MCWDT_CNTLOW(base));
            break;
        case CY_MCWDT_COUNTER2:
            countVal = _FLD2VAL(MCWDT_STRUCT_MCWDT_CNTHIGH_WDT_CTR2, MCWDT_STRUCT_MCWDT_CNTHIGH(base));
            break;

        default:
            CY_ASSERT(0u != 0u);
        break;
    }

    return (countVal);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_ResetCounters
****************************************************************************//**
*
*  Resets all specified counters.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param counters
*  OR of all counters to reset. See the \ref CY_MCWDT_CTR0, CY_MCWDT_CTR1, and
*  CY_MCWDT_CTR2  macros.
*
*  \param waitUs
*  The function waits for some delay in microseconds before returning, because 
*  a reset occurs after one lf_clk cycle passes. The recommended value is 62 us.
*  \note This function resets the counters two times to prevent the case when 
*  the Counter 1 is not reset when the counters are cascaded. The delay waitUs
*  must be greater than 100 us when the counters are cascaded. 
*  The total delay is greater than 2*waitUs because the function has 
*  the delay after the first reset.
*  \note
*  Setting this parameter to a zero means No wait. In this case, it is the 
*  user's responsibility to check whether the selected counters were reset 
*  immediately after the function call. This can be done by the 
*  Cy_MCWDT_GetCount() API.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_ResetCounters(MCWDT_STRUCT_Type *base, uint32_t counters, uint16_t waitUs)
{
    uint32_t resetCounters;

    CY_ASSERT_L2(CY_MCWDT_IS_CNTS_MASK_VALID(counters));
    
    /* Extract particular counters for reset */
    resetCounters = ((0UL != (counters & CY_MCWDT_CTR0)) ? MCWDT_STRUCT_MCWDT_CTL_WDT_RESET0_Msk : 0UL) |
                    ((0UL != (counters & CY_MCWDT_CTR1)) ? MCWDT_STRUCT_MCWDT_CTL_WDT_RESET1_Msk : 0UL) |
                    ((0UL != (counters & CY_MCWDT_CTR2)) ? MCWDT_STRUCT_MCWDT_CTL_WDT_RESET2_Msk : 0UL);

    MCWDT_STRUCT_MCWDT_CTL(base) |= resetCounters;
    
    Cy_SysLib_DelayUs(waitUs);
    
    MCWDT_STRUCT_MCWDT_CTL(base) |= resetCounters;
    
    Cy_SysLib_DelayUs(waitUs);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_GetInterruptStatus
****************************************************************************//**
*
*  Reports the state of all MCWDT interrupts.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \return
*  The OR'd state of the interrupts. See the \ref CY_MCWDT_CTR0, CY_MCWDT_CTR1, and
*  CY_MCWDT_CTR2 macros.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_MCWDT_GetInterruptStatus(MCWDT_STRUCT_Type const *base)
{
    return (MCWDT_STRUCT_MCWDT_INTR(base));
}


/*******************************************************************************
* Function Name: Cy_MCWDT_ClearInterrupt
****************************************************************************//**
*
*  Clears all specified MCWDT interrupts.
*
*  All the WDT interrupts must be cleared by the firmware; otherwise
*  interrupts are generated continuously.
*
*  \param base
*  The base pointer to a structure describes registers.
*
*  \param counters
*  OR of all interrupt sources to clear. See the \ref CY_MCWDT_CTR0, CY_MCWDT_CTR1, and
*  CY_MCWDT_CTR2  macros.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_ClearInterrupt(MCWDT_STRUCT_Type *base, uint32_t counters)
{
    CY_ASSERT_L2(CY_MCWDT_IS_CNTS_MASK_VALID(counters));
    
    MCWDT_STRUCT_MCWDT_INTR(base) = counters;
    (void) MCWDT_STRUCT_MCWDT_INTR(base);
}


/*******************************************************************************
* Function Name: Cy_MCWDT_SetInterrupt
****************************************************************************//**
*
*  Sets MCWDT interrupt sources in the interrupt request register.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param counters
*  OR of all interrupt sources to set. See the \ref CY_MCWDT_CTR0, CY_MCWDT_CTR1, and
*  CY_MCWDT_CTR2  macros.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_SetInterrupt(MCWDT_STRUCT_Type *base, uint32_t counters)
{
    CY_ASSERT_L2(CY_MCWDT_IS_CNTS_MASK_VALID(counters));

    MCWDT_STRUCT_MCWDT_INTR_SET(base) = counters;
}


/*******************************************************************************
* Function Name: Cy_MCWDT_GetInterruptMask
****************************************************************************//**
*
* Returns the CWDT interrupt mask register. This register specifies which bits
* from the MCWDT interrupt request register will trigger an interrupt event.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \return
*  The OR'd state of the interrupt masks. See the \ref CY_MCWDT_CTR0, CY_MCWDT_CTR1, and
*  CY_MCWDT_CTR2  macros.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_MCWDT_GetInterruptMask(MCWDT_STRUCT_Type const *base)
{
    return (MCWDT_STRUCT_MCWDT_INTR_MASK(base));
}


/*******************************************************************************
* Function Name: Cy_MCWDT_SetInterruptMask
****************************************************************************//**
*
* Writes MCWDT interrupt mask register. This register configures which bits
* from MCWDT interrupt request register will trigger an interrupt event.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \param counters
*  OR of all interrupt masks to set. See \ref CY_MCWDT_CTR0, CY_MCWDT_CTR1, and
*  CY_MCWDT_CTR2  macros.
*
*******************************************************************************/
__STATIC_INLINE void Cy_MCWDT_SetInterruptMask(MCWDT_STRUCT_Type *base, uint32_t counters)
{
    CY_ASSERT_L2(CY_MCWDT_IS_CNTS_MASK_VALID(counters));
    
    MCWDT_STRUCT_MCWDT_INTR_MASK(base) = counters;
}


/*******************************************************************************
* Function Name: Cy_MCWDT_GetInterruptStatusMasked
****************************************************************************//**
*
* Returns the MCWDT interrupt masked request register. This register contains
* the logical AND of corresponding bits from the MCWDT interrupt request and 
* mask registers.
* In the interrupt service routine, this function identifies which of the 
* enabled MCWDT interrupt sources caused an interrupt event.
*
*  \param base
*  The base pointer to a structure that describes registers.
*
*  \return
*  The current status of enabled MCWDT interrupt sources. See
*  the \ref CY_MCWDT_CTR0, CY_MCWDT_CTR1, and CY_MCWDT_CTR2 macros.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_MCWDT_GetInterruptStatusMasked(MCWDT_STRUCT_Type const *base)
{
    return (MCWDT_STRUCT_MCWDT_INTR_MASKED(base));
}


/** \} group_mcwdt_functions */

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXS40SRSS_MCWDT */

#endif /* CY_MCWDT_H */

/** \} group_mcwdt */

/* [] END OF FILE */
