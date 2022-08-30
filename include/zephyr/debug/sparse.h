/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_SPARSE_H
#define ZEPHYR_INCLUDE_DEBUG_SPARSE_H

#if defined(__CHECKER__)
#define __sparse_cache __attribute__((address_space(__cache)))
#define __sparse_force __attribute__((force))
#else
#define __sparse_cache
#define __sparse_force
#endif

#endif
