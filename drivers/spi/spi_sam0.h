/*
 * Copyright (c) 2025 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_SAM0_H_
#define ZEPHYR_DRIVERS_SPI_SPI_SAM0_H_

/* GCLK registers */

#define CLKCTRL_OFFSET 0x02
#define PCHCTRL_OFFSET 0x80

#define CLKCTRL_ID(n)  FIELD_PREP(GENMASK(5, 0), n)
#define CLKCTRL_GEN(n) FIELD_PREP(GENMASK(11, 8), n)
#define CLKCTRL_CLKEN  BIT(14)

#define PCHCTRL_GEN(n) FIELD_PREP(GENMASK(3, 0), n)
#define PCHCTRL_CHEN   BIT(6)

/* SERCOM SPI registers */

#define CTRLA_OFFSET 0x00
#define CTRLB_OFFSET 0x04

#if defined(CONFIG_SOC_SERIES_SAMD20)

#define BAUD_OFFSET     0x0A
#define INTENCLR_OFFSET 0x0C
#define INTFLAG_OFFSET  0x0E
#define STATUS_OFFSET   0x10
#define DATA_OFFSET     0x18

#define INTENCLR_MASK 0x07

#define STATUS_SYNCBUSY_BIT 15

#else

#define BAUD_OFFSET     0x0C
#define INTENCLR_OFFSET 0x14
#define INTFLAG_OFFSET  0x18
#define STATUS_OFFSET   0x1A
#define SYNCBUSY_OFFSET 0x1C
#define DATA_OFFSET     0x28

#define INTENCLR_MASK 0x8F

#endif

#define CTRLA_ENABLE_BIT 1
#define CTRLA_MODE(n)    FIELD_PREP(GENMASK(4, 2), n)
#define CTRLA_DOPO_MASK  GENMASK(17, 16)
#define CTRLA_DOPO(n)    FIELD_PREP(CTRLA_DOPO_MASK, n)
#define CTRLA_DIPO_MASK  GENMASK(21, 20)
#define CTRLA_DIPO(n)    FIELD_PREP(CTRLA_DIPO_MASK, n)
#define CTRLA_CPHA_BIT   28
#define CTRLA_CPOL_BIT   29
#define CTRLA_DORD_BIT   30

#define CTRLB_CHSIZE_MASK GENMASK(2, 0)
#define CTRLB_RXEN_BIT    17

#define INTFLAG_DRE BIT(0)
#define INTFLAG_TXC BIT(1)
#define INTFLAG_RXC BIT(2)

#if defined(CONFIG_SOC_SERIES_SAMD51) || defined(CONFIG_SOC_SERIES_SAME51) ||                      \
	defined(CONFIG_SOC_SERIES_SAME53) || defined(CONFIG_SOC_SERIES_SAME54)

#define SYNCBUSY_MASK GENMASK(4, 0)
#else
#define SYNCBUSY_MASK GENMASK(2, 0)
#endif

#endif /* ZEPHYR_DRIVERS_SPI_SPI_SAM0_H_ */
