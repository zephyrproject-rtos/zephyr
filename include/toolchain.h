/*
 * Copyright (c) 2010-2014, Wind River Systems, Inc.
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
 * @brief Macros to abstract toolchain specific capabilities
 *
 * This file contains various macros to abstract compiler capabilities that
 * utilize toolchain specific attributes and/or pragmas.
 */

#ifndef _TOOLCHAIN_H
#define _TOOLCHAIN_H

#if defined(__GNUC__) || (defined(_LINKER) && defined(__GCC_LINKER_CMD__))
#include <toolchain/gcc.h>
#else
#include <toolchain/other.h>
#endif

/**
 * Workaround for documentation parser limitations
 *
 * Current documentation parsers (sphinx under Doxygen) don't seem to
 * understand well unnamed structs / unions. A workaround is to make
 * the parser think there is something like a struct/union/enum name
 * that is actually something with no effect to the compiler.
 *
 * Current choice is to give it a 1B alignment. This basically tells
 * the compiler to do what is doing now: align it wherever it thinks
 * it should, as a 1B alignment "restriction" fits any other alignment
 * restriction we might have.
 */
#define __unnamed_workaround__ __attribute__((__aligned__(1)))

#endif /* _TOOLCHAIN_H */
