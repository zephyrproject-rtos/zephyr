/*
 * Copyright (C) 2024 Abe Kohandel <abe.kohandel@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DT_BINDINGS_INTERRUPT_CONTROLLER_TI_AM335X_INTC_H
#define ZEPHYR_DT_BINDINGS_INTERRUPT_CONTROLLER_TI_AM335X_INTC_H

#define IRQ_UART3 44
#define IRQ_UART4 45
#define IRQ_UART5 46

#define IRQ_TIMER0 66
#define IRQ_TIMER1 67
#define IRQ_TIMER2 68
#define IRQ_TIMER3 69

#define IRQ_UART0 72
#define IRQ_UART1 73
#define IRQ_UART2 74

#define IRQ_TIMER4 92
#define IRQ_TIMER5 93
#define IRQ_TIMER6 94
#define IRQ_TIMER7 95

#define IRQ_FLAGS 0

#define DT_INT(irq, prio) irq prio IRQ_FLAGS

#endif /* ZEPHYR_DT_BINDINGS_INTERRUPT_CONTROLLER_TI_AM335X_INTC_H */
