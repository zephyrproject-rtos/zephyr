/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>

#include <zephyr/bluetooth/mesh.h>

#include <zephyr/logging/log.h>

#ifdef CONFIG_BT_DEBUG_LOG
#define LOG_LEVEL LOG_LEVEL_INF
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

LOG_MODULE_REGISTER(bt_mesh_test, LOG_LEVEL);

#include "mesh.h"
#include "test.h"

int bt_mesh_test(void)
{
	return 0;
}
