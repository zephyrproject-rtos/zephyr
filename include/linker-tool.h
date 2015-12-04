/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
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
 * @brief Toolchain-agnostic linker defs
 *
 * This header file is used to automatically select the proper set of macro
 * definitions (based on the toolchain) for the linker script.
 */

#ifndef __LINKER_TOOL_H
#define __LINKER_TOOL_H

#if defined(_LINKER)
#if defined(__GCC_LINKER_CMD__)
#include <linker-tool-gcc.h>
#else
#error "Unknown toolchain"
#endif
#endif

#endif /* !__LINKER_TOOL_H */
