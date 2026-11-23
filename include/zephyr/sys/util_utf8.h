/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UTF-8 utilities
 *
 * Misc UTF-8 utilities.
 */

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_UTF8_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_UTF8_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup sys-util
 * @{
 */

/**
 * @brief Properly truncate a NULL-terminated UTF-8 string
 *
 * Take a NULL-terminated UTF-8 string and ensure that if the string has been
 * truncated (by setting the NULL terminator) earlier by other means, that
 * the string ends with a properly formatted UTF-8 character (1-4 bytes).
 *
 * Example:
 *
 * @code{.c}
 *      char test_str[] = "€€€";
 *      char trunc_utf8[8];
 *
 *      printf("Original : %s\n", test_str); // €€€
 *      strncpy(trunc_utf8, test_str, sizeof(trunc_utf8));
 *      trunc_utf8[sizeof(trunc_utf8) - 1] = '\0';
 *      printf("Bad      : %s\n", trunc_utf8); // €€�
 *      utf8_trunc(trunc_utf8);
 *      printf("Truncated: %s\n", trunc_utf8); // €€
 * @endcode
 *
 * @param utf8_str NULL-terminated string
 *
 * @return Pointer to the @p utf8_str
 */
char *utf8_trunc(char *utf8_str);

/**
 * @brief Copies a UTF-8 encoded string from @p src to @p dst
 *
 * The resulting @p dst will always be NULL terminated if @p n is larger than 0,
 * and the @p dst string will always be properly UTF-8 truncated.
 *
 * @param dst The destination of the UTF-8 string.
 * @param src The source string
 * @param n   The size of the @p dst buffer. Maximum number of characters copied
 *            is @p n - 1. If 0 nothing will be done, and the @p dst will not be
 *            NULL terminated.
 *
 * @return Pointer to the @p dst
 */
char *utf8_lcpy(char *dst, const char *src, size_t n);

/**
 * @brief Counts the characters in a UTF-8 encoded string @p s
 *
 * Counts the number of UTF-8 characters (code points) in a null-terminated string.
 * This function steps through each UTF-8 sequence by checking leading byte patterns.
 * It does not fully validate UTF-8 correctness, only counts characters.
 *
 * @param s The input string
 *
 * @return Number of UTF-8 characters in @p s on success or (negative) error code
 *  otherwise.
 */
int utf8_count_chars(const char *s);

/**
 * @brief Check that a string is well-formed UTF-8
 *
 * Checks the @p len bytes at @p s against the syntax of UTF-8 byte sequences in
 * @rfc{3629,section-4}. Every sequence is decoded and its code point inspected. The string
 * is refused if it has:
 *
 * - a continuation byte (0x80 to 0xBF) where a sequence starts, or a byte from 0xF8 to 0xFF;
 * - a sequence where a byte after the first is not a continuation byte, or that @p len
 *   cuts short;
 * - an overlong sequence, which is longer than its code point needs, such as 0xC0 0x80
 *   for U+0000;
 * - a UTF-16 surrogate, U+D800 to U+DFFF;
 * - a code point above U+10FFFF.
 *
 * Every other code point is accepted, the noncharacters such as U+FFFE and U+FFFF included.
 *
 * The string does not end at a zero byte: a zero byte within @p len is U+0000, a valid
 * character, and the bytes after it are checked too. For a NULL-terminated string pass the
 * result of strlen().
 *
 * @param s   The string to check. It need not be NULL-terminated.
 * @param len The length of @p s in bytes.
 *
 * @retval true  The string is well-formed UTF-8, or @p len is 0.
 * @retval false The string is not well-formed UTF-8.
 */
bool utf8_is_valid(const char *s, size_t len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_UTF8_H_ */
