/* main.c - OpenThread */

/*
 * Copyright (c) 2023 Telink
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ot_main, LOG_LEVEL_DBG);

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>

static void ot_satate_changed(uint32_t flags, void *context)
{
	struct openthread_context *ot_context = context;

	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(ot_context->instance)) {
		case OT_DEVICE_ROLE_CHILD:
			LOG_INF("OT child");
			break;
		case OT_DEVICE_ROLE_ROUTER:
			LOG_INF("OT router");
			break;
		case OT_DEVICE_ROLE_LEADER:
			LOG_INF("OT leader");
			break;
		case OT_DEVICE_ROLE_DISABLED:
			LOG_INF("OT disabled");
			break;
		case OT_DEVICE_ROLE_DETACHED:
			LOG_INF("OT detached");
			break;
		default:
			LOG_INF("OT unknown");
			break;
		}
	}
}

void main(void)
{
	LOG_INF("***** OpenThread CLI on Zephyr *****");
#ifndef CONFIG_OPENTHREAD_MANUAL_START
	LOG_INF("OT channel     %u",     CONFIG_OPENTHREAD_CHANNEL);
	LOG_INF("OT pan id      %0x04x", CONFIG_OPENTHREAD_PANID);
	LOG_INF("OT pan ext id  %s",     CONFIG_OPENTHREAD_XPANID);
	LOG_INF("OT network key %s",     CONFIG_OPENTHREAD_NETWORKKEY);
#endif /* CONFIG_OPENTHREAD_MANUAL_START */
	openthread_set_state_changed_cb(ot_satate_changed);
}
