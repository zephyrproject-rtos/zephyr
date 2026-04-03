/*
 * Copyright (c) 2016-2025 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/assert.h"

#if defined(CONFIG_BT_CTLR_ASSERT_HANDLER)
void bt_ctlr_assert_handle(char *file, uint32_t line);

#if defined(CONFIG_BT_CTLR_ASSERT_OPTIMIZE_FOR_SIZE)
BUILD_ASSERT(IS_ENABLED(CONFIG_CPU_CORTEX_M));
/* Generate assertion as undefined instruction exception.
 */
#define LL_ASSERT(x) \
	do { \
		if (unlikely(!(x))) { \
			__asm__ inline volatile (".inst 0xde00\n"); \
		} \
	} while (0)

#else /* !CONFIG_BT_CTLR_ASSERT_OPTIMIZE_FOR_SIZE */
/* Generate assertion with file name and line number.
 * NOTE: Variable code size increase per assertion check, depends on full file name path string
 *       length.
 */
#define LL_ASSERT(cond) \
		if (unlikely(!(cond))) { \
			BT_ASSERT_PRINT(cond); \
			bt_ctlr_assert_handle(__FILE__, __LINE__); \
		}
#endif /* !CONFIG_BT_CTLR_ASSERT_OPTIMIZE_FOR_SIZE */

#define LL_ASSERT_MSG(cond, fmt, ...) \
		if (unlikely(!(cond))) { \
			BT_ASSERT_PRINT(cond); \
			BT_ASSERT_PRINT_MSG(fmt, ##__VA_ARGS__); \
			bt_ctlr_assert_handle(__FILE__, __LINE__); \
		}

#else /* !CONFIG_BT_CTLR_ASSERT_HANDLER */

#if defined(CONFIG_BT_CTLR_ASSERT_OPTIMIZE_FOR_SIZE)
BUILD_ASSERT(IS_ENABLED(CONFIG_CPU_CORTEX_M));
/* Generate assertion as undefined instruction exception.
 */
#define LL_ASSERT(x) \
	do { \
		if (unlikely(!(x))) { \
			__asm__ inline volatile (".inst 0xde00\n"); \
		} \
	} while (0)

#else /* !CONFIG_BT_CTLR_ASSERT_OPTIMIZE_FOR_SIZE */
#define LL_ASSERT(cond) \
		BT_ASSERT(cond)
#endif /* !CONFIG_BT_CTLR_ASSERT_OPTIMIZE_FOR_SIZE */

#define LL_ASSERT_MSG(cond, fmt, ...) \
		BT_ASSERT_MSG(cond, fmt, ##__VA_ARGS__)
#endif /* !CONFIG_BT_CTLR_ASSERT_HANDLER */

/* Fatal asserts.
 * The Controller will otherwise misbehave causing memory leak or system-wide memory corruptions due
 * to uncontrolled DMA transfers etc.
 * It is not safe to disable these assertion checks.
 */
#define LL_ASSERT_ERR(cond) LL_ASSERT(cond)

/* Development asserts.
 * The Controller will continue to function without memory leak or corruption with these assertion
 * checks disabled. Example, run-time mis-aligned memory access etc. which do otherwise implicitly
 * cause CPU fault during development testing. But these type of asserted are essentially required
 * for debugging, code and unit test coverage during development cycle.
 */
#if defined(CONFIG_BT_CTLR_ASSERT_DEBUG)
#define LL_ASSERT_DBG(cond) LL_ASSERT(cond)
#else /* !CONFIG_BT_CTLR_ASSERT_DEBUG */
#define LL_ASSERT_DBG(cond) ARG_UNUSED((cond))
#endif /* !CONFIG_BT_CTLR_ASSERT_DEBUG */

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
