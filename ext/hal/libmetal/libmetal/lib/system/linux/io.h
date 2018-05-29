/*
 * Copyright (c) 2017, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	linux/io.h
 * @brief	Linux specific io definitions.
 */

#ifndef __METAL_IO__H__
#error "Include metal/io.h instead of metal/linux/io.h"
#endif

#ifndef __METAL_LINUX_IO__H__
#define __METAL_LINUX_IO__H__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef METAL_INTERNAL

#define metal_sys_io_mem_map(...)

#endif

#ifdef __cplusplus
}
#endif

#endif /* __METAL_LINUX_IO__H__ */
