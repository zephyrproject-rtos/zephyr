/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TFM_PERIPHERALS_CONFIG_H__
#define TFM_PERIPHERALS_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SECURE_UART1
#define TFM_PERIPHERAL_UARTE1_SECURE 1
#endif

#if TEST_NS_SLIH_IRQ || TEST_NS_FLIH_IRQ
#define TFM_PERIPHERAL_TIMER0_SECURE 1
#endif

#ifdef PSA_API_TEST_IPC
#define TFM_PERIPHERAL_EGU5_SECURE 1

#define TFM_PERIPHERAL_WDT_SECURE 1
#endif

#if defined(NRF91_SERIES)
    #include <tfm_peripherals_config_nrf91.h>
#elif defined(NRF5340_XXAA_APPLICATION)
    #include <tfm_peripherals_config_nrf5340_application.h>
#else
    #error "Unknown device."
#endif

#ifdef __cplusplus
}
#endif

#endif /* TFM_PERIPHERAL_CONFIG_H__ */
