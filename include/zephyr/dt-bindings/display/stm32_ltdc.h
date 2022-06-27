/*
 * Copyright (c) 2022 Byte-Lab d.o.o. <dev@byte-lab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_STM32_LTDC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_STM32_LTDC_H_

/* Horizontal synchronization pulse polarity */
#define STM32_LTDC_HSPOL_ACTIVE_LOW     0x00000000
#define STM32_LTDC_HSPOL_ACTIVE_HIGH    0x80000000

/* Vertical synchronization pulse polarity */
#define STM32_LTDC_VSPOL_ACTIVE_LOW     0x00000000
#define STM32_LTDC_VSPOL_ACTIVE_HIGH    0x40000000

/* Data enable pulse polarity */
#define STM32_LTDC_DEPOL_ACTIVE_LOW     0x00000000
#define STM32_LTDC_DEPOL_ACTIVE_HIGH    0x20000000

/* Pixel clock polarity */
#define STM32_LTDC_PCPOL_ACTIVE_LOW     0x00000000
#define STM32_LTDC_PCPOL_ACTIVE_HIGH    0x10000000

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_STM32_LTDC_H_ */
