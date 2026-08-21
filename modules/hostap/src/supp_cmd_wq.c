/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "supp_cmd_wq.h"

/* Provided by supp_main.c; the dedicated workqueue used to run heavy supplicant
 * management operations off the caller's stack.
 */
extern struct k_work_q *get_cmd_wq(void);

void supplicant_run_on_cmd_wq(struct supplicant_cmd *cmd, k_work_handler_t handler)
{
	struct k_work_q *wq = get_cmd_wq();

	/* If invoked from the command workqueue itself (e.g. an operation issues a
	 * nested request), run inline: submitting to the single-threaded workqueue
	 * from its own thread would self-deadlock.
	 */
	if (k_current_get() == &wq->thread) {
		handler(&cmd->work);
		return;
	}

	k_work_init(&cmd->work, handler);
	k_work_submit_to_queue(wq, &cmd->work);

	/* Wait until the workqueue has finalized the work item, not merely run the
	 * handler. work_queue_main() clears K_WORK_RUNNING and finalizes the item
	 * after the handler returns, and cmd lives on the caller's stack, so
	 * returning any earlier would let the workqueue touch a freed frame.
	 */
	k_work_flush(&cmd->work, &cmd->sync);
}
