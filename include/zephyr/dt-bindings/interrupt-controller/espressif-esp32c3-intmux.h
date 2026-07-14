/**
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Espressif ESP32-C3 interrupt source definitions for devicetree
 * @ingroup dt_esp32c3_intmux
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_ESP32C3_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_ESP32C3_INTMUX_H_

/**
 * @defgroup dt_esp32c3_intmux Espressif ESP32-C3 interrupt allocator
 * @brief Devicetree interrupt source numbers for the Espressif ESP32-C3.
 * @ingroup devicetree-interrupt_controller
 *
 * Interrupt source numbers for the Espressif ESP32-C3 interrupt allocator, used with the
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
#define RWBT_INTR_SOURCE                  7  /**< RWBT interrupt, level */
#define RWBLE_INTR_SOURCE                 8  /**< RWBLE interrupt, level */
#define RWBT_NMI_SOURCE                   9  /**< RWBT interrupt, NMI */
#define RWBLE_NMI_SOURCE                  10 /**< RWBLE interrupt, NMI */
#define I2C_MASTER_SOURCE                 11 /**< I2C master interrupt, level */
#define SLC0_INTR_SOURCE                  12 /**< SLC0 interrupt */
#define SLC1_INTR_SOURCE                  13 /**< SLC1 interrupt */
#define APB_CTRL_INTR_SOURCE              14 /**< APB control interrupt */
#define UHCI0_INTR_SOURCE                 15 /**< UHCI0 interrupt, level */
#define GPIO_INTR_SOURCE                  16 /**< GPIO interrupt, level */
#define GPIO_NMI_SOURCE                   17 /**< GPIO interrupt, NMI */
#define SPI1_INTR_SOURCE                  18 /**< SPI1 interrupt, level */
#define SPI2_INTR_SOURCE                  19 /**< SPI2 interrupt, level */
#define I2S1_INTR_SOURCE                  20 /**< I2S1 interrupt, level */
#define UART0_INTR_SOURCE                 21 /**< UART0 interrupt, level */
#define UART1_INTR_SOURCE                 22 /**< UART1 interrupt, level */
#define LEDC_INTR_SOURCE                  23 /**< LED PWM interrupt, level */
#define EFUSE_INTR_SOURCE                 24 /**< eFuse interrupt, level */
#define TWAI_INTR_SOURCE                  25 /**< TWAI interrupt, level */
#define USB_INTR_SOURCE                   26 /**< USB interrupt, level */
#define RTC_CORE_INTR_SOURCE              27 /**< RTC core interrupt, level */
#define RMT_INTR_SOURCE                   28 /**< Remote controller interrupt, level */
#define I2C_EXT0_INTR_SOURCE              29 /**< I2C controller 0 interrupt, level */
#define TIMER1_INTR_SOURCE                30 /**< Timer 1 interrupt (deprecated) */
#define TIMER2_INTR_SOURCE                31 /**< Timer 2 interrupt (deprecated) */
#define TG0_T0_LEVEL_INTR_SOURCE          32 /**< Timer group 0, timer 0 interrupt, level */
#define TG0_WDT_LEVEL_INTR_SOURCE         33 /**< Timer group 0, watchdog interrupt, level */
#define TG1_T0_LEVEL_INTR_SOURCE          34 /**< Timer group 1, timer 0 interrupt, level */
#define TG1_WDT_LEVEL_INTR_SOURCE         35 /**< Timer group 1, watchdog interrupt, level */
#define CACHE_IA_INTR_SOURCE              36 /**< Cache invalid access interrupt, level */
#define SYSTIMER_TARGET0_EDGE_INTR_SOURCE 37 /**< System timer target 0 interrupt, edge */
#define SYSTIMER_TARGET1_EDGE_INTR_SOURCE 38 /**< System timer target 1 interrupt, edge */
#define SYSTIMER_TARGET2_EDGE_INTR_SOURCE 39 /**< System timer target 2 interrupt, edge */
#define SPI_MEM_REJECT_CACHE_INTR_SOURCE  40 /**< SPI memory/cache reject interrupt, level */
#define ICACHE_PRELOAD0_INTR_SOURCE       41 /**< ICache preload interrupt, level */
#define ICACHE_SYNC0_INTR_SOURCE          42 /**< ICache sync done interrupt, level */
#define APB_ADC_INTR_SOURCE               43 /**< APB ADC interrupt, level */
#define DMA_CH0_INTR_SOURCE               44 /**< DMA channel 0 interrupt, level */
#define DMA_CH1_INTR_SOURCE               45 /**< DMA channel 1 interrupt, level */
#define DMA_CH2_INTR_SOURCE               46 /**< DMA channel 2 interrupt, level */
#define RSA_INTR_SOURCE                   47 /**< RSA accelerator interrupt, level */
#define AES_INTR_SOURCE                   48 /**< AES accelerator interrupt, level */
#define SHA_INTR_SOURCE                   49 /**< SHA accelerator interrupt, level */
#define FROM_CPU_INTR0_SOURCE             50 /**< CPU interrupt 0, level */
#define FROM_CPU_INTR1_SOURCE             51 /**< CPU interrupt 1, level */
#define FROM_CPU_INTR2_SOURCE             52 /**< CPU interrupt 2, level */
#define FROM_CPU_INTR3_SOURCE             53 /**< CPU interrupt 3, level */
#define ASSIST_DEBUG_INTR_SOURCE          54 /**< Assist debug module interrupt, level */
#define DMA_APBPERI_PMS_INTR_SOURCE       55 /**< DMA APB peripheral PMS interrupt */
#define CORE0_IRAM0_PMS_INTR_SOURCE       56 /**< Core 0 IRAM0 PMS interrupt */
#define CORE0_DRAM0_PMS_INTR_SOURCE       57 /**< Core 0 DRAM0 PMS interrupt */
#define CORE0_PIF_PMS_INTR_SOURCE         58 /**< Core 0 PIF PMS interrupt */
#define CORE0_PIF_PMS_SIZE_INTR_SOURCE    59 /**< Core 0 PIF PMS size interrupt */
#define BAK_PMS_VIOLATE_INTR_SOURCE       60 /**< Backup PMS violation interrupt */
#define CACHE_CORE0_ACS_INTR_SOURCE       61 /**< Core 0 cache access interrupt */

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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_ESP32C3_INTMUX_H_ */
