/*
 * Copyright (c) 2017 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_SOC_H
#define __INC_SOC_H

/* macros related to interrupt handling */
#define XTENSA_IRQ_NUM_SHIFT			0
#define CAVS_IRQ_NUM_SHIFT			8
#define INTR_CNTL_IRQ_NUM_SHIFT			16
#define XTENSA_IRQ_NUM_MASK			0xff
#define CAVS_IRQ_NUM_MASK			0xff
#define INTR_CNTL_IRQ_NUM_MASK			0xff

/*
 * IRQs are mapped on 3 levels. 4th level is left 0x00.
 *
 * 1. Peripheral Register bit offset.
 * 2. CAVS logic bit offset.
 * 3. Core interrupt number.
 */
#define XTENSA_IRQ_NUMBER(_irq) \
	((_irq >> XTENSA_IRQ_NUM_SHIFT) & XTENSA_IRQ_NUM_MASK)
#define CAVS_IRQ_NUMBER(_irq) \
	(((_irq >> CAVS_IRQ_NUM_SHIFT) & CAVS_IRQ_NUM_MASK) - 1)
#define INTR_CNTL_IRQ_NUM(_irq) \
	(((_irq >> INTR_CNTL_IRQ_NUM_SHIFT) & INTR_CNTL_IRQ_NUM_MASK) - 1)

/* CAVS interrupt logic */
#define CAVS_ICTL_BASE_ADDR			0x00078800
#define CAVS_ICTL_0_IRQ				0x00000006
#define CAVS_ICTL_0_IRQ_FLAGS			0

#define CAVS_ICTL_1_IRQ				0x0000000A
#define CAVS_ICTL_1_IRQ_FLAGS			0

#define CAVS_ICTL_2_IRQ				0x0000000D
#define CAVS_ICTL_2_IRQ_FLAGS			0

#define CAVS_ICTL_3_IRQ				0x00000010
#define CAVS_ICTL_3_IRQ_FLAGS			0

/* DW interrupt controller */
#define DW_ICTL_BASE_ADDR			0x00081800
#define DW_ICTL_IRQ				0x00000706
#define DW_ICTL_IRQ_CAVS_OFFSET			CAVS_IRQ_NUMBER(DW_ICTL_IRQ)
#define DW_ICTL_NUM_IRQS			9
#define DW_ICTL_IRQ_FLAGS			0

/* GPIO */
#define GPIO_DW_0_BASE_ADDR			0x00080C00
#define GPIO_DW_0_BITS				32
#define GPIO_DW_PORT_0_INT_MASK			0
#define GPIO_DW_0_IRQ_FLAGS			0
#define GPIO_DW_0_IRQ				0x00040706
#define GPIO_DW_0_IRQ_ICTL_OFFSET		INTR_CNTL_IRQ_NUM(GPIO_DW_0_IRQ)

/* UART - UART0 */
#define UART_NS16550_PORT_0_BASE_ADDR		0x00080800
#define UART_NS16550_PORT_0_CLK_FREQ		38400000
#define UART_NS16550_PORT_0_IRQ			0x00030706
#define UART_NS16550_P0_IRQ_ICTL_OFFSET		INTR_CNTL_IRQ_NUM(\
						UART_NS16550_PORT_0_IRQ)
#define UART_IRQ_FLAGS				0

/* I2C - I2C0 */
#define I2C_DW_0_BASE_ADDR			0x00080400
#define I2C_DW_0_IRQ				0x00020706
#define I2C_DW_0_IRQ_ICTL_OFFSET		INTR_CNTL_IRQ_NUM(I2C_DW_0_IRQ)
#define I2C_DW_IRQ_FLAGS			0
#define I2C_DW_CLOCK_SPEED			38

/* low power DMACs */
#define LP_GP_DMA_SIZE				0x00001000
#define DW_DMA0_BASE_ADDR			0x0007C000
#define DW_DMA1_BASE_ADDR			(0x0007C000 +\
						1 * LP_GP_DMA_SIZE)
#define DW_DMA2_BASE_ADDR			(0x0007C000 +\
						2 * LP_GP_DMA_SIZE)

#define DW_DMA0_IRQ				0x00001110
#define DW_DMA1_IRQ				0x0000010A
#define DW_DMA2_IRQ				0x0000010D

/* address of DMA ownership register. We need to properly configure
 * this register in order to access the DMA registers.
 */
#define CAVS_DMA0_OWNERSHIP_REG			(0x00071A60)
#define CAVS_DMA1_OWNERSHIP_REG			(0x00071A62)
#define CAVS_DMA2_OWNERSHIP_REG			(0x00071A64)

#define DMA_HANDSHAKE_SSP0_TX			2
#define DMA_HANDSHAKE_SSP0_RX			3
#define DMA_HANDSHAKE_SSP1_TX			4
#define DMA_HANDSHAKE_SSP1_RX			5
#define DMA_HANDSHAKE_SSP2_TX			6
#define DMA_HANDSHAKE_SSP2_RX			7
#define DMA_HANDSHAKE_SSP3_TX			8
#define DMA_HANDSHAKE_SSP3_RX			9

extern void _soc_irq_enable(u32_t irq);
extern void _soc_irq_disable(u32_t irq);
extern void setup_ownership_dma0(void);
extern void setup_ownership_dma1(void);
extern void setup_ownership_dma2(void);
extern void dcache_writeback_region(void *addr, size_t size);

#endif /* __INC_SOC_H */
