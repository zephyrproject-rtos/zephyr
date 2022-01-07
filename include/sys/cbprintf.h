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

/* Determine if _Generic is supported.
 * In general it's a C11 feature but it was added also in:
 * - GCC 4.9.0 https://gcc.gnu.org/gcc-4.9/changes.html
 * - Clang 3.0 https://releases.llvm.org/3.0/docs/ClangReleaseNotes.html
 *
 * @note Z_C_GENERIC is also set for C++ where functionality is implemented
 * using overloading and templates.
 */
#ifndef Z_C_GENERIC
#if defined(__cplusplus) || (((__STDC_VERSION__ >= 201112L) || \
	((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) >= 40900) || \
	((__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__) >= 30000)))
#define Z_C_GENERIC 1
#else
#define Z_C_GENERIC 0
#endif
#endif

/* Z_C_GENERIC is used there */
#include <sys/cbprintf_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup cbprintf_apis Formatted Output APIs
 * @ingroup support_apis
 * @{
 */

/** @brief Required alignment of the buffer used for packaging. */
#ifdef __xtensa__
#define CBPRINTF_PACKAGE_ALIGNMENT 16
#elif defined(CONFIG_X86) && !defined(CONFIG_64BIT)
/* sizeof(long double) is 12 on x86-32, which is not power of 2.
 * So set it manually.
 */
#define CBPRINTF_PACKAGE_ALIGNMENT \
	(IS_ENABLED(CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE) ? \
		16 : MAX(sizeof(double), sizeof(long long)))
#else
#define CBPRINTF_PACKAGE_ALIGNMENT \
	(IS_ENABLED(CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE) ? \
		sizeof(long double) : MAX(sizeof(double), sizeof(long long)))
#endif

/**@defgroup CBPRINTF_PACKAGE_FLAGS Package flags.
 * @{
 */

/** @brief Append indexes of read-only string arguments in the package.
 *
 * When used, package contains locations of read-only string arguments. Package
 * with that information can be converted to fully self-contain package using
 * @ref cbprintf_fsc_package.
 */
#define CBPRINTF_PACKAGE_ADD_STRING_IDXS BIT(0)

/**@} */

/**@defgroup CBPRINTF_MUST_RUNTIME_PACKAGE_FLAGS Package flags.
 * @{
 */

/** @brief Consider constant string pointers as pointing to fixed strings.
 *
 * When flag is set then const (w)char pointers arguments in the string does
 * not trigger runtime packaging.
 */
#define CBPRINTF_MUST_RUNTIME_PACKAGE_CONST_CHAR BIT(0)

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
 * Static packaging can be applied if size of the package can be determined
 * at compile time. In general, package size can be determined at compile time
 * if there are no string arguments which might be copied into package body if
 * they are considered transient.
 *
 * @note By default any char pointers are considered to be pointing at transient
 * strings. This can be narrowed down to non const pointers by using
 * @ref CBPRINTF_MUST_RUNTIME_PACKAGE_CONST_CHAR.
 *
 * @param skip number of read only string arguments in the parameter list. It
 * shall be non-zero if there are known read only string arguments present
 * in the string (e.g. function name prefix in the log message).
 *
 * @param ... String with arguments.
 * @param flags option flags. See @ref CBPRINTF_MUST_RUNTIME_PACKAGE_FLAGS.
 *
 * @retval 1 if string must be packaged in run time.
 * @retval 0 string can be statically packaged.
 */
#define CBPRINTF_MUST_RUNTIME_PACKAGE(skip, flags, ... /* fmt, ... */) \
	Z_CBPRINTF_MUST_RUNTIME_PACKAGE(skip, flags, __VA_ARGS__)

/** @brief Statically package string.
 *
 * Build string package from formatted string. It assumes that formatted
 * string is in the read only memory.
 *
 * If _Generic is not supported then runtime packaging is performed.
 *
 * @param packaged pointer to where the packaged data can be stored. Pass a null
 * pointer to skip packaging but still calculate the total space required.
 * The data stored here is relocatable, that is it can be moved to another
 * contiguous block of memory. It must be aligned to the size of the longest
 * argument. It is recommended to use CBPRINTF_PACKAGE_ALIGNMENT for alignment.
 *
 * @param inlen set to the number of bytes available at @p packaged. If
 * @p packaged is NULL the value is ignored.
 *
 * @param outlen variable updated to the number of bytes required to completely
 * store the packed information. If input buffer was too small it is set to
 * -ENOSPC.
 *
 * @param align_offset input buffer alignment offset in bytes. Where offset 0
 * means that buffer is aligned to CBPRINTF_PACKAGE_ALIGNMENT. Xtensa requires
 * that @p packaged is aligned to CBPRINTF_PACKAGE_ALIGNMENT so it must be
 * multiply of CBPRINTF_PACKAGE_ALIGNMENT or 0.
 *
 * @param flags option flags. See @ref CBPRINTF_PACKAGE_FLAGS.
 *
 * @param ... formatted string with arguments. Format string must be constant.
 */
#define CBPRINTF_STATIC_PACKAGE(packaged, inlen, outlen, align_offset, flags, \
				... /* fmt, ... */) \
	Z_CBPRINTF_STATIC_PACKAGE(packaged, inlen, outlen, \
				  align_offset, flags, __VA_ARGS__)

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
 * contiguous block of memory. However, under condition that alignment is
 * maintained. It must be aligned to at least the size of a pointer.
 *
 * @param len this must be set to the number of bytes available at @p packaged
 * if it is not null. If @p packaged is null then it indicates hypothetical
 * buffer alignment offset in bytes compared to CBPRINTF_PACKAGE_ALIGNMENT
 * alignment. Buffer alignment offset impacts returned size of the package.
 * Xtensa requires that buffer is always aligned to CBPRINTF_PACKAGE_ALIGNMENT
 * so it must be multiply of CBPRINTF_PACKAGE_ALIGNMENT or 0 when @p packaged is
 * null.
 *
 * @param flags option flags. See @ref CBPRINTF_PACKAGE_FLAGS.
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
 * @retval -EFAULT if @p packaged alignment is not acceptable
 * @retval -ENOSPC if @p packaged was not null and the space required to store
 * exceed @p len.
 */
__printf_like(4, 5)
int cbprintf_package(void *packaged,
		     size_t len,
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
 * @param packaged pointer to where the packaged data can be stored.  Pass a
 * null pointer to store nothing but still calculate the total space required.
 * The data stored here is relocatable, that is it can be moved to another
 * contiguous block of memory. The pointer must be aligned to a multiple of
 * the largest element in the argument list.
 *
 * @param len this must be set to the number of bytes available at @p packaged.
 * Ignored if @p packaged is NULL.
 *
 * @param flags option flags. See @ref CBPRINTF_PACKAGE_FLAGS.
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
		      uint32_t flags,
		      const char *format,
		      va_list ap);

/** @brief Convert package to fully self-contained (fsc) package.
 *
 * By default, package does not contain read only strings. However, if needed
 * it may be converted to a fully self-contained package which contains all
 * strings. In order to allow such conversion, original package must be created
 * with @ref CBPRINTF_PACKAGE_ADD_STRING_IDXS flag. Such package will contain
 * necessary data to find read only strings in the package and copy them into
 * package body.
 *
 * @param in_packaged pointer to original package created with
 * @ref CBPRINTF_PACKAGE_ADD_STRING_IDXS.
 *
 * @param in_len @p in_packaged length.
 *
 * @param packaged pointer to location where fully self-contained version of the
 * input package will be written. Pass a null pointer to calculate space required.
 *
 * @param len must be set to the number of bytes available at @p packaged. Not
 * used if @p packaged is null.
 *
 * @retval nonegative the number of bytes successfully stored at @p packaged.
 * This will not exceed @p len. If @p packaged is null, calculated length.
 * @retval -ENOSPC if @p packaged was not null and the space required to store
 * exceed @p len.
 * @retval -EINVAL if @p in_packaged is null.
 */
int cbprintf_fsc_package(void *in_packaged,
			 size_t in_len,
			 void *packaged,
			 size_t len);

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
 * when @kconfig{CONFIG_CBPRINTF_NANO} is selected.
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
 * @kconfig{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced when
 * @kconfig{CONFIG_CBPRINTF_NANO} is selected.
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
 * @kconfig{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced
 * when @kconfig{CONFIG_CBPRINTF_NANO} is selected.
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
 * @kconfig{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced when
 * @kconfig{CONFIG_CBPRINTF_NANO} is selected.
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
 * @kconfig{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced
 * when @kconfig{CONFIG_CBPRINTF_NANO} is selected.
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
 * @kconfig{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced when
 * @kconfig{CONFIG_CBPRINTF_NANO} is selected.
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
 * @kconfig{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced
 * when @kconfig{CONFIG_CBPRINTF_NANO} is selected.
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
 * @kconfig{CONFIG_CBPRINTF_LIBC_SUBSTS} is selected.
 *
 * @note The functionality of this function is significantly reduced when
 * @kconfig{CONFIG_CBPRINTF_NANO} is selected.
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
