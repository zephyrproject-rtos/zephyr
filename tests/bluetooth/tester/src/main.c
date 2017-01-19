/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdint.h>
#include <toolchain.h>

#include "bttester.h"

void main(void)
{
	tester_init();
}
