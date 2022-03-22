/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_CORE2_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_CORE2_H_

#include <logging/log_msg2.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(CONFIG_LOG_MINIMAL)
/** @brief Initialize module for handling logging message. */
void z_log_msg2_init(void);

/** @brief Allocate log message.
 *
 * @param wlen Length in 32 bit words.
 *
 * @return allocated space or null if cannot be allocated.
 */
struct log_msg2 *z_log_msg2_alloc(uint32_t wlen);

/** @brief Commit log message.
 *
 * @param msg Message.
 */
void z_log_msg2_commit(struct log_msg2 *msg);

/** @brief Get pending log message.
 *
 * @param[out] len Message length in bytes is written is @p len is not null.
 *
 * @param Message or null if no pending messages.
 */
union log_msg2_generic *z_log_msg2_claim(void);

/** @brief Free message.
 *
 * @param msg Message.
 */
void z_log_msg2_free(union log_msg2_generic *msg);

/** @brief Check if there are any message pending.
 *
 * @retval true if at least one message is pending.
 * @retval false if no message is pending.
 */
bool z_log_msg2_pending(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_CORE2_H_ */
