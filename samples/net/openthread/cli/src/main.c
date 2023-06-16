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

static void ot_satate_changed(otChangedFlags flags,
	struct openthread_context *ot_context, void *user_data)
{
	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(ot_context->instance)) {
		case OT_DEVICE_ROLE_CHILD:
			LOG_INF("OT child");
			LOG_INF("OT Short address: %04x",
				otLinkGetShortAddress(ot_context->instance));
			LOG_HEXDUMP_INF(otLinkGetExtendedAddress(ot_context->instance),
				OT_EXT_ADDRESS_SIZE, "OT Extended address:");
			break;
		case OT_DEVICE_ROLE_ROUTER:
			LOG_INF("OT router");
			LOG_INF("OT Short address: %04x",
				otLinkGetShortAddress(ot_context->instance));
			LOG_HEXDUMP_INF(otLinkGetExtendedAddress(ot_context->instance),
				OT_EXT_ADDRESS_SIZE, "OT Extended address:");
			break;
		case OT_DEVICE_ROLE_LEADER:
			LOG_INF("OT leader");
			LOG_INF("OT Short address: %04x",
				otLinkGetShortAddress(ot_context->instance));
			LOG_HEXDUMP_INF(otLinkGetExtendedAddress(ot_context->instance),
				OT_EXT_ADDRESS_SIZE, "OT Extended address:");
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
	LOG_INF("OT pan id      %04x",   CONFIG_OPENTHREAD_PANID);
	LOG_INF("OT pan ext id  %s",     CONFIG_OPENTHREAD_XPANID);
	LOG_INF("OT network key %s",     CONFIG_OPENTHREAD_NETWORKKEY);
#endif /* CONFIG_OPENTHREAD_MANUAL_START */
	static struct openthread_state_changed_cb ot_state_cahnge = {
		.state_changed_cb = ot_satate_changed
	};

	openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_cahnge);
}
