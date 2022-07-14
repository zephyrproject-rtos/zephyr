/*
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros
 *
 * This header file is used to specify and describe board-level aspects.
 */

#ifndef _BOARD__H_
#define _BOARD__H_

#include <zephyr/sys/util.h>

/* default system clock */

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(12)

/* IRQs */
//Redo this, not the same interrupts for am62x
#define IRQ_GPIO_PORTA 0
#define IRQ_GPIO_PORTB 1
#define IRQ_GPIO_PORTC 2
#define IRQ_GPIO_PORTD 3
#define IRQ_GPIO_PORTE 4
#define IRQ_UART0 5
#define IRQ_UART1 6
#define IRQ_SSI0 7
#define IRQ_I2C0 8
#define IRQ_PWM_FAULT 9
#define IRQ_PWM_GEN0 10
#define IRQ_PWM_GEN1 11
#define IRQ_PWM_GEN2 12
#define IRQ_QEI0 13
#define IRQ_ADC0_SEQ0 14
#define IRQ_ADC0_SEQ1 15
#define IRQ_ADC0_SEQ2 16
#define IRQ_ADC0_SEQ3 17
#define IRQ_WDOG 18
#define IRQ_TIMER0A 19
#define IRQ_TIMER0B 20
#define IRQ_TIMER1A 21
#define IRQ_TIMER1B 22
#define IRQ_TIMER2A 23
#define IRQ_TIMER2B 24
#define IRQ_ANALOG_COMP0 25
#define IRQ_ANALOG_COMP1 26
#define IRQ_ANALOG_COMP2 27
#define IRQ_SYS_CONTROL 28
#define IRQ_FLASH_MEM_CTRL 29
#define IRQ_GPIO_PORTF 30
#define IRQ_GPIO_PORTG 31
#define IRQ_GPIO_PORTH 32
#define IRQ_UART2 33
#define IRQ_SSI1 34
#define IRQ_TIMER3A 35
#define IRQ_TIMER3B 36
#define IRQ_I2C1 37
#define IRQ_CAN0 38
#define IRQ_CAN1 39
#define IRQ_ETH 40
#define IRQ_HIB 41
#define IRQ_USB 42
#define IRQ_PWM_GEN3 43
#define IRQ_UDMA0_SOFT 44
#define IRQ_UDMA0_ERROR 45
#define IRQ_ADC1_SEQ0 46
#define IRQ_ADC1_SEQ1 47
#define IRQ_ADC1_SEQ2 48
#define IRQ_ADC1_SEQ3 49
#define IRQ_EPI0 50
#define IRQ_GPIO_PORTJ 51
#define IRQ_GPIO_PORTK 52
#define IRQ_GPIO_PORTL 53
#define IRQ_SSI2 54
#define IRQ_SSI3 55
#define IRQ_UART3 56
#define IRQ_UART4 57
#define IRQ_UART5 58
#define IRQ_UART6 59
#define IRQ_UART7 60
#define IRQ_I2C2 61
#define IRQ_I2C3 62
#define IRQ_TIMER4A 63
#define IRQ_TIMER4B 64
#define IRQ_TIMER5A 65
#define IRQ_TIMER5B 66
#define IRQ_FP 67
#define IRQ_RESERVED0 68
#define IRQ_RESERVED1 69
#define IRQ_I2C4 70
#define IRQ_I2C5 71
#define IRQ_GPIO_PORTM 72
#define IRQ_GPIO_PORTN 73
#define IRQ_RESERVED1 74
#define IRQ_TAMPER 75
#define IRQ_GPIO_PORT_P0 76
#define IRQ_GPIO_PORT_P1 77
#define IRQ_GPIO_PORT_P2 78
#define IRQ_GPIO_PORT_P3 79
#define IRQ_GPIO_PORT_P4 80
#define IRQ_GPIO_PORT_P5 81
#define IRQ_GPIO_PORT_P6 82
#define IRQ_GPIO_PORT_P7 83
#define IRQ_GPIO_PORT_Q0 84
#define IRQ_GPIO_PORT_Q1 85
#define IRQ_GPIO_PORT_Q2 86
#define IRQ_GPIO_PORT_Q3 87
#define IRQ_GPIO_PORT_Q4 88
#define IRQ_GPIO_PORT_Q5 89
#define IRQ_GPIO_PORT_Q6 90
#define IRQ_GPIO_PORT_Q7 91
#define IRQ_RESERVED2 92
#define IRQ_RESERVED3 93
#define IRQ_RESERVED4 94
#define IRQ_RESERVED5 95
#define IRQ_RESERVED6 96
#define IRQ_RESERVED7 97
#define IRQ_TIMER6A 98
#define IRQ_TIMER6B 99
#define IRQ_TIMER7A 100
#define IRQ_TIMER7B 101
#define IRQ_I2C6 102
#define IRQ_I2C7 103
#define IRQ_RESERVED8 104
#define IRQ_RESERVED9 105
#define IRQ_RESERVED10 106
#define IRQ_RESERVED11 107
#define IRQ_RESERVED12 108
#define IRQ_I2C8 109
#define IRQ_I2C9 110
#define IRQ_RESERVED13 111
#define IRQ_RESERVED14 112
#define IRQ_RESERVED15 113

#ifndef _ASMLANGUAGE

#endif /* !_ASMLANGUAGE */

#endif /* _BOARD__H_ */
