/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_LOGGER_H_
#define CPP_LOGGER_H_

#ifdef __cplusplus

#include <zephyr/logging/log.h>

class CppLogger
{
public:
	void log_test(void)
	{
		LOG_MODULE_DECLARE(app);

		LOG_INF("test from C++ class");
	}
};

#endif /* __cplusplus */

#endif /* CPP_LOGGER_H_ */
