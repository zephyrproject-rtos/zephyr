/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/sys_log.h>
#include <errno.h>
#include <board.h>

/* This application itself does nothing as there is net-shell that can be used
 * to monitor things.
 */
void main(void)
{
	SYS_LOG_INF("Start application");
}
