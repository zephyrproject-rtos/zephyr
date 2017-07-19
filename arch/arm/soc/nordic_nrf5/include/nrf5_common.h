/*
 * Copyright (c) 2016 Linaro Ltd.
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NRF5_SOC_COMMON_H_
#define _NRF5_SOC_COMMON_H_

/**
 * @file Common definitions for NRF51/NRF52 family processors.
 *
 * The nRF51 IRQs can all be represented by the common definitions.
 *
 * Based on Nordic MDK included header files: nrf51.h and nrf51_to_nrf52.h
 */

#define NRF5_IRQ_POWER_CLOCK_IRQn              0
#define NRF5_IRQ_RADIO_IRQn                    1
#define NRF5_IRQ_UART0_IRQn                    2
#define NRF5_IRQ_SPI0_TWI0_IRQn                3
#define NRF5_IRQ_SPI1_TWI1_IRQn                4
#define NRF5_IRQ_GPIOTE_IRQn                   6
#define NRF5_IRQ_ADC_IRQn                      7
#define NRF5_IRQ_TIMER0_IRQn                   8
#define NRF5_IRQ_TIMER1_IRQn                   9
#define NRF5_IRQ_TIMER2_IRQn                   10
#define NRF5_IRQ_RTC0_IRQn                     11
#define NRF5_IRQ_TEMP_IRQn                     12
#define NRF5_IRQ_RNG_IRQn                      13
#define NRF5_IRQ_ECB_IRQn                      14
#define NRF5_IRQ_CCM_AAR_IRQn                  15
#define NRF5_IRQ_WDT_IRQn                      16
#define NRF5_IRQ_RTC1_IRQn                     17
#define NRF5_IRQ_QDEC_IRQn                     18
#define NRF5_IRQ_LPCOMP_IRQn                   19
#define NRF5_IRQ_SWI0_IRQn                     20
#define NRF5_IRQ_SWI1_IRQn                     21
#define NRF5_IRQ_SWI2_IRQn                     22
#define NRF5_IRQ_SWI3_IRQn                     23
#define NRF5_IRQ_SWI4_IRQn                     24
#define NRF5_IRQ_SWI5_IRQn                     25

/**
 * @file Interrupt numbers for NRF52 family processors.
 *
 * Based on Nordic MDK included header file: nrf52.h
 */

#define NRF52_IRQ_NFCT_IRQn                    5
#define NRF52_IRQ_TIMER3_IRQn                  26
#define NRF52_IRQ_TIMER4_IRQn                  27
#define NRF52_IRQ_PWM0_IRQn                    28
#define NRF52_IRQ_PDM_IRQn                     29
#define NRF52_IRQ_MWU_IRQn                     32
#define NRF52_IRQ_PWM1_IRQn                    33
#define NRF52_IRQ_PWM2_IRQn                    34
#define NRF52_IRQ_SPIM2_SPIS2_SPI2_IRQn        35
#define NRF52_IRQ_RTC2_IRQn                    36
#define NRF52_IRQ_I2S_IRQn                     37
#define NRF52_IRQ_FPU_IRQn                     38

/**
 * @file UART baudrate divisors for NRF51/NRF52 family processors.
 *
 * Based on Nordic MDK included header file: nrf52_bitfields.h
 * Uses the UARTE_BAUDRATE macros since they are more precise.
 */

#define NRF5_UART_BAUDRATE_300                 0x00014000
#define NRF5_UART_BAUDRATE_600                 0x00027000
#define NRF5_UART_BAUDRATE_1200                0x0004f000
#define NRF5_UART_BAUDRATE_2400                0x0009d000
#define NRF5_UART_BAUDRATE_4800                0x0013b000
#define NRF5_UART_BAUDRATE_9600                0x00275000
#define NRF5_UART_BAUDRATE_14400               0x003af000
#define NRF5_UART_BAUDRATE_19200               0x004ea000
#define NRF5_UART_BAUDRATE_28800               0x0075c000
#define NRF5_UART_BAUDRATE_38400               0x009d0000
#define NRF5_UART_BAUDRATE_57600               0x00eb0000
#define NRF5_UART_BAUDRATE_76800               0x013a9000
#define NRF5_UART_BAUDRATE_115200              0x01d60000
#define NRF5_UART_BAUDRATE_230400              0x03b00000
#define NRF5_UART_BAUDRATE_250000              0x04000000
#define NRF5_UART_BAUDRATE_460800              0x07400000
#define NRF5_UART_BAUDRATE_921600              0x0f000000
#define NRF5_UART_BAUDRATE_1000000             0x10000000

#endif /* _NRF5_SOC_COMMON_H_ */
