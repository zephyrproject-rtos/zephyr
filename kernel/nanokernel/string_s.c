/* string_s.c - library of enhanced security functions */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

/* includes */
#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <toolchain.h>
#include <sections.h>
#include <string_s.h>

/*******************************************************************************
*
* __memcpy_s - secure memcpy()
*
* This routine, which is only called if the ENHANCED_SECURITY option is
* enabled, copies elements between buffers.
*
* This routine may fail and consequently invoke _NanoFatalErrorHandler()
* if either <src> or <dest> are NULL, or if the destination buffer <dest>
* is too small.
*
* \param dest         destination buffer
* \param nDestElem    number of elements in destination buffer
* \param src          source buffer
* \param nElem        number of elements to copy
*
* RETURNS: 0 on success, otherwise invokes _NanoFatalErrorHandler()
*/

errno_t __k_memcpy_s(void *dest, size_t nDestElem, const void *src,
					 size_t nElem)
{
	if ((dest == NULL) || (src == NULL) || (nDestElem < nElem)) {
		_NanoFatalErrorHandler(_NANO_ERR_INVALID_STRING_OP,
				       &_default_esf);
	}

	/*
	 * The source and destination buffers have been verified.  It is safe
	 * to use k_memcpy() to do the memory copy.
	 */

	k_memcpy(dest, src, nElem);
	return 0;
}

/*******************************************************************************
*
* __strlen_s - secure strlen()
*
* This routine, which is only called if the ENHANCED_SECURITY option is
* enabled, returns the number of elements in the string <str> to a maximum
* value of <maxElem>.  Note that if <str> is NULL, it returns 0.
*
* \param str        string about which to find length
* \param maxElem    maximum number of elements in the string
*
* RETURNS: number of elements in the null-terminated character string
*/

size_t __strlen_s(const char *str, size_t maxElem)
{
	size_t len = 0; /* the calculated string length */

	if (str == NULL)
		return 0;

	while ((len < maxElem) && (str[len] != '\0')) {
		len++;
	}

	return len;
}

/*******************************************************************************
*
* __strcpy_s - secure strcpy
*
* This routine, which is only called if the ENHANCED_SECURITY option is
* enabled, copies a maximum of <nDestElem> elements from the source string
* <src> to the destination string <dest>.
*
* This routine may fail and consequently invoke _NanoFatalErrorHandler()
* if either <src> or <dest> are NULL, or if the buffer for destination string
* <dest> is too small.
*
* \param dest        destination string buffer
* \param nDestElem   number of elements in destination buffer
* \param src         source string buffer
*
* RETURNS: 0 on success, otherwise invokes _NanoFatalErrorHandler()
*/

errno_t __strcpy_s(char *dest, size_t nDestElem, const char *src)
{
	int i = 0;			/* loop counter */

	if (dest != NULL) {
		if (src != NULL) {
			while (i < nDestElem) {
				dest[i] = src[i];
				if (dest[i] == '\0') {
					return 0;
				}
				i++;
			}
		}
		dest[0] = '\0';
	}

	_NanoFatalErrorHandler (_NANO_ERR_INVALID_STRING_OP, &_default_esf);

    /*
     * the following statement is included in case the compiler
     * doesn't recognize that _NanoFatalErrorHandler() never returns
     * and complains about "missing return value for non-void function"
     * (should be optimized away if compiler recognizes the non-return)
     */

	return 1;
}
