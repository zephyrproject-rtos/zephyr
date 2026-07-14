/**
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Espressif ESP32 interrupt source definitions for devicetree
 * @ingroup dt_esp32_intmux
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_XTENSA_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_XTENSA_INTMUX_H_

/**
 * @defgroup dt_esp32_intmux Espressif ESP32 interrupt allocator
 * @brief Devicetree interrupt source numbers for the Espressif ESP32.
 * @ingroup devicetree-interrupt_controller
 *
 * Interrupt source numbers for the Espressif ESP32 interrupt allocator, used with the
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

#define WIFI_MAC_INTR_SOURCE                0   /* WiFi MAC, level */
#define WIFI_MAC_NMI_SOURCE                 1   /* WiFi MAC, NMI, use if MAC needs fix in NMI */
#define WIFI_BB_INTR_SOURCE                 2   /* WiFi BB, level, we can do some calibration */
#define BT_MAC_INTR_SOURCE                  3   /* will be cancelled */
#define BT_BB_INTR_SOURCE                   4   /* BB, level */
#define BT_BB_NMI_SOURCE                    5   /* BT BB, NMI, use if BB have bug to fix in NMI */
#define RWBT_INTR_SOURCE                    6   /* RWBT, level */
#define RWBLE_INTR_SOURCE                   7   /* RWBLE, level */
#define RWBT_NMI_SOURCE                     8   /* RWBT, NMI, use if RWBT has bug to fix in NMI */
#define RWBLE_NMI_SOURCE                    9   /* RWBLE, NMI, use if RWBT has bug to fix in NMI */
#define SLC0_INTR_SOURCE                    10  /* SLC0, level */
#define SLC1_INTR_SOURCE                    11  /* SLC1, level */
#define UHCI0_INTR_SOURCE                   12  /* UHCI0, level */
#define UHCI1_INTR_SOURCE                   13  /* UHCI1, level */
#define TG0_T0_LEVEL_INTR_SOURCE            14  /* TIMER_GROUP0, TIMER0, level */
#define TG0_T1_LEVEL_INTR_SOURCE            15  /* TIMER_GROUP0, TIMER1, level */
#define TG0_WDT_LEVEL_INTR_SOURCE           16  /* TIMER_GROUP0, WATCHDOG, level */
#define TG0_LACT_LEVEL_INTR_SOURCE          17  /* TIMER_GROUP0, LACT, level */
#define TG1_T0_LEVEL_INTR_SOURCE            18  /* TIMER_GROUP1, TIMER0, level */
#define TG1_T1_LEVEL_INTR_SOURCE            19  /* TIMER_GROUP1, TIMER1, level */
#define TG1_WDT_LEVEL_INTR_SOURCE           20  /* TIMER_GROUP1, WATCHDOG, level */
#define TG1_LACT_LEVEL_INTR_SOURCE          21  /* TIMER_GROUP1, LACT, level */
#define GPIO_INTR_SOURCE                    22  /* interrupt of GPIO, level */
#define GPIO_NMI_SOURCE                     23  /* interrupt of GPIO, NMI */
#define FROM_CPU_INTR0_SOURCE               24  /* int0 from a CPU, level */
#define FROM_CPU_INTR1_SOURCE               25  /* int1 from a CPU, level */
#define FROM_CPU_INTR2_SOURCE               26  /* int2 from a CPU, level, for DPORT Access */
#define FROM_CPU_INTR3_SOURCE               27  /* int3 from a CPU, level, for DPORT Access */
#define SPI0_INTR_SOURCE                    28  /* SPI0, level, for $ Access, do not use this */
#define SPI1_INTR_SOURCE                    29  /* SPI1, level, flash r/w, do not use this */
#define SPI2_INTR_SOURCE                    30  /* SPI2, level */
#define SPI3_INTR_SOURCE                    31  /* SPI3, level */
#define I2S0_INTR_SOURCE                    32  /* I2S0, level */
#define I2S1_INTR_SOURCE                    33  /* I2S1, level */
#define UART0_INTR_SOURCE                   34  /* UART0, level */
#define UART1_INTR_SOURCE                   35  /* UART1, level */
#define UART2_INTR_SOURCE                   36  /* UART2, level */
#define SDIO_HOST_INTR_SOURCE               37  /* SD/SDIO/MMC HOST, level */
#define ETH_MAC_INTR_SOURCE                 38  /* ethernet mac, level */
#define PWM0_INTR_SOURCE                    39  /* PWM0, level, Reserved */
#define PWM1_INTR_SOURCE                    40  /* PWM1, level, Reserved */
#define PWM2_INTR_SOURCE                    41  /* PWM2, level */
#define PWM3_INTR_SOURCE                    42  /* PWM3, level */
#define LEDC_INTR_SOURCE                    43  /* LED PWM, level */
#define EFUSE_INTR_SOURCE                   44  /* efuse, level, not likely to use */
#define TWAI_INTR_SOURCE                    45  /* twai, level */
#define CAN_INTR_SOURCE                     TWAI_INTR_SOURCE
#define RTC_CORE_INTR_SOURCE                46  /* rtc core, level, include rtc watchdog */
#define RMT_INTR_SOURCE                     47  /* remote controller, level */
#define PCNT_INTR_SOURCE                    48  /* pulse count, level */
#define I2C_EXT0_INTR_SOURCE                49  /* I2C controller1, level */
#define I2C_EXT1_INTR_SOURCE                50  /* I2C controller0, level */
#define RSA_INTR_SOURCE                     51  /* RSA accelerator, level */
#define SPI1_DMA_INTR_SOURCE                52  /* SPI1 DMA, for flash r/w, do not use it */
#define SPI2_DMA_INTR_SOURCE                53  /* SPI2 DMA, level */
#define SPI3_DMA_INTR_SOURCE                54  /* interrupt of SPI3 DMA, level */
#define WDT_INTR_SOURCE                     55  /* will be cancelled */
#define TIMER1_INTR_SOURCE                  56  /* will be cancelled */
#define TIMER2_INTR_SOURCE                  57  /* will be cancelled */
#define TG0_T0_EDGE_INTR_SOURCE             58  /* TIMER_GROUP0, TIMER0, EDGE */
#define TG0_T1_EDGE_INTR_SOURCE             59  /* TIMER_GROUP0, TIMER1, EDGE */
#define TG0_WDT_EDGE_INTR_SOURCE            60  /* TIMER_GROUP0, WATCH DOG, EDGE */
#define TG0_LACT_EDGE_INTR_SOURCE           61  /* TIMER_GROUP0, LACT, EDGE */
#define TG1_T0_EDGE_INTR_SOURCE             62  /* TIMER_GROUP1, TIMER0, EDGE */
#define TG1_T1_EDGE_INTR_SOURCE             63  /* TIMER_GROUP1, TIMER1, EDGE */
#define TG1_WDT_EDGE_INTR_SOURCE            64  /* TIMER_GROUP1, WATCHDOG, EDGE */
#define TG1_LACT_EDGE_INTR_SOURCE           65  /* TIMER_GROUP0, LACT, EDGE */
#define MMU_IA_INTR_SOURCE                  66  /* MMU Invalid Access, LEVEL */
#define MPU_IA_INTR_SOURCE                  67  /* MPU Invalid Access, LEVEL */
#define CACHE_IA_INTR_SOURCE                68  /* Cache Invalid Access, LEVEL */
#define MAX_INTR_SOURCE                     69  /* total number of interrupt sources */

/**
 * @brief Default interrupt priority.
 *
 * Zero will allocate low/medium levels of priority (ESP_INTR_FLAG_LOWMED).
 */
#define IRQ_DEFAULT_PRIORITY 0 /**< Irq Default Priority */

#define ESP_INTR_FLAG_SHARED (1 << 8) /**< Can be shared between ISRs */
#define ESP_INTR_FLAG_IRAM   (1 << 10) /**< ISR can run with cache disabled */

/**
 * @brief CPU interrupt lines.
 */
/**
 * Interrupt 0 reserved for WMAC (Wifi)
 */
#define ESP_CPU_IRQ_L1_LVL_0  0 /**< CPU interrupt line 0 */
/**
 * Interrupt 1 reserved for BT/BLE Host HCI DMA when BTDM_CTRL_HCI_MODE_UART_H4 is enabled
 */
#define ESP_CPU_IRQ_L1_LVL_1  1 /**< CPU interrupt line 1 */
#define ESP_CPU_IRQ_L1_LVL_2  2 /**< CPU interrupt line 2 */
#define ESP_CPU_IRQ_L1_LVL_3  3 /**< CPU interrupt line 3 */
/**
 * Interrupt 4 reserved for WBB
 */
#define ESP_CPU_IRQ_L1_LVL_4  4 /**< CPU interrupt line 4 */
/**
 * Interrupt 5 reserved for BT/BLE Controller when Bluetooth HLI is enabled
 */
#define ESP_CPU_IRQ_L1_LVL_5  5  /**< CPU interrupt line 5 */
#define ESP_CPU_IRQ_L1_LVL_8  8  /**< CPU interrupt line 8 */
#define ESP_CPU_IRQ_L1_LVL_9  9  /**< CPU interrupt line 9 */
#define ESP_CPU_IRQ_L1_EDG_10 10 /**< CPU interrupt line 10 */
#define ESP_CPU_IRQ_L1_LVL_12 12 /**< CPU interrupt line 12 */
#define ESP_CPU_IRQ_L1_LVL_13 13 /**< CPU interrupt line 13 */
/**
 * Interrupt 14 reserved for NMI (Non-Maskable Interrupts)
 */
#define ESP_CPU_IRQ_L1_LVL_17 17 /**< CPU interrupt line 17 */
#define ESP_CPU_IRQ_L1_LVL_18 18 /**< CPU interrupt line 18 */
#define ESP_CPU_IRQ_L2_LVL_19 19 /**< CPU interrupt line 19 */
#define ESP_CPU_IRQ_L2_LVL_20 20 /**< CPU interrupt line 20 */
#define ESP_CPU_IRQ_L2_LVL_21 21 /**< CPU interrupt line 21 */
#define ESP_CPU_IRQ_L3_EDG_22 22 /**< CPU interrupt line 22 */
#define ESP_CPU_IRQ_L3_LVL_23 23 /**< CPU interrupt line 23 */
/**
 * Interrupt 24 reserved for T1 WDT
 */
#define ESP_CPU_IRQ_L4_LVL_24 24 /**< CPU interrupt line 24 */
/**
 * Interrupt 25 reserved for Memory access and cache errors when ESP_SYSTEM_CHECK_INT_LEVEL_4
 * is enabled Reserved for BT/BLE Controller when ESP_SYSTEM_CHECK_INT_LEVEL_5 is enabled
 */
#define ESP_CPU_IRQ_L4_LVL_25 25 /**< CPU interrupt line 25 */
/**
 * Interrupt 26 reserved for T1 WDT, Memory access and cache errors when
 * ESP_SYSTEM_CHECK_INT_LEVEL_5 is enabled
 */
#define ESP_CPU_IRQ_L5_LVL_26 26 /**< CPU interrupt line 26 */
#define ESP_CPU_IRQ_L3_LVL_27 27 /**< CPU interrupt line 27 */
/**
 * Interrupt 28 reserved for IPC when ESP_SYSTEM_CHECK_INT_LEVEL_4 is enabled
 */
#define ESP_CPU_IRQ_L4_EDG_28 28 /**< CPU interrupt line 28 */
#define ESP_CPU_IRQ_L4_EDG_30 30 /**< CPU interrupt line 30 */
#define ESP_CPU_IRQ_L5_LVL_31 31 /**< CPU interrupt line 31 */

/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP_XTENSA_INTMUX_H_ */
