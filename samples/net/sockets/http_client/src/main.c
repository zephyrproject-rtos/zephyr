/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log.h"
#define LOG_MODULE_NAME demo
#define LOG_LEVEL CONFIG_DEMO_LOG_LEVEL

LOG_MODULE_REGISTER(LOG_MODULE_NAME);
#include <zephyr.h>
#include <misc/printk.h>
#include "net/http_client.h"


void main(void)
{
	LOG_INF("   [UNISOC UWP566x HTTP DEMO]\n");

	while (1) {
		/** do nothing*/
	}
}
