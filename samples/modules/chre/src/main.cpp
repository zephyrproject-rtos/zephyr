/* Copyright (c) 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>

#include "apps.hpp"
#include "chre/core/event_loop_manager.h"
#include "chre/target_platform/init.h"

inline const char *boolToString(bool cond)
{
	return cond ? "SUCCESS" : "FAIL";
}

void main(void)
{
	auto echo_app = chre::initializeStaticNanoappEchoApp();
	auto& eventLoop = chre::EventLoopManagerSingleton::get()->getEventLoop();
	uint32_t instanceId;

	if (chre::zephyr::init()) {
		printk("Failed to initialize!\n");
		return;
	}

	printk("Hello CHRE!\n");

	k_msleep(500);
	printk("Starting EchoApp... %s\n", boolToString(eventLoop.startNanoapp(echo_app)));
	printk("Nanoapp count=%u\n", eventLoop.getNanoappCount());
	printk("Finding instance ID... %s\n", boolToString(eventLoop.findNanoappInstanceIdByAppId(1, &instanceId)));
	printk("Nanoapp count=%u\n", eventLoop.getNanoappCount());
	printk("Instance ID: %u\n", instanceId);

	printk("Sending event %u...\n", eventLoop.getNanoappCount());
	eventLoop.postEventOrDie(CHRE_EVENT_MESSAGE_FROM_HOST, nullptr, [](uint16_t eventType, void *eventData) {
		printk("Event (%u) complete!\n", eventType);
	});

	k_sleep(K_MSEC(500));
	printk("Ending EchoApp... %s\n",
	       boolToString(eventLoop.unloadNanoapp(instanceId, false)));
	chre::zephyr::deinit();
	printk("Goodbye!\n");
}
