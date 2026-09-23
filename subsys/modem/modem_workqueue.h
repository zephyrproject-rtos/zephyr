/** @file
 * @brief Modem workqueue header file.
 */

/*
 * Copyright (c) 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MODEM_WORKQUEUE_H_
#define ZEPHYR_INCLUDE_MODEM_WORKQUEUE_H_

#include <zephyr/kernel.h>
#include <zephyr/kernel/subsystem_workq.h>

#ifdef __cplusplus
extern "C" {
#endif

K_SUBSYSTEM_WORK_QUEUE_DECLARE(modem, MODEM);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MODEM_WORKQUEUE_H_ */
