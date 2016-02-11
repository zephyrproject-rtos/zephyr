/*
 * Copyright (c) 2016 Wind River Systems, Inc.
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

/**
 * @file
 * @brief _Restrict definition
 *
 * The macro "_Restrict" is intended to be private to the minimal libc library.
 * It evaluates to the "restrict" keyword when a C99 compiler is used, and
 * to "__restrict__" when a C++ compiler is used.
 */

#if !defined(_Restrict_defined)
#define _Restrict_defined

#ifdef __cplusplus
	#define _Restrict __restrict__
#else
	#define _Restrict restrict
#endif

#endif
