/*
 * Copyright (c) 2017, NXP
 * Copyright (c) 2017, Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_SOC_MKW40Z4) || defined(CONFIG_SOC_MKW41Z4)

#define LPUART0_CLK_SRC kCLOCK_CoreSysClk

/* IRQs */

#define IRQ_DMA_CHAN0 0
#define IRQ_DMA_CHAN1 1
#define IRQ_DMA_CHAN2 2
#define IRQ_DMA_CHAN3 3
#define IRQ_RESERVED0 4
#define IRQ_FTFA 5
#define IRQ_LOW_VOLTAGE 6
#define IRQ_LOW_LEAKAGE 7
#define IRQ_I2C0 8
#define IRQ_I2C1 9
#define IRQ_SPI0 10
#define IRQ_TSI0 11
#define IRQ_TRNG0 13
#define IRQ_CMT 14
#define IRQ_ADC0 15
#define IRQ_CMP0 16
#define IRQ_TPM0 17
#define IRQ_TPM1 18
#define IRQ_TPM2 19
#define IRQ_RTC_ALARM 20
#define IRQ_RTC_SEC 21
#define IRQ_PIT 22
#define IRQ_LTC0 23
#define IRQ_RADIO0 24
#define IRQ_DAC0 25
#define IRQ_RADIO1 26
#define IRQ_MCG 27
#define IRQ_LPTMR0 28
#define IRQ_SPI1 29
#define IRQ_GPIO_PORTA 30
#define IRQ_GPIO_PORTB 31
#define IRQ_GPIO_PORTC 31

#endif

#if defined(CONFIG_SOC_MKW22D5) || defined(CONFIG_SOC_MKW24D5)

#define PERIPH_ADDR_BASE_WDOG 0x40052000 /* Watchdog Timer module */

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
#define IRQ_RESERVED1 37
#define IRQ_RESERVED2 38
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
#define IRQ_RESERVED3 55
#define IRQ_RESERVED4 56
#define IRQ_MCG 57
#define IRQ_LOW_PWR_TIMER 58
#define IRQ_GPIO_PORTA 59
#define IRQ_GPIO_PORTB 60
#define IRQ_GPIO_PORTC 61
#define IRQ_GPIO_PORTD 62
#define IRQ_GPIO_PORTE 63
#define IRQ_SOFTWARE 64

#endif

#ifndef _ASMLANGUAGE

#include <fsl_common.h>
#include <device.h>
#include <misc/util.h>
#include <drivers/rand32.h>

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SOC__H_ */
