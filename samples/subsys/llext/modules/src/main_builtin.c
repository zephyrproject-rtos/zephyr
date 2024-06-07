/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

extern void hello_world(void);

int main(void)
{
	LOG_INF("Calling hello world as a builtin");

	hello_world();

	return 0;
}
