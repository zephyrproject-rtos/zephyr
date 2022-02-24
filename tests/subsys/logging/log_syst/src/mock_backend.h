/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_MOCK_BACKEND_H__
#define SRC_MOCK_BACKEND_H__

#include <logging/log_backend.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct log_backend_api mock_log_backend_api;

struct mock_log_backend_msg {
	uint8_t data[32];
};

struct mock_log_backend {
	struct mock_log_backend_msg exp_msgs[64];
};

/**
 * @brief This function validates the log message
 */
void validate_msg(const char *expected_str);

#define MOCK_LOG_BACKEND_DEFINE(name, autostart) \
	static struct mock_log_backend name##_mock; \
	LOG_BACKEND_DEFINE(name, mock_log_backend_api, autostart, &name##_mock)

#ifdef __cplusplus
}
#endif

#endif /* SRC_MOCK_BACKEND_H__ */
