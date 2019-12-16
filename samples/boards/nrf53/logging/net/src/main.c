/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(app);

void expire_handler(struct k_timer *timer)
{
	static int cnt = 0;

	LOG_INF("Timer expired %d", cnt++);
}

K_TIMER_DEFINE(app_timer, expire_handler, NULL);

void main(void)
{
	k_timer_start(&app_timer, K_MSEC(500), K_MSEC(500));
}
