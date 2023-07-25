/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file contains helper macros for dealing with the devicetree
 * radio node's fem property, in the case that it has compatible
 * "nordic,nrf21540-fem".
 *
 * Do not include it directly.
 *
 * For nRF21540 devices:
 *
 *  Value             Property
 *  ---------         --------
 *  PA pin            tx-en-gpios
 *  PA offset         tx-en-settle-time-us
 *  LNA pin           rx-en-gpios
 *  LNA offset        rx-en-settle-time-us
 *  PDN pin           pdn-gpios
 *  PDN offset        pdn-settle-time-us
 *
 * The spi-if property may point at a SPI device node representing the
 * FEM's SPI control interface. See the binding for details.
 */

#define HAL_RADIO_FEM_IS_NRF21540 1

#define HAL_RADIO_GPIO_PA_PROP_NAME         "tx-en-gpios"
#define HAL_RADIO_GPIO_PA_OFFSET_PROP_NAME  "tx-en-settle-time-us"
#define HAL_RADIO_GPIO_LNA_PROP_NAME        "rx-en-gpios"
#define HAL_RADIO_GPIO_LNA_OFFSET_PROP_NAME "rx-en-settle-time-us"

/* This FEM's PA and LNA offset properties have defaults set. */
#define HAL_RADIO_GPIO_PA_OFFSET_MISSING 0
#define HAL_RADIO_GPIO_LNA_OFFSET_MISSING 0

#if FEM_HAS_PROP(tx_en_gpios)
#define HAL_RADIO_GPIO_HAVE_PA_PIN 1
#define HAL_RADIO_GPIO_PA_PROP     tx_en_gpios
#define HAL_RADIO_GPIO_PA_OFFSET   DT_PROP(FEM_NODE, tx_en_settle_time_us)
#endif	/* FEM_HAS_PROP(tx_en_gpios) */

#if FEM_HAS_PROP(rx_en_gpios)
#define HAL_RADIO_GPIO_HAVE_LNA_PIN 1
#define HAL_RADIO_GPIO_LNA_PROP     rx_en_gpios
#define HAL_RADIO_GPIO_LNA_OFFSET   DT_PROP(FEM_NODE, rx_en_settle_time_us)
#endif	/* FEM_HAS_PROP(rx_en_gpios) */

/*
 * The POL_INV macros defined below are just to keep things simple in
 * radio_nrf5_dppi.h, which uses them.
 */

#if FEM_HAS_PROP(pdn_gpios)
#if DT_GPIO_FLAGS(FEM_NODE, pdn_gpios) & GPIO_ACTIVE_LOW
#define HAL_RADIO_GPIO_NRF21540_PDN_POL_INV 1
#endif	/* DT_GPIO_FLAGS(FEM_NODE, pdn_gpios) & GPIO_ACTIVE_LOW */
#endif	/* FEM_HAS_PROP(pdn_gpios) */

#if FEM_HAS_PROP(spi_if)
/* This is the "SPI device" node, i.e. the one with compatible
 * nordic,nrf21540-fem-spi.
 */
#define FEM_SPI_DEV_NODE DT_PHANDLE(FEM_NODE, spi_if)
/* If the SPI device node has a chip select gpio... */
#if DT_SPI_DEV_HAS_CS_GPIOS(FEM_SPI_DEV_NODE)
/* set a macro indicating that, and... */
#define HAL_RADIO_FEM_NRF21540_HAS_CSN 1
/* use it to get the CSN polarity. */
#if DT_SPI_DEV_CS_GPIOS_FLAGS(FEM_SPI_DEV_NODE) & GPIO_ACTIVE_LOW
#define HAL_RADIO_GPIO_NRF21540_CSN_POL_INV 1
#endif	/* DT_SPI_DEV_CS_GPIOS_FLAGS(FEM_SPI_DEV_NODE) & GPIO_ACTIVE_LOW */
#endif	/* DT_SPI_DEV_HAS_CS_GPIOS(FEM_SPI_DEV_NODE) */
#endif	/* FEM_HAS_PROP(spi_if) */
