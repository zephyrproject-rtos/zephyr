/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "hal/cntr.h"

void hal_ticker_instance0_trigger_set(uint32_t value)
{
#if defined(CONFIG_BT_CTLR_NRF_GRTC)
	cntr_cmp_set(HAL_CNTR_GRTC_CC_IDX_TICKER, value);
#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	cntr_cmp_set(0U, value);
#endif /* !CONFIG_BT_CTLR_NRF_GRTC */
}
