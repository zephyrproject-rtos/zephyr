/* limits.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

#ifndef __INC_limits_h__
#define __INC_limits_h__

#define UCHAR_MAX  0xFF
#define SCHAR_MAX  0x7F
#define SCHAR_MIN  (-SCHAR_MAX - 1)

#ifdef __CHAR_UNSIGNED__
	/* 'char' is an unsigned type */
	#define CHAR_MAX  UCHAR_MAX
	#define CHAR_MIN  0
#else
	/* 'char' is a signed type */
	#define CHAR_MAX  SCHAR_MAX
	#define CHAR_MIN  SCHAR_MIN
#endif

#define CHAR_BIT    8
#define LONG_BIT    32
#define WORD_BIT    32

#define INT_MAX     0x7FFFFFFF
#define SHRT_MAX    0x7FFF
#define LONG_MAX    0x7FFFFFFFl
#define LLONG_MAX   0x7FFFFFFFFFFFFFFFll

#define INT_MIN     (-INT_MAX - 1)
#define SHRT_MIN    (-SHRT_MAX - 1)
#define LONG_MIN    (-LONG_MAX - 1l)
#define LLONG_MIN   (-LLONG_MAX - 1ll)

#define SSIZE_MAX   INT_MAX

#define USHRT_MAX   0xFFFF
#define UINT_MAX    0xFFFFFFFFu
#define ULONG_MAX   0xFFFFFFFFul
#define ULLONG_MAX  0xFFFFFFFFFFFFFFFFull

#endif  /* __INC_limits_h__ */
