/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

#define RTC_SAM_WPMR_ENABLE 0x52544301
#define RTC_SAM_WPMR_DISABLE 0x52544300

void soc_sysc_disable_write_protection(void)
{
	REG_RTC_WPMR = RTC_SAM_WPMR_DISABLE;
}

void soc_sysc_enable_write_protection(void)
{
	REG_RTC_WPMR = RTC_SAM_WPMR_ENABLE;
}
