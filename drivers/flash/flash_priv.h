/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_PRIV_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_PRIV_H_

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static inline void flash_page_layout_not_implemented(void)
{
	k_panic();
}
#endif

#endif
