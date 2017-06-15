/**
 * @file dummy.c
 * Static compilation checks.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

/* The Bluetooth subsystem requires the system workqueue to execute at a
 * cooperative priority to function correctly. If this build assert triggers
 * verify your configuration to ensure that cooperative threads are enabled
 * and that the system workqueue priority is negative (cooperative).
 */
BUILD_ASSERT(CONFIG_SYSTEM_WORKQUEUE_PRIORITY < 0);
