/**
 * Copyright (c) 2017 IpTronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_WINC1500_WIFI_WINC1500_CONFIG_H_
#define ZEPHYR_DRIVERS_WIFI_WINC1500_WIFI_WINC1500_CONFIG_H_

/*
   ---------------------------------
   ---------- PIN settings ---------
   ---------------------------------
 */
#define CONF_WINC_PIN_RESET			IOPORT_CREATE_PIN(PIOA, 24)
#define CONF_WINC_PIN_CHIP_ENABLE		IOPORT_CREATE_PIN(PIOA, 6)
#define CONF_WINC_PIN_WAKE			IOPORT_CREATE_PIN(PIOA, 25)

/*
   ---------------------------------
   ---------- SPI settings ---------
   ---------------------------------
 */
#define CONF_WINC_USE_SPI			(1)

/** SPI pin and instance settings. */
#define CONF_WINC_SPI				SPI
#define CONF_WINC_SPI_ID			ID_SPI
#define CONF_WINC_SPI_MISO_GPIO			SPI_MISO_GPIO
#define CONF_WINC_SPI_MISO_FLAGS		SPI_MISO_FLAGS
#define CONF_WINC_SPI_MOSI_GPIO			SPI_MOSI_GPIO
#define CONF_WINC_SPI_MOSI_FLAGS		SPI_MOSI_FLAGS
#define CONF_WINC_SPI_CLK_GPIO			SPI_SPCK_GPIO
#define CONF_WINC_SPI_CLK_FLAGS			SPI_SPCK_FLAGS
#define CONF_WINC_SPI_CS_GPIO			SPI_NPCS0_GPIO
#define CONF_WINC_SPI_CS_FLAGS			PIO_OUTPUT_1
#define CONF_WINC_SPI_NPCS			(0)

/** SPI delay before SPCK and between consecutive transfer. */
#define CONF_WINC_SPI_DLYBS			(0)
#define CONF_WINC_SPI_DLYBCT			(0)

/** SPI interrupt pin. */
#define CONF_WINC_SPI_INT_PIN			IOPORT_CREATE_PIN(PIOA, 1)
#define CONF_WINC_SPI_INT_PIO			PIOA
#define CONF_WINC_SPI_INT_PIO_ID		ID_PIOA
#define CONF_WINC_SPI_INT_MASK			(1 << 1)
#define CONF_WINC_SPI_INT_PRIORITY		(0)

/** Clock polarity & phase. */
#define CONF_WINC_SPI_POL			(0)
#define CONF_WINC_SPI_PHA			(1)

/** SPI clock. */
#define CONF_WINC_SPI_CLOCK			(48000000)

/*
   ---------------------------------
   --------- Debug Options ---------
   ---------------------------------
 */
#include <stdio.h>
#define CONF_WINC_DEBUG				(0)
#define CONF_WINC_PRINTF			printf

#endif /* ZEPHYR_DRIVERS_WIFI_WINC1500_WIFI_WINC1500_CONFIG_H_ */
