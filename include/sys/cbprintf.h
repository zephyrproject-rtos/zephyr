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
#include <sys/cbprintf_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup cbprintf_apis Formatted Output APIs
 * @ingroup support_apis
 * @{
 */

/**@defgroup CBPRINTF_PACKAGE_FLAGS packaging flags.
 * @{ */

/** @brief Indicate that format string is packaged as pointer. */
#define CBPRINTF_PACKAGE_FMT_AS_PTR BIT(0)

/**@} */

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

/** @brief Determine if string must be packaged in run time.
 *
 * Static packaging can be applied if size and of the package can be determined
 * at compile time. In general, package size can be determined at compile time
 * if there are no string arguments which might be copied into package body if
 * they are considered transient.
 *
 * @param skip number of read only string arguments in the parameter list. It
 * shall be non-zero if there are known read only string arguments present
 * in the string (e.g. function name).
 * @param ... String with arguments.
 *
 * @retval 1 if string must be packaged in run time.
 * @retval 0 string can be statically packaged.
 */
#define CBPRINTF_MUST_RUNTIME_PACKAGE(skip, .../* fmt, ... */) \
	Z_CBPRINTF_MUST_RUNTIME_PACKAGE(skip, __VA_ARGS__)

/** @brief Statically package string.
 *
 * Build string package based on formatted string. Macro produces same package
 * as if @ref cbprintf_package was used with CBPRINTF_PACKAGE_FMT_AS_PTR set.
 *
 * @param packaged pointer to where the packaged data can be stored.  Pass a
 * null pointer to store nothing but still calculate the total space required.
 * The data stored here is relocatable, that is it can be moved to another
 * contiguous block of memory.

 * @param len on input this must be set to the number of bytes available at @p
 * packaged. If @p packaged is NULL the input value is ignored.  On output
 * the referenced value will be updated to the number of bytes required to
 * completely store the packed information.  The @p len parameter must not be
 * null.
 *
 * @param fmt_as_ptr Set to 1 to package format string as void pointer without
 * header. It optimizes package size for read only strings. It must be a
 * constant value at compile time.
 *
 * @param ... formatted string with arguments. Format string must be constant.
 */
#define CBPRINTF_STATIC_PACKAGE(packaged, len, fmt_as_ptr, ... /* fmt, ... */) \
	Z_CBPRINTF_STATIC_PACKAGE(packaged, len, fmt_as_ptr, __VA_ARGS__)

/** @brief Calculate package size at compile time.
 *
 * @param fmt_as_ptr Set to 1 to package format string as void pointer without
 * header. It optimizes package size for read only strings.
 *
 * @param ... String with arguments.
 *
 * @return Calculated package size. As it is determined at compile time it can
 * be assigned to a const variable.
 */
#define CBPRINTF_STATIC_PACKAGE_SIZE(fmt_as_ptr, ... /* fmt, ... */) \
	Z_CBPRINTF_STATIC_PACKAGE_SIZE(fmt_as_ptr, __VA_ARGS__)

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
 * @note This function is available only when `CONFIG_CBPRINTF_COMPLETE` is
 * selected.
 *
 * @param packaged pointer to where the packaged data can be stored.  Pass a
 * null pointer to store nothing but still calculate the total space required.
 * The data stored here is relocatable, that is it can be moved to another
 * contiguous block of memory.
 *
 * @param len on input this must be set to the number of bytes available at @p
 * packaged.  If @p packaged is NULL the input value is ignored.  On output
 * the referenced value will be updated to the number of bytes required to
 * completely store the packed information.  The @p len parameter must not be
 * null.
 *
 * @param flags Flags. See @ref CBPRINTF_PACKAGE_FLAGS.
 *
 * @param format a standard ISO C format string with characters and conversion
 * specifications.
 *
 * @param ... arguments corresponding to the conversion specifications found
 * within @p format.
 *
 * @retval nonegative the number of bytes successfully stored at @p packaged.
 * This will not exceed the input value of @p *len.
 * @retval -EINVAL if @p format or @p len are not acceptable
 * @retval -ENOSPC if @p packaged was not null and the space required to store
 * exceed the input value of @p *len.
 */
__printf_like(4, 5)
int cbprintf_package(uint8_t *packaged,
		     size_t *len,
		     uint32_t flags,
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
 * @note This function is available only when `CONFIG_CBPRINTF_COMPLETE` is
 * selected.
 *
 * @param packaged pointer to where the packaged data can be stored.  Pass a
 * null pointer to store nothing but still calculate the total space required.
 * The data stored here is relocatable, that is it can be moved to another
 * contiguous block of memory.
 *
 * @param len on input this must be set to the number of bytes available at @p
 * packaged.  If @p packaged is NULL the input value is ignored.  On output
 * the referenced value will be updated to the number of bytes required to
 * completely store the packed information.  The @p len parameter must not be
 * null.
 *
 * @param flags Flags. See @ref CBPRINTF_PACKAGE_FLAGS.
 *
 * @param format a standard ISO C format string with characters and conversion
 * specifications.
 *
 * @param ap captured stack arguments corresponding to the conversion
 * specifications found within @p format.
 *
 * @retval nonegative the number of bytes successfully stored at @p packaged.
 * This will not exceed the input value of @p *len.
 * @retval -EINVAL if @p format or @p len are not acceptable
 * @retval -ENOSPC if @p packaged was not null and the space required to store
 * exceed the input value of @p *len.
 */
int cbvprintf_package(uint8_t *packaged,
		      size_t *len,
		      uint32_t flags,
		      const char *format,
		      va_list ap);

/** @brief Generate the output for a previously captured format
 * operation.
 *
 * @param out the function used to emit each generated character.
 *
 * @param ctx context provided when invoking out
 *
 * @param flags Flags. See @ref CBPRINTF_PACKAGE_FLAGS.
 *
 * @param packaged the data required to generate the formatted output, as
 * captured by cbprintf_package() or cbvprintf_package().
 *
 * @return the number of characters printed, or a negative error value
 * returned from invoking @p out.
 */
int cbpprintf(cbprintf_cb out,
	      void *ctx,
	      uint32_t flags,
	      const uint8_t *packaged);

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
