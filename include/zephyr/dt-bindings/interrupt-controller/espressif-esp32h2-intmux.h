/**
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Espressif ESP32-H2 interrupt source definitions for devicetree
 * @ingroup dt_esp32h2_intmux
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_ESP32H2_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_ESP32H2_INTMUX_H_

/**
 * @defgroup dt_esp32h2_intmux Espressif ESP32-H2 interrupt allocator
 * @brief Devicetree interrupt source numbers for the Espressif ESP32-H2.
 * @ingroup devicetree-interrupt_controller
 *
 * Interrupt source numbers for the Espressif ESP32-H2 interrupt allocator, used with the
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

#define PMU_INTR_SOURCE                   0  /**< PMU interrupt */
#define EFUSE_INTR_SOURCE                 1  /**< eFuse interrupt, level */
#define LP_RTC_TIMER_INTR_SOURCE          2  /**< LP RTC timer interrupt */
#define LP_BLE_TIMER_INTR_SOURCE          3  /**< LP BLE timer interrupt */
#define LP_WDT_INTR_SOURCE                4  /**< LP watchdog interrupt */
#define LP_PERI_TIMEOUT_INTR_SOURCE       5  /**< LP peripheral timeout interrupt */
#define LP_APM_M0_INTR_SOURCE             6  /**< LP APM M0 interrupt */
#define FROM_CPU_INTR0_SOURCE             7  /**< CPU interrupt 0, level */
#define FROM_CPU_INTR1_SOURCE             8  /**< CPU interrupt 1, level */
#define FROM_CPU_INTR2_SOURCE             9  /**< CPU interrupt 2, level */
#define FROM_CPU_INTR3_SOURCE             10 /**< CPU interrupt 3, level */
#define ASSIST_DEBUG_INTR_SOURCE          11 /**< Assist debug module interrupt, level */
#define TRACE_INTR_SOURCE                 12 /**< Trace interrupt */
#define CACHE_INTR_SOURCE                 13 /**< Cache interrupt */
#define CPU_PERI_TIMEOUT_INTR_SOURCE      14 /**< CPU peripheral timeout interrupt */
#define BT_MAC_INTR_SOURCE                15 /**< BT MAC interrupt */
#define BT_BB_INTR_SOURCE                 16 /**< BT BB interrupt, level */
#define BT_BB_NMI_INTR_SOURCE             17 /**< Bt Bb Nmi Intr Source */
#define COEX_INTR_SOURCE                  18 /**< Coexistence interrupt */
#define BLE_TIMER_INTR_SOURCE             19 /**< BLE timer interrupt */
#define BLE_SEC_INTR_SOURCE               20 /**< BLE SEC interrupt */
#define ZB_MAC_INTR_SOURCE                21 /**< ZigBee MAC interrupt */
#define GPIO_INTR_SOURCE                  22 /**< GPIO interrupt, level */
#define GPIO_NMI_SOURCE                   23 /**< GPIO interrupt, NMI */
#define PAU_INTR_SOURCE                   24 /**< PAU interrupt */
#define HP_PERI_TIMEOUT_INTR_SOURCE       25 /**< HP peripheral timeout interrupt */
#define HP_APM_M0_INTR_SOURCE             26 /**< HP APM M0 interrupt */
#define HP_APM_M1_INTR_SOURCE             27 /**< HP APM M1 interrupt */
#define HP_APM_M2_INTR_SOURCE             28 /**< HP APM M2 interrupt */
#define HP_APM_M3_INTR_SOURCE             29 /**< HP APM M3 interrupt */
#define MSPI_INTR_SOURCE                  30 /**< MSPI interrupt */
#define I2S1_INTR_SOURCE                  31 /**< I2S1 interrupt, level */
#define UHCI0_INTR_SOURCE                 32 /**< UHCI0 interrupt, level */
#define UART0_INTR_SOURCE                 33 /**< UART0 interrupt, level */
#define UART1_INTR_SOURCE                 34 /**< UART1 interrupt, level */
#define LEDC_INTR_SOURCE                  35 /**< LED PWM interrupt, level */
#define TWAI0_INTR_SOURCE                 36 /**< TWAI0 interrupt, level */
#define USB_SERIAL_JTAG_INTR_SOURCE       37 /**< USB Serial JTAG interrupt, level */
#define RMT_INTR_SOURCE                   38 /**< Remote controller interrupt, level */
#define I2C_EXT0_INTR_SOURCE              39 /**< I2C controller 0 interrupt, level */
#define I2C_EXT1_INTR_SOURCE              40 /**< I2C controller 1 interrupt, level */
#define TG0_T0_LEVEL_INTR_SOURCE          41 /**< Timer group 0, timer 0 interrupt, level */
#define TG0_WDT_LEVEL_INTR_SOURCE         42 /**< Timer group 0, watchdog interrupt, level */
#define TG1_T0_LEVEL_INTR_SOURCE          43 /**< Timer group 1, timer 0 interrupt, level */
#define TG1_WDT_LEVEL_INTR_SOURCE         44 /**< Timer group 1, watchdog interrupt, level */
#define SYSTIMER_TARGET0_EDGE_INTR_SOURCE 45 /**< System timer target 0 interrupt, edge */
#define SYSTIMER_TARGET1_EDGE_INTR_SOURCE 46 /**< System timer target 1 interrupt, edge */
#define SYSTIMER_TARGET2_EDGE_INTR_SOURCE 47 /**< System timer target 2 interrupt, edge */
#define APB_ADC_INTR_SOURCE               48 /**< APB ADC interrupt, level */
#define MCPWM0_INTR_SOURCE                49 /**< MCPWM0 interrupt, level */
#define PCNT_INTR_SOURCE                  50 /**< Pulse counter interrupt */
#define PARL_IO_TX_INTR_SOURCE            51 /**< Parallel IO TX interrupt */
#define PARL_IO_RX_INTR_SOURCE            52 /**< Parallel IO RX interrupt */
#define DMA_IN_CH0_INTR_SOURCE            53 /**< DMA IN channel 0 interrupt, level */
#define DMA_IN_CH1_INTR_SOURCE            54 /**< DMA IN channel 1 interrupt, level */
#define DMA_IN_CH2_INTR_SOURCE            55 /**< DMA IN channel 2 interrupt, level */
#define DMA_OUT_CH0_INTR_SOURCE           56 /**< DMA OUT channel 0 interrupt, level */
#define DMA_OUT_CH1_INTR_SOURCE           57 /**< DMA OUT channel 1 interrupt, level */
#define DMA_OUT_CH2_INTR_SOURCE           58 /**< DMA OUT channel 2 interrupt, level */
#define GSPI2_INTR_SOURCE                 59 /**< GSPI2 interrupt */
#define AES_INTR_SOURCE                   60 /**< AES accelerator interrupt, level */
#define SHA_INTR_SOURCE                   61 /**< SHA accelerator interrupt, level */
#define RSA_INTR_SOURCE                   62 /**< RSA accelerator interrupt, level */
#define ECC_INTR_SOURCE                   63 /**< ECC accelerator interrupt, level */
#define ECDSA_INTR_SOURCE                 64 /**< ECDSA interrupt */
#define MAX_INTR_SOURCE                   65 /**< Total number of interrupt sources */

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
 * 6 for "permanently disabled interrupt"
 * Interrupts 3, 4 and 7 are unavailable for PULP CPU as they are bound to Core-Local Interrupts
 * (CLINT)
 */
#define ESP_CPU_IRQ_0  0  /**< CPU interrupt line 0 */
#define ESP_CPU_IRQ_2  2  /**< CPU interrupt line 2 */
#define ESP_CPU_IRQ_5  5  /**< CPU interrupt line 5 */
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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_ESP32H2_INTMUX_H_ */
