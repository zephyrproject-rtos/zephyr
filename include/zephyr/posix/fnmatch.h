/* SPDX-License-Identifier: BSD-3-Clause */

/*    $NetBSD: fnmatch.h,v 1.12.50.1 2011/02/08 16:18:55 bouyer Exp $    */

/*-
 * Copyright (c) 1992, 1993
 *    The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * @file
 * @brief Filename-matching types.
 * @ingroup posix
 *
 * Provides fnmatch() for matching strings against shell-style wildcard
 * patterns as used in filename globbing.
 *
 * @posix_header{fnmatch.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_FNMATCH_H_
#define ZEPHYR_INCLUDE_POSIX_FNMATCH_H_

/**
 * @brief The string does not match the specified pattern.
 */
#define FNM_NOMATCH 1 /* Match failed. */

/**
 * @brief Function not implemented.
 */
#define FNM_NOSYS   2 /* Function not implemented. */

/**
 * @brief Out of resources.
 */
#define FNM_NORES   3 /* Out of resources */

/**
 * @brief Treat backslash as an ordinary character rather than an escape character.
 */
#define FNM_NOESCAPE	0x01 /* Disable backslash escaping. */

/**
 * @brief Slash in string only matches slash in pattern.
 */
#define FNM_PATHNAME	0x02 /* Slash must be matched by slash. */

/**
 * @brief Leading period in string must be exactly matched by period in pattern.
 */
#define FNM_PERIOD	0x04 /* Period must be matched by period. */

/**
 * @brief Match the pattern case-insensitively.
 */
#define FNM_CASEFOLD	0x08 /* Pattern is matched case-insensitive */

/**
 * @brief Ignore a trailing slash and following characters after a match.
 */
#define FNM_LEADING_DIR 0x10 /* Ignore /<tail> after Imatch. */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Match a filename or path against a shell-style pattern.
 *
 * The first argument is the shell pattern (which may use '?', '*', and
 * bracket expressions), the second is the string to test against it, and
 * the third is a combination of @c FNM_* flags (or 0 for default matching).
 *
 * @return 0 if the string matches the pattern, @c FNM_NOMATCH if it does not,
 *         or another non-zero value on error.
 *
 * @posix_func{fnmatch}
 */
int fnmatch(const char *, const char *, int);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_FNMATCH_H_ */
