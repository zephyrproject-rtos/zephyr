/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_XCC_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_XCC_H_

#include <toolchain/gcc.h>
#include <stdbool.h>

/* XCC doesn't support __COUNTER__ but this should be good enough */
#define __COUNTER__ __LINE__

#undef __in_section_unique
#define __in_section_unique(seg) \
	__attribute__((section("." STRINGIFY(seg) "." STRINGIFY(__COUNTER__))))

#ifndef __GCC_LINKER_CMD__
#include <xtensa/config/core.h>

/*
 * XCC does not define the following macros with the expected names, but the
 * HAL defines similar ones. Thus we include it and define the missing macros
 * ourselves.
 */
#ifndef __BYTE_ORDER__
#define __BYTE_ORDER__ XCHAL_MEMORY_ORDER
#endif
#ifndef __ORDER_BIG_ENDIAN__
#define __ORDER_BIG_ENDIAN__ XTHAL_BIGENDIAN
#endif
#ifndef __ORDER_LITTLE_ENDIAN__
#define __ORDER_LITTLE_ENDIAN__ XTHAL_LITTLEENDIAN
#endif

#endif /* __GCC_LINKER_CMD__ */

#define __builtin_unreachable() do { __ASSERT(false, "Unreachable code"); } \
	while (true)

#endif
