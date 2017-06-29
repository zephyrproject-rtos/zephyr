/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** ============================================================================
 *  @file       DebugP.h
 *
 *  @brief      Debug support
 *
 *  The DebugP module allows application to do logging and assert checking.
 *
 *  DebugP_assert calls can be added into code. If the code
 *  is compiled with the compiler define DebugP_ASSERT_ENABLED set to a
 *  non-zero value, the call is passed onto the underlying assert checking.
 *  If DebugP_ASSERT_ENABLED is zero (or not defined), the calls are
 *  resolved to nothing.
 *
 *  This module sits on top of the assert checking of the underlying
 *  RTOS. Please refer to the underlying RTOS port implementation for
 *  more details.
 *
 *  Similarly, DebugP_logN calls can be added into code. If the code
 *  is compiled with the compiler define DebugP_LOG_ENABLED set to a
 *  non-zero value, the call is passed onto the underlying assert checking.
 *  If DebugP_LOG_ENABLED is zero (or not defined), the calls are
 *  resolved to nothing.

 *  This module sits on top of the logging of the underlying
 *  RTOS. Please refer to the underlying RTOS port implementation for
 *  more details.
 *
 *  ============================================================================
 */

#ifndef ti_dpl_DebugP__include
#define ti_dpl_DebugP__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef DebugP_ASSERT_ENABLED
#define DebugP_ASSERT_ENABLED 0
#endif

#ifndef DebugP_LOG_ENABLED
#define DebugP_LOG_ENABLED    0
#endif

#if DebugP_ASSERT_ENABLED
extern void _DebugP_assert(int expression, const char *file, int line);
/*!
 *  @brief  Assert checking function
 *
 *  If the expression is evaluated to true, the API does nothing.
 *  If it is evaluated to false, the underlying RTOS port implementation
 *  handles the assert via its mechanisms.
 *
 *  @param  expression Expression to evaluate
 */
#define DebugP_assert(expression) (_DebugP_assert(expression,      \
                                                  __FILE__, __LINE__))
#else
#define DebugP_assert(expression)
#endif

#if DebugP_LOG_ENABLED
/*!
 *  @brief  Debug log function with 0 parameters
 *
 *  The underlying RTOS port implementation handles the
 *  logging via its mechanisms.
 *
 *  @param  format "printf" format string
 */
extern void DebugP_log0(const char *format);

/*!
 *  @brief  Debug log function with 1 parameters
 *
 *  The underlying RTOS port implementation handles the
 *  logging via its mechanisms.
 *
 *  @param  format "printf" format string
 *  @param  p1 first parameter to format string
 */
extern void DebugP_log1(const char *format, uintptr_t p1);

/*!
 *  @brief  Debug log function with 2 parameters
 *
 *  The underlying RTOS port implementation handles the
 *  logging via its mechanisms.
 *
 *  @param  format "printf" format string
 *  @param  p1 first parameter to format string
 *  @param  p2 second parameter to format string
 */
extern void DebugP_log2(const char *format, uintptr_t p1, uintptr_t p2);

/*!
 *  @brief  Debug log function with 3 parameters
 *
 *  The underlying RTOS port implementation handles the
 *  logging via its mechanisms.
 *
 *  @param  format "printf" format string
 *  @param  p1 first parameter to format string
 *  @param  p2 second parameter to format string
 *  @param  p3 third parameter to format string
 */
extern void DebugP_log3(const char *format, uintptr_t p1, uintptr_t p2, uintptr_t p3);

/*!
 *  @brief  Debug log function with 4 parameters
 *
 *  The underlying RTOS port implementation handles the
 *  logging via its mechanisms.
 *
 *  @param  format "printf" format string
 *  @param  p1 first parameter to format string
 *  @param  p2 second parameter to format string
 *  @param  p3 third parameter to format string
 *  @param  p4 fourth parameter to format string
 */
extern void DebugP_log4(const char *format, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4);
#else
#define DebugP_log0(format)
#define DebugP_log1(format, p1)
#define DebugP_log2(format, p1, p2)
#define DebugP_log3(format, p1, p2, p3)
#define DebugP_log4(format, p1, p2, p3, p4)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ti_dpl_DebugP__include */
