/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __COUNTER_ACE_V1X_RTC_REGS__
#define __COUNTER_ACE_V1X_RTC_REGS__

#if CONFIG_ACE_V1X_RTC_COUNTER

#define ACE_RTC_COUNTER_ID DT_NODELABEL(ace_rtc_counter)

#define ACE_RTCWC	(DT_REG_ADDR(ACE_RTC_COUNTER_ID))
#define ACE_RTCWC_LO ACE_RTCWC
#define ACE_RTCWC_HI ACE_RTCWC + 0x04

#endif

#endif /*__COUNTER_ACE_V1X_RTC_REGS__*/
