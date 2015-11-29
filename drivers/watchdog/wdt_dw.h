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

#ifndef WDT_DW_H_
#define WDT_DW_H_

#include <board.h>
#include <device.h>
#include <watchdog.h>

#define WDT_CR			(0x0)
#define WDT_TORR		(0x4)
#define WDT_CCVR		(0x8)
#define WDT_CRR			(0xC)
#define WDT_STAT		(0x10)
#define WDT_EIO			(0x14)
#define WDT_COMP_PARAM_5	(0xE4)
#define WDT_COMP_PARAM_4	(0xE8)
#define WDT_COMP_PARAM_3	(0xEC)
#define WDT_COMP_PARAM_2	(0xF0)
#define WDT_COMP_PARAM_1	(0xF4)
#define WDT_COMP_VERSION	(0xF8)
#define WDT_COMP_TYPE		(0xFC)


/**
 * WDT Mode type.
 */

#define WDT_CRR_VAL		(0x76)
#define WDT_CR_ENABLE		(1 << 0)
#define WDT_CR_INT_ENABLE	(1 << 1)


#define WDT_DRV_NAME "wdt_dw"

struct wdt_dw_dev_config {
	uint32_t        base_address;
};

int wdt_dw_init(struct device *dev);

#endif /* WDT_DW_H_ */
