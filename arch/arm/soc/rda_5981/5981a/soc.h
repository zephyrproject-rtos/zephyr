/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _RDA5981_SOC_H_
#define _RDA5981_SOC_H_

#include "soc_irq.h"

/* Base addresses */
#define RDA_ROM_BASE          (0x00000000UL)
#define RDA_IRAM_BASE         (0x00100000UL)
#define RDA_DRAM_BASE         (0x00180000UL)
#define RDA_PSRAM_BASE        (0x10000000UL)
#define RDA_FLASH_BASE        (0x14000000UL)
#define RDA_ICACHE_BASE       (0x18000000UL)

#define RDA_PER_BASE          (0x40000000UL)
#define RDA_AHB0_BASE         (0x40000000UL)
#define RDA_APB_BASE          (RDA_AHB0_BASE)
#define RDA_AHB1_BASE         (0x40100000UL)
#define RDA_PERBTBND_BASE     (0x42000000UL)
#define RDA_CM4_BASE          (0xE0000000UL)

/* APB peripherals                                                            */
#define RDA_SCU_BASE          (RDA_APB_BASE  + 0x00000)
#define RDA_GPIO_BASE         (RDA_APB_BASE  + 0x01000)
#define RDA_TIM0_BASE         (RDA_APB_BASE  + 0x02000)
#define RDA_TIM1_BASE         (RDA_APB_BASE  + 0x02008)
#define RDA_TIMINTST_BASE     (RDA_APB_BASE  + 0x02010)
#define RDA_I2C0_BASE         (RDA_APB_BASE  + 0x03000)

/* AHB0 peripherals                                                           */
#define RDA_PWM_BASE          (RDA_AHB0_BASE + 0x04000)
#define RDA_PSRAMCFG_BASE     (RDA_AHB0_BASE + 0x05000)
#define RDA_SDMMC_BASE        (RDA_AHB0_BASE + 0x06000)
#define RDA_I2C_BASE          (RDA_AHB0_BASE + 0x10000)
#define RDA_TRAP_BASE         (RDA_AHB0_BASE + 0x11000)
#define RDA_UART0_BASE        (RDA_AHB0_BASE + 0x12000)
#define RDA_EXIF_BASE         (RDA_AHB0_BASE + 0x13000)
#define RDA_PA_BASE           (RDA_AHB0_BASE + 0x20000)
#define RDA_CE_BASE           (RDA_AHB0_BASE + 0x22000)
#define RDA_MON_BASE          (RDA_AHB0_BASE + 0x24000)
#define RDA_SDIO_BASE         (RDA_AHB0_BASE + 0x30000)
#define RDA_USB_BASE          (RDA_AHB0_BASE + 0x31000)

/* AHB1 peripherals                                                           */
#define RDA_MEMC_BASE         (RDA_AHB1_BASE + 0x00000)
#define RDA_UART1_BASE        (RDA_AHB1_BASE + 0x80000)
#define RDA_DMACFG_BASE       (RDA_AHB1_BASE + 0x81000)

/* EXIF peripherals                                                           */
#define RDA_SPI0_BASE         (RDA_EXIF_BASE + 0x00000)
#define RDA_I2S_BASE          (RDA_EXIF_BASE + 0x0000C)

/* MISC peripherals                                                           */
#define RDA_WDT_BASE          (RDA_SCU_BASE  + 0x0000C)
#define RDA_PINCFG_BASE       (RDA_GPIO_BASE + 0x00044)

#define RDA_HAL_IRQ_NUM	(15 + 16)
#endif /* _RDA5981_SOC_H_ */
