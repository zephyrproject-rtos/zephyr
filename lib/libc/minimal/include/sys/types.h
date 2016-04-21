/*
 * Copyright (c) 2016 Intel Corporation
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

#ifndef __INC_sys_types_h__
#define __INC_sys_types_h__

#if !defined(__ssize_t_defined)
#define __ssize_t_defined

#ifdef __i386
typedef long int ssize_t;
#elif defined(__ARM_ARCH)
typedef int ssize_t;
#elif defined(__arc__)
typedef int ssize_t;
#elif defined(__NIOS2__)
typedef int ssize_t;
#else
#error "The minimal libc library does not recognize the architecture!\n"
#endif

#endif

#if !defined(__off_t_defined)
#define __off_t_defined

#ifdef __i386
typedef long int off_t;
#elif defined(__ARM_ARCH)
typedef int off_t;
#elif defined(__arc__)
typedef int off_t;
#elif defined(__NIOS2__)
typedef int off_t;
#else
#error "The minimal libc library does not recognize the architecture!\n"
#endif

#endif

#endif /* __INC_sys_types_h__ */
