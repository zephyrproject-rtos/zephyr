/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/* Disable syscall tracing to avoid a conflict with the device_get_binding
 * macro defined below
 */
#define DISABLE_SYSCALL_TRACING

/* needed here so the static device_get_binding does not get renamed */
#include <zephyr/device.h>

/* OpenThread not enabled here */
#define CONFIG_OPENTHREAD_L2_LOG_LEVEL LOG_LEVEL_DBG

#ifndef CONFIG_NET_CONFIG_IEEE802154_DEV_NAME
	#define CONFIG_NET_CONFIG_IEEE802154_DEV_NAME ""
#endif /* CONFIG_NET_CONFIG_IEEE802154_DEV_NAME */

#define CONFIG_OPENTHREAD_THREAD_PRIORITY 5
#define OT_WORKER_PRIORITY K_PRIO_COOP(CONFIG_OPENTHREAD_THREAD_PRIORITY)
#define CONFIG_NET_L2_OPENTHREAD 1
#define CONFIG_OPENTHREAD_RADIO_WORKQUEUE_STACK_SIZE 512

/* needed for stubbing device driver */
const struct device *device_get_binding_stub(const char *name);
#define device_get_binding device_get_binding_stub

/* file itself */
#include "radio.c"
