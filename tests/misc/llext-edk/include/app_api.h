/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEST_EDK_H_
#define _TEST_EDK_H_
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

__syscall int foo(int bar);

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/app_api.h>
#endif /* _TEST_EDK_H_ */
