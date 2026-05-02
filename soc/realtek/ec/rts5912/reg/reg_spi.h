/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_SPI_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_SPI_H

/**
 * @brief SPI Controller (SPI)
 */

struct spi_reg {
	volatile uint32_t CMDL;
	volatile uint32_t CMDH;
	volatile uint32_t CMDN;
	const uint32_t RESERVED;
	volatile uint32_t ADDR;
	volatile uint32_t ADDRN;
	volatile uint32_t LEN;
	volatile uint32_t TRSF;
	volatile uint32_t CTRL;
	volatile uint32_t CFG;
	volatile uint32_t CKDV;
	volatile const uint32_t STS;
	volatile uint32_t SIG;
	volatile uint32_t TX;
	volatile const uint32_t RX;
};

/* TRSF */
#define RTS5912_SPI_TRSF_MODE_MASK    GENMASK(3, 0)
#define RTS5912_SPI_TRSF_END_MASK     BIT(6)
#define RTS5912_SPI_TRSF_START_MASK   BIT(7)
/* CTRL */
#define RTS5912_SPI_CTRL_RST_MASK     BIT(0)
#define RTS5912_SPI_CTRL_TRANSEL_MASK BIT(1)
#define RTS5912_SPI_CTRL_MODE_MASK    GENMASK(3, 2)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_SPI_H */
