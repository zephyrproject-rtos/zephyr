/**
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Espressif ESP32-S3 interrupt source definitions for devicetree
 * @ingroup dt_esp32s3_intmux
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32S3_XTENSA_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32S3_XTENSA_INTMUX_H_

/**
 * @defgroup dt_esp32s3_intmux Espressif ESP32-S3 interrupt allocator
 * @brief Devicetree interrupt source numbers for the Espressif ESP32-S3.
 * @ingroup devicetree-interrupt_controller
 *
 * Interrupt source numbers for the Espressif ESP32-S3 interrupt allocator, used with the
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
#define RWBT_INTR_SOURCE                  7  /**< RWBT, level */
#define RWBLE_INTR_SOURCE                 8  /**< RWBLE, level */
#define RWBT_NMI_SOURCE                   9  /**< RWBT, NMI */
#define RWBLE_NMI_SOURCE                  10 /**< RWBLE, NMI */
#define I2C_MASTER_SOURCE                 11 /**< I2C Master, level */
#define SLC0_INTR_SOURCE                  12 /**< SLC0, level */
#define SLC1_INTR_SOURCE                  13 /**< SLC1, level */
#define UHCI0_INTR_SOURCE                 14 /**< UHCI0, level */
#define UHCI1_INTR_SOURCE                 15 /**< UHCI1, level */
#define GPIO_INTR_SOURCE                  16 /**< GPIO, level */
#define GPIO_NMI_SOURCE                   17 /**< GPIO, NMI */
#define GPIO_INTR_SOURCE2                 18 /**< GPIO, level */
#define GPIO_NMI_SOURCE2                  19 /**< GPIO, NMI */
#define SPI1_INTR_SOURCE                  20 /**< SPI1, level */
#define SPI2_INTR_SOURCE                  21 /**< SPI2, level */
#define SPI3_INTR_SOURCE                  22 /**< SPI3, level */
#define LCD_CAM_INTR_SOURCE               24 /**< LCD camera, level */
#define I2S0_INTR_SOURCE                  25 /**< I2S0, level */
#define I2S1_INTR_SOURCE                  26 /**< I2S1, level */
#define UART0_INTR_SOURCE                 27 /**< UART0, level */
#define UART1_INTR_SOURCE                 28 /**< UART1, level */
#define UART2_INTR_SOURCE                 29 /**< UART2, level */
#define SDIO_HOST_INTR_SOURCE             30 /**< SD/SDIO/MMC HOST, level */
#define PWM0_INTR_SOURCE                  31 /**< PWM0, level, Reserved */
#define PWM1_INTR_SOURCE                  32 /**< PWM1, level, Reserved */
#define LEDC_INTR_SOURCE                  35 /**< LED PWM, level */
#define EFUSE_INTR_SOURCE                 36 /**< Efuse, level, not likely to use */
#define TWAI_INTR_SOURCE                  37 /**< Can, level */
#define USB_INTR_SOURCE                   38 /**< USB, level */
#define RTC_CORE_INTR_SOURCE              39 /**< Rtc core and watchdog, level */
#define RMT_INTR_SOURCE                   40 /**< Remote controller, level */
#define PCNT_INTR_SOURCE                  41 /**< Pulse count, level */
#define I2C_EXT0_INTR_SOURCE              42 /**< I2C controller1, level */
#define I2C_EXT1_INTR_SOURCE              43 /**< I2C controller0, level */
#define SPI2_DMA_INTR_SOURCE              44 /**< SPI2 DMA, level */
#define SPI3_DMA_INTR_SOURCE              45 /**< SPI3 DMA, level */
#define WDT_INTR_SOURCE                   47 /**< Deprecated interrupt source */
#define TIMER1_INTR_SOURCE                48 /**< Timer 1 interrupt (deprecated) */
#define TIMER2_INTR_SOURCE                49 /**< Timer 2 interrupt (deprecated) */
#define TG0_T0_LEVEL_INTR_SOURCE          50 /**< TIMER_GROUP0, TIMER0, EDGE */
#define TG0_T1_LEVEL_INTR_SOURCE          51 /**< TIMER_GROUP0, TIMER1, EDGE */
#define TG0_WDT_LEVEL_INTR_SOURCE         52 /**< TIMER_GROUP0, WATCH DOG, EDGE */
#define TG1_T0_LEVEL_INTR_SOURCE          53 /**< TIMER_GROUP1, TIMER0, EDGE */
#define TG1_T1_LEVEL_INTR_SOURCE          54 /**< TIMER_GROUP1, TIMER1, EDGE */
#define TG1_WDT_LEVEL_INTR_SOURCE         55 /**< TIMER_GROUP1, WATCHDOG, EDGE */
#define CACHE_IA_INTR_SOURCE              56 /**< Cache Invalid Access, LEVEL */
#define SYSTIMER_TARGET0_EDGE_INTR_SOURCE 57 /**< System timer 0, EDGE */
#define SYSTIMER_TARGET1_EDGE_INTR_SOURCE 58 /**< System timer 1, EDGE */
#define SYSTIMER_TARGET2_EDGE_INTR_SOURCE 59 /**< System timer 2, EDGE */
#define SPI_MEM_REJECT_CACHE_INTR_SOURCE  60 /**< SPI0/SPI1 Cache/Rejected, LEVEL */
#define DCACHE_PRELOAD0_INTR_SOURCE       61 /**< DCache preload operation, LEVEL */
#define ICACHE_PRELOAD0_INTR_SOURCE       62 /**< ICache perload operation, LEVEL */
#define DCACHE_SYNC0_INTR_SOURCE          63 /**< Data cache sync done, LEVEL */
#define ICACHE_SYNC0_INTR_SOURCE          64 /**< Instr. cache sync done, LEVEL */
#define APB_ADC_INTR_SOURCE               65 /**< APB ADC, LEVEL */
#define DMA_IN_CH0_INTR_SOURCE            66 /**< General DMA RX channel 0, LEVEL */
#define DMA_IN_CH1_INTR_SOURCE            67 /**< General DMA RX channel 1, LEVEL */
#define DMA_IN_CH2_INTR_SOURCE            68 /**< General DMA RX channel 2, LEVEL */
#define DMA_IN_CH3_INTR_SOURCE            69 /**< General DMA RX channel 3, LEVEL */
#define DMA_IN_CH4_INTR_SOURCE            70 /**< General DMA RX channel 4, LEVEL */
#define DMA_OUT_CH0_INTR_SOURCE           71 /**< General DMA TX channel 0, LEVEL */
#define DMA_OUT_CH1_INTR_SOURCE           72 /**< General DMA TX channel 1, LEVEL */
#define DMA_OUT_CH2_INTR_SOURCE           73 /**< General DMA TX channel 2, LEVEL */
#define DMA_OUT_CH3_INTR_SOURCE           74 /**< General DMA TX channel 3, LEVEL */
#define DMA_OUT_CH4_INTR_SOURCE           75 /**< General DMA TX channel 4, LEVEL */
#define RSA_INTR_SOURCE                   76 /**< RSA accelerator, level */
#define AES_INTR_SOURCE                   77 /**< AES accelerator, level */
#define SHA_INTR_SOURCE                   78 /**< SHA accelerator, level */
#define FROM_CPU_INTR0_SOURCE             79 /**< Interrupt0 generated from a CPU, level */
#define FROM_CPU_INTR1_SOURCE             80 /**< Interrupt1 generated from a CPU, level */
#define FROM_CPU_INTR2_SOURCE             81 /**< Interrupt2 generated from a CPU, level */
#define FROM_CPU_INTR3_SOURCE             82 /**< Interrupt3 generated from a CPU, level */
#define ASSIST_DEBUG_INTR_SOURCE          83 /**< Assist debug module, LEVEL */
#define DMA_APBPERI_PMS_INTR_SOURCE       84 /**< DMA APB peripheral PMS interrupt */
#define CORE0_IRAM0_PMS_INTR_SOURCE       85 /**< Core 0 IRAM0 PMS interrupt */
#define CORE0_DRAM0_PMS_INTR_SOURCE       86 /**< Core 0 DRAM0 PMS interrupt */
#define CORE0_PIF_PMS_INTR_SOURCE         87 /**< Core 0 PIF PMS interrupt */
#define CORE0_PIF_PMS_SIZE_INTR_SOURCE    88 /**< Core 0 PIF PMS size interrupt */
#define CORE1_IRAM0_PMS_INTR_SOURCE       89 /**< Core 1 IRAM0 PMS interrupt */
#define CORE1_DRAM0_PMS_INTR_SOURCE       90 /**< Core 1 DRAM0 PMS interrupt */
#define CORE1_PIF_PMS_INTR_SOURCE         91 /**< Core 1 PIF PMS interrupt */
#define CORE1_PIF_PMS_SIZE_INTR_SOURCE    92 /**< Core 1 PIF PMS size interrupt */
#define BACKUP_PMS_VIOLATE_INTR_SOURCE    93 /**< Backup PMS violation interrupt */
#define CACHE_CORE0_ACS_INTR_SOURCE       94 /**< Core 0 cache access interrupt */
#define CACHE_CORE1_ACS_INTR_SOURCE       95 /**< Core 1 cache access interrupt */
#define USB_SERIAL_JTAG_INTR_SOURCE       96 /**< USB Serial JTAG interrupt, level */
#define PREI_BACKUP_INTR_SOURCE           97 /**< Peripheral backup interrupt */
#define DMA_EXTMEM_REJECT_SOURCE          98 /**< DMA external memory reject interrupt */
#define MAX_INTR_SOURCE                   99 /**< Number of interrupt sources */

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
#define ESP_CPU_IRQ_L1_LVL_1  1 /**< CPU interrupt line 1 */
#define ESP_CPU_IRQ_L1_LVL_2  2 /**< CPU interrupt line 2 */
#define ESP_CPU_IRQ_L1_LVL_3  3 /**< CPU interrupt line 3 */
/**
 * Interrupt 4 reserved for WBB
 */
#define ESP_CPU_IRQ_L1_LVL_4  4  /**< CPU interrupt line 4 */
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
 * Interrupt 25 reserved for memory access and cache errors
 */
#define ESP_CPU_IRQ_L4_LVL_25 25 /**< CPU interrupt line 25 */
#define ESP_CPU_IRQ_L5_LVL_26 26 /**< CPU interrupt line 26 */
#define ESP_CPU_IRQ_L3_LVL_27 27 /**< CPU interrupt line 27 */
/**
 * Interrupt 28 reserved for IPC
 */
#define ESP_CPU_IRQ_L4_EDG_28 28 /**< CPU interrupt line 28 */
#define ESP_CPU_IRQ_L4_EDG_30 30 /**< CPU interrupt line 30 */
#define ESP_CPU_IRQ_L5_LVL_31 31 /**< CPU interrupt line 31 */

/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32S3_XTENSA_INTMUX_H_ */
