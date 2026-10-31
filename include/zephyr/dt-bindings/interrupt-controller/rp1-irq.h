/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree interrupt vectors for the Raspberry Pi RP1 MSI-X
 *        interrupt controller.
 *
 * The RP1 exposes 61 MSI-X vectors. These vector numbers are fixed by
 * the RP1 and are independent of the host interrupt controller. A host
 * maps them to its own IRQ numbering in the interrupt-map property of
 * the RP1 interrupt controller node.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_RP1_IRQ_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_RP1_IRQ_H_

#define RP1_NUM_IRQS 61

/* RP1 MSI-X vector numbers, see the RP1 datasheet. */
#define RP1_IRQ_IO_BANK0   0
#define RP1_IRQ_IO_BANK1   1
#define RP1_IRQ_IO_BANK2   2
#define RP1_IRQ_AUDIO_IN   3
#define RP1_IRQ_AUDIO_OUT  4
#define RP1_IRQ_PWM0       5
#define RP1_IRQ_ETH        6
#define RP1_IRQ_I2C0       7
#define RP1_IRQ_I2C1       8
#define RP1_IRQ_I2C2       9
#define RP1_IRQ_I2C3       10
#define RP1_IRQ_I2C4       11
#define RP1_IRQ_I2C5       12
#define RP1_IRQ_I2C6       13
#define RP1_IRQ_I2S0       14
#define RP1_IRQ_I2S1       15
#define RP1_IRQ_I2S2       16
#define RP1_IRQ_SDIO0      17
#define RP1_IRQ_SDIO1      18
#define RP1_IRQ_SPI0       19
#define RP1_IRQ_SPI1       20
#define RP1_IRQ_SPI2       21
#define RP1_IRQ_SPI3       22
#define RP1_IRQ_SPI4       23
#define RP1_IRQ_SPI5       24
#define RP1_IRQ_UART0      25
#define RP1_IRQ_TIMER0     26
#define RP1_IRQ_TIMER1     27
#define RP1_IRQ_TIMER2     28
#define RP1_IRQ_TIMER3     29
#define RP1_IRQ_USBHOST0   30
#define RP1_IRQ_USBHOST0_0 31
#define RP1_IRQ_USBHOST0_1 32
#define RP1_IRQ_USBHOST0_2 33
#define RP1_IRQ_USBHOST0_3 34
#define RP1_IRQ_USBHOST1   35
#define RP1_IRQ_USBHOST1_0 36
#define RP1_IRQ_USBHOST1_1 37
#define RP1_IRQ_USBHOST1_2 38
#define RP1_IRQ_USBHOST1_3 39
#define RP1_IRQ_DMA        40
#define RP1_IRQ_PWM1       41
#define RP1_IRQ_UART1      42
#define RP1_IRQ_UART2      43
#define RP1_IRQ_UART3      44
#define RP1_IRQ_UART4      45
#define RP1_IRQ_UART5      46
#define RP1_IRQ_MIPI0      47
#define RP1_IRQ_MIPI1      48
#define RP1_IRQ_VIDEO_OUT  49
#define RP1_IRQ_PIO0       50
#define RP1_IRQ_PIO1       51
#define RP1_IRQ_ADC_FIFO   52
#define RP1_IRQ_PCIE_OUT   53
#define RP1_IRQ_SPI6       54
#define RP1_IRQ_SPI7       55
#define RP1_IRQ_SPI8       56
#define RP1_IRQ_PROC_MISC  57
#define RP1_IRQ_SYSCFG     58
#define RP1_IRQ_CLOCKS     59
#define RP1_IRQ_VBUSCTRL   60

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_RP1_IRQ_H_ */
