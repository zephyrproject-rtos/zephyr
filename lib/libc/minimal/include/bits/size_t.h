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
 * @brief size_t definition
 */

#if !defined(__size_t_defined)
#define __size_t_defined

#ifdef __i386
typedef unsigned long int size_t;
#elif defined(__ARM_ARCH)
typedef unsigned int size_t;
#elif defined(__arc__)
typedef unsigned int size_t;
#else
#error "The minimal libc library does not recognize the architecture!\n"
#endif

#endif
