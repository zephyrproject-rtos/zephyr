/*
 * Copyright (c) 2025 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_WCH_CLOCK_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_WCH_CLOCK_COMMON_H_

#define WCH_CLOCK_CONFIG(bus, bit) (((WCH_CLOCK_CONFIG_BUS_##bus##) << 5) | (bit))
#define WCH_CLOCK_CONFIG_BUS(sys)  (((sys) >> 5) & 0xFF)
#define WCH_CLOCK_CONFIG_BIT(sys)  ((sys) & 0x1F)

#define WCH_CLKID_CLK_SYS 0
#define WCH_CLKID_CLK_PLL 1
#define WCH_CLKID_CLK_HB  2
#define WCH_CLKID_CLK_ADC 3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_WCH_CLOCK_COMMON_H_ */
