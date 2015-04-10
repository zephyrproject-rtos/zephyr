/* string_s.h - prototypes of enhanced security functions */

/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
The library contains the implementation of the enhanced security functions
required by Security Development Lifecycle
*/

#ifndef _STRING_S_H
#define _STRING_S_H

#include <cputype.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>

typedef int errno_t; /* the error type that secure functions return */

#if defined(CONFIG_ENHANCED_SECURITY)
typedef errno_t res_s_t;
#else
/* if no enhanced security or compiling for user-space, functions
 * return nothing */
typedef void res_s_t;
#endif

errno_t __k_memcpy_s(void *dest,
		     size_t numberOfElements,
		     const void *src,
		     size_t count);

size_t __strlen_s(const char *str, size_t numberOfElements);

errno_t __strcpy_s(char *dest, size_t numberOfElements, const char *src);

static inline res_s_t k_memcpy_s(void *dest,
						       size_t numberOfElements,
						       const void *src,
						       size_t count)
{
#if defined(CONFIG_ENHANCED_SECURITY)
	return __k_memcpy_s(dest, numberOfElements, src, count);
#else  /* !CONFIG_ENHANCED_SECURITY */
	ARG_UNUSED(numberOfElements);
	k_memcpy(dest, src, count);
#endif /* CONFIG_ENHANCED_SECURITY */
}

static inline size_t strlen_s(const char *str, size_t numberOfElements)
{
#if defined(CONFIG_ENHANCED_SECURITY)
	return __strlen_s(str, numberOfElements);
#else  /* !CONFIG_ENHANCED_SECURITY */
	ARG_UNUSED(numberOfElements);
	return strlen(str);
#endif /* CONFIG_ENHANCED_SECURITY */
}

static inline res_s_t strcpy_s(char *dest,
			       size_t numberOfElements,
			       const char *src)
{
#if defined(CONFIG_ENHANCED_SECURITY)
	return __strcpy_s(dest, numberOfElements, src);
#else  /* !CONFIG_ENHANCED_SECURITY */
	ARG_UNUSED(numberOfElements);
	strcpy(dest, src);
#endif /* CONFIG_ENHANCED_SECURITY */
}

#endif /* _STRING_S_H */
