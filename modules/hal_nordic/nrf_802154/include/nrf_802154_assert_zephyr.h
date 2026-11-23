/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRF_802154_ASSERT_ZEPHYR_H__
#define NRF_802154_ASSERT_ZEPHYR_H__

#if defined(CONFIG_NRF_802154_ASSERT_ZEPHYR)

#include <zephyr/sys/__assert.h>

#define NRF_802154_ASSERT(condition) __ASSERT_NO_MSG(condition)

#elif defined(CONFIG_NRF_802154_ASSERT_ZEPHYR_MINIMAL)

extern void nrf_802154_assert_handler(void);

#define NRF_802154_ASSERT(condition) \
	do {                             \
		if (!(condition)) {          \
			nrf_802154_assert_handler(); \
		}                            \
	} while (0)

#endif /* CONFIG_NRF_802154_ASSERT_ZEPHYR_MINIMAL */

#endif /* NRF_802154_ASSERT_ZEPHYR_H__*/
