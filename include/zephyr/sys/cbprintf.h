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
#include <zephyr/toolchain.h>
#include <string.h>

#ifdef CONFIG_CBPRINTF_LIBC_SUBSTS
#include <stdio.h>
#endif /* CONFIG_CBPRINTF_LIBC_SUBSTS */

/* Determine if _Generic is supported using macro from toolchain.h.
 *
 * @note Z_C_GENERIC is also set for C++ where functionality is implemented
 * using overloading and templates.
 */
#ifndef Z_C_GENERIC
#if defined(__cplusplus) || TOOLCHAIN_HAS_C_GENERIC
#define Z_C_GENERIC 1
#else
#define Z_C_GENERIC 0
#endif
#endif

/**
 * @brief cbprintf package descriptor.
 */
struct cbprintf_package_desc {
	/** Package length (in 32 bit words) */
	uint8_t len;

	/** Number of appended strings in the package. */
	uint8_t str_cnt;

	/** Number of read-only strings, indexes appended to the package */
	uint8_t ro_str_cnt;

	/** Number of read-write strings, indexes appended to the package */
	uint8_t rw_str_cnt;

#ifdef CONFIG_CBPRINTF_PACKAGE_HEADER_STORE_CREATION_FLAGS
	/** Flags used to create the package */
	uint32_t pkg_flags;
#endif
} __packed;

/** @brief cbprintf package header
 *
 * cbprintf package header, without the format string pointer.
 */
union cbprintf_package_hdr {
	/** Header description */
	struct cbprintf_package_desc desc;

	void *raw;

#if defined(CONFIG_CBPRINTF_PACKAGE_HEADER_STORE_CREATION_FLAGS) && !defined(CONFIG_64BIT)
	void *raw2[2];
#endif

#ifdef CONFIG_CBPRINTF_PACKAGE_HEADER_STORE_CREATION_FLAGS
#ifdef __xtensa__
	/*
	 * On Xtensa, the first argument needs to be aligned to 8-byte.
	 * With 32-bit pointers, we need another 4 bytes padding so
	 * that whole struct cbprintf_package_hdr_ext is of multiple of
	 * 8 bytes.
	 */
	uint32_t xtensa_padding;
#endif
#endif

} __packed;

/** @brief cbprintf package header with format string pointer.
 *
 * cbprintf package header with format string pointer.
 */
struct cbprintf_package_hdr_ext {
	/** Header of package */
	union cbprintf_package_hdr hdr;

	/** Pointer to format string */
	char *fmt;

	/*
	 * When extending this struct, make sure this align
	 * to pointer size.
	 */
} __packed;

/* Z_C_GENERIC is used there */
#include <zephyr/sys/cbprintf_internal.h>

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
#else
#define CBPRINTF_PACKAGE_ALIGNMENT \
	Z_POW2_CEIL(COND_CODE_1(CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE, \
		(sizeof(long double)), (MAX(sizeof(double), sizeof(long long)))))
#endif

BUILD_ASSERT(Z_IS_POW2(CBPRINTF_PACKAGE_ALIGNMENT));

/**@defgroup CBPRINTF_PACKAGE_FLAGS Package flags.
 * @{
 */

/** @brief Assume that const char pointer is pointing to read only (constant) strings.
 *
 * Flag is valid only for @ref CBPRINTF_STATIC_PACKAGE.
 */
#define CBPRINTF_PACKAGE_CONST_CHAR_RO BIT(0)

/** @brief Append locations (within the package) of read-only string pointers.`*/
#define CBPRINTF_PACKAGE_ADD_RO_STR_POS BIT(1)

/** @brief Append locations (within the package) of read-write string pointers.
 *
 * When this flag is not used then read-write strings are appended to the package.
 */
#define CBPRINTF_PACKAGE_ADD_RW_STR_POS BIT(2)

#define Z_CBPRINTF_PACKAGE_FIRST_RO_STR_BITS 3
#define Z_CBPRINTF_PACKAGE_FIRST_RO_STR_OFFSET 3
#define Z_CBPRINTF_PACKAGE_FIRST_RO_STR_MASK BIT_MASK(Z_CBPRINTF_PACKAGE_FIRST_RO_STR_BITS)

/** @brief Indicate that @p n first string format arguments are char pointers to
 * read-only location.
 *
 * Runtime algorithm (address analysis) is skipped for those strings.
 *
 * @param n Number of string arguments considered as read-only.
 */
#define CBPRINTF_PACKAGE_FIRST_RO_STR_CNT(n) \
	(n << Z_CBPRINTF_PACKAGE_FIRST_RO_STR_OFFSET)

/** @brief Get number of first format string arguments which are known to be read-only
 * string.
 */
#define Z_CBPRINTF_PACKAGE_FIRST_RO_STR_CNT_GET(flags) \
	(((flags) >> Z_CBPRINTF_PACKAGE_FIRST_RO_STR_OFFSET) & Z_CBPRINTF_PACKAGE_FIRST_RO_STR_MASK)

/** @brief Append indexes of read-only string arguments in the package.
 *
 * When used, package contains locations of read-only string arguments. Package
 * with that information can be converted to fully self-contain package using
 * @ref cbprintf_fsc_package.
 */
#define CBPRINTF_PACKAGE_ADD_STRING_IDXS \
	(CBPRINTF_PACKAGE_ADD_RO_STR_POS | CBPRINTF_PACKAGE_CONST_CHAR_RO)

/** @brief Indicate the incoming arguments are tagged.
 *
 * When set, this indicates that the incoming arguments are tagged, and
 * need to be processed accordingly.
 */
#define CBPRINTF_PACKAGE_ARGS_ARE_TAGGED BIT(6)

/**@} */

/**@defgroup CBPRINTF_PACKAGE_CONVERT_FLAGS Package flags.
 * @{
 */

/** @brief Append read-only strings from source package to destination package.
 *
 * If package was created with @ref CBPRINTF_PACKAGE_ADD_RO_STR_POS
 * or @ref CBPRINTF_PACKAGE_ADD_RW_STR_POS it contains arrays of indexes where
 * string address can be found in the package. When flag is set, read-only strings
 * are copied into destination package. Address of strings indicated as read-write
 * are also checked and if determined to be read-only they are also copied.
 */
#define CBPRINTF_PACKAGE_CONVERT_RO_STR BIT(0)
#define CBPRINTF_PACKAGE_COPY_RO_STR CBPRINTF_PACKAGE_CONVERT_RO_STR __DEPRECATED_MACRO

/** @brief Append read-write strings from source package to destination package.
 *
 * If package was created with @ref CBPRINTF_PACKAGE_ADD_RW_STR_POS it contains
 * arrays of indexes where string address can be found in the package. When flag
 * is set, list of read-write strings is examined and if they are not determined
 * to be read-only, they are copied into the destination package.
 * If @ref CBPRINTF_PACKAGE_CONVERT_RO_STR is not set, remaining string locations
 * are considered as pointing to read-only location and they are copy to the
 * package if @ref CBPRINTF_PACKAGE_CONVERT_KEEP_RO_STR is set.
 */
#define CBPRINTF_PACKAGE_CONVERT_RW_STR BIT(1)
#define CBPRINTF_PACKAGE_COPY_RW_STR CBPRINTF_PACKAGE_CONVERT_RW_STR __DEPRECATED_MACRO

/** @brief Keep read-only location indexes in the package.
 *
 * If it is set read-only string pointers are kept in the package after copy. If
 * not set they are discarded.
 */
#define CBPRINTF_PACKAGE_CONVERT_KEEP_RO_STR BIT(2)
#define CBPRINTF_PACKAGE_COPY_KEEP_RO_STR CBPRINTF_PACKAGE_CONVERT_KEEP_RO_STR __DEPRECATED_MACRO

/** @brief Check format string if %p argument was treated as %s in the package.
 *
 * Static packaging is done based only on types of arguments used for a format
 * string. Without looking into format specifiers present in the string. Because
 * of that if (unsigned) char pointer is used for %p it will be considered as
 * a string location and during conversion an attempt to append a string to a
 * package may be performed. This can lead to misbehavior, in the best case
 * package will be bigger and in the worst case memory fault or security violation
 * may occur.
 *
 * When this flag is set, format string will be checked to detect cases when
 * string candidate is a pointer used for %p and string appending from unexpected
 * location is avoided. Additionally, an log warning is generated to encourage
 * user to cast such argument to void *. It is recommended because there are
 * configurations where string is not accessible and inspection cannot be done.
 * In those cases there are no means to detect such cases.
 */
#define CBPRINTF_PACKAGE_CONVERT_PTR_CHECK BIT(3)

/**@} */

/**@defgroup Z_CBVPRINTF_PROCESS_FLAGS cbvprintf processing flags.
 * @{
 */

/** @brief Indicates the arguments are tagged.
 *
 * This tells z_cbvprintf_impl() that the incoming arguments are
 * tagged, and should be processed accordingly.
 */
#define Z_CBVPRINTF_PROCESS_FLAG_TAGGED_ARGS BIT(0)

/**@} */

#include <zephyr/sys/cbprintf_enums.h>

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
#ifdef __CHECKER__
typedef int (*cbprintf_cb)(int c, void *ctx);
#else
typedef int (*cbprintf_cb)(/* int c, void *ctx */);
#endif

/** @brief Signature for a cbprintf multibyte callback function.
 *
 * @param buf data.
 * @param len data length.
 * @param ctx a pointer to an object that provides context for the operation.
 *
 * return Amount of copied data or negative error code.
 */
typedef int (*cbprintf_convert_cb)(const void *buf, size_t len, void *ctx);

/** @brief Signature for a external formatter function identical to cbvprintf.
 *
 * This function expects the following parameters:
 *
 * @param out the function used to emit each generated character.
 *
 * @param ctx a pointer to an object that provides context for the
 * external formatter.
 *
 * @param fmt a standard ISO C format string with characters and
 * conversion specifications.
 *
 * @param ap captured stack arguments corresponding to the conversion
 * specifications found within @p fmt.
 *
 * @return vprintf like return values: the number of characters printed,
 * or a negative error value returned from external formatter.
 */
typedef int (*cbvprintf_external_formatter_func)(cbprintf_cb out, void *ctx,
						 const char *fmt, va_list ap);

/** @brief Determine if string must be packaged in run time.
 *
 * Static packaging can be applied if size of the package can be determined
 * at compile time. In general, package size can be determined at compile time
 * if there are no string arguments which might be copied into package body if
 * they are considered transient.
 *
 * @note By default any char pointers are considered to be pointing at transient
 * strings. This can be narrowed down to non const pointers by using
 * @ref CBPRINTF_PACKAGE_CONST_CHAR_RO.
 *
 * @param ... String with arguments.
 * @param flags option flags. See @ref CBPRINTF_PACKAGE_FLAGS.
 *
 * @retval 1 if string must be packaged in run time.
 * @retval 0 string can be statically packaged.
 */
#define CBPRINTF_MUST_RUNTIME_PACKAGE(flags, ... /* fmt, ... */) \
	Z_CBPRINTF_MUST_RUNTIME_PACKAGE(flags, __VA_ARGS__)

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

/** @brief Convert a package.
 *
 * Converting may include appending strings used in the package to the package body.
 * If input package was created with @ref CBPRINTF_PACKAGE_ADD_RO_STR_POS or
 * @ref CBPRINTF_PACKAGE_ADD_RW_STR_POS, it contains information where strings
 * are located within the package. This information can be used to copy strings
 * during the conversion.
 *
 * @p cb is called with portions of the output package. At the end of the conversion
 * @p cb is called with null buffer.
 *
 * @param in_packaged Input package.
 *
 * @param in_len Input package length. If 0 package length will be retrieved
 * from the @p in_packaged
 *
 * @param cb callback called with portions of the converted package. If null only
 * length of the output package is calculated.
 *
 * @param ctx Context provided to the @p cb.
 *
 * @param flags Flags. See @ref CBPRINTF_PACKAGE_CONVERT_FLAGS.
 *
 * @param[in, out] strl if @p packaged is null, it is a pointer to the array where
 * @p strl_len first string lengths will is stored. If @p packaged is not null,
 * it contains lengths of first @p strl_len strings. It can be used to optimize
 * copying so that string length is calculated only once (at length calculation
 * phase when @p packaged is null.)
 *
 * @param strl_len Number of elements in @p strl array.
 *
 * @retval Positive output package size.
 * @retval -ENOSPC if @p packaged was not null and the space required to store
 * exceed @p len.
 */
int cbprintf_package_convert(void *in_packaged,
			     size_t in_len,
			     cbprintf_convert_cb cb,
			     void *ctx,
			     uint32_t flags,
			     uint16_t *strl,
			     size_t strl_len);

/* @interal Context used for package copying. */
struct z_cbprintf_buf_desc {
	void *buf;
	size_t size;
	size_t off;
};

/* @internal Function callback used for package copying. */
static inline int z_cbprintf_cpy(const void *buf, size_t len, void *ctx)
{
	struct z_cbprintf_buf_desc *desc = (struct z_cbprintf_buf_desc *)ctx;

	if ((desc->size - desc->off) < len) {
		return -ENOSPC;
	}

	memcpy(&((uint8_t *)desc->buf)[desc->off], buf, len);
	desc->off += len;

	return len;
}

/** @brief Copy package with optional appending of strings.
 *
 * @ref cbprintf_package_convert is used to convert and store converted package
 * in the new location.
 *
 * @param in_packaged Input package.
 *
 * @param in_len Input package length. If 0 package length will be retrieved
 * from the @p in_packaged
 *
 * @param[out] packaged Output package. If null only length of the output package
 * is calculated.
 *
 * @param len Available space in the location pointed by @p packaged. Not used when
 * @p packaged is null.
 *
 * @param flags Flags. See @ref CBPRINTF_PACKAGE_CONVERT_FLAGS.
 *
 * @param[in, out] strl if @p packaged is null, it is a pointer to the array where
 * @p strl_len first string lengths will is stored. If @p packaged is not null,
 * it contains lengths of first @p strl_len strings. It can be used to optimize
 * copying so that string length is calculated only once (at length calculation
 * phase when @p packaged is null.)
 *
 * @param strl_len Number of elements in @p strl array.
 *
 * @retval Positive Output package size.
 * @retval -ENOSPC if @p packaged was not null and the space required to store
 * exceed @p len.
 */
static inline int cbprintf_package_copy(void *in_packaged,
					size_t in_len,
					void *packaged,
					size_t len,
					uint32_t flags,
					uint16_t *strl,
					size_t strl_len)
{
	struct z_cbprintf_buf_desc buf_desc = {
		.buf = packaged,
		.size = len,
		.off = 0,
	};

	return cbprintf_package_convert(in_packaged, in_len,
					packaged ? z_cbprintf_cpy : NULL, &buf_desc,
					flags, strl, strl_len);
}

/** @brief Convert package to fully self-contained (fsc) package.
 *
 * Package may not be self contain since strings by default are stored by address.
 * Package may be partially self-contained when transient (not read only) strings
 * are appended to the package. Such package can be decoded only when there is an
 * access to read-only strings.
 *
 * Fully self-contained has (fsc) contains all strings used in the package. A package
 * can be converted to fsc package if it was create with @ref CBPRINTF_PACKAGE_ADD_RO_STR_POS
 * flag. Such package will contain necessary data to find read only strings in
 * the package and copy them into the package body.
 *
 * @param in_packaged pointer to original package created with
 * @ref CBPRINTF_PACKAGE_ADD_RO_STR_POS.
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
static inline int cbprintf_fsc_package(void *in_packaged,
				       size_t in_len,
				       void *packaged,
				       size_t len)
{
	return cbprintf_package_copy(in_packaged, in_len, packaged, len,
				     CBPRINTF_PACKAGE_CONVERT_RO_STR |
				     CBPRINTF_PACKAGE_CONVERT_RW_STR, NULL, 0);
}

/** @brief Generate the output for a previously captured format
 * operation using an external formatter.
 *
 * @param out the function used to emit each generated character.
 *
 * @param formatter external formatter function.
 *
 * @param ctx a pointer to an object that provides context for the
 * external formatter.
 *
 * @param packaged the data required to generate the formatted output, as
 * captured by cbprintf_package() or cbvprintf_package(). The alignment
 * requirement on this data is the same as when it was initially created.
 *
 * @note Memory indicated by @p packaged will be modified in a non-destructive
 * way, meaning that it could still be reused with this function again.
 *
 * @return printf like return values: the number of characters printed,
 * or a negative error value returned from external formatter.
 */
int cbpprintf_external(cbprintf_cb out,
		       cbvprintf_external_formatter_func formatter,
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
 * @param flags flags on how to process the inputs.
 *              @see Z_CBVPRINTF_PROCESS_FLAGS.
 *
 * @return the number of characters generated, or a negative error value
 * returned from invoking @p out.
 */
int z_cbvprintf_impl(cbprintf_cb out, void *ctx, const char *format,
		     va_list ap, uint32_t flags);

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
#ifdef CONFIG_PICOLIBC
int cbvprintf(cbprintf_cb out, void *ctx, const char *format, va_list ap);
#else
static inline
int cbvprintf(cbprintf_cb out, void *ctx, const char *format, va_list ap)
{
	return z_cbvprintf_impl(out, ctx, format, ap, 0);
}
#endif

/** @brief varargs-aware *printf-like output through a callback with tagged arguments.
 *
 * This is essentially vsprintf() except the output is generated
 * character-by-character using the provided @p out function.  This allows
 * formatting text of unbounded length without incurring the cost of a
 * temporary buffer.
 *
 * Note that the argument list @p ap are tagged.
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
static inline
int cbvprintf_tagged_args(cbprintf_cb out, void *ctx,
			  const char *format, va_list ap)
{
	return z_cbvprintf_impl(out, ctx, format, ap,
				Z_CBVPRINTF_PROCESS_FLAG_TAGGED_ARGS);
}

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
static inline
int cbpprintf(cbprintf_cb out, void *ctx, void *packaged)
{
#if defined(CONFIG_CBPRINTF_PACKAGE_SUPPORT_TAGGED_ARGUMENTS)
	union cbprintf_package_hdr *hdr =
		(union cbprintf_package_hdr *)packaged;

	if ((hdr->desc.pkg_flags & CBPRINTF_PACKAGE_ARGS_ARE_TAGGED)
	    == CBPRINTF_PACKAGE_ARGS_ARE_TAGGED) {
		return cbpprintf_external(out, cbvprintf_tagged_args,
					  ctx, packaged);
	}
#endif

	return cbpprintf_external(out, cbvprintf, ctx, packaged);
}

#ifdef CONFIG_CBPRINTF_LIBC_SUBSTS

#ifdef CONFIG_PICOLIBC

#define fprintfcb(stream, ...) fprintf(stream, __VA_ARGS__)
#define vfprintfcb(stream, format, ap) (stream, format, ap)
#define printfcb(format, ...) printf(format, __VA_ARGS__)
#define vprintfcb(format, ap) vfprintf(format, ap)
#define snprintfcb(str, size, ...) snprintf(str, size, __VA_ARGS__)
#define vsnprintfcb(str, size, format, ap) vsnprintf(str, size, format, ap)

#else

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

#endif /* CONFIG_PICOLIBC */
#endif /* CONFIG_CBPRINTF_LIBC_SUBSTS */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_CBPRINTF_H_ */
