/*
 * Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_SOC_TCC_TCC7045_SOC_H_
#define ZEPHYR_SOC_TCC_TCC7045_SOC_H_

#define TCC_NULL_PTR (void *)0

#define TCC_ON  1U
#define TCC_OFF 0U

#define MCU_BSP_UART_BASE  0xA0200000UL
#define MCU_BSP_GDMA_BASE  0xA0800000UL
#define MCU_BSP_TIC_BASE   0xA0F10000UL
#define MCU_BSP_GPIO_BASE  0xA0F22000UL
#define CLOCK_BASE_ADDR    0xA0F24000UL
#define MCU_BSP_PMIO_BASE  0xA0F28800UL
#define MCU_BSP_TIMER_BASE 0xA0F2A000UL

#define SYS_PWR_EN (GPIO_GPC(2UL))

int32_t soc_div64_to_32(uint64_t *pullDividend, uint32_t uiDivisor, uint32_t *puiRem);

#endif /* ZEPHYR_SOC_TCC_TCC7045_SOC_H_ */
