/*
 * Copyright (c) 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel/dedicated_workq.h>

#include "modem_workqueue.h"

K_DEDICATED_WORK_QUEUE_DEFINE(modem, MODEM);
