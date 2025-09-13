/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"
#include "creds/creds.h"
#include "net_sample_common.h"
#include <errno.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/net/mqtt_offload.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#if defined(CONFIG_PM_DEVICE)
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#endif
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mqtt_offload_sample, LOG_LEVEL_DBG);

#if defined(CONFIG_MODEM)
const struct device *modem = DEVICE_DT_GET(DT_ALIAS(modem));
#endif

#define APP_BANNER "MQTT Offload sample"

static void start_client(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	start_thread();
}

#if defined(CONFIG_MODEM)
static int modem_init(void)
{
	if (!device_is_ready(modem)) {
		LOG_ERR("Modem device %s is not ready", modem->name);
		return -ENODEV;
	}

	LOG_INF("Modem device %s is ready", modem->name);
	return 0;
}
#endif
#ifdef CONFIG_PM_DEVICE
static int modem_power_on(void)
{
	LOG_INF("Powering on modem\n");
	return pm_device_action_run(modem, PM_DEVICE_ACTION_RESUME);
}
#endif

int main(void)
{
#ifdef CONFIG_MODEM
	modem_init();
#endif
#ifdef CONFIG_PM_DEVICE
	modem_power_on();
#endif
	wait_for_network();
	start_client(NULL, NULL, NULL);
	return 0;
}
