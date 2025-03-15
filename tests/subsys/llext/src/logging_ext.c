/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This very simple hello world C code can be used as a test case for building
 * probably the simplest loadable extension. It requires a single symbol be
 * linked, section relocation support, and the ability to export and call out to
 * a function.
 */

#include <stdint.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(logging_ext);

static const uint32_t number = 42;

void test_entry(void)
{
	LOG_INF("hello world");
	LOG_INF("A number is %" PRIu32, number);
}
EXPORT_SYMBOL(test_entry);
