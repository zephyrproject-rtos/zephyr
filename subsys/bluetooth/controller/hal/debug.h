/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_CTLR_ASSERT_HANDLER
void bt_ctlr_assert_handle(char *file, uint32_t line);
#define LL_ASSERT(cond) if (!(cond)) { \
				bt_ctlr_assert_handle(__FILE__, \
						      __LINE__); \
			}
#else
#define LL_ASSERT(cond) BT_ASSERT(cond)
#endif

#include "hal/debug_vendor_hal.h"
