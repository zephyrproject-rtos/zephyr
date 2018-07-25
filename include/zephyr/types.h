/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __Z_TYPES_H__
#define __Z_TYPES_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef __INT8_TYPE__       s8_t;
typedef __INT16_TYPE__      s16_t;
typedef __INT32_TYPE__      s32_t;
typedef __INT64_TYPE__      s64_t;

typedef __UINT8_TYPE__      u8_t;
typedef __UINT16_TYPE__     u16_t;
typedef __UINT32_TYPE__     u32_t;
typedef __UINT64_TYPE__     u64_t;

#ifdef __cplusplus
}
#endif

#endif /* __Z_TYPES_H__ */
