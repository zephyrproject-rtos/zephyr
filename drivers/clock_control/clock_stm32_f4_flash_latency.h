/*
 * Copyright 2026 Gowtham Palanichamy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Opt-in flash wait-state computation from actual board VDD.
 *
 * Included only when the `st,supply-microvolt` property is present on the
 * rcc node (see clock_stm32_ll_common.c). Applies only to
 * STM32F405xx/407xx/415xx/417xx -- the only STM32F4 sub-family covered by
 * the table below; other F4 members (e.g. F42x/43x, which use a different
 * table with a higher HCLK ceiling, or F401/411/446 etc., which aren't
 * characterized here at all) are rejected at compile time.
 *
 * Table reproduced verbatim from RM0090 Rev 21, Table 11 "Number of wait
 * states according to CPU clock (HCLK) frequency (STM32F405xx/07xx and
 * STM32F415xx/17xx)", Section 3.5.1. Each row is the exclusive-lower /
 * inclusive-upper HCLK bound for that wait-state count, per voltage band.
 * Voltage band selection below treats each band's lower bound as
 * inclusive (e.g. exactly 2.7V uses the 2.7-3.6V column); ST's own bands
 * share endpoints (2.7V appears in both the 2.4-2.7V and 2.7-3.6V column
 * headings), so this is spec-legal, not an approximation.
 *
 * The 1.8-2.1V column is characterized by ST only with the flash prefetch
 * buffer disabled (RM0090 Section 3.5.1, "Prefetch OFF"). Flash prefetch
 * defaults to enabled and is turned on by soc_early_init_hook() before
 * this ever runs, so a BUILD_ASSERT below enforces that prefetch is off
 * whenever the selected band is 1.8-2.1V.
 */
#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_STM32_F4_FLASH_LATENCY_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_STM32_F4_FLASH_LATENCY_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <stm32_ll_system.h>

#if !(defined(CONFIG_SOC_STM32F405XX) || defined(CONFIG_SOC_STM32F407XE) || \
	defined(CONFIG_SOC_STM32F407XG) || defined(CONFIG_SOC_STM32F415XX) || \
	defined(CONFIG_SOC_STM32F417XX))
#error "st,supply-microvolt is only characterized (RM0090 Table 11) for " \
	"STM32F405xx/407xx/415xx/417xx"
#endif

/* Sentinel for "outside Table 11's characterized range" -- distinct from
 * every real LL_FLASH_LATENCY_n value (all small non-negative constants).
 */
#define STM32F4_LATENCY_INVALID (-1)

/* Actual board VDD supply, in microvolts, from the opt-in DT property. */
#define STM32F4_SUPPLY_MICROVOLT DT_PROP(DT_NODELABEL(rcc), st_supply_microvolt)

/*
 * HCLK, straight from the rcc node's `clock-frequency` property.
 * clock-frequency is documented (st,stm32-rcc.yaml) and used tree-wide as
 * "SYSCLK / AHB prescaler" -- i.e. it is already HCLK, not SYSCLK -- and
 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC (what clock_stm32_ll_common.c passes
 * into the vendor RCC_CALC_FLASH_FREQ() macro) is derived directly from
 * this same property with no further division. Reading the DT property
 * again here (rather than dividing CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC by
 * the prescaler a second time) avoids under-provisioning wait-states on
 * any board with ahb-prescaler != 1.
 */
#define STM32F4_FLASH_HCLK_HZ DT_PROP(DT_NODELABEL(rcc), clock_frequency)

/* RM0090 Rev 21, Table 11 -- one row-selector macro per voltage band.
 * Resolves to STM32F4_LATENCY_INVALID when hz is outside that band's
 * characterized range.
 */
#define STM32F4_LATENCY_2V7_3V6(hz) \
	((hz) <= MHZ(30)  ? LL_FLASH_LATENCY_0 : \
	 (hz) <= MHZ(60)  ? LL_FLASH_LATENCY_1 : \
	 (hz) <= MHZ(90)  ? LL_FLASH_LATENCY_2 : \
	 (hz) <= MHZ(120) ? LL_FLASH_LATENCY_3 : \
	 (hz) <= MHZ(150) ? LL_FLASH_LATENCY_4 : \
	 (hz) <= MHZ(168) ? LL_FLASH_LATENCY_5 : STM32F4_LATENCY_INVALID)

#define STM32F4_LATENCY_2V4_2V7(hz) \
	((hz) <= MHZ(24)  ? LL_FLASH_LATENCY_0 : \
	 (hz) <= MHZ(48)  ? LL_FLASH_LATENCY_1 : \
	 (hz) <= MHZ(72)  ? LL_FLASH_LATENCY_2 : \
	 (hz) <= MHZ(96)  ? LL_FLASH_LATENCY_3 : \
	 (hz) <= MHZ(120) ? LL_FLASH_LATENCY_4 : \
	 (hz) <= MHZ(144) ? LL_FLASH_LATENCY_5 : \
	 (hz) <= MHZ(168) ? LL_FLASH_LATENCY_6 : STM32F4_LATENCY_INVALID)

#define STM32F4_LATENCY_2V1_2V4(hz) \
	((hz) <= MHZ(22)  ? LL_FLASH_LATENCY_0 : \
	 (hz) <= MHZ(44)  ? LL_FLASH_LATENCY_1 : \
	 (hz) <= MHZ(66)  ? LL_FLASH_LATENCY_2 : \
	 (hz) <= MHZ(88)  ? LL_FLASH_LATENCY_3 : \
	 (hz) <= MHZ(110) ? LL_FLASH_LATENCY_4 : \
	 (hz) <= MHZ(132) ? LL_FLASH_LATENCY_5 : \
	 (hz) <= MHZ(154) ? LL_FLASH_LATENCY_6 : \
	 (hz) <= MHZ(168) ? LL_FLASH_LATENCY_7 : STM32F4_LATENCY_INVALID)

/* Prefetch OFF column -- see file header note above. */
#define STM32F4_LATENCY_1V8_2V1(hz) \
	((hz) <= MHZ(20)  ? LL_FLASH_LATENCY_0 : \
	 (hz) <= MHZ(40)  ? LL_FLASH_LATENCY_1 : \
	 (hz) <= MHZ(60)  ? LL_FLASH_LATENCY_2 : \
	 (hz) <= MHZ(80)  ? LL_FLASH_LATENCY_3 : \
	 (hz) <= MHZ(100) ? LL_FLASH_LATENCY_4 : \
	 (hz) <= MHZ(120) ? LL_FLASH_LATENCY_5 : \
	 (hz) <= MHZ(140) ? LL_FLASH_LATENCY_6 : \
	 (hz) <= MHZ(160) ? LL_FLASH_LATENCY_7 : STM32F4_LATENCY_INVALID)

/* Voltage-band selector: each band's lower bound is treated as inclusive. */
#define STM32F4_FLASH_LATENCY_FROM_VDD(vdd_uv, hz) \
	((vdd_uv) >= 2700000 ? STM32F4_LATENCY_2V7_3V6(hz) : \
	 (vdd_uv) >= 2400000 ? STM32F4_LATENCY_2V4_2V7(hz) : \
	 (vdd_uv) >= 2100000 ? STM32F4_LATENCY_2V1_2V4(hz) : \
	 (vdd_uv) >= 1800000 ? STM32F4_LATENCY_1V8_2V1(hz) : STM32F4_LATENCY_INVALID)

#define STM32F4_FLASH_LATENCY \
	STM32F4_FLASH_LATENCY_FROM_VDD(STM32F4_SUPPLY_MICROVOLT, STM32F4_FLASH_HCLK_HZ)

BUILD_ASSERT(STM32F4_FLASH_LATENCY != STM32F4_LATENCY_INVALID,
	     "st,supply-microvolt / clock-frequency combination on the rcc "
	     "devicetree node is outside RM0090 Table 11's characterized "
	     "wait-state range for STM32F405xx/407xx/415xx/417xx. Lower the "
	     "clock, or remove st,supply-microvolt to fall back to the "
	     "VOS-only default (at your own risk).");

BUILD_ASSERT(STM32F4_SUPPLY_MICROVOLT >= 2100000 || !IS_ENABLED(CONFIG_STM32_FLASH_PREFETCH),
	     "RM0090 Table 11 characterizes the 1.8-2.1V band with the flash "
	     "prefetch buffer disabled; set CONFIG_STM32_FLASH_PREFETCH=n.");

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_STM32_F4_FLASH_LATENCY_H_ */
