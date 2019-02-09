/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/types.h>
#include <errno.h>
#include <init.h>
#include <fs.h>


#define LOG_LEVEL CONFIG_DEVICENET_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(devicenet);

