/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DeviceFamily_CC13X2
#include <ti/drivers/rf/RF.h>

#define HAL_TICKER_CNTR_CLK_FREQ_HZ 4000000U

#define HAL_TICKER_CNTR_CMP_OFFSET_MIN RF_convertUsToRatTicks(96)

#define HAL_TICKER_CNTR_SET_LATENCY 0

#define HAL_TICKER_US_TO_TICKS(x) RF_convertUsToRatTicks(x)

/* Macro returning remainder in nanoseconds */
#define HAL_TICKER_REMAINDER(x) \
	( \
		( \
			((u64_t) (x) * 1000000000UL) \
			- ((u64_t)HAL_TICKER_US_TO_TICKS(x) \
			* (1000000000UL/HAL_TICKER_CNTR_CLK_FREQ_HZ)) \
		) \
		/ 1000UL \
	)

#define HAL_TICKER_TICKS_TO_US(x) RF_convertRatTicksToUs(x)

#define HAL_TICKER_CNTR_MSBIT 31

#define HAL_TICKER_CNTR_MASK 0xFFFFFFFF

/* Macro defining the remainder resolution/range
 * ~ 1000000 * HAL_TICKER_TICKS_TO_US(1)
 */
#define HAL_TICKER_REMAINDER_RANGE \
	HAL_TICKER_TICKS_TO_US(1000000)

/* Macro defining the margin for positioning re-scheduled nodes */
#define HAL_TICKER_RESCHEDULE_MARGIN \
		HAL_TICKER_US_TO_TICKS(150)
