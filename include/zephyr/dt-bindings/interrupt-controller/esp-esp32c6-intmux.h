/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C6_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C6_INTMUX_H_

#define WIFI_MAC_INTR_SOURCE                0  /* interrupt of WiFi MAC, level*/
#define WIFI_MAC_NMI_SOURCE                 1  /* interrupt of WiFi MAC, NMI*/
#define WIFI_PWR_INTR_SOURCE                2
#define WIFI_BB_INTR_SOURCE                 3  /* interrupt of WiFi BB, level*/
#define BT_MAC_INTR_SOURCE                  4  /* will be cancelled*/
#define BT_BB_INTR_SOURCE                   5  /* interrupt of BT BB, level*/
#define BT_BB_NMI_SOURCE                    6  /* interrupt of BT BB, NMI*/
#define LP_TIMER_INTR_SOURCE                7
#define COEX_INTR_SOURCE                    8
#define BLE_TIMER_INTR_SOURCE               9
#define BLE_SEC_INTR_SOURCE                10
#define I2C_MASTER_SOURCE                  11  /* interrupt of I2C Master, level*/
#define ZB_MAC_SOURCE                      12
#define PMU_INTR_SOURCE                    13
#define EFUSE_INTR_SOURCE                  14  /* interrupt of efuse, level, not likely to use*/
#define LP_RTC_TIMER_INTR_SOURCE           15
#define LP_UART_INTR_SOURCE                16
#define LP_I2C_INTR_SOURCE                 17
#define LP_WDT_INTR_SOURCE                 18
#define LP_PERI_TIMEOUT_INTR_SOURCE        19
#define LP_APM_M0_INTR_SOURCE              20
#define LP_APM_M1_INTR_SOURCE              21
#define FROM_CPU_INTR0_SOURCE              22  /* interrupt0 generated from a CPU, level*/
#define FROM_CPU_INTR1_SOURCE              23  /* interrupt1 generated from a CPU, level*/
#define FROM_CPU_INTR2_SOURCE              24  /* interrupt2 generated from a CPU, level*/
#define FROM_CPU_INTR3_SOURCE              25  /* interrupt3 generated from a CPU, level*/
#define ASSIST_DEBUG_INTR_SOURCE           26  /* interrupt of Assist debug module, LEVEL*/
#define TRACE_INTR_SOURCE                  27
#define CACHE_INTR_SOURCE                  28
#define CPU_PERI_TIMEOUT_INTR_SOURCE       29
#define GPIO_INTR_SOURCE                   30  /* interrupt of GPIO, level*/
#define GPIO_NMI_SOURCE                    31  /* interrupt of GPIO, NMI*/
#define PAU_INTR_SOURCE                    32
#define HP_PERI_TIMEOUT_INTR_SOURCE        33
#define MODEM_PERI_TIMEOUT_INTR_SOURCE     34
#define HP_APM_M0_INTR_SOURCE              35
#define HP_APM_M1_INTR_SOURCE              36
#define HP_APM_M2_INTR_SOURCE              37
#define HP_APM_M3_INTR_SOURCE              38
#define LP_APM0_INTR_SOURCE                39
#define MSPI_INTR_SOURCE                   40
#define I2S1_INTR_SOURCE                   41  /* interrupt of I2S1, level*/
#define UHCI0_INTR_SOURCE                  42  /* interrupt of UHCI0, level*/
#define UART0_INTR_SOURCE                  43  /* interrupt of UART0, level*/
#define UART1_INTR_SOURCE                  44  /* interrupt of UART1, level*/
#define LEDC_INTR_SOURCE                   45  /* interrupt of LED PWM, level*/
#define TWAI0_INTR_SOURCE                  46  /* interrupt of can0, level*/
#define TWAI1_INTR_SOURCE                  47  /* interrupt of can1, level*/
#define USB_SERIAL_JTAG_INTR_SOURCE        48  /* interrupt of USB, level*/
#define RMT_INTR_SOURCE                    49  /* interrupt of remote controller, level*/
#define I2C_EXT0_INTR_SOURCE               50  /* interrupt of I2C controller1, level*/
#define TG0_T0_LEVEL_INTR_SOURCE           51  /* interrupt of TIMER_GROUP0, TIMER0, level*/
#define TG0_T1_LEVEL_INTR_SOURCE           52  /* interrupt of TIMER_GROUP0, TIMER1, level*/
#define TG0_WDT_LEVEL_INTR_SOURCE          53  /* interrupt of TIMER_GROUP0, WATCH DOG, level*/
#define TG1_T0_LEVEL_INTR_SOURCE           54  /* interrupt of TIMER_GROUP1, TIMER0, level*/
#define TG1_T1_LEVEL_INTR_SOURCE           55  /* interrupt of TIMER_GROUP1, TIMER1, level*/
#define TG1_WDT_LEVEL_INTR_SOURCE          56  /* interrupt of TIMER_GROUP1, WATCHDOG, level*/
#define SYSTIMER_TARGET0_EDGE_INTR_SOURCE  57  /* interrupt of system timer 0, EDGE*/
#define SYSTIMER_TARGET1_EDGE_INTR_SOURCE  58  /* interrupt of system timer 1, EDGE*/
#define SYSTIMER_TARGET2_EDGE_INTR_SOURCE  59  /* interrupt of system timer 2, EDGE*/
#define APB_ADC_INTR_SOURCE                60  /* interrupt of APB ADC, LEVEL*/
#define MCPWM0_INTR_SOURCE                 61  /* interrupt of MCPWM0, LEVEL*/
#define PCNT_INTR_SOURCE                   62
#define PARL_IO_INTR_SOURCE                63
#define SLC0_INTR_SOURCE                   64
#define SLC_INTR_SOURCE                    65
#define DMA_IN_CH0_INTR_SOURCE             66  /* interrupt of general DMA IN channel 0, LEVEL*/
#define DMA_IN_CH1_INTR_SOURCE             67  /* interrupt of general DMA IN channel 1, LEVEL*/
#define DMA_IN_CH2_INTR_SOURCE             68  /* interrupt of general DMA IN channel 2, LEVEL*/
#define DMA_OUT_CH0_INTR_SOURCE            69  /* interrupt of general DMA OUT channel 0, LEVEL*/
#define DMA_OUT_CH1_INTR_SOURCE            70  /* interrupt of general DMA OUT channel 1, LEVEL*/
#define DMA_OUT_CH2_INTR_SOURCE            71  /* interrupt of general DMA OUT channel 2, LEVEL*/
#define GSPI2_INTR_SOURCE                  72
#define AES_INTR_SOURCE                    73  /* interrupt of AES accelerator, level*/
#define SHA_INTR_SOURCE                    74  /* interrupt of SHA accelerator, level*/
#define RSA_INTR_SOURCE                    75  /* interrupt of RSA accelerator, level*/
#define ECC_INTR_SOURCE                    76  /* interrupt of ECC accelerator, level*/
#define MAX_INTR_SOURCE                    77

/* Zero will allocate low/medium levels of priority (ESP_INTR_FLAG_LOWMED) */
#define IRQ_DEFAULT_PRIORITY	0

#define ESP_INTR_FLAG_SHARED	(1<<8)	/* Interrupt can be shared between ISRs */

/* LP Core intmux */
#define LP_CORE_IO_INTR_SOURCE    0
#define LP_CORE_I2C_INTR_SOURCE   1
#define LP_CORE_UART_INTR_SOURCE  2
#define LP_CORE_TIMER_INTR_SOURCE 3
#define LP_CORE_PMU_INTR_SOURCE   5

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32C6_INTMUX_H_ */
