/* ctype.h */

/*
 * Copyright (c) 2015 Intel Corporation.
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

#ifndef __INC_ctype_h__
#define __INC_ctype_h__

#ifdef __cplusplus
extern "C" {
#endif

static inline int isupper(int a)
{
	return ((unsigned)(a)-'A') < 26;
}

static inline int isalpha(int c)
{
	return (((unsigned)c|32)-'a') < 26;
}

static inline int isspace(int c)
{
	return c == ' ' || ((unsigned)c-'\t') < 5;
}

static inline int isgraph(int c)
{
	return ((((unsigned)c) > ' ') && (((unsigned)c) <= '~'));
}

static inline int isprint(int c)
{
	return ((((unsigned)c) >= ' ') && (((unsigned)c) <= '~'));
}

static inline int isdigit(int a)
{
	return (((unsigned)(a)-'0') < 10);
}

static inline int isxdigit(int a)
{
	unsigned int ua = (unsigned int)a;

	return ((ua - '0') < 10) || ((ua - 'a') < 6) || ((ua - 'A') < 6);
}

static inline int tolower(int chr)
{
	return (chr >= 'A' && chr <= 'Z') ? (chr + 32) : (chr);
}

static inline int toupper(int chr)
{
	return (chr >= 'a' && chr <= 'z') ? (chr - 32) : (chr);
}

#ifdef __cplusplus
}
#endif

#endif  /* __INC_ctype_h__ */
