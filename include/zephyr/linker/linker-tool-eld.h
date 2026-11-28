/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker script definitions for ELD.
 */

#ifndef ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_ELD_H_
#define ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_ELD_H_

#include <zephyr/linker/linker-tool-gcc.h>
#include <zephyr/sys/util_macro.h>

#ifdef CONFIG_XIP
#define Z_LINKER_ELD_UNWRAP(...) __VA_ARGS__
#define Z_LINKER_ELD_NOLOAD_NOLOAD 1
#define Z_LINKER_ELD_IS_NOLOAD(options) \
	IS_ENABLED(UTIL_CAT(Z_LINKER_ELD_NOLOAD_, Z_LINKER_ELD_UNWRAP options))

/*
 * ELD 22 applies the preceding ROM/RAM delta to ALIGN_WITH_INPUT even
 * when a NOLOAD section uses RAM for both addresses. Such sections need
 * only their natural input alignment, not the copied-data address delta.
 */
#undef SECTION_DATA_PROLOGUE
#define SECTION_DATA_PROLOGUE(name, options, align) \
	name options : COND_CODE_1(Z_LINKER_ELD_IS_NOLOAD(options), \
				 (), (ALIGN_WITH_INPUT))
#endif

#endif /* ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_ELD_H_ */
