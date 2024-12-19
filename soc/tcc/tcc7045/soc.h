/*
 * Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _BOARD_SOC__H_
#define _BOARD_SOC__H_

#ifndef FALSE
#define FALSE (0U)
#endif

#ifndef TRUE
#define TRUE (1U)
#endif

#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif

#ifndef NULL
#define NULL (0)
#endif

#ifndef ON
#define ON  (TRUE)
#define OFF (FALSE)
#endif

#define MCU_BSP_UART_BASE  (0xA0200000UL)
#define MCU_BSP_GDMA_BASE  (0xA0800000UL)
#define MCU_BSP_TIC_BASE   (0xA0F10000UL)
#define MCU_BSP_GPIO_BASE  (0xA0F22000UL)
#define CLOCK_BASE_ADDR    (0xA0F24000UL)
#define MCU_BSP_PMIO_BASE  (0xA0F28800UL)
#define MCU_BSP_TIMER_BASE (0xA0F2A000UL)

#define SYS_PWR_EN (GPIO_GPC(2UL))

int32_t soc_div64_to_32(unsigned long long *pullDividend, uint32_t uiDivisor, uint32_t *puiRem);

typedef unsigned char boolean; /* for use with TRUE/FALSE        */

#endif /* _BOARD_SOC__H_ */
