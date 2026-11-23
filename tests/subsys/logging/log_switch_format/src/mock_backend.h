/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_MOCK_BACKEND_H__
#define SRC_MOCK_BACKEND_H__

#include <zephyr/logging/log_backend.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct log_backend_api mock_log_backend_api;
struct mock_log_backend {
	bool panic;
};

/**
 * @brief This function validates the log message.
 *
 * @param type Field in message header to indicate the type of SyS-T message.
 * @param optional_flags Optional Flags in message header.
 * @param module_id ModuleID in a Message Header distinguish between multiple
 *					instances of the same Origin.
 * @param sub_type Enumerated types for representing the sub-type of a Message.
 * @param payload The content of the Message.
 */
void validate_log_type(const char *raw_data_str, uint32_t log_type);

#define MOCK_LOG_BACKEND_DEFINE(name, autostart) \
	static struct mock_log_backend name##_mock; \
	LOG_BACKEND_DEFINE(name, mock_log_backend_api, autostart, &name##_mock)

#ifdef __cplusplus
}
#endif

#endif /* SRC_MOCK_BACKEND_H__ */
