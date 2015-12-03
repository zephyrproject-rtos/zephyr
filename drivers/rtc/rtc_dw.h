/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _RTC_DW_H_
#define _RTC_DW_H_

#include <board.h>
#include <device.h>
#include <rtc.h>

#define RTC_CCVR		(0x0)
#define RTC_CMR			(0x4)
#define RTC_CLR			(0x8)
#define RTC_CCR			(0xC)
#define RTC_STAT		(0x10)
#define RTC_RSTAT		(0x14)
#define RTC_EOI			(0x18)
#define RTC_COMP_VERSION	(0x1C)

#define RTC_INTERRUPT_ENABLE	(1 << 0)
#define RTC_INTERRUPT_MASK	(1 << 1)
#define RTC_ENABLE		(1 << 2)
#define RTC_WRAP_ENABLE		(1 << 3)

#define RTC_CLK_DIV_EN		(1 << 2)
#define RTC_CLK_DIV_MASK	(0xF << 3)
#define RTC_CLK_DIV_1_HZ	(0xF << 3)
#define RTC_CLK_DIV_32768_HZ	(0x0 << 3)
#define RTC_CLK_DIV_8192_HZ	(0x2 << 3)
#define RTC_CLK_DIV_4096_HZ	(0x3 << 3)

struct rtc_dw_runtime {
	void (*rtc_dw_cb_fn)(struct device *dev);
};

struct rtc_dw_dev_config {
	uint32_t base_address;
};

int rtc_dw_init(struct device *dev);

#endif /* _RTC_DW_H_ */
