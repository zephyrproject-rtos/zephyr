/***************************************************************************//**
 * @file em_common.h
 * @brief General purpose utilities.
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
#ifndef EM_COMMON_H
#define EM_COMMON_H

#include "em_device.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @defgroup emlib EMLIB
 * @brief Low-level peripheral library
 * @details
 *  EMLIB is a low-level peripheral support library that provides a unified API for
 *  all EFM32, EZR32 and EFR32 MCUs and SoCs from Silicon Laboratories.
 *
 *  EMLIB modules are provided for all peripherals and core features. The library
 *  implements no interrupt handlers. Static data, critical sections and module
 *  interdependencies are kept at a minimum.
 *
 * @note
 *  EMLIB functions assert on error if DEBUG_EFM is defined. See @ref ASSERT
 *  for more information on error handling and default assertion and how to
 *  implement a custom handler.
 *
 * @n
 * @note
 *  EMLIB does not implement support for radio features. Please refer to stack
 *  documentation for more information on RF support.
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup COMMON
 * @brief General purpose utilities and cross-compiler support.
 * @details
 *  This SDK supports the following compilers/IDEs:
 *  @li Simplicity Studio
 *  @li IAR Embedded Workbench
 *  @li Atollic TrueSTUDIO IDE
 *  @li Rowley Associates CrossWorks for ARM
 *  @li Keil µVision IDE
 *  @li Plain armgcc
 *
 * Certain compiler features such as alignment is implemented differently in the tools.
 * Therefore, macros such as @ref SL_ALIGN are provided to enable compiler independent
 * code.
 *
 * @note RAM code macros are implemented in a separate module @ref RAMFUNC.
 * Cross-compiler RAM code support needs extended documentation and it is therefore
 * implemented as a separate module.
 *
 * @{
 ******************************************************************************/

#if !defined(__GNUC__)
/* Not GCC compilers */

/** @brief Macro for getting minimum value. */
#define SL_MIN(a, b) ((a) < (b) ? (a) : (b))

/** @brief Macro for getting maximum value. */
#define SL_MAX(a, b) ((a) > (b) ? (a) : (b))

/** @brief Macros for handling packed structs. */
#define STRINGIZE(X) #X
#define SL_PACK_START(X) _Pragma(STRINGIZE(pack(X)))
#define SL_PACK_END()    _Pragma("pack()")
#define SL_ATTRIBUTE_PACKED

#if defined(__CC_ARM)
/** @brief MDK-ARM compiler: Macros for handling aligned structs. */
#define SL_ALIGN(X) __align(X)

/** MDK-ARM compiler: Macro for handling weak symbols. */
#define SL_WEAK __attribute__ ((weak))

/** MDK-ARM compiler: Macro for handling non-returning functions. */
#define SL_NORETURN __attribute__ ((noreturn))

/** MDK-ARM compiler: Macro for handling section placement */
#define SL_ATTRIBUTE_SECTION(X) __attribute__ ((section(X)))
#endif

#if defined(__ICCARM__)
/** @brief IAR Embedded Workbench: Macros for handling aligned structs. */
#define SL_ALIGN(X) _Pragma(STRINGIZE(data_alignment=X))

/** @brief IAR Embedded Workbench: Macros for handling weak symbols. */
#define SL_WEAK __weak

/** @brief IAR Embedded Workbench: Macro for handling non-returning functions. */
#define SL_NORETURN __noreturn

/** IAR Embedded Workbench: Macro for handling section placement */
#define SL_ATTRIBUTE_SECTION(X) @ X
#endif

#define SL_ATTRIBUTE_ALIGN(X)

#else // !defined(__GNUC__)
/* GCC compilers */

/** @brief Macro for getting minimum value. No sideeffects, a and b are evaluated once only. */
#define SL_MIN(a, b) __extension__({__typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b;})

/** @brief Macro for getting maximum value. No sideeffects, a and b are evaluated once only. */
#define SL_MAX(a, b) __extension__({__typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b;})

/** @brief GCC style macro for handling packed structs. */
#define SL_ATTRIBUTE_PACKED __attribute__ ((packed))

/** @brief Macro for handling packed structs.
 *  @n Use this macro before the struct definition.
 *  @n X denotes the maximum alignment of struct members. X is not supported with
 *  GCC. GCC always use 1 byte maximum alignment.
 */
#define SL_PACK_START(x)

/** @brief Macro for handling packed structs.
 *  @n Use this macro after the struct definition.
 *  @n With GCC, add SL_ATTRIBUTE_PACKED after the closing } of the struct
 *  definition.
 */
#define SL_PACK_END()

/** @brief GCC style macro for aligning a variable. */
#define SL_ATTRIBUTE_ALIGN(X) __attribute__ ((aligned(X)))

/** @brief Macro for aligning a variable.
 *  @n Use this macro before the variable definition.
 *  @n X denotes the storage alignment value in bytes.
 *  @n To be gcc compatible use SL_ATTRIBUTE_ALIGN(X) before the ; on normal
 *  variables. Use SL_ATTRIBUTE_ALIGN(X) before the opening { on struct variables.
 */
#define SL_ALIGN(X)

/** @brief Macro for defining a weak symbol. */
#define SL_WEAK __attribute__ ((weak))

/** @brief Macro for handling non-returning functions. */
#define SL_NORETURN __attribute__ ((noreturn))

/** Macro for placing a variable in a section.
 *  @n Use this macro after the variable definition, before the = or ;.
 *  @n X denotes the section to place the variable in.
 */
#define SL_ATTRIBUTE_SECTION(X) __attribute__ ((section(X)))

#endif // !defined(__GNUC__)

/***************************************************************************//**
 * @brief
 *   Count trailing number of zeros. Use CLZ instruction if available.
 *
 * @param[in] value
 *   Data value to check for number of trailing zero bits.
 *
 * @return
 *   Number of trailing zeros in value.
 ******************************************************************************/
__STATIC_INLINE uint32_t SL_CTZ(uint32_t value)
{
#if (__CORTEX_M >= 3)
  return __CLZ(__RBIT(value));

#else
  uint32_t zeros;
  for(zeros=0; (zeros<32) && ((value&0x1) == 0); zeros++, value>>=1);
  return zeros;
#endif
}


/* Deprecated function. New code should use @ref SL_CTZ. */
__STATIC_INLINE uint32_t EFM32_CTZ(uint32_t value)
{
  return SL_CTZ(value);
}


/** @} (end addtogroup COMMON) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* EM_COMMON_H */
