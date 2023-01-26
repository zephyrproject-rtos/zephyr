/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "btp/btp.h"

void main(void)
{
	tester_init();
}
