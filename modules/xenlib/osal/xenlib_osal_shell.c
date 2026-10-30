/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xenlib Operating System Abstraction Layer (OSAL) shell runtime interface
 * implementation
 *
 */
#include <xenlib_osal_platform.h>

void xenlib_osal_shell_set_bypass(const xenlib_osal_shell_t *shell,
					xenlib_osal_shell_bypass_cb_t bypass,
					void *user_data)
{
	shell_set_bypass(shell, bypass, user_data);
}
