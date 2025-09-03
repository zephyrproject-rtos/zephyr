/**
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Espressif ESP32-C6 interrupt source definitions for devicetree
 * @ingroup dt_esp32c6_intmux
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_ESP32C6_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_ESP32C6_INTMUX_H_

/**
 * @defgroup dt_esp32c6_intmux Espressif ESP32-C6 interrupt allocator
 * @brief Devicetree interrupt source numbers for the Espressif ESP32-C6.
 * @ingroup devicetree-interrupt_controller
 *
 * Interrupt source numbers for the Espressif ESP32-C6 interrupt allocator, used with the
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

#define WIFI_MAC_INTR_SOURCE              0  /**< Wi-Fi MAC, level */
#define WIFI_MAC_NMI_SOURCE               1  /**< Wi-Fi MAC, NMI */
#define WIFI_PWR_INTR_SOURCE              2  /**< Wi-Fi power interrupt */
#define WIFI_BB_INTR_SOURCE               3  /**< Wi-Fi BB, level */
#define BT_MAC_INTR_SOURCE                4  /**< Deprecated interrupt source */
#define BT_BB_INTR_SOURCE                 5  /**< BT BB, level */
#define BT_BB_NMI_SOURCE                  6  /**< BT BB, NMI */
#define LP_TIMER_INTR_SOURCE              7  /**< LP timer interrupt */
#define COEX_INTR_SOURCE                  8  /**< Coexistence interrupt */
#define BLE_TIMER_INTR_SOURCE             9  /**< BLE timer interrupt */
#define BLE_SEC_INTR_SOURCE               10 /**< BLE SEC interrupt */
#define I2C_MASTER_SOURCE                 11 /**< I2C Master, level */
#define ZB_MAC_SOURCE                     12 /**< ZigBee MAC interrupt */
#define PMU_INTR_SOURCE                   13 /**< PMU interrupt */
#define EFUSE_INTR_SOURCE                 14 /**< Efuse, level, not likely to use */
#define LP_RTC_TIMER_INTR_SOURCE          15 /**< LP RTC timer interrupt */
#define LP_UART_INTR_SOURCE               16 /**< LP UART interrupt */
#define LP_I2C_INTR_SOURCE                17 /**< LP I2C interrupt */
#define LP_WDT_INTR_SOURCE                18 /**< LP watchdog interrupt */
#define LP_PERI_TIMEOUT_INTR_SOURCE       19 /**< LP peripheral timeout interrupt */
#define LP_APM_M0_INTR_SOURCE             20 /**< LP APM M0 interrupt */
#define LP_APM_M1_INTR_SOURCE             21 /**< LP APM M1 interrupt */
#define FROM_CPU_INTR0_SOURCE             22 /**< Interrupt0 generated from a CPU, level */
#define FROM_CPU_INTR1_SOURCE             23 /**< Interrupt1 generated from a CPU, level */
#define FROM_CPU_INTR2_SOURCE             24 /**< Interrupt2 generated from a CPU, level */
#define FROM_CPU_INTR3_SOURCE             25 /**< Interrupt3 generated from a CPU, level */
#define ASSIST_DEBUG_INTR_SOURCE          26 /**< Assist debug module, LEVEL */
#define TRACE_INTR_SOURCE                 27 /**< Trace interrupt */
#define CACHE_INTR_SOURCE                 28 /**< Cache interrupt */
#define CPU_PERI_TIMEOUT_INTR_SOURCE      29 /**< CPU peripheral timeout interrupt */
#define GPIO_INTR_SOURCE                  30 /**< GPIO, level */
#define GPIO_NMI_SOURCE                   31 /**< GPIO, NMI */
#define PAU_INTR_SOURCE                   32 /**< PAU interrupt */
#define HP_PERI_TIMEOUT_INTR_SOURCE       33 /**< HP peripheral timeout interrupt */
#define MODEM_PERI_TIMEOUT_INTR_SOURCE    34 /**< Modem peripheral timeout interrupt */
#define HP_APM_M0_INTR_SOURCE             35 /**< HP APM M0 interrupt */
#define HP_APM_M1_INTR_SOURCE             36 /**< HP APM M1 interrupt */
#define HP_APM_M2_INTR_SOURCE             37 /**< HP APM M2 interrupt */
#define HP_APM_M3_INTR_SOURCE             38 /**< HP APM M3 interrupt */
#define LP_APM0_INTR_SOURCE               39 /**< LP APM0 interrupt */
#define MSPI_INTR_SOURCE                  40 /**< MSPI interrupt */
#define I2S1_INTR_SOURCE                  41 /**< I2S1, level */
#define UHCI0_INTR_SOURCE                 42 /**< UHCI0, level */
#define UART0_INTR_SOURCE                 43 /**< UART0, level */
#define UART1_INTR_SOURCE                 44 /**< UART1, level */
#define LEDC_INTR_SOURCE                  45 /**< LED PWM, level */
#define TWAI0_INTR_SOURCE                 46 /**< Can0, level */
#define TWAI1_INTR_SOURCE                 47 /**< Can1, level */
#define USB_SERIAL_JTAG_INTR_SOURCE       48 /**< USB, level */
#define RMT_INTR_SOURCE                   49 /**< Remote controller, level */
#define I2C_EXT0_INTR_SOURCE              50 /**< I2C controller1, level */
#define TG0_T0_LEVEL_INTR_SOURCE          51 /**< TIMER_GROUP0, TIMER0, level */
#define TG0_T1_LEVEL_INTR_SOURCE          52 /**< TIMER_GROUP0, TIMER1, level */
#define TG0_WDT_LEVEL_INTR_SOURCE         53 /**< TIMER_GROUP0, WATCH DOG, level */
#define TG1_T0_LEVEL_INTR_SOURCE          54 /**< TIMER_GROUP1, TIMER0, level */
#define TG1_T1_LEVEL_INTR_SOURCE          55 /**< TIMER_GROUP1, TIMER1, level */
#define TG1_WDT_LEVEL_INTR_SOURCE         56 /**< TIMER_GROUP1, WATCHDOG, level */
#define SYSTIMER_TARGET0_EDGE_INTR_SOURCE 57 /**< System timer 0, EDGE */
#define SYSTIMER_TARGET1_EDGE_INTR_SOURCE 58 /**< System timer 1, EDGE */
#define SYSTIMER_TARGET2_EDGE_INTR_SOURCE 59 /**< System timer 2, EDGE */
#define APB_ADC_INTR_SOURCE               60 /**< APB ADC, LEVEL */
#define MCPWM0_INTR_SOURCE                61 /**< MCPWM0, LEVEL */
#define PCNT_INTR_SOURCE                  62 /**< Pulse counter interrupt */
#define PARL_IO_INTR_SOURCE               63 /**< Parallel IO interrupt */
#define SLC0_INTR_SOURCE                  64 /**< SLC0 interrupt */
#define SLC_INTR_SOURCE                   65 /**< SLC interrupt */
#define DMA_IN_CH0_INTR_SOURCE            66 /**< General DMA IN channel 0, LEVEL */
#define DMA_IN_CH1_INTR_SOURCE            67 /**< General DMA IN channel 1, LEVEL */
#define DMA_IN_CH2_INTR_SOURCE            68 /**< General DMA IN channel 2, LEVEL */
#define DMA_OUT_CH0_INTR_SOURCE           69 /**< General DMA OUT channel 0, LEVEL */
#define DMA_OUT_CH1_INTR_SOURCE           70 /**< General DMA OUT channel 1, LEVEL */
#define DMA_OUT_CH2_INTR_SOURCE           71 /**< General DMA OUT channel 2, LEVEL */
#define GSPI2_INTR_SOURCE                 72 /**< GSPI2 interrupt */
#define AES_INTR_SOURCE                   73 /**< AES accelerator, level */
#define SHA_INTR_SOURCE                   74 /**< SHA accelerator, level */
#define RSA_INTR_SOURCE                   75 /**< RSA accelerator, level */
#define ECC_INTR_SOURCE                   76 /**< ECC accelerator, level */
#define MAX_INTR_SOURCE                   77 /**< Total number of interrupt sources */

/**
 * @brief Default interrupt priority.
 *
 * Zero will allocate low/medium levels of priority (ESP_INTR_FLAG_LOWMED).
 */
#define IRQ_DEFAULT_PRIORITY 1 /**< Irq Default Priority */

#define ESP_INTR_FLAG_SHARED (1 << 8) /**< Can be shared between ISRs */

/**
 * LP Core intmux
 */
#define LP_CORE_IO_INTR_SOURCE    0 /**< LP core IO interrupt */
#define LP_CORE_I2C_INTR_SOURCE   1 /**< LP core I2C interrupt */
#define LP_CORE_UART_INTR_SOURCE  2 /**< LP core UART interrupt */
#define LP_CORE_TIMER_INTR_SOURCE 3 /**< LP core timer interrupt */
#define LP_CORE_PMU_INTR_SOURCE   5 /**< LP core PMU interrupt */

/**
 * @brief CPU interrupt lines.
 *
 * 1 is for Wi-Fi
 * 6 for "permanently disabled interrupt"
 * 14 is for TEE
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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_ESP32C6_INTMUX_H_ */
