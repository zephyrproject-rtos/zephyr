/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "nrf_802154_assert_zephyr.h"

#if defined(CONFIG_NRF_802154_ASSERT_ZEPHYR_MINIMAL)

__weak void nrf_802154_assert_handler(void)
{
#ifdef CONFIG_USERSPACE
	/* User threads aren't allowed to induce kernel panics; generate
	 * an oops instead.
	 */
	if (k_is_user_context()) {
		k_oops();
	}
#endif

	k_panic();
}

#endif /* CONFIG_NRF_802154_ASSERT_ZEPHYR_MINIMAL */
