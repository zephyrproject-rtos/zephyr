/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C3_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C3_INTMUX_H_

#define WIFI_MAC_INTR_SOURCE                0
#define WIFI_MAC_NMI_SOURCE                 1
#define WIFI_PWR_INTR_SOURCE                2
#define WIFI_BB_INTR_SOURCE                 3
#define BT_MAC_INTR_SOURCE                  4
#define BT_BB_INTR_SOURCE                   5
#define BT_BB_NMI_SOURCE                    6
#define RWBT_INTR_SOURCE                    7
#define RWBLE_INTR_SOURCE                   8
#define RWBT_NMI_SOURCE                     9
#define RWBLE_NMI_SOURCE                    10
#define I2C_MASTER_SOURCE                   11
#define SLC0_INTR_SOURCE                    12
#define SLC1_INTR_SOURCE                    13
#define APB_CTRL_INTR_SOURCE                14
#define UHCI0_INTR_SOURCE                   15
#define GPIO_INTR_SOURCE                    16
#define GPIO_NMI_SOURCE                     17
#define SPI1_INTR_SOURCE                    18
#define SPI2_INTR_SOURCE                    19
#define I2S1_INTR_SOURCE                    20
#define UART0_INTR_SOURCE                   21
#define UART1_INTR_SOURCE                   22
#define LEDC_INTR_SOURCE                    23
#define EFUSE_INTR_SOURCE                   24
#define TWAI_INTR_SOURCE                    25
#define USB_INTR_SOURCE                     26
#define RTC_CORE_INTR_SOURCE                27
#define RMT_INTR_SOURCE                     28
#define I2C_EXT0_INTR_SOURCE                29
#define TIMER1_INTR_SOURCE                  30
#define TIMER2_INTR_SOURCE                  31
#define TG0_T0_LEVEL_INTR_SOURCE            32
#define TG0_WDT_LEVEL_INTR_SOURCE           33
#define TG1_T0_LEVEL_INTR_SOURCE            34
#define TG1_WDT_LEVEL_INTR_SOURCE           35
#define CACHE_IA_INTR_SOURCE                36
#define SYSTIMER_TARGET0_EDGE_INTR_SOURCE   37
#define SYSTIMER_TARGET1_EDGE_INTR_SOURCE   38
#define SYSTIMER_TARGET2_EDGE_INTR_SOURCE   39
#define SPI_MEM_REJECT_CACHE_INTR_SOURCE    40
#define ICACHE_PRELOAD0_INTR_SOURCE         41
#define ICACHE_SYNC0_INTR_SOURCE            42
#define APB_ADC_INTR_SOURCE                 43
#define DMA_CH0_INTR_SOURCE                 44
#define DMA_CH1_INTR_SOURCE                 45
#define DMA_CH2_INTR_SOURCE                 46
#define RSA_INTR_SOURCE                     47
#define AES_INTR_SOURCE                     48
#define SHA_INTR_SOURCE                     49
#define FROM_CPU_INTR0_SOURCE               50
#define FROM_CPU_INTR1_SOURCE               51
#define FROM_CPU_INTR2_SOURCE               52
#define FROM_CPU_INTR3_SOURCE               53
#define ASSIST_DEBUG_INTR_SOURCE            54
#define DMA_APBPERI_PMS_INTR_SOURCE         55
#define CORE0_IRAM0_PMS_INTR_SOURCE         56
#define CORE0_DRAM0_PMS_INTR_SOURCE         57
#define CORE0_PIF_PMS_INTR_SOURCE           58
#define CORE0_PIF_PMS_SIZE_INTR_SOURCE      59
#define BAK_PMS_VIOLATE_INTR_SOURCE         60
#define CACHE_CORE0_ACS_INTR_SOURCE         61

/* RISC-V supports priority values from 1 (lowest) to 15.
 * As interrupt controller for Xtensa and RISC-V is shared, this is
 * set to an intermediate and compatible value.
 */
#define IRQ_DEFAULT_PRIORITY	3

#define ESP_INTR_FLAG_SHARED	(1<<8)	/* Interrupt can be shared between ISRs */

#endif
