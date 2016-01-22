/* limits.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __INC_limits_h__
#define __INC_limits_h__

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif  /* __INC_limits_h__ */
