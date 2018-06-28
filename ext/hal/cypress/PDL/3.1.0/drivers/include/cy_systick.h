/***************************************************************************//**
* \file cy_systick.h
* \version 1.10
*
* Provides the API declarations of the SysTick driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CY_SYSTICK_H_
#define _CY_SYSTICK_H_

/**
* \defgroup group_arm_system_timer ARM System Timer (SysTick)
* \{
* Provides vendor-specific SysTick API.
*
* The SysTick timer is part of the CPU. The timer is a down counter with a 24-bit reload/tick value that is clocked by
* the FastClk/SlowClk. The timer has the capability to generate an interrupt when the set number of ticks expires and
* the counter is reloaded. This interrupt is available as part of the Nested Vectored Interrupt Controller (NVIC) for
* service by the CPU and can be used for general-purpose timing control in user code.
*
* The timer is independent of the CPU (except for the clock), which is useful in applications requiring
* precise timing that do not have a dedicated timer/counter available for the job.
*
* \section group_systick_configuration Configuration Considerations
*
* The \ref Cy_SysTick_Init() performs all required driver's initialization and enables the timer. The function accepts
* two parameters: clock source \ref cy_en_systick_clock_source_t and the timer interval. You must ensure
* the selected clock source for SysTick is enabled.
* The callbacks can be registered/unregistered any time after \ref Cy_SysTick_Init() by calling
* \ref Cy_SysTick_SetCallback().
*
* Changing the SysTick clock source and/or its frequency will change the interrupt interval and therefore
* \ref Cy_SysTick_SetReload() should be called to compensate for this change.
*
* \section group_systick_more_information More Information
*
* Refer to the SysTick section of the ARM reference guide for complete details on the registers and their use.
* See also the "CPU Subsystem (CPUSS)" chapter of the device technical reference manual (TRM).
*
* \section group_systick_MISRA MISRA-C Compliance
*
* <table class="doxtable">
*   <tr>
*       <th>MISRA Rule</th>
*       <th>Rule Class (Required/Advisory)</th>
*       <th>Rule Description</th>
*       <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*       <td>8.12</td>
*       <td>Required</td>
*       <td>When an array is declared with external linkage, its size shall be
*           stated explicitly or defined implicitly by initialization.</td>
*       <td>The warning is related to the __ramVectors symbol defined in the assembly startup code.
*           It's size is device-specific and unknown to the SysTick driver.</td>
*   </tr>
* </table>
*
* \section group_systick_changelog Changelog
*
* <table class="doxtable">
* <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
* <tr>
*   <td>1.10</td>
*   <td>Flattened the driver source code organization into the single
*       source directory and the single include directory
*   </td>
*   <td>Simplified the driver library directory structure</td>
* </tr>
* <tr>
* <td>1.0.1</td>
* <td>Fixed a warning issued when the compilation of C++ source code was
*     enabled.</td> 
* <td></td>
* </tr>
* <tr>
* <td>1.0</td>
* <td>Initial version</td>
* <td></td>
* </tr>
* </table>
*
* \defgroup group_systick_macros Macros
* \defgroup group_systick_functions Functions
* \defgroup group_systick_data_structures Data Structures
*/

#include <stdint.h>
#include "cy_syslib.h"
#include "cy_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \cond */
extern cy_israddress __ramVectors[];
typedef void (*Cy_SysTick_Callback)(void);
/** \endcond */

/**
* \addtogroup group_systick_data_structures
* \{
*/
/** SysTick clocks sources */
typedef enum
{
    CY_SYSTICK_CLOCK_SOURCE_CLK_LF    = 0u,     /**< The low frequency clock clk_lf is selected. */
    CY_SYSTICK_CLOCK_SOURCE_CLK_IMO   = 1u,     /**< The internal main oscillator (IMO) clock clk_imo is selected. */
    CY_SYSTICK_CLOCK_SOURCE_CLK_ECO   = 2u,     /**< The external crystal oscillator (ECO) clock clk_eco is selected. */
    CY_SYSTICK_CLOCK_SOURCE_CLK_TIMER = 3u,     /**< The SRSS clk_timer is selected. */
    CY_SYSTICK_CLOCK_SOURCE_CLK_CPU   = 4u,     /**< The CPU clock is selected. */
} cy_en_systick_clock_source_t;

/** \} group_systick_data_structures */


/**
* \addtogroup group_systick_functions
* \{
*/

void Cy_SysTick_Init(cy_en_systick_clock_source_t clockSource, uint32_t interval);
void Cy_SysTick_Enable(void);
void Cy_SysTick_Disable(void);
Cy_SysTick_Callback Cy_SysTick_SetCallback(uint32_t number, Cy_SysTick_Callback function);
Cy_SysTick_Callback Cy_SysTick_GetCallback(uint32_t number);
void Cy_SysTick_SetClockSource(cy_en_systick_clock_source_t clockSource);
cy_en_systick_clock_source_t Cy_SysTick_GetClockSource(void);
__STATIC_INLINE void Cy_SysTick_EnableInterrupt(void);
__STATIC_INLINE void Cy_SysTick_DisableInterrupt(void);
__STATIC_INLINE void Cy_SysTick_SetReload(uint32_t value);
__STATIC_INLINE uint32_t Cy_SysTick_GetReload(void);
__STATIC_INLINE uint32_t Cy_SysTick_GetValue(void);
__STATIC_INLINE uint32_t Cy_SysTick_GetCountFlag(void);
__STATIC_INLINE void Cy_SysTick_Clear(void);

/** \} group_systick_functions */


/**
* \addtogroup group_systick_macros
* \{
*/

/** Driver major version */
#define SYSTICK_DRV_VERSION_MAJOR       1

/** Driver minor version */
#define SYSTICK_DRV_VERSION_MINOR       10

/** Number of the callbacks assigned to the SysTick interrupt */
#define CY_SYS_SYST_NUM_OF_CALLBACKS         (5u)

/** \} group_systick_macros */


/** \cond */
/** Interrupt number in the vector table */
#define CY_SYSTICK_IRQ_NUM                   (15u)
/** \endcond */

/**
* \addtogroup group_systick_functions
* \{
*/

/*******************************************************************************
* Function Name: Cy_SysTick_EnableInterrupt
****************************************************************************//**
*
* Enables the SysTick interrupt.
*
* \sideeffect Clears the SysTick count flag if it was set
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysTick_EnableInterrupt(void)
{
    SYSTICK_CTRL = SYSTICK_CTRL | SysTick_CTRL_TICKINT_Msk;
}


/*******************************************************************************
* Function Name: Cy_SysTick_DisableInterrupt
****************************************************************************//**
*
* Disables the SysTick interrupt.
*
* \sideeffect Clears the SysTick count flag if it was set
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysTick_DisableInterrupt(void)
{
    SYSTICK_CTRL = SYSTICK_CTRL & ~SysTick_CTRL_TICKINT_Msk;
}


/*******************************************************************************
* Function Name: Cy_SysTick_SetReload
****************************************************************************//**
*
* Sets the value the counter is set to on a startup and after it reaches zero.
* This function does not change or reset the current sysTick counter value, so
* it should be cleared using the Cy_SysTick_Clear() API.
*
* \param value: The valid range is [0x0-0x00FFFFFF]. The counter reset value.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysTick_SetReload(uint32_t value)
{
    SYSTICK_LOAD = (value & SysTick_LOAD_RELOAD_Msk);
}


/*******************************************************************************
* Function Name: Cy_SysTick_GetReload
****************************************************************************//**
*
* Gets the value the counter is set to on a startup and after it reaches zero.
*
* \return The counter reset value.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SysTick_GetReload(void)
{
    return (SYSTICK_LOAD);
}


/*******************************************************************************
* Function Name: Cy_SysTick_GetValue
****************************************************************************//**
*
* Gets the current SysTick counter value.
*
* \return The current SysTick counter value.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SysTick_GetValue(void)
{
    return (SYSTICK_VAL);
}

/*******************************************************************************
* Function Name: Cy_SysTick_Clear
****************************************************************************//**
*
* Clears the SysTick counter for a well-defined startup.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysTick_Clear(void)
{
    SYSTICK_VAL = 0u;
}


/*******************************************************************************
* Function Name: Cy_SysTick_GetCountFlag
****************************************************************************//**
*
* Gets the values of the count flag. The count flag is set once the SysTick
* counter reaches zero. The flag is cleared on read.
*
* \return Returns a non-zero value if a flag is set; otherwise a zero is
* returned.
*
* \sideeffect Clears the SysTick count flag if it was set.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SysTick_GetCountFlag(void)
{
    return (SYSTICK_CTRL & SysTick_CTRL_COUNTFLAG_Msk);
}


/** \} group_systick_functions */

#ifdef __cplusplus
}
#endif

#endif /* _CY_SYSTICK_H_ */

/** \} group_systick */


/* [] END OF FILE */
