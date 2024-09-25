/*
 * Copyright (c) 2016 - 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This file is undergoing transition towards native Zephyr nrf USB driver. */

/** @cond INTERNAL_HIDDEN */

#ifndef NRF_USBD_COMMON_ERRATA_H__
#define NRF_USBD_COMMON_ERRATA_H__

#include <nrfx.h>
#include <nrf_erratas.h>

#ifndef NRF_USBD_COMMON_ERRATA_ENABLE
/**
 * @brief The constant that informs if errata should be enabled at all.
 *
 * If this constant is set to 0, all the Errata bug fixes will be automatically disabled.
 */
#define NRF_USBD_COMMON_ERRATA_ENABLE 1
#endif

/* Errata: ISO double buffering not functional. **/
static inline bool nrf_usbd_common_errata_166(void)
{
	return NRF_USBD_COMMON_ERRATA_ENABLE && nrf52_errata_166();
}

/* Errata: USBD might not reach its active state. **/
static inline bool nrf_usbd_common_errata_171(void)
{
	return NRF_USBD_COMMON_ERRATA_ENABLE && nrf52_errata_171();
}

/* Errata: USB cannot be enabled. **/
static inline bool nrf_usbd_common_errata_187(void)
{
	return NRF_USBD_COMMON_ERRATA_ENABLE && nrf52_errata_187();
}

/* Errata: USBD cannot receive tasks during DMA. **/
static inline bool nrf_usbd_common_errata_199(void)
{
	return NRF_USBD_COMMON_ERRATA_ENABLE && nrf52_errata_199();
}

/* Errata: Device remains in SUSPEND too long. */
static inline bool nrf_usbd_common_errata_211(void)
{
	return NRF_USBD_COMMON_ERRATA_ENABLE && nrf52_errata_211();
}

/* Errata: Unexpected behavior after reset. **/
static inline bool nrf_usbd_common_errata_223(void)
{
	return NRF_USBD_COMMON_ERRATA_ENABLE && nrf52_errata_223();
}

#endif /* NRF_USBD_COMMON_ERRATA_H__ */

/** @endcond */
