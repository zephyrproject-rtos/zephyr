/*
 * Copyright (c) 2025 Embeint Holdings Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODEM_AT_USER_PIPE_
#define ZEPHYR_MODEM_AT_USER_PIPE_

#include <zephyr/modem/chat.h>

/**
 * @brief Claim the AT command user pipe to run commands
 *
 * @note This function will not block if the underlying pipe is not opened
 *
 * @param chat Chat instance that will be used with the user pipe
 * @param timeout Maximum duration to wait for other users to release the pipe
 *
 * @retval 0 On success
 * @retval -EPERM Modem is not ready
 * @retval -EBUSY User pipe already claimed
 */
int modem_at_user_pipe_claim(struct modem_chat *chat, k_timeout_t timeout);

/**
 * @brief Release the AT command user pipe to other users
 *
 * Must be called after @ref modem_at_user_pipe_claim when pipe is no longer
 * in use.
 */
void modem_at_user_pipe_release(void);

#endif /* ZEPHYR_MODEM_AT_USER_PIPE_ */
