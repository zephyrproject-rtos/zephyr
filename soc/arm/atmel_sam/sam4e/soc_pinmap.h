/*
 * Copyright (c) 2020 Vincent van der Locht
 * Copyright (c) 2019 Gerson Fernando Budke
 * Copyright (c) 2017 Justin Watson
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM4E MCU pin definitions.
 *
 * This file contains pin configuration data required by different MCU
 * modules to correctly configure GPIO controller.
 */

#ifndef _ATMEL_SAM4E_SOC_PINMAP_H_
#define _ATMEL_SAM4E_SOC_PINMAP_H_

#include <soc.h>

/* Ethernet MAC (GMAC) */

#define PINS_GMAC_MASK (PIO_PD0A_GTXCK | PIO_PD1A_GTXEN | \
			PIO_PD2A_GTX0 | PIO_PD3A_GTX1 | PIO_PD15A_GTX2 | \
			PIO_PD16A_GTX3 | PIO_PD4A_GRXDV | PIO_PD7A_GRXER | \
			PIO_PD14A_GRXCK | PIO_PD5A_GRX0 | PIO_PD6A_GRX1 | \
			PIO_PD11A_GRX2 | PIO_PD12A_GRX3 | PIO_PD13A_GCOL | \
			PIO_PD10A_GCRS | PIO_PD8A_GMDC | PIO_PD9A_GMDIO)
#define PIN_GMAC_SET1 {PINS_GMAC_MASK, PIOD, ID_PIOD, SOC_GPIO_FUNC_A}
#define PINS_GMAC0 {PIN_GMAC_SET1}

#endif /* _ATMEL_SAM4E_SOC_PINMAP_H_ */
