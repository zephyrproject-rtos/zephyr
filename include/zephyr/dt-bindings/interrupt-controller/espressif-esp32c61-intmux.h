/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP32-C61 interrupt multiplexer definitions for device tree bindings
 * @ingroup dt_esp32c61_intmux
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C61_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C61_INTMUX_H_

#include <zephyr/dt-bindings/interrupt-controller/espressif-intmux-common.h>

/**
 * @defgroup dt_esp32c61_intmux Espressif ESP32-C61 interrupt allocator
 * @brief Devicetree interrupt source numbers for the Espressif ESP32-C61.
 * @ingroup devicetree-interrupt_controller
 *
 * Interrupt source numbers for the Espressif ESP32-C61 interrupt allocator, used with the
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

#define WIFI_MAC_INTR_SOURCE           0  /**< Wi-Fi MAC interrupt, level */
#define WIFI_MAC_NMI_SOURCE            1  /**< Wi-Fi MAC interrupt, NMI */
#define WIFI_PWR_INTR_SOURCE           2  /**< Wi-Fi power interrupt */
#define WIFI_BB_INTR_SOURCE            3  /**< Wi-Fi BB interrupt, level */
#define BT_MAC_INTR_SOURCE             4  /**< BT MAC interrupt */
#define BT_BB_INTR_SOURCE              5  /**< BT BB interrupt, level */
#define BT_BB_NMI_SOURCE               6  /**< BT BB interrupt, NMI */
#define LP_TIMER_INTR_SOURCE           7  /**< LP timer interrupt */
#define COEX_INTR_SOURCE               8  /**< Coexistence interrupt */
#define BLE_TIMER_INTR_SOURCE          9  /**< BLE timer interrupt */
#define BLE_SEC_INTR_SOURCE            10 /**< BLE SEC interrupt */
#define I2C_MASTER_SOURCE              11 /**< I2C master interrupt, level */
#define ZB_MAC_SOURCE                  12 /**< ZigBee MAC interrupt */
#define PMU_INTR_SOURCE                13 /**< PMU interrupt */
#define EFUSE_INTR_SOURCE              14 /**< eFuse interrupt, level */
#define LP_RTC_TIMER_INTR_SOURCE       15 /**< LP RTC timer interrupt */
#define LP_WDT_INTR_SOURCE             16 /**< LP watchdog interrupt */
#define LP_PERI_TIMEOUT_INTR_SOURCE    17 /**< LP peripheral timeout interrupt */
#define LP_APM_M0_INTR_SOURCE          18 /**< LP APM M0 interrupt */
#define FROM_CPU_INTR0_SOURCE          19 /**< CPU interrupt 0, level */
#define FROM_CPU_INTR1_SOURCE          20 /**< CPU interrupt 1, level */
#define FROM_CPU_INTR2_SOURCE          21 /**< CPU interrupt 2, level */
#define FROM_CPU_INTR3_SOURCE          22 /**< CPU interrupt 3, level */
#define ASSIST_DEBUG_INTR_SOURCE       23 /**< Assist debug module interrupt, level */
#define TRACE_INTR_SOURCE              24 /**< Trace interrupt */
#define CACHE_INTR_SOURCE              25 /**< Cache interrupt */
#define CPU_PERI_TIMEOUT_INTR_SOURCE   26 /**< CPU peripheral timeout interrupt */
#define GPIO_INTR_SOURCE               27 /**< GPIO interrupt, level */
#define GPIO_EXT_SOURCE                28 /**< GPIO external interrupt */
#define PAU_INTR_SOURCE                29 /**< PAU interrupt */
#define HP_PERI_TIMEOUT_INTR_SOURCE    30 /**< HP peripheral timeout interrupt */
#define MODEM_PERI_TIMEOUT_INTR_SOURCE 31 /**< Modem peripheral timeout interrupt */
#define HP_APM_M0_INTR_SOURCE          32 /**< HP APM M0 interrupt */
#define HP_APM_M1_INTR_SOURCE          33 /**< HP APM M1 interrupt */
#define HP_APM_M2_INTR_SOURCE          34 /**< HP APM M2 interrupt */
#define HP_APM_M3_INTR_SOURCE          35 /**< HP APM M3 interrupt */
#define CPU_APM_M0_INTR_SOURCE         36 /**< CPU APM M0 interrupt */
#define CPU_APM_M1_INTR_SOURCE         37 /**< CPU APM M1 interrupt */
#define MSPI_INTR_SOURCE               38 /**< MSPI interrupt */
#define I2S0_INTR_SOURCE               39 /**< I2S0 interrupt, level */
#define UART0_INTR_SOURCE              40 /**< UART0 interrupt, level */
#define UART1_INTR_SOURCE              41 /**< UART1 interrupt, level */
#define UART2_INTR_SOURCE              42 /**< UART2 interrupt, level */
#define LEDC_INTR_SOURCE               43 /**< LED PWM interrupt, level */
#define USB_SERIAL_JTAG_INTR_SOURCE    44 /**< USB Serial JTAG interrupt, level */
#define I2C_EXT0_INTR_SOURCE           45 /**< I2C controller 0 interrupt, level */
#define TG0_T0_LEVEL_INTR_SOURCE       46 /**< Timer group 0, timer 0 interrupt, level */
#define TG0_T1_LEVEL_INTR_SOURCE       47 /**< Timer group 0, timer 1 interrupt, level */
#define TG0_WDT_LEVEL_INTR_SOURCE      48 /**< Timer group 0, watchdog interrupt, level */
#define TG1_T0_LEVEL_INTR_SOURCE       49 /**< Timer group 1, timer 0 interrupt, level */
#define TG1_T1_LEVEL_INTR_SOURCE       50 /**< Timer group 1, timer 1 interrupt, level */
#define TG1_WDT_LEVEL_INTR_SOURCE      51 /**< Timer group 1, watchdog interrupt, level */
#define SYSTIMER_TARGET0_INTR_SOURCE   52 /**< System timer target 0 interrupt */
#define SYSTIMER_TARGET1_INTR_SOURCE   53 /**< System timer target 1 interrupt */
#define SYSTIMER_TARGET2_INTR_SOURCE   54 /**< System timer target 2 interrupt */
#define APB_ADC_INTR_SOURCE            55 /**< APB ADC interrupt, level */
#define SLC0_INTR_SOURCE               56 /**< SLC0 interrupt */
#define SLC1_INTR_SOURCE               57 /**< SLC1 interrupt */
#define DMA_IN_CH0_INTR_SOURCE         58 /**< DMA IN channel 0 interrupt, level */
#define DMA_IN_CH1_INTR_SOURCE         59 /**< DMA IN channel 1 interrupt, level */
#define DMA_OUT_CH0_INTR_SOURCE        60 /**< DMA OUT channel 0 interrupt, level */
#define DMA_OUT_CH1_INTR_SOURCE        61 /**< DMA OUT channel 1 interrupt, level */
#define GSPI2_INTR_SOURCE              62 /**< GSPI2 interrupt */
#define SHA_INTR_SOURCE                63 /**< SHA accelerator interrupt, level */
#define ECC_INTR_SOURCE                64 /**< ECC accelerator interrupt, level */
#define ECDSA_INTR_SOURCE              65 /**< ECDSA interrupt */
#define MAX_INTR_SOURCE                66 /**< Total number of interrupt sources */

/**
 * @brief Default interrupt priority.
 *
 * The CPU interrupt priority is a per-line hardware field with a valid range of
 * 1..7; 0 leaves the line permanently masked. Matches the priority the HAL's
 * esp_cpu_intr_get_desc() reports for this SoC.
 */
#define IRQ_DEFAULT_PRIORITY 1 /**< Irq Default Priority */

/**
 * @brief CPU interrupt lines.
 *
 * The interrupt matrix routes a peripheral source onto one of these lines.
 * 1 is reserved for the Wi-Fi controller and 6 for the allocator's
 * "disabled interrupt" sink, so neither is listed here. 30 and 31 are further
 * reserved for ESP-TEE when that is enabled.
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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C61_INTMUX_H_ */
