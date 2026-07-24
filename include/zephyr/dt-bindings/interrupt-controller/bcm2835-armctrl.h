/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree IRQ-number bindings for the BCM2835 ARMC peripheral
 *        interrupt controller (banks 0..2; see datasheet ch. 7).
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_BCM2835_ARMCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_BCM2835_ARMCTRL_H_

/** @cond INTERNAL_HIDDEN */

/*
 * Zephyr IRQ-number layout for the BCM2710 SoC (BCM2836 L1 intc +
 * BCM2835 ARMC peripheral intc cascaded behind it):
 *
 *     0..31     reserved for the L1 ARM-local intc (only 0..9 used --
 *               see <zephyr/dt-bindings/interrupt-controller/bcm2836-l1.h>).
 *               IRQ 8 (L1 GPU IRQ) is the cascade entry; the SoC IRQ
 *               glue resolves it transparently inside z_soc_irq_get_active
 *               so it is never seen by application ISRs.
 *
 *     32 + (bank << 5) + bit        -- ARMC peripheral IRQs:
 *         bank 0 (basic):           32..39   (IRQ_BASIC_PENDING bits 0..7)
 *         bank 1 (PEND1, GPU 0..31): 64..95
 *         bank 2 (PEND2, GPU 32..63): 96..127
 *
 * The bank-shift encoding (offset by +32 for the L1 reservation)
 * keeps the SoC-layer shortcut/cascade decode a straightforward
 * arithmetic mapping of the BCM2835 ARMC pending-register layout.
 */
#define BCM2835_IRQ_BASE        32

#define BCM2835_HWIRQ(bank, n)  (BCM2835_IRQ_BASE + ((bank) << 5) + (n))

#define BCM2835_BASIC_IRQ(n)    BCM2835_HWIRQ(0, n)   /* 32..39 */
#define BCM2835_BANK1(n)        BCM2835_HWIRQ(1, n)   /* 64..95 */
#define BCM2835_BANK2(n)        BCM2835_HWIRQ(2, n)   /* 96..127 */

/* basic bank shortcuts */
#define BCM2835_IRQ_ARM_TIMER       BCM2835_BASIC_IRQ(0)
#define BCM2835_IRQ_ARM_MAILBOX     BCM2835_BASIC_IRQ(1)
#define BCM2835_IRQ_ARM_DOORBELL0   BCM2835_BASIC_IRQ(2)
#define BCM2835_IRQ_ARM_DOORBELL1   BCM2835_BASIC_IRQ(3)
#define BCM2835_IRQ_GPU0_HALT       BCM2835_BASIC_IRQ(4)
#define BCM2835_IRQ_GPU1_HALT       BCM2835_BASIC_IRQ(5)
#define BCM2835_IRQ_ILLEGAL_TYPE0   BCM2835_BASIC_IRQ(6)
#define BCM2835_IRQ_ILLEGAL_TYPE1   BCM2835_BASIC_IRQ(7)

/* GPU PEND1 (peripheral lower bank) */
#define BCM2835_IRQ_TIMER0          BCM2835_BANK1(0)
#define BCM2835_IRQ_TIMER1          BCM2835_BANK1(1)
#define BCM2835_IRQ_TIMER2          BCM2835_BANK1(2)
#define BCM2835_IRQ_TIMER3          BCM2835_BANK1(3)
/* USB OTG (Synopsys DWC2). GPU IRQ 9 per the BCM2835 ARM Peripherals
 * datasheet ch. 7 "ARM peripherals interrupts table".
 */
#define BCM2835_IRQ_USB             BCM2835_BANK1(9)  /* DWC2 USB OTG */
/* DMA channel IRQs: channels 0..10 have dedicated lines at PEND1 bits
 * 16..26; channels 11..14 share bit 27. (PEND1 bit 28, the "any DMA
 * channel" line, is not exposed.)
 */
#define BCM2835_IRQ_DMA(n)          BCM2835_BANK1(16 + (n)) /* n = 0..10 */
#define BCM2835_IRQ_DMA_SHARED      BCM2835_BANK1(27)       /* channels 11..14 */
#define BCM2835_IRQ_AUX             BCM2835_BANK1(29) /* mini-UART, SPI1, SPI2 */

/* GPU PEND2 (peripheral upper bank) */
#define BCM2835_IRQ_I2C_SPI_SLV     BCM2835_BANK2(11)
#define BCM2835_IRQ_GPIO0           BCM2835_BANK2(17)
#define BCM2835_IRQ_GPIO1           BCM2835_BANK2(18)
#define BCM2835_IRQ_GPIO2           BCM2835_BANK2(19)
#define BCM2835_IRQ_GPIO3           BCM2835_BANK2(20)
#define BCM2835_IRQ_I2C             BCM2835_BANK2(21)
#define BCM2835_IRQ_SPI             BCM2835_BANK2(22)
#define BCM2835_IRQ_PCM             BCM2835_BANK2(23)
#define BCM2835_IRQ_SDHOST          BCM2835_BANK2(24) /* Legacy SDHost (microSD slot) */
#define BCM2835_IRQ_UART            BCM2835_BANK2(25) /* PL011 UART0 */
#define BCM2835_IRQ_ARASAN_SDIO     BCM2835_BANK2(30) /* Arasan SDHCI / EMMC */

#ifndef IRQ_TYPE_LEVEL
#define IRQ_TYPE_LEVEL  2
#endif
#ifndef IRQ_TYPE_EDGE
#define IRQ_TYPE_EDGE   4
#endif

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_BCM2835_ARMCTRL_H_ */
