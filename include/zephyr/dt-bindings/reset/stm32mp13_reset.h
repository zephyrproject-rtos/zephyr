/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32MP13_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32MP13_RESET_H_

/**
 * Pack RCC register offset and bit in one 32-bit value.
 *
 * bits[4..0] stores the reset controller bit in 32bit RCC register
 * bits[16..5] stores the reset controller set register offset from RCC base
 * bits[28..17] stores the reset controller clear register offset from RCC base
 *
 * @param bus STM32 bus name
 * @param bit Reset bit
 */
#define STM32_RESET(bus, bit)                                                                      \
	(((STM32_RESET_BUS_##bus##_CLR) << 17U) | ((STM32_RESET_BUS_##bus##_SET) << 5U) | (bit))

/* RCC bus reset register offset */
#define STM32_RESET_BUS_AHB2_SET   0x6D0
#define STM32_RESET_BUS_AHB2_CLR   0x6D4
#define STM32_RESET_BUS_AHB4_SET   0x6E0
#define STM32_RESET_BUS_AHB4_CLR   0x6E4
#define STM32_RESET_BUS_AHB5_SET   0x6E8
#define STM32_RESET_BUS_AHB5_CLR   0x6EC
#define STM32_RESET_BUS_AHB6_SET   0x6F0
#define STM32_RESET_BUS_AHB6_CLR   0x6F4
#define STM32_RESET_BUS_APB1_SET   0x6A0
#define STM32_RESET_BUS_APB1_CLR   0x6A4
#define STM32_RESET_BUS_APB2_SET   0x6A8
#define STM32_RESET_BUS_APB2_CLR   0x6AC
#define STM32_RESET_BUS_APB3_SET   0x6B0
#define STM32_RESET_BUS_APB3_CLR   0x6B4
#define STM32_RESET_BUS_APB4_SET   0x6B8
#define STM32_RESET_BUS_APB4_CLR   0x6BC
#define STM32_RESET_BUS_APB5_SET   0x6C0
#define STM32_RESET_BUS_APB5_CLR   0x6C4
#define STM32_RESET_BUS_APB6_SET   0x6C8
#define STM32_RESET_BUS_APB6_CLR   0x6CC

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32MP13_RESET_H_ */
