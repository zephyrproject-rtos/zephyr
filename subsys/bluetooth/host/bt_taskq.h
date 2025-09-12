/* bt_taskq.h - Workqueue for quick non-blocking Bluetooth tasks */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/**
 * @brief Bluetooth task workqueue
 *
 * bt_taskq is a workqueue intended for quick non-blocking work
 * items ("tasks") in the Bluetooth subsystem. This workqueue
 * must always exist and is not controlled by any Kconfig
 * option.
 *
 * Blocking means "waiting for something while running". A
 * task is NOT allowed to block. If a task need to "wait", it
 * should instead return immediately and schedule itself to run
 * later.
 *
 * Work items submitted to this queue should be:
 * - Quick to execute (non-blocking).
 * - Not perform long-running operations.
 * - Not block.
 *
 * @warning Non-blocking violation pitfalls:
 * - net_buf_unref() on a foreign buffer could have a blocking
 *   destroy callback
 * - Any user-defined callback might be blocking
 * - Avoid any operations that could sleep or block the thread
 *
 * Use bt_long_wq for long-running or potentially blocking
 * operations instead.
 *
 * Available in APPLICATION initialization level and later.
 */
extern struct k_work_q bt_taskq;
