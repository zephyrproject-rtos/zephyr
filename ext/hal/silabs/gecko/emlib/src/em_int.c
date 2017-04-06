/**************************************************************************//**
 * @file em_int.c
 * @brief Interrupt enable/disable unit API
 * @version 5.1.2
 ******************************************************************************
 * @section License
 * <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/

#include <stdint.h>
#include "em_int.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup INT
 * @brief Safe nesting of interrupt disable/enable API
 * @{
 * @deprecated
 *   These functions are deprecated and marked for removal in a later release.
 *   Please use the @ref CORE module instead. See @ref core_porting for
 *   information on how to convert existing code bases to use @ref CORE.
 *
 * @details
 *  This module contains functions to safely disable and enable interrupts
 *  at CPU level. INT_Disable() disables interrupts globally and increments a lock
 *  level counter (counting semaphore). INT_Enable() decrements the lock level
 *  counter and enable interrupts if the counter reaches zero.
 *
 *  These functions would normally be used to secure critical regions, and
 *  to make sure that a critical section that calls into another critical
 *  section does not unintentionally terminate the callee critical section.
 *
 *  These functions should also be used inside interrupt handlers:
 *  @verbatim
 *  void SysTick_Handler(void)
 *  {
 *    INT_Disable();
 *      .
 *      .
 *      .
 *    INT_Enable();
 *  }
 * @endverbatim
 ******************************************************************************/

/** Interrupt lock level counter. Set to zero initially as we normally enter
 * main with interrupts enabled  */
uint32_t INT_LockCnt = 0;

/** @} (end addtogroup INT) */
/** @} (end addtogroup emlib) */
