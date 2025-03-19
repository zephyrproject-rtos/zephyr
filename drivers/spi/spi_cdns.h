/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CDNS_SPI_H_
#define ZEPHYR_DRIVERS_CDNS_SPI_H_

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>

/* SPI register address offset */
#define CDNS_SPI_CR_OFFSET	0x00U
#define CDNS_SPI_SR_OFFSET	0x04U
#define CDNS_SPI_IER_OFFSET	0x08U
#define CDNS_SPI_IDR_OFFSET	0x0CU
#define CDNS_SPI_IMR_OFFSET	0x10U
#define CDNS_SPI_ER_OFFSET	0x14U
#define CDNS_SPI_DR_OFFSET	0x18U
#define CDNS_SPI_TXD_OFFSET	0x1CU
#define CDNS_SPI_RXD_OFFSET	0x20U
#define CDNS_SPI_SICR_OFFSET	0x24U
#define CDNS_SPI_TXWR_OFFSET	0x28U
#define CDNS_SPI_RXWR_OFFSET	0x2CU

/* SPI register masks */
#define CDNS_SPI_CR_RESET_STATE	BIT(17)
#define CDNS_SPI_CR_MANTXSTRT	BIT(16)
#define CDNS_SPI_CR_MANSTRTEN	BIT(15)
#define CDNS_SPI_CR_CPHA	BIT(2)
#define CDNS_SPI_CR_CPOL	BIT(1)
#define CDNS_SPI_CR_SSCTRL_MASK	GENMASK(13, 10)
#define CDNS_SPI_CR_PERI_SEL	BIT(9)
#define CDNS_SPI_CR_BAUD_DIV	GENMASK(5, 3)
#define CDNS_SPI_CR_MSTREN	BIT(0)
#define CDNS_SPI_CR_SSFORCE	BIT(14)
#define CDNS_SPI_CR_BAUD_DIV_4	BIT(3)
#define CDNS_SPI_CR_DEFAULT	(CDNS_SPI_CR_MSTREN | CDNS_SPI_CR_MANSTRTEN | \
				 CDNS_SPI_CR_SSFORCE | CDNS_SPI_CR_BAUD_DIV_4)

#define CDNS_SPI_IXR_ALL		GENMASK(6, 0)
#define CDNS_SPI_IXR_TXUF_MASK		BIT(6)
#define CDNS_SPI_IXR_RXOVR_MASK		BIT(0)
#define CDNS_SPI_IXR_TXOW_MASK		BIT(2)
#define CDNS_SPI_IXR_MODF_MASK		BIT(1)
#define CDNS_SPI_IXR_RXNEMPTY_MASK	BIT(4)
#define CDNS_SPI_IXR_DFLT_MASK		(CDNS_SPI_IXR_TXOW_MASK | CDNS_SPI_IXR_MODF_MASK)

#define CDNS_SPI_ER_ENABLE		BIT(0)

#define CDNS_SPI_BAUD_DIV_MIN		1U
#define CDNS_SPI_BAUD_DIV_SHIFT		3U
#define CDNS_SPI_BAUD_DIV_MAX		7U
#define CDNS_SPI_CR_SSCTRL_SHIFT	10U
#define CDNS_SPI_CR_SSCTRL_MAXIMUM	15U
#define CDNS_SPI_FIFO_DEPTH		128U

#endif /* ZEPHYR_DRIVERS_CDNS_SPI_H_ */
