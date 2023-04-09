/*
 * Copyright (c) 2023 Ionut Pavel <iocapa@iocapa.com>
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

void main(void)
{
	while (1) {
		k_msleep(1000);
		LOG_INF("One second passed...");
	}
}
