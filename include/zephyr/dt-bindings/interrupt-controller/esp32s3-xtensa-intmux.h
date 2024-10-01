/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32S3_XTENSA_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32S3_XTENSA_INTMUX_H_

#define WIFI_MAC_INTR_SOURCE                  0 /* interrupt of WiFi MAC, level*/
#define WIFI_MAC_NMI_SOURCE                   1 /* interrupt of WiFi MAC, NMI */
#define WIFI_PWR_INTR_SOURCE                  2
#define WIFI_BB_INTR_SOURCE                   3 /* interrupt of WiFi BB, level*/
#define BT_MAC_INTR_SOURCE                    4 /* will be cancelled*/
#define BT_BB_INTR_SOURCE                     5 /* interrupt of BT BB, level*/
#define BT_BB_NMI_SOURCE                      6 /* interrupt of BT BB, NMI*/
#define RWBT_INTR_SOURCE                      7 /* interrupt of RWBT, level*/
#define RWBLE_INTR_SOURCE                     8 /* interrupt of RWBLE, level*/
#define RWBT_NMI_SOURCE                       9 /* interrupt of RWBT, NMI*/
#define RWBLE_NMI_SOURCE                      10 /* interrupt of RWBLE, NMI*/
#define I2C_MASTER_SOURCE                     11 /* interrupt of I2C Master, level*/
#define SLC0_INTR_SOURCE                      12 /* interrupt of SLC0, level*/
#define SLC1_INTR_SOURCE                      13 /* interrupt of SLC1, level*/
#define UHCI0_INTR_SOURCE                     14 /* interrupt of UHCI0, level*/
#define UHCI1_INTR_SOURCE                     15 /* interrupt of UHCI1, level*/
#define GPIO_INTR_SOURCE                      16 /* interrupt of GPIO, level*/
#define GPIO_NMI_SOURCE                       17 /* interrupt of GPIO, NMI*/
#define GPIO_INTR_SOURCE2                     18 /* interrupt of GPIO, level*/
#define GPIO_NMI_SOURCE2                      19 /* interrupt of GPIO, NMI*/
#define SPI1_INTR_SOURCE                      20 /* interrupt of SPI1, level*/
#define SPI2_INTR_SOURCE                      21 /* interrupt of SPI2, level*/
#define SPI3_INTR_SOURCE                      22 /* interrupt of SPI3, level*/
#define LCD_CAM_INTR_SOURCE                   24 /* interrupt of LCD camera, level*/
#define I2S0_INTR_SOURCE                      25 /* interrupt of I2S0, level*/
#define I2S1_INTR_SOURCE                      26 /* interrupt of I2S1, level*/
#define UART0_INTR_SOURCE                     27 /* interrupt of UART0, level*/
#define UART1_INTR_SOURCE                     28 /* interrupt of UART1, level*/
#define UART2_INTR_SOURCE                     29 /* interrupt of UART2, level*/
#define SDIO_HOST_INTR_SOURCE                 30 /* interrupt of SD/SDIO/MMC HOST, level*/
#define PWM0_INTR_SOURCE                      31 /* interrupt of PWM0, level, Reserved*/
#define PWM1_INTR_SOURCE                      32 /* interrupt of PWM1, level, Reserved*/
#define LEDC_INTR_SOURCE                      35 /* interrupt of LED PWM, level*/
#define EFUSE_INTR_SOURCE                     36 /* interrupt of efuse, level, not likely to use*/
#define TWAI_INTR_SOURCE                      37 /* interrupt of can, level*/
#define USB_INTR_SOURCE                       38 /* interrupt of USB, level*/
#define RTC_CORE_INTR_SOURCE                  39 /* interrupt of rtc core and watchdog, level*/
#define RMT_INTR_SOURCE                       40 /* interrupt of remote controller, level*/
#define PCNT_INTR_SOURCE                      41 /* interrupt of pulse count, level*/
#define I2C_EXT0_INTR_SOURCE                  42 /* interrupt of I2C controller1, level*/
#define I2C_EXT1_INTR_SOURCE                  43 /* interrupt of I2C controller0, level*/
#define SPI2_DMA_INTR_SOURCE                  44 /* interrupt of SPI2 DMA, level*/
#define SPI3_DMA_INTR_SOURCE                  45 /* interrupt of SPI3 DMA, level*/
#define WDT_INTR_SOURCE                       47 /* will be cancelled*/
#define TIMER1_INTR_SOURCE                    48
#define TIMER2_INTR_SOURCE                    49
#define TG0_T0_LEVEL_INTR_SOURCE              50 /* interrupt of TIMER_GROUP0, TIMER0, EDGE*/
#define TG0_T1_LEVEL_INTR_SOURCE              51 /* interrupt of TIMER_GROUP0, TIMER1, EDGE*/
#define TG0_WDT_LEVEL_INTR_SOURCE             52 /* interrupt of TIMER_GROUP0, WATCH DOG, EDGE*/
#define TG1_T0_LEVEL_INTR_SOURCE              53 /* interrupt of TIMER_GROUP1, TIMER0, EDGE*/
#define TG1_T1_LEVEL_INTR_SOURCE              54 /* interrupt of TIMER_GROUP1, TIMER1, EDGE*/
#define TG1_WDT_LEVEL_INTR_SOURCE             55 /* interrupt of TIMER_GROUP1, WATCHDOG, EDGE*/
#define CACHE_IA_INTR_SOURCE                  56 /* interrupt of Cache Invalid Access, LEVEL*/
#define SYSTIMER_TARGET0_EDGE_INTR_SOURCE     57 /* interrupt of system timer 0, EDGE*/
#define SYSTIMER_TARGET1_EDGE_INTR_SOURCE     58 /* interrupt of system timer 1, EDGE*/
#define SYSTIMER_TARGET2_EDGE_INTR_SOURCE     59 /* interrupt of system timer 2, EDGE*/
#define SPI_MEM_REJECT_CACHE_INTR_SOURCE      60 /* interrupt of SPI0/SPI1 Cache/Rejected, LEVEL*/
#define DCACHE_PRELOAD0_INTR_SOURCE           61 /* interrupt of DCache preload operation, LEVEL*/
#define ICACHE_PRELOAD0_INTR_SOURCE           62 /* interrupt of ICache perload operation, LEVEL*/
#define DCACHE_SYNC0_INTR_SOURCE              63 /* interrupt of data cache sync done, LEVEL*/
#define ICACHE_SYNC0_INTR_SOURCE              64 /* interrupt of instr. cache sync done, LEVEL*/
#define APB_ADC_INTR_SOURCE                   65 /* interrupt of APB ADC, LEVEL*/
#define DMA_IN_CH0_INTR_SOURCE                66 /* interrupt of general DMA RX channel 0, LEVEL*/
#define DMA_IN_CH1_INTR_SOURCE                67 /* interrupt of general DMA RX channel 1, LEVEL*/
#define DMA_IN_CH2_INTR_SOURCE                68 /* interrupt of general DMA RX channel 2, LEVEL*/
#define DMA_IN_CH3_INTR_SOURCE                69 /* interrupt of general DMA RX channel 3, LEVEL*/
#define DMA_IN_CH4_INTR_SOURCE                70 /* interrupt of general DMA RX channel 4, LEVEL*/
#define DMA_OUT_CH0_INTR_SOURCE               71 /* interrupt of general DMA TX channel 0, LEVEL*/
#define DMA_OUT_CH1_INTR_SOURCE               72 /* interrupt of general DMA TX channel 1, LEVEL*/
#define DMA_OUT_CH2_INTR_SOURCE               73 /* interrupt of general DMA TX channel 2, LEVEL*/
#define DMA_OUT_CH3_INTR_SOURCE               74 /* interrupt of general DMA TX channel 3, LEVEL*/
#define DMA_OUT_CH4_INTR_SOURCE               75 /* interrupt of general DMA TX channel 4, LEVEL*/
#define RSA_INTR_SOURCE                       76 /* interrupt of RSA accelerator, level*/
#define AES_INTR_SOURCE                       77 /* interrupt of AES accelerator, level*/
#define SHA_INTR_SOURCE                       78 /* interrupt of SHA accelerator, level*/
#define FROM_CPU_INTR0_SOURCE                 79 /* interrupt0 generated from a CPU, level*/
#define FROM_CPU_INTR1_SOURCE                 80 /* interrupt1 generated from a CPU, level*/
#define FROM_CPU_INTR2_SOURCE                 81 /* interrupt2 generated from a CPU, level*/
#define FROM_CPU_INTR3_SOURCE                 82 /* interrupt3 generated from a CPU, level*/
#define ASSIST_DEBUG_INTR_SOURCE              83 /* interrupt of Assist debug module, LEVEL*/
#define DMA_APBPERI_PMS_INTR_SOURCE           84
#define CORE0_IRAM0_PMS_INTR_SOURCE           85
#define CORE0_DRAM0_PMS_INTR_SOURCE           86
#define CORE0_PIF_PMS_INTR_SOURCE             87
#define CORE0_PIF_PMS_SIZE_INTR_SOURCE        88
#define CORE1_IRAM0_PMS_INTR_SOURCE           89
#define CORE1_DRAM0_PMS_INTR_SOURCE           90
#define CORE1_PIF_PMS_INTR_SOURCE             91
#define CORE1_PIF_PMS_SIZE_INTR_SOURCE        92
#define BACKUP_PMS_VIOLATE_INTR_SOURCE        93
#define CACHE_CORE0_ACS_INTR_SOURCE           94
#define CACHE_CORE1_ACS_INTR_SOURCE           95
#define USB_SERIAL_JTAG_INTR_SOURCE           96
#define PREI_BACKUP_INTR_SOURCE               97
#define DMA_EXTMEM_REJECT_SOURCE              98
#define MAX_INTR_SOURCE                       99 /* number of interrupt sources */

/* For Xtensa architecture, zero will allocate low/medium
 * levels of priority (ESP_INTR_FLAG_LOWMED)
 */
#define IRQ_DEFAULT_PRIORITY	0

#define ESP_INTR_FLAG_SHARED	(1<<8)	/* Interrupt can be shared between ISRs */

#endif
