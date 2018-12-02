/*
 * Copyright (c) 2018 Zilogic Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SPI_PL022_H_
#define _SPI_PL022_H_

#include "spi_context.h"

enum reg_offset {
	SPI_CR0_OFFSET   = 0x00,
	SPI_CR1_OFFSET   = 0x04,
	SPI_DR_OFFSET    = 0x08,
	SPI_SR_OFFSET    = 0x0C,
	SPI_CPSR_OFFSET  = 0x10,
	SPI_IMSC_OFFSET  = 0x14,
	SPI_RIS_OFFSET   = 0x18,
	SPI_MIS_OFFSET   = 0x1C,
	SPI_ICR_OFFSET   = 0x20,
	SPI_DMACR_OFFSET = 0x24
};

enum status_flags {
	TX_EMPTY,
	TX_NOFULL,
	RX_NOEMPTY,
	RX_FULL,
	BUSY
};

enum interrupt_status {
	SPI_RORMIS,
	SPI_RTMIS,
	SPI_RXMIS,
	SPI_TXMIS
};

#define SPI_INT_MASK(value)        ((1 << value))

#define CPOL                       6
#define CPHA                       7
#define LOOPBACK                   0
#define SSE                        1
#define MODE                       2

#define FRF_SHIFT                  4
#define FRF_SPI                    0
#define DSS_SHIFT                  0
#define DSS_8BITS                  0x7

#define CLK_PRESCALE_MAX           254
#define CLK_PRESCALE_MIN           2
#define CLK_PRESCALE_SHIFT         0

#define SERIAL_CLKRATE_MAX         255
#define SERIAL_CLKRATE_MIN         1
#define SERIAL_CLKRATE_SHIFT       8

#define BYTE_MASK                  0xFF
#define NIBBLE_MASK                0xF
#define TWOBIT_MASK                0x3

#define SPI_REG_ADDR(addr, offset) (addr + offset)

typedef void (*irq_config_func_t)();

/* Device configuration parameters */
struct spi_pl022_config {
	u32_t base;
	irq_config_func_t config_func;
#ifdef CONFIG_CLOCK_CONTROL_STELLARIS
	struct stellaris_clock_t pclk;
#endif
};

/* Device run time data */
struct spi_pl022_data {
	struct spi_context ctx;
};

#endif  /* _SPI_PL022_H_ */
