/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_MOCK_BACKEND_H__
#define SRC_MOCK_BACKEND_H__

#include <zephyr/logging/log_backend.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOCK_ENTRY_TEXT_LEN 96
#define MOCK_ENTRY_NAME_LEN 24

/** @brief Captured log message. */
struct mock_log_entry {
	uint8_t domain_id;
	int16_t source_id;
	uint8_t level;
	char domain[MOCK_ENTRY_NAME_LEN];
	char source[MOCK_ENTRY_NAME_LEN];
	/* Formatted message without prefix and trailing new line. Hexdump included. */
	char text[MOCK_ENTRY_TEXT_LEN];
};

/** @brief Get the mock backend instance. */
const struct log_backend *mock_backend_get_instance(void);

/** @brief Remove all captured messages. */
void mock_backend_reset(void);

/** @brief Get number of messages captured since the last reset. */
size_t mock_backend_count(void);

/** @brief Get a copy of the captured message.
 *
 * @param idx Message index (0 is the oldest one).
 * @param entry Location where message is copied.
 *
 * @retval true if message exists.
 */
bool mock_backend_get(size_t idx, struct mock_log_entry *entry);

/** @brief Get number of messages reported as dropped by the logging core. */
uint32_t mock_backend_dropped(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_MOCK_BACKEND_H__ */
