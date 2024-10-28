/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_XTENSA_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_XTENSA_INTMUX_H_

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

/* For Xtensa architecture, zero will allocate low/medium
 * levels of priority (ESP_INTR_FLAG_LOWMED)
 */
#define IRQ_DEFAULT_PRIORITY	0

#define ESP_INTR_FLAG_SHARED	(1<<8)	/* Interrupt can be shared between ISRs */

#endif
