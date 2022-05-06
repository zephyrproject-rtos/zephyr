/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_THREAD_MONITOR_H_
#define ZEPHYR_INCLUDE_THREAD_MONITOR_H_

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

/**
 * @def SYS_THREAD_MONITOR_HEAD
 *
 * @brief Head element of the thread monitor list.
 *
 * @details Access the head element of the thread monitor list.
 *
 */
#define SYS_THREAD_MONITOR_HEAD ((struct k_thread *)(_kernel.threads))

/**
 * @def SYS_THREAD_MONITOR_NEXT
 *
 * @brief Gets a thread node's next element.
 *
 * @details Given a node in a thread monitor list, gets the next
 * element in the list.
 *
 * @param obj Object to get the next element from.
 */
#define SYS_THREAD_MONITOR_NEXT(obj) (((struct k_thread *)obj)->next_thread)

#endif /* ZEPHYR_INCLUDE_THREAD_MONITOR_H_ */
