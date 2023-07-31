/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(handler_demo);

#if !defined(CONFIG_MCUMGR_GRP_EXAMPLE_APP) && !defined(CONFIG_MCUMGR_GRP_EXAMPLE_MODULE)
#error Building this application with neither CONFIG_MCUMGR_GRP_EXAMPLE_APP or \
	CONFIG_MCUMGR_GRP_EXAMPLE_MODULE enabled is not valid
#endif

#ifdef CONFIG_MCUMGR_GRP_EXAMPLE_OTHER_HOOK
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <example_mgmt.h>
#include <example_mgmt_callbacks.h>

static struct mgmt_callback test_callback;
static bool last_run;

enum mgmt_cb_return test_function(uint32_t event, enum mgmt_cb_return prev_status, int32_t *rc,
				  uint16_t *group, bool *abort_more, void *data, size_t data_size)
{
	if (event == MGMT_EVT_OP_EXAMPLE_OTHER) {
		last_run = !last_run;

		if (last_run) {
			/* Return a dummy error for a demo */
			*group = MGMT_GROUP_ID_EXAMPLE;
			*rc = EXAMPLE_MGMT_ERR_REJECTED_BY_HOOK;

			LOG_INF("Received hook, rejecting!");
			return MGMT_CB_ERROR_ERR;
		}

		LOG_INF("Received hook, allowing");
	} else {
		LOG_ERR("Received unknown event: %d", event);
	}

	/* Return OK status code to continue with acceptance to underlying handler */
	return MGMT_CB_OK;
}
#endif

int main(void)
{
#ifdef CONFIG_MCUMGR_GRP_EXAMPLE_OTHER_HOOK
	/* Register for the example hook */
	test_callback.callback = test_function;
	test_callback.event_id = MGMT_EVT_OP_EXAMPLE_OTHER;
	mgmt_callback_register(&test_callback);
#endif

	return 0;
}
