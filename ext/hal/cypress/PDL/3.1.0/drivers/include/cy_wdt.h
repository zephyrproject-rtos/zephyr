/***************************************************************************//**
* \file cy_wdt.h
* \version 1.10
*
*  This file provides constants and parameter values for the WDT driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*
*******************************************************************************/

/**
* \defgroup group_wdt Watchdog Timer (WDT)
* \{
*
* The Watchdog timer (WDT) has a 16-bit free-running up-counter. The WDT can 
* issue counter match interrupts, and a device reset if its interrupts are not 
* handled. Use the Watchdog timer for two main purposes.<br>
* The <b> First use case </b> is recovering from a CPU or firmware failure.
* A timeout period is set up in the Watchdog timer, and if a timeout occurs, the
* device is reset (WRES). <br>
* The <b>Second use case</b> is to generate periodic interrupts.
* It is strongly recommended not to use the WDT for periodic interrupt
* generation. However, if absolutely required, see information below.
*
* A "reset cause" register exists, and the firmware should check this register 
* at a start-up. An appropriate action can be taken if a WRES reset is detected.
*
* The user's firmware periodically resets the timeout period (clears or "feeds" 
* the watchdog) before a timeout occurs. If the firmware fails to do so, that is
* considered to be a CPU crash or a firmware failure, and the reason for a 
* device reset.
* The WDT can generate an interrupt instead of a device reset. The Interrupt 
* Service Routine (ISR) can handle the interrupt either as a periodic interrupt,
* or as an early indication of a firmware failure and respond accordingly.
* However, it is not recommended to use the WDT for periodic interrupt 
* generation. The Multi-counter Watchdog Timers (MCWDT) can be used to generate 
* periodic interrupts if such are presented in the device.
*
* <b> Functional Description </b> <br>
* The WDT generates an interrupt when the count value in the counter equals the 
* configured match value.
*
* Note that the counter is not reset on a match. In such case the WDT
* reset period is:
* WDT_Reset_Period = ILO_Period * (2*2^(16-IgnoreBits) + MatchValue);
* When the counter reaches a match value, it generates an interrupt and then 
* keeps counting up until it overflows and rolls back to zero and reaches the 
* match value again, at which point another interrupt is generated.
*
* To use a WDT to generate a periodic interrupt, the match value should be 
* incremented in the ISR. As a result, the next WDT interrupt is generated when 
* the counter reaches a new match value.
*
* You can also reduce the entire WDT counter period by 
* specifying the number of most significant bits that are ignored in the WDT 
* counter. For example, if the Cy_WDT_SetIgnoreBits() function is called with
* parameter 3, the WDT counter becomes a 13-bit free-running up-counter.
*
* <b> Power Modes </b> <br>
* WDT can operate in all possible low power modes. 
* Operation during Hibernate mode is possible because the logic and 
* high-voltage internal low oscillator (ILO) are supplied by the external 
* high-voltage supply (Vddd). The WDT can be configured to wake the device from 
* Hibernate mode.
*
* In Active or LPActive mode, an interrupt request from the WDT is sent to the 
* CPU via IRQ 22. In Sleep, LPSleep or Deep Sleep power mode, the CPU subsystem 
* is powered down, so the interrupt request from the WDT is sent directly to the
* WakeUp Interrupt Controller (WIC) which will then wake up the CPU. The 
* CPU then acknowledges the interrupt request and executes the ISR.
*
* <b> Clock Source </b> <br>
* The WDT is clocked by the ILO. The WDT must be disabled before disabling 
* the ILO. According to the device datasheet, the ILO accuracy is +/-30% over 
* voltage and temperature. This means that the timeout period may vary by 30% 
* from the configured value. Appropriate margins should be added while 
* configuring WDT intervals to make sure that unwanted device resets do not 
* occur on some devices.
* 
* Refer to the device datasheet for more information on the oscillator accuracy.
*
* <b> Register Locking </b> <br>
* You can prevent accidental corruption of the WDT configuration by calling 
* the Cy_WDT_Lock() function. When the WDT is locked, any writing to the WDT_*, 
* CLK_ILO_CONFIG, CLK_SELECT.LFCLK_SEL, and CLK_TRIM_ILO_CTL registers is 
* ignored.
* Call the Cy_WDT_Unlock() function to allow WDT registers modification.
*
* <b> Clearing WDT </b> <br>
* The ILO clock is asynchronous to the SysClk. Therefore it generally 
* takes three ILO cycles for WDT register changes to come into effect. It is 
* important to remember that a WDT should be cleared at least four cycles 
* (3 + 1 for sure) before a timeout occurs, especially when small 
* match values / low-toggle bit numbers are used.
*
* \warning It may happen that a WDT reset can be generated 
* faster than a device start-up. To prevent this, calculate the 
* start-up time and WDT reset time. The WDT reset time should be always greater
* than device start-up time.
*
* <b> Reset Detection </b> <br>
* Use the Cy_SysLib_GetResetReason() function to detect whether the WDT has 
* triggered a device reset.
*
* <b> Interrupt Configuration </b> <br>
* The Global Signal Reference and Interrupt components can be used for ISR 
* configuration. If the WDT is configured to generate an interrupt, pending 
* interrupts must be cleared within the ISR (otherwise, the interrupt will be 
* generated continuously).
* A pending interrupt to the WDT block must be cleared by calling the 
* Cy_WDT_ClearInterrupt() function. The call to the function will clear the 
* unhandled WDT interrupt counter.
*
* Use the WDT ISR as a timer to trigger certain actions 
* and to change a next WDT match value.
*
* Ensure that the interrupts from the WDT are passed to the CPU to avoid 
* unregistered interrupts. Unregistered WDT interrupts result in a continuous 
* device reset. To avoid this, call Cy_WDT_UnmaskInterrupt().
* After that, call the WDT API functions for interrupt
* handling/clearing.
*
* \section group_wdt_configuration Configuration Considerations
*
* To start the WDT, make sure that ILO is enabled.
* After the ILO is enabled, ensure that the WDT is unlocked and disabled by 
* calling the Cy_WDT_Unlock() and Cy_WDT_Disable() functions. Set the WDT match 
* value by calling Cy_WDT_SetMatch() with the required match value. If needed, 
* set the ignore bits for reducing the WDT counter period by calling 
* Cy_WDT_SetIgnoreBits() function. After the WDT configuration is set, 
* call Cy_WDT_Enable().
*
* \note Enable a WDT if the power supply can produce
* sudden brownout events that may compromise the CPU functionality. This 
* ensures that the system can recover after a brownout.
*
* When the WDT is used to protect against system crashes, the 
* WDT interrupt should be cleared by a portion of the code that is not directly 
* associated with the WDT interrupt.
* Otherwise, it is possible that the main firmware loop has crashed or is in an 
* endless loop, but the WDT interrupt vector continues to operate and service 
* the WDT. The user should:
* * Feed the watchdog by clearing the interrupt bit regularly in the main body 
* of the firmware code.
*
* * Guarantee that the interrupt is cleared at least once every WDT period.
*
* * Use the WDT ISR only as a timer to trigger certain actions and to change the
* next match value.
*
* \section group_wdt_section_more_information More Information
*
* For more information on the WDT peripheral, refer to the technical reference
* manual (TRM).
*
* \section group_wdt_MISRA MISRA-C Compliance
* The WDT driver does not have any specific deviations.
*
* \section group_wdt_changelog Changelog
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
*     <td>1.0.2</td>
*     <td>Minor documentation updates</td>
*     <td>Corrected info about a reset generation</td>
*   </tr>
*   <tr>
*     <td>1.0.1</td>
*     <td>General documentation updates</td>
*     <td>Added info about periodic interrupt generation use case</td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_wdt_macros Macros
* \defgroup group_wdt_functions Functions
*
*/

#if !defined(CY_WDT_H)
#define CY_WDT_H

#include <stdint.h>
#include <stdbool.h>
#include "cy_device_headers.h"
#include "cy_device.h"

#if defined(__cplusplus)
extern "C" {
#endif


/*******************************************************************************
*       Function Constants
*******************************************************************************/

/**
* \addtogroup group_wdt_macros
* \{
*/

/** The driver major version */
#define CY_WDT_DRV_VERSION_MAJOR                       1

/** The driver minor version */
#define CY_WDT_DRV_VERSION_MINOR                       10

/** The internal define for the first iteration of WDT unlocking */
#define CY_SRSS_WDT_LOCK_BIT0                           ((uint32_t)0x01U << 30U)

/** The internal define for the second iteration of WDT unlocking */
#define CY_SRSS_WDT_LOCK_BIT1                           ((uint32_t)0x01U << 31U)

/** The WDT default match value */
#define CY_SRSS_WDT_DEFAULT_MATCH_VALUE                 ((uint32_t) 4096U)

/** The default match value of the WDT ignore bits */
#define CY_SRSS_WDT_DEFAULT_IGNORE_BITS                 (0U)

/** The default match value of the WDT ignore bits */
#define CY_SRSS_WDT_LOCK_BITS                           (3U)

/** The WDT driver identifier */
#define CY_WDT_ID                                       CY_PDL_DRV_ID(0x34u)

/** \} group_wdt_macros */

/** \cond Internal */

/** The WDT maximum match value */
#define WDT_MAX_MATCH_VALUE                      (0xFFFFuL)

/** The WDT maximum match value */
#define WDT_MAX_IGNORE_BITS                      (0xFuL)

/* Internal macro to validate match value */
#define CY_WDT_IS_MATCH_VAL_VALID(match)         ((match) <= WDT_MAX_MATCH_VALUE)

/* Internal macro to validate match value */
#define CY_WDT_IS_IGNORE_BITS_VALID(bitsNum)     ((bitsNum) <= WDT_MAX_IGNORE_BITS)

/** \endcond */


/*******************************************************************************
*        Function Prototypes
*******************************************************************************/

/**
* \addtogroup group_wdt_functions
* @{
*/
/* WDT API */
void Cy_WDT_Init(void);
void Cy_WDT_Lock(void);
void Cy_WDT_Unlock(void);
void Cy_WDT_SetMatch(uint32_t match);
void Cy_WDT_SetIgnoreBits(uint32_t bitsNum);
void Cy_WDT_ClearInterrupt(void);
void Cy_WDT_ClearWatchdog(void);

__STATIC_INLINE void Cy_WDT_Enable(void);
__STATIC_INLINE void Cy_WDT_Disable(void);
__STATIC_INLINE uint32_t Cy_WDT_GetMatch(void);
__STATIC_INLINE uint32_t Cy_WDT_GetCount(void);
__STATIC_INLINE uint32_t Cy_WDT_GetIgnoreBits(void);
__STATIC_INLINE void Cy_WDT_MaskInterrupt(void);
__STATIC_INLINE void Cy_WDT_UnmaskInterrupt(void);


/*******************************************************************************
* Function Name: Cy_WDT_Enable
****************************************************************************//**
*
* Enables the Watchdog timer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_WDT_Enable(void)
{
    SRSS_WDT_CTL |= _VAL2FLD(SRSS_WDT_CTL_WDT_EN, 1u);
}


/*******************************************************************************
* Function Name: Cy_WDT_Disable
****************************************************************************//**
*
* Disables the Watchdog timer. The Watchdog timer should be unlocked before being
* disabled. Call the Cy_WDT_Unlock() API to unlock the WDT.
*
*******************************************************************************/
__STATIC_INLINE void Cy_WDT_Disable(void)
{
    SRSS_WDT_CTL &= ((uint32_t) ~(_VAL2FLD(SRSS_WDT_CTL_WDT_EN, 1u)));
}


/*******************************************************************************
* Function Name: Cy_WDT_GetMatch
****************************************************************************//**
*
* Reads the WDT counter match comparison value.
*
* \return The counter match value.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_WDT_GetMatch(void)
{
    return ((uint32_t) _FLD2VAL(SRSS_WDT_MATCH_MATCH, SRSS_WDT_MATCH));
}


/*******************************************************************************
* Function Name: Cy_WDT_GetCount
****************************************************************************//**
*
* Reads the current WDT counter value.
*
* \return A live counter value.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_WDT_GetCount(void)
{
    return ((uint32_t) _FLD2VAL(SRSS_WDT_CNT_COUNTER, SRSS_WDT_CNT));
}


/*******************************************************************************
* Function Name: Cy_WDT_GetIgnoreBits
****************************************************************************//**
*
* Reads the number of the most significant bits of the Watchdog timer that are 
* not checked against the match.
*
* \return The number of the most significant bits.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_WDT_GetIgnoreBits(void)
{
    return((uint32_t) _FLD2VAL(SRSS_WDT_MATCH_IGNORE_BITS ,SRSS_WDT_MATCH));
}


/*******************************************************************************
* Function Name: Cy_WDT_MaskInterrupt
****************************************************************************//**
*
* After masking interrupts from the WDT, they are not passed to the CPU.
* This function does not disable the WDT-reset generation.
*
*******************************************************************************/
__STATIC_INLINE void Cy_WDT_MaskInterrupt(void)
{
    SRSS_SRSS_INTR_MASK &= (uint32_t)(~ _VAL2FLD(SRSS_SRSS_INTR_MASK_WDT_MATCH, 1u));
}


/*******************************************************************************
* Function Name: Cy_WDT_UnmaskInterrupt
****************************************************************************//**
*
* After unmasking interrupts from the WDT, they are passed to CPU.
* This function does not impact the reset generation.
*
*******************************************************************************/
__STATIC_INLINE void Cy_WDT_UnmaskInterrupt(void)
{
    SRSS_SRSS_INTR_MASK |= _VAL2FLD(SRSS_SRSS_INTR_MASK_WDT_MATCH, 1u);
}
/** \} group_wdt_functions */

#if defined(__cplusplus)
}
#endif

#endif /* CY_WDT_H */

/** \} group_wdt */


/* [] END OF FILE */
