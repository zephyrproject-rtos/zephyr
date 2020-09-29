/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* needed here so the static device_get_binding does not get renamed */
#include <device.h>

/* OpenThread not enabled here */
#define CONFIG_OPENTHREAD_L2_LOG_LEVEL LOG_LEVEL_DBG

#ifndef CONFIG_NET_CONFIG_IEEE802154_DEV_NAME
	#define CONFIG_NET_CONFIG_IEEE802154_DEV_NAME ""
#endif /* CONFIG_NET_CONFIG_IEEE802154_DEV_NAME */

#define CONFIG_OPENTHREAD_THREAD_PRIORITY 5
#define OT_WORKER_PRIORITY K_PRIO_COOP(CONFIG_OPENTHREAD_THREAD_PRIORITY)
#define CONFIG_NET_L2_OPENTHREAD 1

/* needed for stubbing device driver */
const struct device *device_get_binding_stub(const char *name);
#define device_get_binding device_get_binding_stub

/* file itself */
#include "radio.c"
