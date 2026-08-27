/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* required by whd_* header files. */

/* See cybsp_types.h, which is usually required by this file, but not available */
/* here unless we pull in board-specific headers from the upstream driver. */
#define CYBSP_SDIO_INTERFACE (0)
#define CYBSP_SPI_INTERFACE  (1)

#ifdef CONFIG_AIROC_WIFI_BUS_SDIO
#define CYBSP_WIFI_INTERFACE_TYPE CYBSP_SDIO_INTERFACE
#elif CONFIG_AIROC_WIFI_BUS_SPI
#define CYBSP_WIFI_INTERFACE_TYPE CYBSP_SPI_INTERFACE
#endif
