/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32MP1_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32MP1_RESET_H_

/**
 * Pack RCC register offset and bit in one 32-bit value.
 *
 * 5 LSBs are used to keep bit number in 32-bit RCC register.
 * Next 12 bits are used to keep reset set register offset.
 * Next 12 bits are used to keep reset clear register offset.
 *
 * @param bus STM32 bus name
 * @param bit Reset bit
 */
#define STM32_RESET(bus, bit)                                                                      \
	(((STM32_RESET_BUS_##bus##_CLR) << 17U) | ((STM32_RESET_BUS_##bus##_SET) << 5U) | (bit))

/* RCC bus reset register offset */
#define STM32_RESET_BUS_AHB2_SET   0x998
#define STM32_RESET_BUS_AHB2_CLR   0x99C
#define STM32_RESET_BUS_AHB3_SET   0x9A0
#define STM32_RESET_BUS_AHB3_CLR   0x9A4
#define STM32_RESET_BUS_AHB4_SET   0x9A8
#define STM32_RESET_BUS_AHB4_CLR   0x9AC
#define STM32_RESET_BUS_AHB5_SET   0x190
#define STM32_RESET_BUS_AHB5_CLR   0x194
#define STM32_RESET_BUS_AHB6_SET   0x198
#define STM32_RESET_BUS_AHB6_CLR   0x19C
#define STM32_RESET_BUS_TZAHB6_SET 0x1A0
#define STM32_RESET_BUS_TZAHB6_CLR 0x1A4
#define STM32_RESET_BUS_APB1_SET   0x980
#define STM32_RESET_BUS_APB1_CLR   0x984
#define STM32_RESET_BUS_APB2_SET   0x988
#define STM32_RESET_BUS_APB2_CLR   0x98C
#define STM32_RESET_BUS_APB3_SET   0x990
#define STM32_RESET_BUS_APB3_CLR   0x994
#define STM32_RESET_BUS_APB4_SET   0x180
#define STM32_RESET_BUS_APB4_CLR   0x184
#define STM32_RESET_BUS_APB5_SET   0x188
#define STM32_RESET_BUS_APB5_CLR   0x18C

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32MP1_RESET_H_ */
