/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/assert.h"

#ifdef CONFIG_BT_CTLR_ASSERT_HANDLER
void bt_ctlr_assert_handle(char *file, uint32_t line);
#define LL_ASSERT(cond) \
		if (!(cond)) { \
			BT_ASSERT_PRINT(cond); \
			bt_ctlr_assert_handle(__FILE__, __LINE__); \
		}
#define LL_ASSERT_MSG(cond, fmt, ...) \
		if (!(cond)) { \
			BT_ASSERT_PRINT(cond); \
			BT_ASSERT_PRINT_MSG(fmt, ##__VA_ARGS__); \
			bt_ctlr_assert_handle(__FILE__, __LINE__); \
		}
#else
#define LL_ASSERT(cond) \
		BT_ASSERT(cond)
#define LL_ASSERT_MSG(cond, fmt, ...) \
		BT_ASSERT_MSG(cond, fmt, ##__VA_ARGS__)
#endif

#if defined(CONFIG_BT_CTLR_ASSERT_VENDOR)
#define LL_ASSERT_INFO1(cond, param) \
		BT_ASSERT_VND(cond, param, 0)
#define LL_ASSERT_INFO2(cond, param1, param2) \
		BT_ASSERT_VND(cond, param1, param2)
#else
#define LL_ASSERT_INFO1(cond, param) \
		LL_ASSERT_MSG(cond, "param: %u", param)
#define LL_ASSERT_INFO2(cond, param1, param2) \
		LL_ASSERT_MSG(cond, "param1: %u param2: %u", param1, param2)
#endif /* CONFIG_BT_CTLR_ASSERT_VENDOR */

#if defined(CONFIG_BT_CTLR_ASSERT_OVERHEAD_START)
#define LL_ASSERT_OVERHEAD(overhead) \
	LL_ASSERT_MSG(false, "%s: Actual EVENT_OVERHEAD_START_US = %u", \
		      __func__, HAL_TICKER_TICKS_TO_US(overhead));
#else /* !CONFIG_BT_CTLR_ASSERT_OVERHEAD_START */
#define LL_ASSERT_OVERHEAD(overhead) ARG_UNUSED(overhead)
#endif /* !CONFIG_BT_CTLR_ASSERT_OVERHEAD_START */

#include "hal/debug_vendor_hal.h"
