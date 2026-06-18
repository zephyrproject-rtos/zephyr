/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_HOSTAP_SUPP_CMD_WQ_H_
#define ZEPHYR_MODULES_HOSTAP_SUPP_CMD_WQ_H_

#include <zephyr/kernel.h>

/* Work bundle embedded by callers in their own (typed) operation context. The
 * mechanism is intentionally agnostic of the operation - it only deals with the
 * kernel work item - so it stays free of wpa_supplicant headers and can be unit
 * tested in isolation.
 */
struct supplicant_cmd {
	struct k_work work;
	struct k_work_sync sync;
};

/**
 * @brief Run a supplicant management operation on the command workqueue.
 *
 * Submits @a cmd to the dedicated supplicant command workqueue using @a handler
 * and blocks until the work item has been finalized, keeping the deep supplicant
 * call chain off the caller's stack.
 *
 * @a handler recovers its enclosing, fully typed operation context with
 * CONTAINER_OF() and stores any result there; the caller reads it once this
 * function returns. If invoked from the command workqueue thread itself,
 * @a handler runs inline to avoid self-deadlocking the single-threaded queue.
 */
void supplicant_run_on_cmd_wq(struct supplicant_cmd *cmd, k_work_handler_t handler);

#endif /* ZEPHYR_MODULES_HOSTAP_SUPP_CMD_WQ_H_ */
