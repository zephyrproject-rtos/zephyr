/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_KERNEL_THREADS_THREAD_APIS_SRC_TEST_THREAD_APIS_H_
#define ZEPHYR_TESTS_KERNEL_THREADS_THREAD_APIS_SRC_TEST_THREAD_APIS_H_

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
K_THREAD_STACK_EXTERN(tstack);
extern size_t tstack_size;
extern struct k_thread tdata;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTS_KERNEL_THREADS_THREAD_APIS_SRC_TEST_THREAD_APIS_H_ */
