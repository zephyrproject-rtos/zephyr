/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/__assert.h>
#include <zephyr.h>


/**
 *
 * @brief Assert Action Handler
 *
 * This routine implements the action to be taken when an assertion fails.
 *
 * System designers may wish to substitute this implementation to take other
 * actions, such as logging program counter, line number, debug information
 * to a persistent repository and/or rebooting the system.
 *
 * @param N/A
 *
 * @return N/A
 */
__weak void assert_post_action(const char *file, unsigned int line)
{
	ARG_UNUSED(file);
	ARG_UNUSED(line);

#ifdef CONFIG_USERSPACE
	/* User threads aren't allowed to induce kernel panics; generate
	 * an oops instead.
	 */
	if (_is_user_context()) {
		k_oops();
	}
#endif

	k_panic();
}
