/* Copyright (c) 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log_ctrl.h>

#include "apps.hpp"
#include "chre/core/event_loop_manager.h"
#include "chre/target_platform/init.h"

inline const char *boolToString(bool cond)
{
	return cond ? "SUCCESS" : "FAIL";
}

int main(void)
{
	auto echo_app = chre::initializeStaticNanoappEchoApp();
	auto& eventLoop = chre::EventLoopManagerSingleton::get()->getEventLoop();
	uint32_t instanceId;

	if (chre::zephyr::init()) {
		printk("Failed to initialize!\n");
		return -1;
	}

	printk("Hello CHRE!\n");

	k_msleep(500);
	/*
	 * Flush all log messages that resulted from initialization to avoid
	 * getting them mingled with those printk messages below.
	 */
	while (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD) && log_data_pending()) {
		k_msleep(100);
	}
	printk("Starting EchoApp... %s\n", boolToString(eventLoop.startNanoapp(echo_app)));
	printk("Nanoapp count=%zu\n", eventLoop.getNanoappCount());
	printk("Finding instance ID... %s\n", boolToString(eventLoop.findNanoappInstanceIdByAppId(1, &instanceId)));
	printk("Nanoapp count=%zu\n", eventLoop.getNanoappCount());
	printk("Instance ID: %u\n", instanceId);

	printk("Sending event %zu...\n", eventLoop.getNanoappCount());
	eventLoop.postEventOrDie(CHRE_EVENT_MESSAGE_FROM_HOST, nullptr, [](uint16_t eventType, void *eventData) {
		printk("Event (%u) complete!\n", eventType);
	});

	k_sleep(K_MSEC(500));
	printk("Ending EchoApp... %s\n",
	       boolToString(eventLoop.unloadNanoapp(instanceId, false)));
	chre::zephyr::deinit();
	printk("Goodbye!\n");
	return 0;
}
