/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_XCC_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_XCC_H_

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_H_
#error Please do not include toolchain-specific headers directly, use <zephyr/toolchain.h> instead
#endif

/* toolchain/gcc.h errors out if __BYTE_ORDER__ cannot be determined
 * there. However, __BYTE_ORDER__ is actually being defined later in
 * this file. So define __BYTE_ORDER__ to skip the check in gcc.h
 * and undefine after including gcc.h.
 *
 * Clang has it defined so there is no need to work around.
 */
#ifndef __clang__
#define __BYTE_ORDER__
#endif

#ifdef __clang__
#include <zephyr/toolchain/llvm.h>
#else
#include <zephyr/toolchain/gcc.h>
#endif

#ifndef __clang__
#undef __BYTE_ORDER__
#endif

#include <stdbool.h>

#ifndef __INT8_C
#define __INT8_C(x)	x
#endif

#ifndef INT8_C
#define INT8_C(x)	__INT8_C(x)
#endif

#ifndef __UINT8_C
#define __UINT8_C(x)	x ## U
#endif

#ifndef UINT8_C
#define UINT8_C(x)	__UINT8_C(x)
#endif

#ifndef __INT16_C
#define __INT16_C(x)	x
#endif

#ifndef INT16_C
#define INT16_C(x)	__INT16_C(x)
#endif

#ifndef __UINT16_C
#define __UINT16_C(x)	x ## U
#endif

#ifndef UINT16_C
#define UINT16_C(x)	__UINT16_C(x)
#endif

#ifndef __INT32_C
#define __INT32_C(x)	x
#endif

#ifndef INT32_C
#define INT32_C(x)	__INT32_C(x)
#endif

#ifndef __UINT32_C
#define __UINT32_C(x)	x ## U
#endif

#ifndef UINT32_C
#define UINT32_C(x)	__UINT32_C(x)
#endif

#ifndef __INT64_C
#define __INT64_C(x)	x
#endif

#ifndef INT64_C
#define INT64_C(x)	__INT64_C(x)
#endif

#ifndef __UINT64_C
#define __UINT64_C(x)	x ## ULL
#endif

#ifndef UINT64_C
#define UINT64_C(x)	__UINT64_C(x)
#endif

#ifndef __INTMAX_C
#define __INTMAX_C(x)	x
#endif

#ifndef INTMAX_C
#define INTMAX_C(x)	__INTMAX_C(x)
#endif

#ifndef __UINTMAX_C
#define __UINTMAX_C(x)	x ## ULL
#endif

#ifndef UINTMAX_C
#define UINTMAX_C(x)	__UINTMAX_C(x)
#endif

#ifndef __COUNTER__
/* XCC (GCC-based compiler) doesn't support __COUNTER__
 * but this should be good enough
 */
#define __COUNTER__ __LINE__
#endif

#undef __in_section_unique
#define __in_section_unique(seg) \
	__attribute__((section("." STRINGIFY(seg) "." STRINGIFY(__COUNTER__))))

#undef __in_section_unique_named
#define __in_section_unique_named(seg, name) \
	__attribute__((section("." STRINGIFY(seg) \
			       "." STRINGIFY(__COUNTER__) \
			       "." STRINGIFY(name))))

#ifndef __GCC_LINKER_CMD__
#include <xtensa/config/core.h>

/*
 * XCC does not define the following macros with the expected names, but the
 * HAL defines similar ones. Thus we include it and define the missing macros
 * ourselves.
 */
#if XCHAL_MEMORY_ORDER == XTHAL_BIGENDIAN
#define __BYTE_ORDER__		__ORDER_BIG_ENDIAN__
#elif XCHAL_MEMORY_ORDER == XTHAL_LITTLEENDIAN
#define __BYTE_ORDER__		__ORDER_LITTLE_ENDIAN__
#else
#error "Cannot determine __BYTE_ORDER__"
#endif

#endif /* __GCC_LINKER_CMD__ */

#define __builtin_unreachable() __builtin_trap()

/* Not a full barrier, just a SW barrier */
#define __sync_synchronize() do { __asm__ __volatile__ ("" ::: "memory"); } \
	while (false)

#ifdef __deprecated
/*
 * XCC does not support using deprecated attribute in enum,
 * so just nullify it here to avoid compilation errors.
 */
#undef __deprecated
#define __deprecated
#endif

#endif
