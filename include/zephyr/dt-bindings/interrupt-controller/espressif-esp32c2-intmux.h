/**
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Espressif ESP32-C2 interrupt source definitions for devicetree
 * @ingroup dt_esp32c2_intmux
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_ESP32C2_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_ESP32C2_INTMUX_H_

/**
 * @defgroup dt_esp32c2_intmux Espressif ESP32-C2 interrupt allocator
 * @brief Devicetree interrupt source numbers for the Espressif ESP32-C2.
 * @ingroup devicetree-interrupt_controller
 *
 * Interrupt source numbers for the Espressif ESP32-C2 interrupt allocator, used with the
 * <tt>espressif,esp32-intc</tt> compatible interrupt controller. An interrupt is described by three
 * cells: the interrupt source, the priority and a flags cell. Source numbers follow the pattern
 * @c \<SIGNAL\>_INTR_SOURCE; @ref IRQ_DEFAULT_PRIORITY selects the default priority.
 *
 * @code{.dts}
 * &uart0 {
 *         interrupts = <UART0_INTR_SOURCE IRQ_DEFAULT_PRIORITY 0>;
 * };
 * @endcode
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#define WIFI_MAC_INTR_SOURCE              0  /**< Wi-Fi MAC interrupt, level */
#define WIFI_MAC_NMI_SOURCE               1  /**< Wi-Fi MAC interrupt, NMI */
#define WIFI_PWR_INTR_SOURCE              2  /**< Wi-Fi power interrupt */
#define WIFI_BB_INTR_SOURCE               3  /**< Wi-Fi BB interrupt, level */
#define BT_MAC_INTR_SOURCE                4  /**< BT MAC interrupt */
#define BT_BB_INTR_SOURCE                 5  /**< BT BB interrupt, level */
#define BT_BB_NMI_SOURCE                  6  /**< BT BB interrupt, NMI */
#define LP_TIMER_SOURCE                   7  /**< LP timer interrupt */
#define COEX_SOURCE                       8  /**< Coexistence interrupt */
#define BLE_TIMER_SOURCE                  9  /**< BLE timer interrupt */
#define BLE_SEC_SOURCE                    10 /**< BLE SEC interrupt */
#define I2C_MASTER_SOURCE                 11 /**< I2C master interrupt, level */
#define APB_CTRL_INTR_SOURCE              12 /**< APB control interrupt */
#define GPIO_INTR_SOURCE                  13 /**< GPIO interrupt, level */
#define GPIO_NMI_SOURCE                   14 /**< GPIO interrupt, NMI */
#define SPI1_INTR_SOURCE                  15 /**< SPI1 interrupt, level */
#define SPI2_INTR_SOURCE                  16 /**< SPI2 interrupt, level */
#define UART0_INTR_SOURCE                 17 /**< UART0 interrupt, level */
#define UART1_INTR_SOURCE                 18 /**< UART1 interrupt, level */
#define LEDC_INTR_SOURCE                  19 /**< LED PWM interrupt, level */
#define EFUSE_INTR_SOURCE                 20 /**< eFuse interrupt, level */
#define RTC_CORE_INTR_SOURCE              21 /**< RTC core interrupt, level */
#define I2C_EXT0_INTR_SOURCE              22 /**< I2C controller 0 interrupt, level */
#define TG0_T0_LEVEL_INTR_SOURCE          23 /**< Timer group 0, timer 0 interrupt, level */
#define TG0_WDT_LEVEL_INTR_SOURCE         24 /**< Timer group 0, watchdog interrupt, level */
#define CACHE_IA_INTR_SOURCE              25 /**< Cache invalid access interrupt, level */
#define SYSTIMER_TARGET0_EDGE_INTR_SOURCE 26 /**< System timer target 0 interrupt, edge */
#define SYSTIMER_TARGET1_EDGE_INTR_SOURCE 27 /**< System timer target 1 interrupt, edge */
#define SYSTIMER_TARGET2_EDGE_INTR_SOURCE 28 /**< System timer target 2 interrupt, edge */
#define SPI_MEM_REJECT_CACHE_INTR_SOURCE  29 /**< SPI memory/cache reject interrupt, level */
#define ICACHE_PRELOAD0_INTR_SOURCE       30 /**< ICache preload interrupt, level */
#define ICACHE_SYNC0_INTR_SOURCE          31 /**< ICache sync done interrupt, level */
#define APB_ADC_INTR_SOURCE               32 /**< APB ADC interrupt, level */
#define DMA_CH0_INTR_SOURCE               33 /**< DMA channel 0 interrupt, level */
#define SHA_INTR_SOURCE                   34 /**< SHA accelerator interrupt, level */
#define ECC_INTR_SOURCE                   35 /**< ECC accelerator interrupt, level */
#define FROM_CPU_INTR0_SOURCE             36 /**< CPU interrupt 0, level */
#define FROM_CPU_INTR1_SOURCE             37 /**< CPU interrupt 1, level */
#define FROM_CPU_INTR2_SOURCE             38 /**< CPU interrupt 2, level */
#define FROM_CPU_INTR3_SOURCE             39 /**< CPU interrupt 3, level */
#define ASSIST_DEBUG_INTR_SOURCE          40 /**< Assist debug module interrupt, level */
#define CORE0_PIF_PMS_SIZE_INTR_SOURCE    41 /**< Core 0 PIF PMS size interrupt */
#define CACHE_CORE0_ACS_INTR_SOURCE       42 /**< Core 0 cache access interrupt */

/**
 * @brief Default interrupt priority.
 *
 * Zero will allocate low/medium levels of priority (ESP_INTR_FLAG_LOWMED).
 */
#define IRQ_DEFAULT_PRIORITY 1 /**< Irq Default Priority */

#define ESP_INTR_FLAG_SHARED (1 << 8) /**< Can be shared between ISRs */

/**
 * @brief CPU interrupt lines.
 *
 * 1 is for Wi-Fi
 * 6 for "permanently disabled interrupt", named INT_MUX_DISABLED_INTNO in the interrupt allocator
 */
#define ESP_CPU_IRQ_0  0  /**< CPU interrupt line 0 */
#define ESP_CPU_IRQ_2  2  /**< CPU interrupt line 2 */
#define ESP_CPU_IRQ_3  3  /**< CPU interrupt line 3 */
#define ESP_CPU_IRQ_4  4  /**< CPU interrupt line 4 */
#define ESP_CPU_IRQ_5  5  /**< CPU interrupt line 5 */
#define ESP_CPU_IRQ_7  7  /**< CPU interrupt line 7 */
#define ESP_CPU_IRQ_8  8  /**< CPU interrupt line 8 */
#define ESP_CPU_IRQ_9  9  /**< CPU interrupt line 9 */
#define ESP_CPU_IRQ_10 10 /**< CPU interrupt line 10 */
#define ESP_CPU_IRQ_11 11 /**< CPU interrupt line 11 */
#define ESP_CPU_IRQ_12 12 /**< CPU interrupt line 12 */
#define ESP_CPU_IRQ_13 13 /**< CPU interrupt line 13 */
#define ESP_CPU_IRQ_14 14 /**< CPU interrupt line 14 */
#define ESP_CPU_IRQ_15 15 /**< CPU interrupt line 15 */
#define ESP_CPU_IRQ_16 16 /**< CPU interrupt line 16 */
#define ESP_CPU_IRQ_17 17 /**< CPU interrupt line 17 */
#define ESP_CPU_IRQ_18 18 /**< CPU interrupt line 18 */
#define ESP_CPU_IRQ_19 19 /**< CPU interrupt line 19 */
#define ESP_CPU_IRQ_20 20 /**< CPU interrupt line 20 */
#define ESP_CPU_IRQ_21 21 /**< CPU interrupt line 21 */
#define ESP_CPU_IRQ_22 22 /**< CPU interrupt line 22 */
#define ESP_CPU_IRQ_23 23 /**< CPU interrupt line 23 */
#define ESP_CPU_IRQ_24 24 /**< CPU interrupt line 24 */
#define ESP_CPU_IRQ_25 25 /**< CPU interrupt line 25 */
#define ESP_CPU_IRQ_26 26 /**< CPU interrupt line 26 */
#define ESP_CPU_IRQ_27 27 /**< CPU interrupt line 27 */
#define ESP_CPU_IRQ_28 28 /**< CPU interrupt line 28 */
#define ESP_CPU_IRQ_29 29 /**< CPU interrupt line 29 */
#define ESP_CPU_IRQ_30 30 /**< CPU interrupt line 30 */
#define ESP_CPU_IRQ_31 31 /**< CPU interrupt line 31 */

/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_ESP32C2_INTMUX_H_ */
