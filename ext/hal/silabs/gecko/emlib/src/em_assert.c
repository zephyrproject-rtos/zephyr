/***************************************************************************//**
 * @file em_assert.c
 * @brief Assert API
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2016 Silicon Laboratories, Inc. www.silabs.com</b>
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

#include "em_assert.h"
#include <stdbool.h>

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup ASSERT
 * @details
 *  This module contains functions to control the ASSERT peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs.
 * @{
 ******************************************************************************/

#if defined(DEBUG_EFM)
/***************************************************************************//**
 * @brief
 *   EFM internal assert handling.
 *
 *   This function is invoked through EFM_ASSERT() macro usage only and should
 *   not be used explicitly.
 *
 *   This implementation enters an indefinite loop, allowing
 *   the use of a debugger to determine a cause of failure. By defining
 *   DEBUG_EFM_USER to the preprocessor for all files, a user-defined version
 *   of this function must be defined and will be invoked instead, possibly
 *   providing output of assertion location.
 *
 * @note
 *   This function is not used unless @ref DEBUG_EFM is defined
 *   during preprocessing of EFM_ASSERT() usage.
 *
 * @param[in] file
 *   Name of the source file where assertion failed.
 *
 * @param[in] line
 *   A line number in the source file where assertion failed.
 ******************************************************************************/
void assertEFM(const char *file, int line)
{
  (void)file;  /* Unused parameter */
  (void)line;  /* Unused parameter */

  while (true) {
  }
}
#endif /* DEBUG_EFM */

/** @} (end addtogroup ASSERT) */
/** @} (end addtogroup emlib) */
