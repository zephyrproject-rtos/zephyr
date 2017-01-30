/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

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
#define IRQ_LPUART0 12
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

#ifndef _ASMLANGUAGE

#include <device.h>
#include <misc/util.h>
#include <drivers/rand32.h>

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SOC__H_ */
