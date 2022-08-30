/* Copyright (c) 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cinttypes>
#include <zephyr/sys/printk.h>

#include "chre_api/chre/event.h"
#include "chre/core/event_loop_manager.h"
#include "chre/platform/static_nanoapp_init.h"
#include "chre/util/system/napp_permissions.h"

namespace
{
constexpr const uint64_t kAppId = 1;
constexpr const uint32_t kAppVersion = 1;

chre::Nanoapp *nanoapp = nullptr;

bool nanoappStart(void)
{
	printk("EchoApp::nanoappStart()\n");
	nanoapp = chre::EventLoopManagerSingleton ::get()->getEventLoop().getCurrentNanoapp();
	nanoapp->registerForBroadcastEvent(CHRE_EVENT_MESSAGE_FROM_HOST);
	return true;
}

void nanoappHandleEvent(uint32_t sender_instance_id, uint16_t event_type, const void *event_data)
{
	printk("EchoApp::nanoappHandleEvent(sender_instance_id=%u, event_type=%u, event_data@%p)\n",
	       sender_instance_id, event_type, event_data);
}

void nanoappEnd()
{
	nanoapp->unregisterForBroadcastEvent(0);
	nanoapp = nullptr;
	printk("EchoApp::nanoappEnd()\n");
}

} /* anonymous namespace */

CHRE_STATIC_NANOAPP_INIT(EchoApp, kAppId, kAppVersion, chre::NanoappPermissions::CHRE_PERMS_NONE);
