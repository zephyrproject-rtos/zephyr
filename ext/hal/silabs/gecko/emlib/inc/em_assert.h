/***************************************************************************//**
 * @file em_assert.h
 * @brief Emlib peripheral API "assert" implementation.
 * @version 5.1.2
 *******************************************************************************
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

#ifndef EM_ASSERT_H
#define EM_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

#if defined(DOXY_DOC_ONLY)
/** @brief Included for documentation purposes only. This define is not present by default.
  * @ref DEBUG_EFM should be defined from the compiler to enable the default internal
  * assert handler. */
#define DEBUG_EFM
/** @endcond */
#endif

#if defined(DEBUG_EFM) || defined(DEBUG_EFM_USER)
/***************************************************************************//**
 * @addtogroup ASSERT
 * @brief Error checking module.
 * @details
 * By default, EMLIB library assert usage is not included in order to reduce
 * footprint and processing overhead. Further, EMLIB assert usage is decoupled
 * from ISO C assert handling (NDEBUG usage), to allow a user to use ISO C
 * assert without including EMLIB assert statements.
 *
 * Below are available defines for controlling EMLIB assert inclusion. The defines
 * are typically defined for a project to be used by the preprocessor.
 *
 * @li If @ref DEBUG_EFM is defined, the internal EMLIB library assert handling will
 * be used. This is implemented as a simple while(true) loop. @ref DEBUG_EFM is not
 * defined by default.
 *
 * @li If DEBUG_EFM_USER is defined instead, the user must provide their own
 * implementation of the assertEFM() function.
 *
 * @li If both @ref DEBUG_EFM and DEBUG_EFM_USER are undefined then all EFM_ASSERT()
 * statements are no operation.
 *
 * @note
 * The internal EMLIB assert is documented here because @ref DEBUG_EFM is defined in
 * the doxygen configuration.
 * @{
 ******************************************************************************/
/* Due to footprint considerations, we only pass file name and line number, */
/* not the assert expression (nor function name (C99)) */
void assertEFM(const char *file, int line);
#define EFM_ASSERT(expr)    ((expr) ? ((void)0) : assertEFM(__FILE__, __LINE__))

#else

/** Default assertion is no operation */
#define EFM_ASSERT(expr)    ((void)(expr))

#endif /* defined(DEBUG_EFM) || defined(DEBUG_EFM_USER) */

/** @} (end addtogroup ASSERT) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* EM_ASSERT_H */
