/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_CBPRINTF_H_
#define ZEPHYR_INCLUDE_SYS_CBPRINTF_H_

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <toolchain.h>

#ifdef CONFIG_CBPRINTF_LIBC_SUBSTS
#include <stdio.h>
#endif /* CONFIG_CBPRINTF_LIBC_SUBSTS */

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

/** @brief Capture state required to output formatted data later.
 *
 * Like cbprintf() but instead of processing the arguments and emitting the
 * formatted results immediately all arguments are captured so this can be
 * done in a different context, e.g. when the output function can block.
 *
 * In addition to the values extracted from arguments this will ensure that
 * copies are made of the necessary portions of any string parameters that are
 * not confirmed to be stored in read-only memory (hence assumed to be safe to
 * refer to directly later).
 *
 * @param packaged pointer to where the packaged data can be stored.  Pass a
 * null pointer to store nothing but still calculate the total space required.
 * The data stored here is relocatable, that is it can be moved to another
 * contiguous block of memory. The pointer must be aligned to a multiple of
 * the largest element in the argument list.
 *
 * @param len this must be set to the number of bytes available at @p packaged.
 * Ignored if @p packaged is NULL.
 *
 * @param format a standard ISO C format string with characters and conversion
 * specifications.
 *
 * @param ... arguments corresponding to the conversion specifications found
 * within @p format.
 *
 * @retval nonegative the number of bytes successfully stored at @p packaged.
 * This will not exceed @p len.
 * @retval -EINVAL if @p format is not acceptable
 * @retval -ENOSPC if @p packaged was not null and the space required to store
 * exceed @p len.
 */
__printf_like(3, 4)
int cbprintf_package(void *packaged,
		     size_t len,
		     const char *format,
		     ...);

/** @brief Capture state required to output formatted data later.
 *
 * Like cbprintf() but instead of processing the arguments and emitting the
 * formatted results immediately all arguments are captured so this can be
 * done in a different context, e.g. when the output function can block.
 *
 * In addition to the values extracted from arguments this will ensure that
 * copies are made of the necessary portions of any string parameters that are
 * not confirmed to be stored in read-only memory (hence assumed to be safe to
 * refer to directly later).
 *
 * @param packaged pointer to where the packaged data can be stored.  Pass a
 * null pointer to store nothing but still calculate the total space required.
 * The data stored here is relocatable, that is it can be moved to another
 * contiguous block of memory. The pointer must be aligned to a multiple of
 * the largest element in the argument list.
 *
 * @param len this must be set to the number of bytes available at @p packaged.
 * Ignored if @p packaged is NULL.
 *
 * @param format a standard ISO C format string with characters and conversion
 * specifications.
 *
 * @param ap captured stack arguments corresponding to the conversion
 * specifications found within @p format.
 *
 * @retval nonegative the number of bytes successfully stored at @p packaged.
 * This will not exceed @p len.
 * @retval -EINVAL if @p format is not acceptable
 * @retval -ENOSPC if @p packaged was not null and the space required to store
 * exceed @p len.
 */
int cbvprintf_package(void *packaged,
		      size_t len,
		      const char *format,
		      va_list ap);

/** @brief Generate the output for a previously captured format
 * operation.
 *
 * @param out the function used to emit each generated character.
 *
 * @param ctx context provided when invoking out
 *
 * @param packaged the data required to generate the formatted output, as
 * captured by cbprintf_package() or cbvprintf_package(). The alignment
 * requirement on this data is the same as when it was initially created.
 *
 * @note Memory indicated by @p packaged will be modified in a non-destructive
 * way, meaning that it could still be reused with this function again.
 *
 * @return the number of characters printed, or a negative error value
 * returned from invoking @p out.
 */
int cbpprintf(cbprintf_cb out,
	      void *ctx,
	      void *packaged);

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
 * when @option{CONFIG_CBPRINTF_NANO} is selected.
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
 * @note This function is available only when
 * @option{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced when
 * @option{CONFIG_CBPRINTF_NANO} is selected.
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

#ifdef CONFIG_CBPRINTF_LIBC_SUBSTS

/** @brief fprintf using Zephyrs cbprintf infrastructure.
 *
 * @note This function is available only when
 * @option{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced
 * when @option{CONFIG_CBPRINTF_NANO} is selected.
 *
 * @param stream the stream to which the output should be written.
 *
 * @param format a standard ISO C format string with characters and
 * conversion specifications.
 *
 * @param ... arguments corresponding to the conversion specifications found
 * within @p format.
 *
 * return The number of characters printed.
 */
__printf_like(2, 3)
int fprintfcb(FILE * stream, const char *format, ...);

/** @brief vfprintf using Zephyrs cbprintf infrastructure.
 *
 * @note This function is available only when
 * @option{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced when
 * @option{CONFIG_CBPRINTF_NANO} is selected.
 *
 * @param stream the stream to which the output should be written.
 *
 * @param format a standard ISO C format string with characters and conversion
 * specifications.
 *
 * @param ap a reference to the values to be converted.
 *
 * @return The number of characters printed.
 */
int vfprintfcb(FILE *stream, const char *format, va_list ap);

/** @brief printf using Zephyrs cbprintf infrastructure.
 *
 * @note This function is available only when
 * @option{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced
 * when @option{CONFIG_CBPRINTF_NANO} is selected.
 *
 * @param format a standard ISO C format string with characters and
 * conversion specifications.
 *
 * @param ... arguments corresponding to the conversion specifications found
 * within @p format.
 *
 * @return The number of characters printed.
 */
__printf_like(1, 2)
int printfcb(const char *format, ...);

/** @brief vprintf using Zephyrs cbprintf infrastructure.
 *
 * @note This function is available only when
 * @option{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced when
 * @option{CONFIG_CBPRINTF_NANO} is selected.
 *
 * @param format a standard ISO C format string with characters and conversion
 * specifications.
 *
 * @param ap a reference to the values to be converted.
 *
 * @return The number of characters printed.
 */
int vprintfcb(const char *format, va_list ap);

/** @brief snprintf using Zephyrs cbprintf infrastructure.
 *
 * @note This function is available only when
 * @option{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced
 * when @option{CONFIG_CBPRINTF_NANO} is selected.
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
 * @return The number of characters that would have been written to @p
 * str, excluding the terminating null byte.  This is greater than the
 * number actually written if @p size is too small.
 */
__printf_like(3, 4)
int snprintfcb(char *str, size_t size, const char *format, ...);

/** @brief vsnprintf using Zephyrs cbprintf infrastructure.
 *
 * @note This function is available only when
 * @option{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced when
 * @option{CONFIG_CBPRINTF_NANO} is selected.
 *
 * @param str where the formatted content should be written
 *
 * @param size maximum number of chaacters for the formatted output, including
 * the terminating null byte.
 *
 * @param format a standard ISO C format string with characters and conversion
 * specifications.
 *
 * @param ap a reference to the values to be converted.
 *
 * @return The number of characters that would have been written to @p
 * str, excluding the terminating null byte.  This is greater than the
 * number actually written if @p size is too small.
 */
int vsnprintfcb(char *str, size_t size, const char *format, va_list ap);

#endif /* CONFIG_CBPRINTF_LIBC_SUBSTS */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_CBPRINTF_H_ */
