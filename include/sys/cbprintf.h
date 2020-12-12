/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_CBPRINTF_H_
#define ZEPHYR_INCLUDE_SYS_CBPRINTF_H_

#include <stdarg.h>
#include <stddef.h>
#include <toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup cbprintf_apis Formatted Output APIs
 * @ingroup support_apis
 * @{
 */

/** @brief Signature for a cbprintf callback function.
 *
 * This function expects two parameters:
 *
 * * @p c a character to output.  The output behavior should be as if
 *   this was cast to an unsigned char.
 * * @p ctx a pointer to an object that provides context for the
 *   output operation.
 *
 * The declaration does not specify the parameter types.  This allows a
 * function like @c fputc to be used without requiring all context pointers to
 * be to a @c FILE object.
 *
 * @return the value of @p c cast to an unsigned char then back to
 * int, or a negative error code that will be returned from
 * cbprintf().
 */
typedef int (*cbprintf_cb)(/* int c, void *ctx */);

/** @brief *printf-like output through a callback.
 *
 * This is essentially printf() except the output is generated
 * character-by-character using the provided @p out function.  This allows
 * formatting text of unbounded length without incurring the cost of a
 * temporary buffer.
 *
 * All formatting specifiers of C99 are recognized, and most are supported if
 * the functionality is enabled.
 *
 * @note The functionality of this function is significantly reduced
 * when `CONFIG_CBPRINTF_NANO` is selected.
 *
 * @param out the function used to emit each generated character.
 *
 * @param ctx context provided when invoking out
 *
 * @param format a standard ISO C format string with characters and conversion
 * specifications.
 *
 * @param ... arguments corresponding to the conversion specifications found
 * within @p format.
 *
 * @return the number of characters printed, or a negative error value
 * returned from invoking @p out.
 */
__printf_like(3, 4)
int cbprintf(cbprintf_cb out, void *ctx, const char *format, ...);

/** @brief varargs-aware *printf-like output through a callback.
 *
 * This is essentially vsprintf() except the output is generated
 * character-by-character using the provided @p out function.  This allows
 * formatting text of unbounded length without incurring the cost of a
 * temporary buffer.
 *
 * @note This function is available only when `CONFIG_CBPRINTF_LIBC_SUBSTS` is
 * selected.
 *
 * @note The functionality of this function is significantly reduced when
 * `CONFIG_CBPRINTF_NANO` is selected.
 *
 * @param out the function used to emit each generated character.
 *
 * @param ctx context provided when invoking out
 *
 * @param format a standard ISO C format string with characters and conversion
 * specifications.
 *
 * @param ap a reference to the values to be converted.
 *
 * @return the number of characters generated, or a negative error value
 * returned from invoking @p out.
 */
int cbvprintf(cbprintf_cb out, void *ctx, const char *format, va_list ap);

/** @brief snprintf using Zephyrs cbprintf infrastructure.
 *
 * @note The functionality of this function is significantly reduced
 * when `CONFIG_CBPRINTF_NANO` is selected.
 *
 * @param str where the formatted content should be written
 *
 * @param size maximum number of chaacters for the formatted output,
 * including the terminating null byte.
 *
 * @param format a standard ISO C format string with characters and
 * conversion specifications.
 *
 * @param ... arguments corresponding to the conversion specifications found
 * within @p format.
 *
 * return The number of characters that would have been written to @p
 * str, excluding the terminating null byte.  This is greater than the
 * number actually written if @p size is too small.
 */
__printf_like(3, 4)
int snprintfcb(char *str, size_t size, const char *format, ...);

/** @brief vsnprintf using Zephyrs cbprintf infrastructure.
 *
 * @note This function is available only when `CONFIG_CBPRINTF_LIBC_SUBSTS` is
 * selected.
 *
 * @note The functionality of this function is significantly reduced when
 * `CONFIG_CBPRINTF_NANO` is selected.
 *
 * @param str where the formatted content should be written
 *
 * @param size maximum number of chaacters for the formatted output, including
 * the terminating null byte.
 *
 * @param format a standard ISO C format string with characters and conversion
 * specifications.
 *
 * @param ... arguments corresponding to the conversion specifications found
 * within @p format.
 *
 * @param ap a reference to the values to be converted.
 *
 * return The number of characters that would have been written to @p
 * str, excluding the terminating null byte.  This is greater than the
 * number actually written if @p size is too small.
 */
int vsnprintfcb(char *str, size_t size, const char *format, va_list ap);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_CBPRINTF_H_ */
