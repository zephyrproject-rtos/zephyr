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

#ifndef ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_H_
#define ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_H_

#if defined(_LINKER)
#if defined(__GCC_LINKER_CMD__)
#include <linker/linker-tool-gcc.h>
#elif defined(__MWDT_LINKER_CMD__)
#include <linker/linker-tool-mwdt.h>
#else
#error "Unknown toolchain"
#endif
#endif

#endif /* ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_H_ */
