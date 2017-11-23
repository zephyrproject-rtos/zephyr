/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FLASH_PRIV_H__
#define __FLASH_PRIV_H__

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static inline void flash_page_layout_not_implemented(void)
{
	k_panic();
}
#endif

#endif
