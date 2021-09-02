/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C3_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C3_INTMUX_H_

#define WIFI_MAC_INTR_SOURCE				0	/**< interrupt of WiFi MAC, level*/
#define WIFI_MAC_NMI_SOURCE					1	/**< interrupt of WiFi MAC, NMI, use if MAC have bug to fix in NMI*/
#define WIFI_PWR_INTR_SOURCE				2	/**< */
#define WIFI_BB_INTR_SOURCE					3	/**< interrupt of WiFi BB, level, we can do some calibartion*/
#define BT_MAC_INTR_SOURCE					4	/**< will be cancelled*/
#define BT_BB_INTR_SOURCE					5	/**< interrupt of BT BB, level*/
#define BT_BB_NMI_SOURCE					6	/**< interrupt of BT BB, NMI, use if BB have bug to fix in NMI*/
#define RWBT_INTR_SOURCE					7	/**< interrupt of RWBT, level*/
#define RWBLE_INTR_SOURCE					8	/**< interrupt of RWBLE, level*/
#define RWBT_NMI_SOURCE						9	/**< interrupt of RWBT, NMI, use if RWBT have bug to fix in NMI*/
#define RWBLE_NMI_SOURCE					10	/**< interrupt of RWBLE, NMI, use if RWBT have bug to fix in NMI*/
#define I2C_MASTER_SOURCE					11	/**< interrupt of I2C Master, level*/
#define SLC0_INTR_SOURCE					12	/**< interrupt of SLC0, level*/
#define SLC1_INTR_SOURCE					13	/**< interrupt of SLC1, level*/
#define APB_CTRL_INTR_SOURCE				14	/**< interrupt of APB ctrl, ?*/
#define UHCI0_INTR_SOURCE					15	/**< interrupt of UHCI0, level*/
#define GPIO_INTR_SOURCE					16	/**< interrupt of GPIO, level*/
#define GPIO_NMI_SOURCE						17	/**< interrupt of GPIO, NMI*/
#define SPI1_INTR_SOURCE					18	/**< interrupt of SPI1, level, SPI1 is for flash read/write, do not use this*/
#define SPI2_INTR_SOURCE					19	/**< interrupt of SPI2, level*/
#define I2S1_INTR_SOURCE					20	/**< interrupt of I2S1, level*/
#define UART0_INTR_SOURCE					21	/**< interrupt of UART0, level*/
#define UART1_INTR_SOURCE					22	/**< interrupt of UART1, level*/
#define LEDC_INTR_SOURCE					23	/**< interrupt of LED PWM, level*/
#define EFUSE_INTR_SOURCE					24	/**< interrupt of efuse, level, not likely to use*/
#define TWAI_INTR_SOURCE					25	/**< interrupt of can, level*/
#define USB_INTR_SOURCE						26	/**< interrupt of USB, level*/
#define RTC_CORE_INTR_SOURCE				27	/**< interrupt of rtc core, level, include rtc watchdog*/
#define RMT_INTR_SOURCE						28	/**< interrupt of remote controller, level*/
#define I2C_EXT0_INTR_SOURCE				29	/**< interrupt of I2C controller1, level*/
#define TIMER1_INTR_SOURCE					30
#define TIMER2_INTR_SOURCE					31
#define TG0_T0_LEVEL_INTR_SOURCE			32	/**< interrupt of TIMER_GROUP0, TIMER0, level*/
#define TG0_WDT_LEVEL_INTR_SOURCE			33	/**< interrupt of TIMER_GROUP0, WATCH DOG, level*/
#define TG1_T0_LEVEL_INTR_SOURCE			34	/**< interrupt of TIMER_GROUP1, TIMER0, level*/
#define TG1_WDT_LEVEL_INTR_SOURCE			35	/**< interrupt of TIMER_GROUP1, WATCHDOG, level*/
#define CACHE_IA_INTR_SOURCE				36	/**< interrupt of Cache Invalied Access, LEVEL*/
#define SYSTIMER_TARGET0_EDGE_INTR_SOURCE	37	/**< interrupt of system timer 0, EDGE*/
#define SYSTIMER_TARGET1_EDGE_INTR_SOURCE	38	/**< interrupt of system timer 1, EDGE*/
#define SYSTIMER_TARGET2_EDGE_INTR_SOURCE	39	/**< interrupt of system timer 2, EDGE*/
#define SPI_MEM_REJECT_CACHE_INTR_SOURCE	40	/**< interrupt of SPI0 Cache access and SPI1 access rejected, LEVEL*/
#define ICACHE_PRELOAD0_INTR_SOURCE			41	/**< interrupt of ICache perload operation, LEVEL*/
#define ICACHE_SYNC0_INTR_SOURCE			42	/**< interrupt of instruction cache sync done, LEVEL*/
#define APB_ADC_INTR_SOURCE					43	/**< interrupt of APB ADC, LEVEL*/
#define DMA_CH0_INTR_SOURCE					44	/**< interrupt of general DMA channel 0, LEVEL*/
#define DMA_CH1_INTR_SOURCE					45	/**< interrupt of general DMA channel 1, LEVEL*/
#define DMA_CH2_INTR_SOURCE					46	/**< interrupt of general DMA channel 2, LEVEL*/
#define RSA_INTR_SOURCE						47	/**< interrupt of RSA accelerator, level*/
#define AES_INTR_SOURCE						48	/**< interrupt of AES accelerator, level*/
#define SHA_INTR_SOURCE						49	/**< interrupt of SHA accelerator, level*/
#define FROM_CPU_INTR0_SOURCE				50	/**< interrupt0 generated from a CPU, level*/ /* Used for FreeRTOS */
#define FROM_CPU_INTR1_SOURCE				51	/**< interrupt1 generated from a CPU, level*/ /* Used for FreeRTOS */
#define FROM_CPU_INTR2_SOURCE				52	/**< interrupt2 generated from a CPU, level*/ /* Used for DPORT Access */
#define FROM_CPU_INTR3_SOURCE				53	/**< interrupt3 generated from a CPU, level*/ /* Used for DPORT Access */
#define ASSIST_DEBUG_INTR_SOURCE			54	/**< interrupt of Assist debug module, LEVEL*/
#define DMA_APBPERI_PMS_INTR_SOURCE			55
#define CORE0_IRAM0_PMS_INTR_SOURCE			56
#define CORE0_DRAM0_PMS_INTR_SOURCE			57
#define CORE0_PIF_PMS_INTR_SOURCE			58
#define CORE0_PIF_PMS_SIZE_INTR_SOURCE		59
#define BAK_PMS_VIOLATE_INTR_SOURCE			60
#define CACHE_CORE0_ACS_INTR_SOURCE			61

#endif
