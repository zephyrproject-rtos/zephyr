/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <linker/linker-tool-gcc.h>
#else
#error "Unknown toolchain"
#endif
#endif

#endif /* !__LINKER_TOOL_H */
