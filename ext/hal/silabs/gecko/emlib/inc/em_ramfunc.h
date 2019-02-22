/***************************************************************************//**
 * @file em_ramfunc.h
 * @brief RAM code support.
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

#ifndef EM_RAMFUNC_H
#define EM_RAMFUNC_H

#ifdef __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */
/***************************************************************************//**
 * @addtogroup emlib
 * @{
 * @addtogroup RAMFUNC
 * @brief RAM code support
 * @details
 *  Provides support for executing code from RAM.
 *  Provides a unified method to manage RAM code across all supported tools.
 * @{

  @note
   Other cross-compiler support macros are implemented in @ref COMMON.

  @note
    Functions executing from RAM should not be declared as static.

  @warning
    Standard library facilities are available to the tool with GCC in hosted
	mode (default), regardless of the section attribute. Calls to standard
    libraries placed in the default section may therefore occur. To disable
    hosted mode, add '-ffreestanding' to the build command line. This is the
    only way to guarantee no calls to standard libraries with GCC.
    Read more at https://gcc.gnu.org/onlinedocs/gcc-5.3.0/gcc/Standards.html

  @warning
    Keil/ARM uVision users must add a section named "ram_code" in their linker
    scatter file. This section must be in RAM memory. Look in the MCU SDK for
    example scatter files (ram_code.sct).

  @n @section ramfunc_usage Usage

  In your .h file:
  @verbatim
  #include "em_ramfunc.h"

  SL_RAMFUNC_DECLARATOR
  void MyPrint(const char* string);
  @endverbatim

  Issues have been observed with armgcc when there is no declarator. It is
  recommended to have a declarator also for internal functions, but move the
  declarator to the .c file.

  In your .c file:
  @verbatim
  #include "em_ramfunc.h"

  SL_RAMFUNC_DEFINITION_BEGIN
  void MyPrint(const char* string)
  {
  ...
  }
  SL_RAMFUNC_DEFINITION_END
  @endverbatim

 ******************************************************************************/
/* *INDENT-ON* */

/*******************************************************************************
 ******************************   DEFINES    ***********************************
 ******************************************************************************/

/**
 * @brief
 *    This define is not present by default. By compiling with define
 *    @ref SL_RAMFUNC_DISABLE, code placed in RAM by SL_RAMFUNC macros
 *    will be placed in default code space (Flash) instead.
 *
 * @note
 *    This define is not present by default.
 */
#if defined(DOXY_DOC_ONLY)
#define SL_RAMFUNC_DISABLE
#endif

#if defined(SL_RAMFUNC_DISABLE)
/** @brief Compiler ported function declarator for RAM code. */
#define SL_RAMFUNC_DECLARATOR

/** @brief Compiler ported function definition begin marker for RAM code. */
#define SL_RAMFUNC_DEFINITION_BEGIN

/** @brief Compiler ported function definition end marker for RAM code. */
#define SL_RAMFUNC_DEFINITION_END

#elif defined(__CC_ARM)
/* MDK-ARM compiler */
#define SL_RAMFUNC_DECLARATOR
#define SL_RAMFUNC_DEFINITION_BEGIN    _Pragma("arm section code=\"ram_code\"")
#define SL_RAMFUNC_DEFINITION_END      _Pragma("arm section code")

#elif defined(__ICCARM__)
/* IAR Embedded Workbench */
#define SL_RAMFUNC_DECLARATOR          __ramfunc
#define SL_RAMFUNC_DEFINITION_BEGIN    SL_RAMFUNC_DECLARATOR
#define SL_RAMFUNC_DEFINITION_END

#elif defined(__GNUC__) && defined(__CROSSWORKS_ARM)
/* Rowley Crossworks */
#define SL_RAMFUNC_DECLARATOR          __attribute__ ((section(".fast")))
#define SL_RAMFUNC_DEFINITION_BEGIN    SL_RAMFUNC_DECLARATOR
#define SL_RAMFUNC_DEFINITION_END

#elif defined(__GNUC__)
/* Simplicity Studio, Atollic and Vanilla armgcc */
#define SL_RAMFUNC_DECLARATOR          __attribute__ ((section(".ramfunc")))
#define SL_RAMFUNC_DEFINITION_BEGIN    SL_RAMFUNC_DECLARATOR
#define SL_RAMFUNC_DEFINITION_END

#endif

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/* Deprecated macro names and user configuration */
#if defined(RAMFUNC_DISABLE)
#define SL_RAMFUNC_DISABLE
#endif

#define RAMFUNC_DECLARATOR          SL_RAMFUNC_DECLARATOR
#define RAMFUNC_DEFINITION_BEGIN    SL_RAMFUNC_DEFINITION_BEGIN
#define RAMFUNC_DEFINITION_END      SL_RAMFUNC_DEFINITION_END
/** @endcond */

/** @} (end addtogroup RAMFUNC) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* EM_RAMFUNC_H */
