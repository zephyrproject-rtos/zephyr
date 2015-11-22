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


/**
 * Watchdog timer register block type.
 */
struct wdt_dw {
	volatile uint32_t wdt_cr;           /**< Control Register */
	volatile uint32_t wdt_torr;         /**< Timeout Range Register */
	volatile uint32_t wdt_ccvr;         /**< Current Counter Value Register */
	volatile uint32_t wdt_crr;          /**< Current Restart Register */
	volatile uint32_t wdt_stat;         /**< Interrupt Status Register */
	volatile uint32_t wdt_eoi;          /**< Interrupt Clear Register */
	volatile uint32_t wdt_comp_param_5; /**<  Component Parameters */
	volatile uint32_t wdt_comp_param_4; /**<  Component Parameters */
	volatile uint32_t wdt_comp_param_3; /**<  Component Parameters */
	volatile uint32_t wdt_comp_param_2; /**<  Component Parameters */
	volatile uint32_t wdt_comp_param_1; /**<  Component Parameters Register 1 */
	volatile uint32_t wdt_comp_version; /**<  Component Version Register */
	volatile uint32_t wdt_comp_type;    /**< Component Type Register */
};

/** WDT register block */
#define WDT_DW ((struct wdt_dw *)WDT_BASE_ADDR)


#define WDT_CRR_VAL                 0x76
#define WDT_CR_ENABLE               (1 << 0)
#define WDT_CR_INT_ENABLE           (1 << 1)        /* interrupt mode enable - mode1 */


#define WDT_DRV_NAME "wdt_dw"

struct wdt_dw_dev_config {
	uint32_t        base_address;
};

int wdt_dw_init(struct device *dev);

#endif /* WDT_DW_H_ */
