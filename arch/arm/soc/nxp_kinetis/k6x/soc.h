/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the fsl_frdm_k64f platform
 *
 * This header file is used to specify and describe board-level aspects for the
 * 'fsl_frdm_k64f' platform.
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* default system clock */

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(120)
#define BUSCLK_DEFAULT_IOSC_HZ (SYSCLK_DEFAULT_IOSC_HZ / \
				CONFIG_K64_BUS_CLOCK_DIVIDER)

/* address bases */

#define PERIPH_ADDR_BASE_MPU 0x4000D000 /* Memory Protection Unit */

#define PERIPH_ADDR_BASE_PCR 0x40049000 /* Port and pin Configuration */

#define PERIPH_ADDR_BASE_SIM 0x40047000 /* System Integration module */

#define PERIPH_ADDR_BASE_WDOG 0x40052000 /* Watchdog Timer module */

#define PERIPH_ADDR_BASE_MCG 0x40064000 /* Multipurpose Clock Generator */

#define PERIPH_ADDR_BASE_OSC 0x40065000 /* Oscillator module */

#define PERIPH_ADDR_BASE_PMC 0x4007D000 /* Power Mgt Controller module */

/* IRQs */

#define IRQ_DMA_CHAN0 0
#define IRQ_DMA_CHAN1 1
#define IRQ_DMA_CHAN2 2
#define IRQ_DMA_CHAN3 3
#define IRQ_DMA_CHAN4 4
#define IRQ_DMA_CHAN5 5
#define IRQ_DMA_CHAN6 6
#define IRQ_DMA_CHAN7 7
#define IRQ_DMA_CHAN8 8
#define IRQ_DMA_CHAN9 9
#define IRQ_DMA_CHAN10 10
#define IRQ_DMA_CHAN11 11
#define IRQ_DMA_CHAN12 12
#define IRQ_DMA_CHAN13 13
#define IRQ_DMA_CHAN14 14
#define IRQ_DMA_CHAN15 15
#define IRQ_DMA_ERR 16
#define IRQ_MCM 17
#define IRQ_FLASH_CMD 18
#define IRQ_FLASH_COLLISION 19
#define IRQ_LOW_VOLTAGE 20
#define IRQ_LOW_LEAKAGE 21
#define IRQ_WDOG_OR_EVM 22
#define IRQ_RAND_NUM_GEN 23
#define IRQ_I2C0 24
#define IRQ_I2C1 25
#define IRQ_SPI0 26
#define IRQ_SPI1 27
#define IRQ_I2S0_TX 28
#define IRQ_I2S0_RX 29
#define IRQ_RESERVED0 30
#define IRQ_UART0_STATUS 31
#define IRQ_UART0_ERROR 32
#define IRQ_UART1_STATUS 33
#define IRQ_UART1_ERROR 34
#define IRQ_UART2_STATUS 35
#define IRQ_UART2_ERROR 36
#define IRQ_UART3_STATUS 37
#define IRQ_UART3_ERROR 38
#define IRQ_ADC0 39
#define IRQ_CMP0 40
#define IRQ_CMP1 41
#define IRQ_FTM0 42
#define IRQ_FTM1 43
#define IRQ_FTM2 44
#define IRQ_CMT 45
#define IRQ_RTC_ALARM 46
#define IRQ_RTC_SEC 47
#define IRQ_TIMER0 48
#define IRQ_TIMER1 49
#define IRQ_TIMER2 50
#define IRQ_TIMER3 51
#define IRQ_PDB 52
#define IRQ_USB_OTG 53
#define IRQ_USB_CHARGE 54
#define IRQ_RESERVED1 55
#define IRQ_DAC0 56
#define IRQ_MCG 57
#define IRQ_LOW_PWR_TIMER 58
#define IRQ_GPIO_PORTA 59
#define IRQ_GPIO_PORTB 60
#define IRQ_GPIO_PORTC 61
#define IRQ_GPIO_PORTD 62
#define IRQ_GPIO_PORTE 63
#define IRQ_SOFTWARE 64
#define IRQ_SPI2 65
#define IRQ_UART4_STATUS 66
#define IRQ_UART4_ERROR 67
#define IRQ_RESERVED2 68 /* IRQ_UART5_STATUS - UART5 not implemented */
#define IRQ_RESERVED3 69 /* IRQ_UART5_ERROR - UART5 not implemented */
#define IRQ_CMP2 70
#define IRQ_FTM3 71
#define IRQ_DAC1 72
#define IRQ_ADC1 73
#define IRQ_I2C2 74
#define IRQ_CAN0_MSG_BUF 75
#define IRQ_CAN0_BUS_OFF 76
#define IRQ_CAN0_ERROR 77
#define IRQ_CAN0_TX_WARN 78
#define IRQ_CAN0_RX_WARN 79
#define IRQ_CAN0_WAKEUP 80
#define IRQ_SDHC 81
#define IRQ_ETH_IEEE1588_TMR 82
#define IRQ_ETH_TX 83
#define IRQ_ETH_RX 84
#define IRQ_ETH_ERR_MISC 85

#ifndef _ASMLANGUAGE

#include <fsl_common.h>
#include <device.h>
#include <misc/util.h>
#include <drivers/rand32.h>

/*
 * PWM/FTM configuration settings
 */
#define PWM_K64_FTM_0_REG_BASE	0x40038000
#define PWM_K64_FTM_1_REG_BASE	0x40039000
#define PWM_K64_FTM_2_REG_BASE	0x4003A000
#define PWM_K64_FTM_3_REG_BASE	0x400B9000

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SOC__H_ */
