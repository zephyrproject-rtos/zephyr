/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C2_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C2_INTMUX_H_

#define WIFI_MAC_INTR_SOURCE                   0
#define WIFI_MAC_NMI_SOURCE                    1
#define WIFI_PWR_INTR_SOURCE                   2
#define WIFI_BB_INTR_SOURCE                    3
#define BT_MAC_INTR_SOURCE                     4
#define BT_BB_INTR_SOURCE                      5
#define BT_BB_NMI_SOURCE                       6
#define LP_TIMER_SOURCE                        7
#define COEX_SOURCE                            8
#define BLE_TIMER_SOURCE                       9
#define BLE_SEC_SOURCE                         10
#define I2C_MASTER_SOURCE                      11
#define APB_CTRL_INTR_SOURCE                   12
#define GPIO_INTR_SOURCE                       13
#define GPIO_NMI_SOURCE                        14
#define SPI1_INTR_SOURCE                       15
#define SPI2_INTR_SOURCE                       16
#define UART0_INTR_SOURCE                      17
#define UART1_INTR_SOURCE                      18
#define LEDC_INTR_SOURCE                       19
#define EFUSE_INTR_SOURCE                      20
#define RTC_CORE_INTR_SOURCE                   21
#define I2C_EXT0_INTR_SOURCE                   22
#define TG0_T0_LEVEL_INTR_SOURCE               23
#define TG0_WDT_LEVEL_INTR_SOURCE              24
#define CACHE_IA_INTR_SOURCE                   25
#define SYSTIMER_TARGET0_EDGE_INTR_SOURCE      26
#define SYSTIMER_TARGET1_EDGE_INTR_SOURCE      27
#define SYSTIMER_TARGET2_EDGE_INTR_SOURCE      28
#define SPI_MEM_REJECT_CACHE_INTR_SOURCE       29
#define ICACHE_PRELOAD0_INTR_SOURCE            30
#define ICACHE_SYNC0_INTR_SOURCE               31
#define APB_ADC_INTR_SOURCE                    32
#define DMA_CH0_INTR_SOURCE                    33
#define SHA_INTR_SOURCE                        34
#define ECC_INTR_SOURCE                        35
#define FROM_CPU_INTR0_SOURCE                  36
#define FROM_CPU_INTR1_SOURCE                  37
#define FROM_CPU_INTR2_SOURCE                  38
#define FROM_CPU_INTR3_SOURCE                  39
#define ASSIST_DEBUG_INTR_SOURCE               40
#define CORE0_PIF_PMS_SIZE_INTR_SOURCE         41
#define CACHE_CORE0_ACS_INTR_SOURCE            42

/* RISC-V supports priority values from 1 (lowest) to 15.
 * As interrupt controller for Xtensa and RISC-V is shared, this is
 * set to an intermediate and compatible value.
 */
#define IRQ_DEFAULT_PRIORITY	3

#define ESP_INTR_FLAG_SHARED	(1<<8)	/* Interrupt can be shared between ISRs */

#endif
