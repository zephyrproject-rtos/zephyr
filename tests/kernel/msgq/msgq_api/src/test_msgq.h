/*
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_MSGQ_H__
#define __TEST_MSGQ_H__

#include <zephyr.h>
#include <irq_offload.h>
#include <ztest.h>
#include <limits.h>

#define TIMEOUT 100
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define MSG_SIZE 4
#define MSGQ_LEN 2
#define MSG0 0xABCD
#define MSG1 0x1234
#define OVERFLOW_SIZE_MSG SIZE_MAX
#endif /* __TEST_MSGQ_H__ */
