/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inc/hw_ints.h>

/* for HAL_SWI_RADIO_IRQ */
#define LL_SWI4_IRQn (INT_I2C_IRQ - 16)
/* for HAL_SWI_JOB_IRQ */
#define LL_SWI5_IRQn (INT_AUX_SWEV1 - 16)

#define LL_RTC0_IRQn (INT_AON_RTC_COMB - 16)
#define LL_RADIO_IRQn (INT_RFC_HW_COMB - 16)
