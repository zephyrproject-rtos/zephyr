/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NORDICSEMI_NRF71_LFXO_PIXO_H_
#define _NORDICSEMI_NRF71_LFXO_PIXO_H_

#if defined(CONFIG_SOC_NRF71_LFXO_PIXO)

/**
 * Lock the LFXO mirrored registers so the PIXO configuration is kept in low power.
 *
 * Must be called before the LFXO is started.
 */
void nrf71_lfxo_pixo_mirror_lock(void);

/**
 * Switch the LFXO to PIXO mode once it runs in Pierce mode.
 */
void nrf71_lfxo_pixo_switch_start(void);

/**
 * Switch the LFXO from PIXO back to Pierce mode before entering System OFF.
 */
void nrf71_lfxo_prepare_system_off(void);

#else

static inline void nrf71_lfxo_pixo_mirror_lock(void)
{
}

static inline void nrf71_lfxo_pixo_switch_start(void)
{
}

static inline void nrf71_lfxo_prepare_system_off(void)
{
}

#endif /* CONFIG_SOC_NRF71_LFXO_PIXO */

#endif /* _NORDICSEMI_NRF71_LFXO_PIXO_H_ */
