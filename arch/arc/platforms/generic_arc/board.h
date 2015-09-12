/* board.h - board configuration macros for the generic arc BSP */

/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This header file is used to specify and describe board-level aspects for the
generic arc BSP.
*/

#ifndef _BOARD__H_
#define _BOARD__H_

#include <misc/util.h>

/* default system clock */

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(32)

/* address bases */

#define PERIPH_ADDR_BASE_ADC 0x80015000 /* ADC */

#define PERIPH_ADDR_BASE_CREG_MST0 0x80018000 /* CREG Master 0 */
#define PERIPH_ADDR_BASE_CREG_SLV0 0x80018080 /* CREG Slave 0 */
#define PERIPH_ADDR_BASE_CREG_SLV1 0x80018180 /* CREG Slave 1 */

#define PERIPH_ADDR_BASE_GPIO0 0x80017800 /* GPIO 0 */
#define PERIPH_ADDR_BASE_GPIO1 0x80017900 /* GPIO 1 */

#define PERIPH_ADDR_BASE_I2C_MST0 0x80012000 /* I2C Master 0 */
#define PERIPH_ADDR_BASE_I2C_MST1 0x80012100 /* I2C Master 1 */

#define PERIPH_ADDR_BASE_SPI_MST0 0x80010000 /* SPI Master 0 */
#define PERIPH_ADDR_BASE_SPI_MST1 0x80010100 /* SPI Master 1 */

#ifdef CONFIG_NSIM
#define PERIPH_ADDR_BASE_UART0 0x4242 /* UART A */
#else
#define PERIPH_ADDR_BASE_UART0 0xB0002000 /* UART A */
#define PERIPH_ADDR_BASE_UART1 0xB0002400 /* UART B */
#endif

/* IRQs */

#define IRQ_TIMER0 16
#define IRQ_TIMER1 17
#define IRQ_I2C0_RX_AVAIL 18
#define IRQ_I2C0_TX_REQ 19
#define IRQ_I2C0_STOP_DET 20
#define IRQ_I2C0_ERR 21
#define IRQ_I2C1_RX_AVAIL 22
#define IRQ_I2C1_TX_REQ 23
#define IRQ_I2C1_STOP_DET 24
#define IRQ_I2C1_ERR 25
#define IRQ_SPI0_ERR_INT 26
#define IRQ_SPI0_RX_AVAIL 27
#define IRQ_SPI0_TX_REQ 28
#define IRQ_SPI1_ERR_INT 29
#define IRQ_SPI1_RX_AVAIL 30
#define IRQ_SPI1_TX_REQ 31
#define IRQ_ADC_IRQ 32
#define IRQ_ADC_ERR 33
#define IRQ_GPIO0_INTR 34
#define IRQ_GPIO1_INTR 35
#define IRQ_I2C_MST0_INTR 36
#define IRQ_I2C_MST1_INTR 37
#define IRQ_SPI_MST0_INTR 38
#define IRQ_SPI_MST1_INTR 39
#define IRQ_SPI_SLV_INTR 40
#define IRQ_UART0_INTR 41
#define IRQ_UART1_INTR 42
#define IRQ_I2S_INTR 43
#define IRQ_GPIO_INTR 44
#define IRQ_PWM_TIMER_INTR 45
#define IRQ_USB_INTR 46
#define IRQ_RTC_INTR 47
#define IRQ_WDOG_INTR 48
#define IRQ_DMA_CHAN0 49
#define IRQ_DMA_CHAN1 50
#define IRQ_DMA_CHAN2 51
#define IRQ_DMA_CHAN3 52
#define IRQ_DMA_CHAN4 53
#define IRQ_DMA_CHAN5 54
#define IRQ_DMA_CHAN6 55
#define IRQ_DMA_CHAN7 56
#define IRQ_MAILBOXES_INTR 57
#define IRQ_COMPARATORS_INTR 58
#define IRQ_SYS_PMU_INTR 59
#define IRQ_DMA_CHANS_ERR 60
#define IRQ_INT_SRAM_CTLR 61
#define IRQ_INT_FLASH0_CTLR 62
#define IRQ_INT_FLASH1_CTLR 63
#define IRQ_ALWAYS_ON_TMR 64
#define IRQ_ADC_PWR 65
#define IRQ_ADC_CALIB 66
#define IRQ_ALWAYS_ON_GPIO 67

#ifndef _ASMLANGUAGE

#define EXC_FROM_IRQ(irq) ((irq) + 16)
#define VECTOR_FROM_IRQ(irq) EXC_FROM_IRQ(irq)
#define VECTOR_ADDR(vector) ((uint32_t *)((int)vector << 2))

#include <misc/util.h>
#include <drivers/rand32.h>

/* ARCv2 timer 0 configuration settings for the system clock */
#ifdef CONFIG_NANOKERNEL
#define CONFIG_ARCV2_TIMER0_CLOCK_FREQ 32000000 /* 32MHz reference clock \
							*/
#define CONFIG_ARCV2_TIMER1_CLOCK_FREQ CONFIG_ARCV2_TIMER0_CLOCK_FREQ
#endif /* CONFIG_NANOKERNEL */

#define CONFIG_ARCV2_TIMER0_INT_LVL IRQ_TIMER0
#define CONFIG_ARCV2_TIMER0_INT_PRI 0

#define CONFIG_ARCV2_TIMER1_INT_LVL IRQ_TIMER1
#define CONFIG_ARCV2_TIMER1_INT_PRI 1

/*
 * UART configuration settings
 *
 * This BSP only supports the nanokernel.  Therefore:
 * - only polled mode is supported (interrupt-driven mode is NOT supported); and
 * - only the target console is supported (hostserver driver is NOT supported).
 */
#define CONFIG_UART_CONSOLE_CLK_FREQ SYSCLK_DEFAULT_IOSC_HZ
#define CONFIG_UART_CONSOLE_BAUDRATE 115200
#define CONFIG_UART_CONSOLE_REGS PERIPH_ADDR_BASE_UART0
#define CONFIG_UART_CONSOLE_IRQ IRQ_UART0_INTR
#define CONFIG_UART_CONSOLE_INT_PRI 0

#define UART_REG_ADDR_INTERVAL 4 /* for ns16550 driver */

#endif /* !_ASMLANGUAGE */

#endif /* _BOARD__H_ */
